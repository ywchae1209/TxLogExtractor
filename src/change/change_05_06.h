#pragma once
#include <fmt/format.h>
#include <optional>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_ktubu.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

namespace ora {
    using coral::decode_At, coral::Result;
    using std::optional;
    using namespace combinator;

    /// 5.6 #2 KTUXVOFF
    struct Ktuxvoff {
        uint16_t off; // Rollback Record Offset ??
        uint16_t flg; // Flags

        static Result<Ktuxvoff> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// {5, 6, "KTUIRB", "Rollback record index in an undo block"},
    struct Change_0506 {
        Ktubu              ubu;  // #1
        optional<Ktuxvoff> voff; // #2  Rollback Record offset + flag

        uint32_t objn() const { return ubu.header.objn; }
        uint32_t objd() const {return ubu.header.objd; };

        static Result<Change_0506> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Ktuxvoff> Ktuxvoff::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 8) {
            return err_of(fmt::format("[ktuxvoff] buf ({}) < {}", buf.size(), 8));
        }

        return Ktuxvoff{
            .off = decode_At<uint16_t>(buf, isLittle, 0),
            .flg = decode_At<uint16_t>(buf, isLittle, 4)
        };
    }

    inline Result<Change_0506> Change_0506::parse(SpanCursor &ctx) {

        Change_0506 out;

        // [# 1] ub
        auto ktub = ctx.one<Ktubu>("Ch5_6:ubu", [&](auto s) { return Ktubu::decode_ktub(s, ctx.isLittle, true); });
        if (!ktub) return tl::make_unexpected(ktub.error());
        out.ubu = std::move(*ktub);

        // [# 2] voff
        auto voff = ctx.one_of<Ktuxvoff>("Ch5_6:voff", Ktuxvoff::decode);
        if (!voff) return out;
        out.voff = std::move(*voff);

        return out;
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Ktuxvoff& a) {
        return fmt::format("Ktuxvoff : 0x{:04x} Ktuxvflg: 0x{:04x}", a.off, a.flg);
    }

    static std::string to_string(const Change_0506& a) {
        return fmt::format("Ch 5.6: {}\n"
                           "        {}",
                           to_string(a.ubu),
                           a.voff ? to_string(*a.voff) : "");
    }
}