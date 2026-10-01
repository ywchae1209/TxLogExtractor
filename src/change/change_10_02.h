#pragma once
#include <fmt/format.h>
#include <optional>
#include <string_view>
#include <vector>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_ktb.h"
#include "../elements/layout_kdx.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

namespace ora {

    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    /// {10, 2, "KDXLIN", "Index redo: insert leaf row"}, (0x0A02 == Opcode 10.2)
    struct Change_1002 {
        KtbVector       ktb;
        optional<Kdxle> xle;

        // raw
        optional<RawFld> key_entry_data; // # 3
        optional<RawFld> slot_data;      // # 4

        // ARRAY Insert view
        optional<vector<uint16_t>> key_entry_sizes;          // #5
        optional<vector<tcb::span<const char>>> key_entries; // from #3 (Key Payload)
        optional<vector<uint16_t>> row_slots;                // from #4 (ROWID/Sloot Data List)

        static Result<Change_1002> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_1002> Change_1002::parse( SpanCursor &ctx ) {

        Change_1002 out;

        // [# 1] Field 1: ktb
        auto ktb = ctx.one_of<KtbVector>("Ch10_2:ktb", decode_ktb);
        if (!ktb) return tl::make_unexpected(ktb.error());
        out.ktb = *ktb;

        // [# 2]  kdxle
        auto xle = ctx.one_of<Kdxle>( "Ch10_2:xle", Kdxle::decode);
        if (!xle) return out;
        out.xle = *xle;

        // [# 3, 4] raw ~ raw
        if (auto s = ctx.one_raw("Ch10_2:f3"); s) out.key_entry_data = std::move(*s); else return out;
        if (auto s = ctx.one_raw("Ch10_2:f4"); s) out.slot_data = std::move(*s); else return out;

        // Mode 1: SINGLE Insert (code == 0) or fallback
        // ----------------------------------------------------------------------------
        if (!out.xle->is_array()) {
            return out;
        }

        // Mode 2: ARRAY Insert (code == 0x20)
        // ----------------------------------------------------------------------------
        // [# 5] sizes
        auto count = out.xle->key_cnt;
        auto o_sizes = ctx.one_array<uint16_t>("Ch10_2:f5", count);
        if (!o_sizes || o_sizes->empty()) return out;

        out.key_entry_sizes = *o_sizes;
        out.key_entries = coral::splits_by(out.key_entry_data->bytes, *out.key_entry_sizes);
        out.row_slots = coral::decode_array<uint16_t>(out.slot_data->bytes, ctx.isLittle, count);
        return out;
    }

    // --------------------------------------------------------------------------------
    inline std::string to_string(const Change_1002& a) {
        return fmt::format("Ch 10.2:\n  {}{}\n  key_entry_sizes#: {}, key_entries#: {}, row_slots#: {}",
                           to_string(a.ktb),
                           a.xle ? "\n  " + to_string(*a.xle) : "",
                           a.key_entry_sizes ? a.key_entry_sizes->size(): 0,
                           a.key_entries ? a.key_entries->size(): 0,
                           a.row_slots ? a.row_slots->size(): 0 );
    }

}