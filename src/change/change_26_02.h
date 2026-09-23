#pragma once
#include <fmt/format.h>
#include <vector>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_kdli.h"
#include "../elements/layout_ktb.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::decode_At, coral::Result;
    using std::vector;
    using namespace combinator;

    /// {26, 2, "KDLIRREDO", "Generic LOB redo"}, (0x1A02 == Opcode 26.2)
    struct Change_2602 {
        KtbVector ktb;          // # 1: Transaction Layer Redo
        KdliHead head;          // # 2: LOB Common Header
        vector<KdliElem> elems; // # 3 ~ N

        /// ktb ~ kdliHead ~ kdliElems
        [[nodiscard]] static Result<Change_2602> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_2602> Change_2602::parse( SpanCursor &ctx) {

        Change_2602 out;

        // [# 1] Ktb
        auto ktb = ctx.one_of<KtbVector>("Ch26_2:ktb", decode_ktb);
        if (!ktb) return tl::make_unexpected(ktb.error());
        out.ktb = *ktb;

        // [# 2] KdliHead
        auto head = ctx.one_of<KdliHead>("Ch26_2:head", decode_kdli_head);
        if (!head) return tl::make_unexpected(head.error());
        out.head = *head;

        // [# 3 ~ N] KdliElem
        auto elems = ctx.rest_of<KdliElem>("Ch26_2:elems", decode_kdli);
        if (!elems) return tl::make_unexpected(elems.error());
        out.elems = std::move(*elems);

        return out;
    }

    // --------------------------------------------------------------------------------
    // todo :: to_string
}
