#pragma once
#include "../coral_combinator.h"

namespace ora {

    using  coral::Result;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// {17, 15, "KCVOPHBR", "Heart-beat redo"}
    /// - just a marker. no Elements
    struct Change_1715 {
        static Result<Change_1715> parse( SpanCursor& ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_1715> Change_1715::parse( SpanCursor& ctx) {
        return Change_1715{};
    }

    // --------------------------------------------------------------------------------
    inline std::string to_string(Change_1715 &c) {
        return fmt::format("Change_1715");
    }
}
