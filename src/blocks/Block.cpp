#include "Block.h"

#include "BlockSource.h"
#include "layout_BlockHead.h"
#include "../coral_show.h"

#include "range/v3/view/transform.hpp"

namespace ora {

    //// Block own bytes.
    // --------------------------------------------------------------------------------
    Block Block_of(std::vector<char> raw, const bool isLittle) {

        Block block{};

        block.head = BlockHead_of(raw, isLittle);
        block.raw = std::move(raw);

        return block;
    }

    Block Block_of( std::vector<char> raw, const BlockCtx& ctx, const bool showReason) {

        Block block{};

        block.head = BlockHead_of(raw, ctx.isLittle);
        block.bounds = bound_candidates(raw, block.head.offset, block.head.block_no, ctx, showReason );
        block.raw = std::move(raw);

        return block;
    }

    // --------------------------------------------------------------------------------
    using coral::toHex;
    using fmt::format;

    std::string to_string(const BoundInfo& p) {
        return format("{}:{} {}", p.vld, p.foo, toHex0(p.scn));
    }

    // --------------------------------------------------------------------------------
    std::string to_string( const BHValid &valid) {

        switch (valid) {
            case BHValid::Ok: return "";
            case BHValid::Empty: return "Empty(1)";
            case BHValid::Log_sqn_mismatch: return "Log_sqn_mismatch(2)";
            case BHValid::Error: return "Error(2)";
            default: return "Unknown Error Code (" + std::to_string(static_cast<int>(valid)) + ")";
        }
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const RecordBound &p, const size_t block_no ) {
        auto next_blocks = block_no + p.next_blocks;
        return format("{}_{} :> {}{}.@{}{}({})",
                           p.len, to_string(p.boundInfo),
                           coral::key_color, next_blocks, p.next_offset, coral::reset_color,
                           toHex(p.next_offset));
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const std::vector<RecordBound> &offsets, const size_t block_no ) {

        auto mapped = offsets | ranges::views::transform([block_no](auto &p) { return to_string(p, block_no); });
        return format("[{}]", fmt::join(mapped, ", "));
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const BlockHead &h, bool trim) {
        return trim
                   ? format("#{:<7} off= {:<3}({}) ", h.block_no, h.offset, toHex(h.offset))
                   : format( "#{:<7} "
                       "LSN:{}({}) Sig:{} "
                       "off= {:<3}({}) "
                       "{}",
                       h.block_no,
                       h.log_seq_no, toHex(h.log_seq_no), toHex(h.signature),
                       h.offset, toHex(h.offset),
                       to_string(h.valid) );

    }

    // --------------------------------------------------------------------------------
    std::string to_string( const Block &b, bool trim ) {
        return trim
                   ? format("B {}", to_string(b.head, trim))
                   : format("B {} {}",
                            to_string(b.head, trim),
                            to_string(b.bounds, b.head.block_no));
    }

    // --------------------------------------------------------------------------------
    void show(const Block &b, std::ostream &os ) {
        fmt::println(os, "{}", to_string(b));
        // coral::show_HexDump(b.raw);
    }
}
