#pragma once
#include <optional>
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_decode.h"
#include "../coral_combinator.h"
#include "../elements/layout_5.h"
#include "../elements/layout_5_ktub.h"

namespace ora {
    using coral::decode_At, coral::Result;
    using std::optional;
    using namespace combinator;

    /// 5.6 #2 KTUXVOFF
    ///
    struct Ktuxvoff {
        uint16_t off; // Rollback Record Offset ??
        uint16_t flg; // Flags
    };
    static std::string to_string(const Ktuxvoff& a) {
        return fmt::format("Ktuxvoff : 0x{:04x} Ktuxvflg: 0x{:04x}", a.off, a.flg);
    }

    inline Result<Ktuxvoff> decode_ktuxvoff(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 8) {
            return err_of(fmt::format("[ktuxvoff] buf ({}) < {}", buf.size(), 8));
        }

        return Ktuxvoff{
            .off = decode_At<uint16_t>(buf, isLittle, 0),
            .flg = decode_At<uint16_t>(buf, isLittle, 4)
        };
    }

    /// {5, 6, "KTUIRB", "Rollback record index in an undo block"},
    /// - ub :: Undo Block
    /// - voff :: Rollback Record offset + flag
    struct Change_0506 {
        Ktub               ub;   // #1
        optional<Ktuxvoff> voff; // #2

        uint32_t objn() const {return ub.header.objn; };
        uint32_t objd() const {return ub.header.objd; };

    };

    // --------------------------------------------------------------------------------
    inline Result<Change_0506> parse_0506(SpanCursor &ctx) {

        Change_0506 out;

        // [# 1] ub
        auto ktub = ctx.one<Ktub>("Ch5_6:ub", [&](auto s) { return decode_ktub(s, ctx.isLittle, true); });
        if (!ktub) return tl::make_unexpected(ktub.error());
        out.ub = std::move(*ktub);

        // [# 2] voff
        auto voff = ctx.one_of<Ktuxvoff>("Ch5_6:voff", decode_ktuxvoff);
        if (!voff) return out;
        out.voff = std::move(*voff);

        return out;
    }

    static std::string to_string(const Change_0506& a) {
        return fmt::format("Ch 5.6: {}\n"
                           "        {}",
                           to_string(a.ub),
                           a.voff ? to_string(*a.voff) : "");
    }


}