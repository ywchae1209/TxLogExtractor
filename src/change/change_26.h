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

    // --------------------------------------------------------------------------------
    /// {26, 2, "KDLIRREDO", "Generic LOB redo"}, (0x1A02 == Opcode 26.2)
    struct Change_2602 {
        KtbVector        ktb;    // # 1: Transaction Layer Redo
        KdliHead         head;   // # 2: LOB Common Header
        vector<KdliElem> elems;  // # 3 ~ N
    };

    // --------------------------------------------------------------------------------
    [[nodiscard]] inline Result<Change_2602> parse_2602( SpanCursor &ctx) {

        Change_2602 out;

        // [# 1] Ktb
        auto ktb = ctx.one<KtbVector>("Ch26_2:ktb", [&](auto s) { return decode_ktb(s, ctx.isLittle); });
        if (!ktb) return tl::make_unexpected(ktb.error());
        out.ktb = *ktb;

        // [# 2] KdliHead
        auto head = ctx.one<KdliHead>("Ch26_2:head", [&](auto s) { return decode_kdli_common(s, ctx.isLittle); });
        if (!head) return tl::make_unexpected(head.error());
        out.head = *head;

        // [# 3 ~ N] KdliElem
        while (ctx.has_remaining()) {
            auto elm = ctx.one<KdliElem>("Ch26_2:elem", [&](auto s) { return decode_kdli(s, ctx.isLittle); });
            if (!elm) break;

            out.elems.push_back(std::move(*elm));
        }

        return out;
    }

    // --------------------------------------------------------------------------------
    /// {26, 6, "KDLIRBIMG", "Direct LOB direct-load redo"}, (0x1A06 == Opcode 26.6)
    struct Change_2606 {
        uint32_t         objd{0};      // # 2 (offset 24): Data Object ID    -- KTB.. ???
        KdliHead         head;         // # 3: LOB Common Header
        vector<KdliElem> elems;        // # 4, 5, 7: Kdli Sub-Elements
        vector<char>     data_payload; // # 6 (opc == 6): Raw LOB Binary Payload
    };

    // --------------------------------------------------------------------------------
    [[nodiscard]] inline Result<Change_2606> parse_2606(SpanCursor &ctx) {

        Change_2606 out;

        // [# 1] just size check
        auto span1 = ctx.next("Ch26_6:f1", 12);
        if (!span1) return err_of("[Ch26_6] span1 < 12 or empty");

        // [# 2]
        auto span2 = ctx.next("Ch26_6:f2", 32);
        if (!span2) return err_of("[Ch26_6] span2 < 32 or empty");

        out.objd = decode_At<uint32_t>(*span2, ctx.isLittle, 24);

        // [# 3] KdliHead
        auto head = ctx.one<KdliHead>("Ch26_6:head", [&](auto s) { return decode_kdli_common(s, ctx.isLittle); });
        if (!head) return tl::make_unexpected(head.error());
        out.head = *head;

        // [# 4 ~ N] KdliElem
        while (ctx.has_remaining()) {
            auto elm = ctx.one<KdliElem>("Ch26_6:kdli", [&](auto s) { return decode_kdli(s, ctx.isLittle); });
            if (!elm) break;
            out.elems.push_back(std::move(*elm));
        }
        return out;
    }
}