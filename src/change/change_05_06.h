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
        auto voff = ctx.one<Ktuxvoff>("Ch5_6:voff", [&](auto s) { return decode_ktuxvoff(s, ctx.isLittle); });
        if (!voff) return out;
        out.voff = std::move(*voff);

        return out;
    }
}