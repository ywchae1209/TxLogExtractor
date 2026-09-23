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

namespace ora {

    using coral::decode_At, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::vector;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// 10.18 #2
    struct Kdxlup {
        uint16_t itl{0};       // Transaction Layer Index
        uint16_t sno{0};       // Target Slot Number
        uint16_t row_size{0};  // Row/Update Payload Size
        static Result<Kdxlup> decode(const tcb::span<const char> buf, const bool isLittle);
    };


    /// {10, 18, "KDXLUP", "Index redo: update keydata(KDICLUP)"},
    struct Change_1018 {
        KtbVector ktb;
        optional<Kdxlup> hdr;
        optional<RawFld> key_entry_data; // Field 3: Update Key Entry Payload

        static Result<Change_1018> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Kdxlup> Kdxlup::decode( const tcb::span<const char> buf, const bool isLittle) {

        if (buf.size() < 6) {
            return err_of(fmt::format("[kdxlup] buf size ({}) < 6", buf.size()));
        }

        return Kdxlup {
            .itl      = decode_At<uint16_t>(buf, isLittle, 0),
            .sno      = decode_At<uint16_t>(buf, isLittle, 2),
            .row_size = decode_At<uint16_t>(buf, isLittle, 4)
        };
    }

    inline Result<Change_1018> Change_1018::parse( SpanCursor &ctx ) {

        Change_1018 out;

        // [# 1] ktb
        auto o_ktb = ctx.one_of<KtbVector>("Ch10_18:Ktb", decode_ktb);
        if (!o_ktb) return tl::make_unexpected(o_ktb.error());
        out.ktb = *o_ktb;

        // [# 2] kdxlup
        auto o_hdr = ctx.one_of<Kdxlup>("Ch10_18:Kdxlup", Kdxlup::decode);
        if (!o_hdr) return out;
        out.hdr = *o_hdr;

        // [# 3] Key Data Payload (Update Delta)
        if (auto raw = ctx.one_raw("Ch10_18:RawFld")) out.key_entry_data = std::move(*raw);

        return out;
    }

    // --------------------------------------------------------------------------------
    // todo :: to_string
}