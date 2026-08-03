#include "RecordSource.h"

namespace ora {

    using std::optional, std::vector;

    inline bool record_within_block(const RecordBound& offset) {
        return offset.skip_blocks == 0 || (offset.skip_blocks == 1 && offset.next_offset == 16);
    }
    inline bool last_record_in_block(const RecordBound& offset) {
        return  (offset.skip_blocks >= 1) && !record_within_block(offset);
    }

    // ------------------------------------------------------
    Block DefaultRecordSource::getOrThrow(std::string_view context) {
        auto block = block_source->getNext();
        if (!block) {
            throw std::runtime_error(
                std::string(context) + " :: Truncated log file or missing extension block" );
        }
        return std::move(*block);
    }

    Record prepare_Record( const Block& block, const uint16_t start, const size_t len) {

        Record out;
        out.rba = RBA{
            block.head.log_seq_no,
            block.head.block_no,
            start
        };
        out.len = len;
        out.raw.resize(len);

        return out;
    }

    constexpr uint64_t MAX_REDO_RECORD_LEN = 32 * 1024 * 1024;

    // return :: -1 ~> EOF
    int DefaultRecordSource::fill_Records() {

        Block block;

        if (last_block) {
            block = std::move(*last_block);
            last_block = std::nullopt;
        } else {
            auto b0 = block_source->getNext();
            if (!b0) return -1;
            block = std::move(*b0);
        }

        auto out = 0;
        auto start = block.head.offset;

        for (const auto& info : block.bounds) {

            assert( info.len <= MAX_REDO_RECORD_LEN );
            auto record = prepare_Record(block, start, info.len);

            // fill record.raw
            // --------------------------------------------------------------------------------
            // Single-Block Record
            if (record_within_block(info)) {

                std::memcpy(record.raw.data(), block.view.data() + start, info.len);
                record.last_block_no = block.head.block_no;
                record.last_offset = start + info.len;

                out_buffer.push_back(std::move(record));
                out++;

                start = info.next_offset;

                continue;   // Single-Block may contain many Records
            }

            // Multi-Block Record
            // --------------------------------------------------------------------------------
            const auto block_sz = block.view.size(); // block's view has same-size.
            const auto payload = block_sz - 16;

            auto first_chunk = block_sz - start;
            std::memcpy(record.raw.data(), block.view.data() + start, first_chunk);

            auto pos = first_chunk;
            auto need = info.len - first_chunk;
            auto left = info.skip_blocks; // for validity

            while ( need >= payload ) {
                auto interim = getOrThrow("fill_Records (interim)");
                std::memcpy(record.raw.data() + pos, interim.view.data() + 16, payload);
                record.last_block_no = interim.head.block_no;

                left --;
                pos += payload;
                need -= payload;
            }

            if (need > 0) {
                auto last = getOrThrow("fill_Records (last)");
                std::memcpy(record.raw.data() + pos, last.view.data() + 16, need);
                record.last_block_no = last.head.block_no;

                if (!last.bounds.empty()) last_block = last;

                if (!last.bounds.empty() && last.head.offset != info.next_offset) {
                    fmt::println("last.head.offset != info.next_offset {} != {}", last.head.offset, info.next_offset);
                    throw std::runtime_error("last.head.offset != info.next_offset");
                }

                left --;
            }

            record.last_offset = info.next_offset;

            if (left != 0) throw std::runtime_error("fill_Records :: Corrupted skip_blocks metadata");

            out_buffer.push_back(std::move(record));
            out++;
            break;  // Multi-Block-Record must last in current-Block.
        }
        return out;
    }

    optional<Record> DefaultRecordSource::getNext() {

        while (out_buffer.empty()) {
            auto n = fill_Records();
            if (n == -1) // EOF
                return std::nullopt;
        }

        auto out = std::move(out_buffer.front());

        auto view = tcb::span<const char>(out.raw);
        out.head = RecordHead_of(view, block_source->isLittleEndian());

        assert(out.len == out.raw.size());

        out_buffer.pop_front();
        return out;
    }

}
