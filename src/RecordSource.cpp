#include "RecordSource.h"

namespace ora {

    using std::optional, std::vector;

    Block DefaultRecordSource::getOrThrow(std::string_view prefix) {
        auto block = block_source->getNext();
        if (!block)
            throw std::runtime_error(
                fmt::format("{} :: Truncated log file or missing extension block", prefix));

        return std::move(*block);
    }

    std::optional<Block> DefaultRecordSource::get_LatestOrNext() {
        if (!latest_block)
            return block_source->getNext();

        auto b = std::move(*latest_block);
        latest_block = std::nullopt;

        return b;
    }

    int DefaultRecordSource::fill_Records() {

        auto block_opt =  get_LatestOrNext();
        if (!block_opt) return -1; // return :: -1 ~> EOF

        Block block = std::move(*block_opt);

        auto out = 0;                       // filled-record count
        auto start = block.head.offset;

        for (const auto& bound : block.bounds) {

            constexpr uint64_t MAX_REDO_RECORD_LEN = 32 * 1024 * 1024;
            assert(bound.len <= MAX_REDO_RECORD_LEN);

            const auto rba = RBA{block.head.log_seq_no, block.head.block_no, start};
            const auto raw_sz = bound.len;

            auto raw = std::vector<char>(bound.len);

            // fill raw
            // --------------------------------------------------------------------------------
            auto need = bound.len;

            const auto body0 = block_sz - start;
            const bool is_once = need <= body0;
            const auto len0 = is_once ? need : body0;

            auto wPtr = 0;
            std::memcpy( raw.data(), block.raw.data() + start, len0);
            wPtr += len0;
            need -= len0;

            if (is_once) {
                auto record = Record(rba, raw, block.head.block_no, start + len0,
                                     block_sz, isLittleEndian, over12c);

                assert( record.raw.size() == raw_sz);
                assert( block.head.block_no + bound.next_blocks == record.end_block);
                assert( bound.next_offset == record.end_offset );
                this->out_buffer.push_back(std::move(record));
                out++;

                start += len0;  // may single-block with multi-record.
                continue;
            }

            while ( need > 0 ) {
                Block b = getOrThrow("fill_Records (multi-block)");

                const auto is_last = need <= body_sz;
                const auto len = is_last ? need : body_sz;

                std::memcpy( raw.data() + wPtr, b.raw.data() + 16, len);
                wPtr += len;    // multi-block record
                need -= len;

                if (is_last) {
                    auto record = Record(rba, raw, b.head.block_no, 16 + len,
                                         block_sz, isLittleEndian, over12c);
                    assert(record.raw.size() == raw_sz);
                    assert(bound.next_offset == record.end_offset);
                    assert(block.head.block_no + bound.next_blocks == record.end_block);
                    this->out_buffer.push_back(std::move(record));
                    out++;

                    this->latest_block = (body_sz != need) ? std::make_optional(b) : std::nullopt;
                }
            }
        }

        return out;
    }

    optional<Record> DefaultRecordSource::getNext() {

        while (out_buffer.empty()) {
            const auto n = fill_Records();
            if (n == -1) // EOF
                return std::nullopt;
        }
        auto out = std::move(out_buffer.front());

        out_buffer.pop_front();

        return out;
    }
}
