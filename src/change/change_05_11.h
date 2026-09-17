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
    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

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
        auto rb = ctx.one<Ktubrb>("Ch5_11:ubrb", [&](auto s) { return decode_ktubrb(s, ctx.isLittle); });
        if (!rb) return out;
        out.ubrb = std::move(*rb);

        return out;
    }
}