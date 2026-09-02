#include "Block.h"

#include "BlockSource.h"
#include "layout_BlockHead.h"
#include "layout_FileHead.h"
#include "../coral_show.h"

#include "range/v3/view/transform.hpp"

namespace ora {

    void Block::set_head(const bool isLittle) {
        head = decode_BlockHead(raw, isLittle);
    }

    void Block::set_bounds(const BlockCtx &ctx, const bool showReason) {
        bounds = bound_candidates(raw_span(), head.offset, head.block_no, ctx, showReason);
    }
    // --------------------------------------------------------------------------------
    using coral::toHex;
    using fmt::format;

    std::string to_string(const BoundInfo &p) {
        return format("{}:{} {}", p.vld, p.foo, toHex0(p.scn));
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const RecordBound &p, const size_t block_no) {
        auto next_blocks = block_no + p.next_blocks;
        return format("{}_{} :> {}{}.@{}{}({})",
                      p.len, to_string(p.boundInfo),
                      coral::key_color, next_blocks, p.next_offset, coral::reset_color,
                      toHex(p.next_offset));
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const std::vector<RecordBound> &offsets, const size_t block_no) {
        auto mapped = offsets | ranges::views::transform([block_no](auto &p) { return to_string(p, block_no); });
        return format("[{}]", fmt::join(mapped, ", "));
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const BlockHead &h, bool trim) {
        return trim
                   ? format("#{:<7} off= {:<3}({}) ", h.block_no, h.offset, toHex(h.offset))
                   : format("#{:<7} "
                            "LSN:{}({}) "
                            "off= {:<3}({}) ",
                            h.block_no,
                            h.log_seq_no, toHex(h.log_seq_no),
                            h.offset, toHex(h.offset)
                   );
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const Block &b, bool trim) {
        return trim
                   ? format("B {}", to_string(b.head, trim))
                   : format("B {} {}",
                            to_string(b.head, true),
                            to_string(b.bounds, b.head.block_no));
    }

    // --------------------------------------------------------------------------------
    void show(const Block &b, const bool dump, std::ostream &os) {
        fmt::println(os, "{}", to_string(b));
        if (dump)
            coral::show_HexDump(b.raw);
    }
}
