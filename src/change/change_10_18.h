#pragma once
#include <optional>
#include <string_view>
#include <vector>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_kdx.h"
#include "../elements/layout_ktb.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::decode_At, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::vector;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// {10, 18, "KDXLUP", "Index redo: update keydata(KDICLUP)"},
    struct Change_1018 {
        KtbVector ktb;
        optional<Kdxlup> hdr;
        optional<RawFld> key_entry_data; // #3: Update Key Entry Payload

        static Result<Change_1018> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------

    inline Result<Change_1018> Change_1018::parse( SpanCursor &ctx ) {

        Change_1018 out;

        // [# 1] ktb
        if (auto a = ctx.one_of<KtbVector>("Ch10_18:Ktb", decode_ktb)) out.ktb = *a;
        else return tl::make_unexpected(a.error());

        // [# 2] kdxlup
        if (auto a = ctx.one_of<Kdxlup>("Ch10_18:Kdxlup", Kdxlup::decode)) out.hdr = *a;
        else return out;

        // [# 3] Key Data Payload (Update Delta)
        if (auto raw = ctx.one_raw("Ch10_18:RawFld")) out.key_entry_data = std::move(*raw);

        return out;
    }

    // --------------------------------------------------------------------------------
    // todo :: to_string
    inline std::string to_string(const Change_1018 &h) {
        return "todo";
    }
}