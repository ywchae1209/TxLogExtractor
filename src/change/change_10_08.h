#pragma once
#include <fmt/format.h>
#include <cstdint>
#include <optional>
#include <variant>
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
    // Case A Header: kdxln (New Block Header )
    // --------------------------------------------------------------------------------
    struct Kdxln {
        uint8_t  itl{0};   // Offset 0: Transaction Layer Index
        uint8_t  nco{0};   // Offset 1: Number of Cols
        uint8_t  dsz{0};   // Offset 2: Data Size
        uint8_t  col{0};   // Offset 3: Column Count
        uint8_t  flg{0};   // Offset 4: Flag
        uint32_t nxt{0};   // Offset 8: Next Leaf Block DBA
        uint32_t prv{0};   // Offset 12: Previous Leaf Block DBA
    };

    [[nodiscard]] inline Result<Kdxln> decode_kdxln(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 16) {
            return err_of(fmt::format("[kdxln] buf size ({}) < 16", buf.size()));
        }

        return Kdxln {
            .itl = decode_At<uint8_t>(buf, isLittle, 0),
            .nco = decode_At<uint8_t>(buf, isLittle, 1),
            .dsz = decode_At<uint8_t>(buf, isLittle, 2),
            .col = decode_At<uint8_t>(buf, isLittle, 3),
            .flg = decode_At<uint8_t>(buf, isLittle, 4),
            .nxt = decode_At<uint32_t>(buf, isLittle, 8),
            .prv = decode_At<uint32_t>(buf, isLittle, 12)
        };
    }

    // --------------------------------------------------------------------------------
    // Case B Header: kdxlenxt (Split Header - 최소 4 Bytes)
    // --------------------------------------------------------------------------------
    struct Kdxlenxt {
        uint32_t nxt{0};   // Offset 0: Next Leaf Block DBA
    };

    [[nodiscard]] inline Result<Kdxlenxt> decode_kdxlenxt(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 4) {
            return err_of(fmt::format("[kdxlenxt] buf size ({}) < 4", buf.size()));
        }

        Kdxlenxt h;
        h.nxt = decode_At<uint32_t>(buf, isLittle, 0);
        return h;
    }

    using KdxHead = std::variant<std::monostate,Kdxln, Kdxlenxt>;

    // --------------------------------------------------------------------------------
    /// {10, 8, "KDXLNE", "Index redo: init header of leaf block"}, (0x0A08 == Opcode 10.8)
    struct Change_1008 {
        optional<KtbVector>   ktb;        // Case A에서만 인입됨 (Case B는 empty)
        KdxHead hdr;

        // Raw Payloads
        optional<RawFld>     slot_data;       // # 3: Row Index / Slot Table
        optional<RawFld>     key_entry_data;  // # 4: Rows Payload Data

        // Parsed
        optional<vector<uint16_t>>  row_slots; // from # 3
    };

    // --------------------------------------------------------------------------------
    [[nodiscard]] inline Result<Change_1008> parse_1008( SpanCursor &ctx) {

        Change_1008 out{};

        // [# 1] Field 1: ktbRedo / Branch Check
        auto span1 = ctx.next("Ch10_8:f1");
        if (!span1) return out;

        if (!span1->empty()) {
            // ------------------------------------------------------------------------
            // Case A: Newly Allocated Block (# 1 > 0)
            // ------------------------------------------------------------------------
            auto o_ktb = decode_ktb(*span1, ctx.isLittle);
            if (!o_ktb) return tl::make_unexpected(o_ktb.error());
            out.ktb = *o_ktb;

            // [# 2] kdxln (16 bytes min)
            auto o_hdr = ctx.one<Kdxln>("Ch10_8:kdxln", [&](auto s) { return decode_kdxln(s, ctx.isLittle); });
            if (!o_hdr) return out;
            out.hdr = *o_hdr;

        } else {
            // ------------------------------------------------------------------------
            // Case B: Block Being Split (# 1 == 0)
            // ------------------------------------------------------------------------
            // [# 2] kdxlenxt (4 bytes min)
            auto o_hdr = ctx.one<Kdxlenxt>("Ch10_8:kdxlenxt", [&](auto s) { return decode_kdxlenxt(s, ctx.isLittle);
            });
            if (!o_hdr) return out;
            out.hdr = *o_hdr;
        }

        // [# 3] Row Index / Slot Data
        if (auto s = ctx.raw("Ch10_8:f3"); s) { out.slot_data = std::move(*s); }

        // [# 4] Key Entry Payload Data
        if (auto s = ctx.raw("Ch10_8:f4"); s) { out.key_entry_data = std::move(*s); }

        // [-] from #3
        if (out.slot_data) { out.row_slots = out.slot_data->as_array(ctx.isLittle); }


        return out;
    }

}