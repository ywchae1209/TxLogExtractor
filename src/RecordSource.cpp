#include "RecordSource.h"

namespace ora {

    using std::optional, std::vector;

    inline bool record_within_block(const RecordBound& offset) {
        return offset.next_blocks == 0 ||
               (offset.next_blocks == 1 && offset.next_offset == 16);
    }
    inline bool last_record_in_block(const RecordBound& offset) {
        return  (offset.next_blocks >= 1) && !record_within_block(offset);
    }

    // ------------------------------------------------------
    Block DefaultRecordSource::getOrThrow(std::string_view context) {
        auto block = block_source->getNext();
        if (!block)
            throw std::runtime_error( std::string(context) + " :: Truncated log file or missing extension block" );

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


    std::optional<Block> DefaultRecordSource::get_LatestOrNext() {
        if (latest_block) {
            auto b = std::move(*latest_block);
            latest_block = std::nullopt;
            return b;
        }
        return block_source->getNext();
    }

    // return :: -1 ~> EOF
    int DefaultRecordSource::fill_Records() {

        constexpr uint64_t MAX_REDO_RECORD_LEN = 32 * 1024 * 1024;

        auto block_opt =  get_LatestOrNext();
        if (!block_opt) return -1;

        Block block = std::move(*block_opt);

        auto out = 0;       // record count
        auto start = block.head.offset;
        auto expect = block.bounds.size();

        for (const auto& info : block.bounds) {

            assert( info.len <= MAX_REDO_RECORD_LEN );
            auto record = prepare_Record(block, start, info.len);

            // --------------------------------------------------------------------------------
            // fill record.raw
            // --------------------------------------------------------------------------------
            // Single-Block Record
            if ( record_within_block(info)) {

                std::memcpy(record.raw.data(), block.view.data() + start, info.len);
                record.end_block = block.head.block_no;
                record.end_offset = start + info.len;

                this->out_buffer.push_back(std::move(record));
                out++;

                start = info.next_offset;
                continue;   // Single-Block may contain many Records
            }

            // Multi-Block Record
            // --------------------------------------------------------------------------------
            auto payload     = block_sz - 16;
            auto first_chunk = block_sz - start;
            std::memcpy(record.raw.data(), block.view.data() + start, first_chunk);

            uint32_t end_block;
            uint16_t end_offset;

            auto pos = first_chunk;
            auto need = info.len - first_chunk;
            auto left = info.next_blocks;               // for validity

            while ( need >= payload ) {
                auto interim = getOrThrow("fill_Records (interim)");
                std::memcpy(record.raw.data() + pos, interim.view.data() + 16, payload);
                end_block = interim.head.block_no;

                left --;
                pos += payload;
                need -= payload;
            }

            end_offset = need + 16;

            if (need > 0) {
                auto last = getOrThrow("fill_Records (last)");
                std::memcpy(record.raw.data() + pos, last.view.data() + 16, need);
                end_block  = last.head.block_no;

                this->latest_block = last.bounds.empty() ? std::nullopt : std::make_optional(last);
                assert (last.bounds.empty() || last.head.offset == info.next_offset);
                left --;
            }
            assert (left == 0 || (left == 1 && end_offset == 16)) ;

            record.end_block = end_block;
            record.end_offset = end_offset;

            this->out_buffer.push_back(std::move(record));
            out++;
            assert( out == expect);

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
