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

    /// {26, 6, "KDLIRBIMG", "Direct LOB direct-load redo"}, (0x1A06 == Opcode 26.6)
    struct Change_2606 {
        uint32_t         objd{0};      // # 2 (offset 24): Data Object ID    -- KTB.. ???
        KdliHead         head;         // # 3: LOB Common Header
        vector<KdliElem> elems;        // # 4, 5, 7: Kdli Sub-Elements
        vector<char>     data_payload; // # 6 (opc == 6): Raw LOB Binary Payload


        static Result<Change_2606> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_2606> Change_2606::parse(SpanCursor &ctx) {

        Change_2606 out;

        // [# 1] just size check
        auto span1 = ctx.next("Ch26_6:f1", 12);
        if (!span1) return err_of("[Ch26_6] span1 < 12 or empty");

        // [# 2]
        auto span2 = ctx.next("Ch26_6:f2", 32);
        if (!span2) return err_of("[Ch26_6] span2 < 32 or empty");

        out.objd = decode_At<uint32_t>(*span2, ctx.isLittle, 24);

        // [# 3] KdliHead
        auto head = ctx.one_of<KdliHead>("Ch26_6:head", decode_kdli_head);
        if (!head) return tl::make_unexpected(head.error());
        out.head = *head;

        // [# 3 ~ N] KdliElem
        auto elems = ctx.rest_of<KdliElem>("Ch26_6:elems", decode_kdli);
        if (!elems) return tl::make_unexpected(elems.error());
        out.elems = std::move(*elems);

        return out;
    }

    // --------------------------------------------------------------------------------
    // todo :: to_string
}