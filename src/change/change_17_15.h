#pragma once
#include "../coral_combinator.h"

namespace ora {

    using  coral::Result;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// {17, 15, "KCVOPHBR", "Heart-beat redo"} --- just a marker. no Elements
    struct Change_1715 {
        static Result<Change_1715> parse( SpanCursor& ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_1715> Change_1715::parse( SpanCursor& ctx) {
        static constexpr auto out = Change_1715{};
        return out;
    }

    // --------------------------------------------------------------------------------
    inline std::string to_string(Change_1715 &c) {
        static constexpr auto out = "Ch 17.15: Heart-beat";
        return out;
    }
}
