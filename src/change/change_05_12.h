#pragma once
#include <optional>
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_decode.h"
#include "../coral_combinator.h"
#include "../elements/layout_5.h"

namespace ora {
    using coral::decode_At0, coral::Result;
    using std::optional;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// {5, 12, "KTURST", "Change transaction state (in transaction table entry)"},
    struct Change_0512 {
        Ktust              ust;     // #1: Transaction Status Update Info
        optional<uint32_t> status;  // #2: Unknown / Status Flag ??? (4 Bytes, optional)
    };

    // --------------------------------------------------------------------------------
    [[nodiscard]] inline Result<Change_0512> parse_0512(SpanCursor &ctx) {

        Change_0512 out;

        // [# 1] ust
        auto ust = ctx.one<Ktust>("Ch5_12:ust", [&](auto s) { return decode_ktust(s, ctx.isLittle); });
        if (!ust) return tl::make_unexpected(ust.error());
        out.ust = std::move(*ust);

        // [# 2] uint32
        auto state = ctx.one<uint32_t>("Ch5_12:status", 4,[&](auto s){ return decode_At0<uint32_t>(s, ctx.isLittle); });

        if (!state) return out;
        out.status = std::move(*state);
        return out;

    }
}