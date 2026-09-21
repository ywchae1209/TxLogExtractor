#pragma once

#include <fmt/format.h>
#include <optional>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_5_ktub.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

namespace ora {
    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    /// 5.11 #2 Optional Field: Rollback DBA Information (8 Bytes)
    struct Ktubrb {
        uint32_t prev_dba; // Offset 0: Previous Rollback Block DBA
        uint16_t wrp;      // Offset 4: Wrap Sequence
        uint16_t rec_flg;  // Offset 6: Record Index / Flag
    };
    static std::string to_string(const Ktubrb& a) {
        return fmt::format("Ktubrb : "
                           "prev dba: 0x{:08x} wrp: {} rec_flg: 0x{:04x}",
                           a.prev_dba, a.wrp, a.rec_flg);
    }
    inline Result<Ktubrb> decode_ktubrb(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 8) {
            return err_of(fmt::format("[ktubrb] buf ({}) < {}", buf.size(), 8));
        }

        return Ktubrb{
            .prev_dba = decode_At<uint32_t>(buf, isLittle, 0),
            .wrp      = decode_At<uint16_t>(buf, isLittle, 4),
            .rec_flg  = decode_At<uint16_t>(buf, isLittle, 6)
        };
    }

    /// {5, 11, "KTUBRB", "Rollback DBA in transaction table entry"}
    /// - ub :: Undo Block
    /// - ubrb :: UB Roll-Back (Rollback DBA Info)
    struct Change_0511 {
        Ktub             ub;   // #1
        optional<Ktubrb> ubrb; // #2

        uint32_t objn() const { return ub.header.objn; }
        uint32_t objd() const { return ub.header.objd; }
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_0511> parse_0511( SpanCursor& ctx)
    {
        Change_0511 out;

        // [# 1] ub
        auto ktub = ctx.one<Ktub>("Ch5_11:ub", [&](auto s) { return decode_ktub(s, ctx.isLittle, ctx.over19); });
        if (!ktub) return tl::make_unexpected(ktub.error());
        out.ub = std::move(*ktub);


        // [# 2] ubrb
        auto rb = ctx.one_of<Ktubrb>("Ch5_11:ubrb", decode_ktubrb);
        if (!rb) return out;
        out.ubrb = std::move(*rb);

        return out;
    }
    static std::string to_string(const Change_0511& a) {
        return fmt::format("Ch 5.11: {}\n"
                           "         {}",
                           to_string(a.ub),
                           a.ubrb ? to_string(*a.ubrb) : "");
    }
}