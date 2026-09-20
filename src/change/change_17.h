#pragma once
#include "../coral_combinator.h"
#include "../elements/layout_17.h"

namespace ora {

    using  coral::Result;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// {17, 15, "KCVOPHBR", "Heart-beat redo"}
    /// - just a marker. no Elements
    struct Change_1715 {};

    [[nodiscard]] inline Result<Change_1715> parse_1715( SpanCursor& ctx) {
        return Change_1715{};
    }

    // --------------------------------------------------------------------------------
    /// {17, 27, "KTRTH", "Thread Enable Marker / Recover Thread"}, (0x111B == Opcode 17.27)
    /// - rth ::: Thread Enable Marker
    struct Change_1727 {
        Ktrth rth; // # 1
    };

    [[nodiscard]] inline Result<Change_1727> parse_1727( SpanCursor &ctx) {

        // [# 1] rth
        auto rth = ctx.one<Ktrth>("Ch17_27:rth", [&](auto s) { return decode_ktrth(s, ctx.isLittle); });
        if (!rth) return tl::make_unexpected(rth.error());

        return Change_1727{ .rth = std::move(*rth) };
    }

}
