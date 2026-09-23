#pragma once
#include <fmt/format.h>
#include <optional>
#include <string_view>
#include <vector>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_ktb.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

namespace ora {

    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    /// 10.2 #2 Leaf Row Header (kdxle)
    struct Kdxle {
        uint8_t itl{0};                     // Offset 0
        uint8_t code{0};                    // Offset 1 (0: SINGLE, 0x20: ARRAY)
        uint16_t sno{0};                    // Offset 2
        uint16_t row_size{0};               // Offset 4
        uint16_t key_cnt{0};                // Offset 8 (if ARRAY)
        std::vector<uint16_t> target_slots; // Offset 12 ~ (if ARRAY)

        [[nodiscard]] constexpr bool is_single() const noexcept { return code == 0; }
        [[nodiscard]] constexpr bool is_array()  const noexcept { return code == 0x20; }

        static Result<Kdxle> Kdxle::decode(tcb::span<const char> buf, bool isLittle);
    };

    /// {10, 2, "KDXLIN", "Index redo: insert leaf row"}, (0x0A02 == Opcode 10.2)
    struct Change_1002 {
        KtbVector       ktb;
        optional<Kdxle> xle;

        // raw
        optional<RawFld> key_entry_data; // hold: # 3
        optional<RawFld> slot_data;      // hold: # 4

        // ARRAY Insert view
        optional<vector<uint16_t>> key_entry_sizes;          // sizes#5
        optional<vector<tcb::span<const char>>> key_entries; // from #3 (Key Payload)
        optional<vector<uint16_t>> row_slots;                // from #4 (ROWID/Sloot Data List)

        static Result<Change_1002> parse(SpanCursor &ctx);
    };


    // --------------------------------------------------------------------------------
    inline Result<Kdxle> Kdxle::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 6) return err_of(fmt::format("[kdxle] buf size ({}) < 6", buf.size()));

        Kdxle h;
        h.itl      = decode_At<uint8_t>(buf, isLittle, 0);
        h.code     = decode_At<uint8_t>(buf, isLittle, 1);
        h.sno      = decode_At<uint16_t>(buf, isLittle, 2);
        h.row_size = decode_At<uint16_t>(buf, isLittle, 4);

        if (h.is_array()) {
            if (buf.size() >= 10) {
                h.key_cnt = decode_At<uint16_t>(buf, isLittle, 8);
            }
            const auto req = 12 + static_cast<size_t>(h.key_cnt) * 2;
            if (buf.size() >= req) {
                h.target_slots.reserve(h.key_cnt);
                for (uint16_t i = 0; i < h.key_cnt; ++i) {
                    h.target_slots.push_back(decode_At<uint16_t>(buf, isLittle, 12 + (i * 2)));
                }
            }
        }
        return h;
    }

    // --------------------------------------------------------------------------------
    inline Result<Change_1002> Change_1002::parse( SpanCursor &ctx ) {

        Change_1002 out;

        // [# 1] Field 1: ktbRedo
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
    //  tood :: to_string
}