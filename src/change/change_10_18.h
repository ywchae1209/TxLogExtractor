#pragma once
#include <fmt/format.h>
#include <optional>
#include <string_view>
#include <vector>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../elements/layout_ktb.h"
#include "../coral_combinator.h"
#include "../coral_decode.h"

/// {10, 18, "KDXLUP", "Index redo: update keydata(KDICLUP)"},

namespace ora {

    using coral::decode_At, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::vector;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// 10.18 #2
    struct Kdxlup {
        uint16_t itl{0};       // Offset 0: Transaction Layer Index
        uint16_t sno{0};       // Offset 2: Target Slot Number
        uint16_t row_size{0};  // Offset 4: Row/Update Payload Size
    };

    inline Result<Kdxlup> decode_kdxlup( const tcb::span<const char> buf, const bool isLittle) {

        if (buf.size() < 6) {
            return err_of(fmt::format("[kdxlup] buf size ({}) < 6", buf.size()));
        }

        Kdxlup h;
        h.itl      = decode_At<uint16_t>(buf, isLittle, 0);
        h.sno      = decode_At<uint16_t>(buf, isLittle, 2);
        h.row_size = decode_At<uint16_t>(buf, isLittle, 4);

        return h;
    }

    // --------------------------------------------------------------------------------
    struct Change_1018 {
        KtbVector ktb;
        optional<Kdxlup> hdr;
        optional<RawFld> key_entry_data; // Field 3: Update Key Entry Payload
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_1018> parse_1018( SpanCursor &ctx ) {

        Change_1018 out;

        // [# 1] ktb
        auto o_ktb = ctx.one<KtbVector>("Ch10_18:Ktb", [&](auto s) { return decode_ktb(s, ctx.isLittle); });
        if (!o_ktb) return tl::make_unexpected(o_ktb.error());
        out.ktb = *o_ktb;

        // [# 2] kdxlup
        auto o_hdr = ctx.one<Kdxlup>("Ch10_18:Kdxlup", [&](auto s) { return decode_kdxlup(s, ctx.isLittle); });
        if (!o_hdr) return out;
        out.hdr = *o_hdr;

        // [# 3] Key Data Payload (Update Delta)
        if (auto raw = ctx.raw("Ch10_18:RawFld")) out.key_entry_data = std::move(*raw);

        return out;
    }
}