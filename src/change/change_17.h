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

    inline std::string to_string(Change_1715 &c) {
        return fmt::format("Change_1715");

    }
    // --------------------------------------------------------------------------------
    /// {17, 27, "KTRTH", "Thread Enable Marker / Recover Thread"}, (0x111B == Opcode 17.27)
    /// - rth ::: Thread Enable Marker
    struct Change_1727 {
        Ktrth rth; // # 1
    };

    [[nodiscard]] inline Result<Change_1727> parse_1727( SpanCursor &ctx) {

        // [# 1] rth
        auto rth = ctx.one_of<Ktrth>("Ch17_27:rth", decode_ktrth);
        if (!rth) return tl::make_unexpected(rth.error());

        return Change_1727{ .rth = std::move(*rth) };
    }

    inline std::string to_string(Change_1727 &c) {
        return fmt::format("Ch17_27: {}",to_string(c.rth));

    }

}
