#include "RecordSource.h"

#include <cassert>

namespace ora {

    using std::optional, std::vector, std::runtime_error;
    using fmt::format;

    Block DefaultRecordSource::getOrThrow(std::string_view prefix) const {
        auto block = block_source->getNext();
        if (!block)
            throw runtime_error( format("{} Not enough blocks: check file-size with NAB", prefix));

        return std::move(*block);
    }

    std::optional<Block> DefaultRecordSource::get_LatestOrNext() {

        if (!latest_block)
            return block_source->getNext();

        auto b = std::move(*latest_block);
        latest_block = std::nullopt;

        return b;
    }

    static bool prefer_new(const RecordBound &b0,
                           const RecordBound &b1,
                           const RBA &rba0,
                           const uint32_t block_no,
                           const std::optional<LwnCtx> &lwn ) {

        const auto log = [&](const std::string s) {
            fmt::println(std::cerr, "-- choose {} :: {}+{}.@{} >>> {}+{}.@{}, ",
                s,
                rba0.block_no, b0.next_blocks, b0.next_offset,
                     block_no, b1.next_blocks, b1.next_offset); };

        // ------------------------------------------------------------
        const auto vld0 = b0.boundInfo.vld;
        const auto vld1 = b1.boundInfo.vld;
        const auto choice = choose_new_vld(vld0, vld1);

        if (choice) {
            if (*choice) log("vld");
            return *choice;
        }

        // ------------------------------------------------------------
        const auto s0 = scn_to64(b0.boundInfo.scn);
        const auto s1 = scn_to64(b1.boundInfo.scn);

        if (lwn) {
            const auto top = lwn->nxt();

            const bool out = (s0 == s1) ? b0.len > b1.len
                            : (top < s0 && top < s1) ? std::min(s0, s1) == s1
                            : (top > s0 && top > s1) ? std::max(s0, s1) == s1
                            : std::min(s0, s1) == s1;

            if (out) log("lwn-scn");
            return out;
        }

        // ------------------------------------------------------------
        const auto latest = s0 < s1;
        if (latest) log("latest");

        return latest;

    }

    static bool choose_new( const RBA &rba0,
                            const RecordBound &b0,
                            const Block &b,
                            const std::vector<RecordBound> &fbs,
                            const std::optional<LwnCtx>& lwn){

        const uint16_t offset = b.offset();

        if (offset == 0) return false;
        if (offset > 16) return false;

        const auto sz = fbs.size();

        if ( sz == 0) return false;
        if ( sz == 1) return prefer_new(b0, fbs[0], rba0, b.block_no(), lwn);

        // sz > 1
        //-------------------------------------------------------------
        const auto& b1 = fbs[0];
        fmt::println(std::cerr, "-- choose mul :: {}+{}.@{} >>> {}+{}.@{}",
                     rba0.block_no, b0.next_blocks, b0.next_offset,
                     b.block_no(), b1.next_blocks, b1.next_offset);
        return true;
    }


    int DefaultRecordSource::fill_Records() {

        auto block_opt = get_LatestOrNext();
        if (!block_opt) return -1;              // return :: -1 ~> EOF

        Block block = std::move(*block_opt);

        auto out = 0;                           // filled-record count
        auto start = block.offset();

        for (const auto& bound : block.bounds) {

            const auto rba = RBA{block.head.log_seq_no, block.block_no(), start};
            if (!fairBound(bound, lwn_ctx, true, rba))
                break;

            // fill raw
            // --------------------------------------------------------------------------------
            auto raw = std::vector<char>(bound.len);
            auto need = bound.len;
            const auto room0 = ctx.block_sz - start;
            const bool inSet = need <= room0;
            const auto take0 = inSet ? need : room0;

            auto wPtr = 0;
            std::memcpy( raw.data(), block.raw.data() + start, take0);
            wPtr += take0;
            need -= take0;

            if (inSet) {
                auto record = Record_of(rba, raw, bound, ctx);
                update_water(record);
                this->out_buffer.push_back(std::move(record));
                out++;

                start += take0;  // may single-block with multi-record.
                continue;
            }

            // --------------------------------------------------------------------------------
            bool first = true;
            while ( need > 0 ) {
                Block b = getOrThrow(format("fill_Records: B #{}.@{} len: {} ", block.block_no(), start, bound.len));
                const auto fbs = fairBounds(b, lwn_ctx, true);

                if (first) {
                    first = false;
                    if ( choose_new(rba, bound, b, fbs, lwn_ctx)) {
                        this->latest_block = b; // this case need inspection.
                        return out;
                    }
                }

                const auto last = need <= body_sz;
                const auto take = last ? need : body_sz;

                if (!last) {
                    const auto bsz = fbs.size();
                    if( bsz > 1) throw runtime_error(format("too many records : #{}", b.block_no()));
                    if (bsz == 1 && prefer_new(bound, fbs[0], rba, b.block_no(), lwn_ctx))
                            throw runtime_error(format("prefer new found interim. #{}", b.block_no()));
                }

                std::memcpy( raw.data() + wPtr, b.raw.data() + 16, take);
                wPtr += take;
                need -= take;

                if (last) {
                    auto record = Record_of(rba, raw, bound, ctx);
                    update_water(record);

                    const bool use_all = body_sz == take;
                    const bool not_new = b.offset() < record.end_offset; // if not_new, block.offset not updated.

                    this->latest_block = use_all || not_new
                                             ? std::nullopt
                                             : std::make_optional(b);

                    if (latest_block) {
                        if (b.offset() != record.end_offset) // this case need inspection.
                            fmt::println(std::cerr, "-- last offset differ. #{}.@{} ~ #{}.@{} != #{}.@{}(block)",
                                         rba.block_no, rba.offset,
                                         record.end_block, record.end_offset,
                                         b.block_no(), b.offset());
                    }

                    this->out_buffer.push_back(std::move(record));
                    out++;
                    return out;
                }
            }
        }

        return out;
    }

    optional<Record> DefaultRecordSource::getNext() {

        while (out_buffer.empty()) {
            // try {
                const auto n = fill_Records();
                if (n == -1) // EOF
                    return std::nullopt;
            // } catch (const std::exception& e) {
            //     fmt::println(std::cerr, "[DefaultRecordSource::getNext] {}", e.what());
            //     return std::nullopt;
            // }

        }
        auto out = std::move(out_buffer.front());
        out_buffer.pop_front();
        return out;
    }
}
