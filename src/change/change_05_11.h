#pragma once

#include <fmt/format.h>
#include <optional>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_ktubu.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

namespace ora {
    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    /// 5.11 #2 Optional Field: Rollback DBA Information (8 Bytes)
    struct Ktubrb {
        uint32_t prev_dba; // Previous Rollback Block DBA
        uint16_t wrp;      // Wrap Sequence
        uint16_t rec_flg;  // Record Index / Flag

        static Result<Ktubrb> decode(tcb::span<const char> buf, bool isLittle) ;
    };

    /// {5, 11, "KTUBRB", "Rollback DBA in transaction table entry"}
    struct Change_0511 {
        Ktubu            ubu;  // #1 Undo Block
        optional<Ktubrb> ubrb; // #2 UB Roll-Back (Rollback DBA Info)

        uint32_t objn() const { return ubu.header.objn; }
        uint32_t objd() const { return ubu.header.objd; }

        static Result<Change_0511> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Ktubrb> Ktubrb::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 8) {
            return err_of(fmt::format("[ktubrb] buf ({}) < {}", buf.size(), 8));
        }

        return Ktubrb{
            .prev_dba = decode_At<uint32_t>(buf, isLittle, 0),
            .wrp      = decode_At<uint16_t>(buf, isLittle, 4),
            .rec_flg  = decode_At<uint16_t>(buf, isLittle, 6)
        };
    }

    inline Result<Change_0511> Change_0511::parse( SpanCursor& ctx)
    {
        Change_0511 out;

        // [# 1] ub
        auto ktub = ctx.one<Ktubu>("Ch5_11:ub", [&](auto s) { return Ktubu::decode_ktub(s, ctx.isLittle, ctx.over19); });
        if (!ktub) return tl::make_unexpected(ktub.error());
        out.ubu = std::move(*ktub);

        // [# 2] ubrb
        auto rb = ctx.one_of<Ktubrb>("Ch5_11:ubrb", Ktubrb::decode);
        if (!rb) return out;
        out.ubrb = std::move(*rb);

        return out;
    }
    // --------------------------------------------------------------------------------
    static std::string to_string(const Ktubrb& a) {
        return fmt::format("Ktubrb : "
                           "prev dba: 0x{:08x} wrp: {} rec_flg: 0x{:04x}",
                           a.prev_dba, a.wrp, a.rec_flg);
    }

    static std::string to_string(const Change_0511& a) {
        return fmt::format("Ch 5.11: {}\n"
                           "         {}",
                           to_string(a.ubu),
                           a.ubrb ? to_string(*a.ubrb) : "");
    }
}