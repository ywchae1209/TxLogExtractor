#pragma once
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"

#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_oraBlock.h"

namespace ora {

    using coral::Result;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// {19, 1, "KCBLCOLB", "Direct block logging"},
    /// - oraBlock :: Direct-Loaded OraBlock
    /// - elm2 :: unknown -- todo
    struct Change_1901 {
        OraBlock oraBlock; // # 1
        uint8_t elm2;      // # 2

        static Result<Change_1901> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_1901> Change_1901::parse(SpanCursor &ctx) {

        Change_1901 out;

        // [# 1] OraBlock
        auto o_block = ctx.one_of<OraBlock>("Ch19_1:OraBlock", decode_ora_block);
        if (!o_block) return tl::make_unexpected(o_block.error());
        out.oraBlock = std::move(*o_block);

        // [# 2] unknown
        if (auto s = ctx.next("Ch19_1:f2", 1); s) out.elm2 = decode_at<uint8_t, true>(*s, 0);

        return out;
    }
}
