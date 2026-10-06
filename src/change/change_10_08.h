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

    /// 10.8 #2 Case A Header: kdxln (New Block Header)
    struct Kdxln {
        uint8_t  itl{0};   // Transaction Layer Index
        uint8_t  nco{0};   // Number of Cols
        uint8_t  dsz{0};   // Data Size
        uint8_t  col{0};   // Column Count
        uint8_t  flg{0};   // Flag
        uint32_t nxt{0};   // Next Leaf Block DBA
        uint32_t prv{0};   // Previous Leaf Block DBA

        static Result<Kdxln> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 10.8 #2 Case B Header: kdxlenxt (Split Header - 최소 4 Bytes)
    struct Kdxlnext {
        uint32_t nxt{0};   // Next Leaf Block DBA
        static Result<Kdxlnext> decode(tcb::span<const char> buf, bool isLittle);
    };

    using KdxHead = std::variant<std::monostate, //
                                 Kdxln,          //
                                 Kdxlnext        //
                                 >;


    /// {10, 8, "KDXLNE", "Index redo: init header of leaf block"}
    struct Change_1008 {
        optional<KtbVector>   ktb;           // Case A에서만 인입됨 (Case B는 empty)
        KdxHead hdr;

        // Raw Payloads
        optional<RawFld>     slot_data;       // # 3: Row Index / Slot Table
        optional<RawFld>     key_entry_data;  // # 4: Rows Payload Data

        // Parsed
        optional<vector<uint16_t>>  row_slots; // from # 3

        static Result<Change_1008> parse(SpanCursor &ctx);
    };


    // --------------------------------------------------------------------------------
    inline Result<Kdxln> Kdxln::decode(tcb::span<const char> buf, bool isLittle) {
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

    inline Result<Kdxlnext> Kdxlnext::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 4) {
            return err_of(fmt::format("[kdxlenxt] buf size ({}) < 4", buf.size()));
        }

        Kdxlnext h;
        h.nxt = decode_At<uint32_t>(buf, isLittle, 0);
        return h;
    }


    inline Result<Change_1008> Change_1008::parse( SpanCursor &ctx) {

        Change_1008 out{};

        // [# 1] Field 1: ktb / Branch Check
        auto span1 = ctx.next("Ch10_8:f1");
        if (!span1) return out;

        if (!span1->empty()) {
            // ------------------------------------------------------------------------
            // Case A: Newly Allocated Block (# 1 > 0)
            // ------------------------------------------------------------------------
            auto o_ktb = KtbVector::decode(*span1, ctx.isLittle);
            if (!o_ktb) return tl::make_unexpected(o_ktb.error());
            out.ktb = *o_ktb;

            // [# 2] kdxln (16 bytes min)
            auto o_hdr = ctx.one_of<Kdxln>("Ch10_8:kdxln", Kdxln::decode);
            if (!o_hdr) return out;
            out.hdr = *o_hdr;

        } else {
            // ------------------------------------------------------------------------
            // Case B: Block Being Split (# 1 == 0)
            // ------------------------------------------------------------------------
            // [# 2] kdxlenxt (4 bytes min)
            auto o_hdr = ctx.one_of<Kdxlnext>("Ch10_8:kdxlenxt", Kdxlnext::decode);
            if (!o_hdr) return out;
            out.hdr = *o_hdr;
        }

        // [# 3] Row Index / Slot Data
        if (auto s = ctx.one_raw("Ch10_8:f3"); s) { out.slot_data = std::move(*s); }

        // [# 4] Key Entry Payload Data
        if (auto s = ctx.one_raw("Ch10_8:f4"); s) { out.key_entry_data = std::move(*s); }

        // [-] from #3
        if (out.slot_data) { out.row_slots = out.slot_data->as_array(ctx.isLittle); }

        return out;
    }
    // --------------------------------------------------------------------------------

    inline std::string to_string(const Kdxln &a) {
        return fmt::format("KDXLN : itl: {}, nco: {}, dsz: {}, col: {}, flg: {}, nxt: {}, prv: {}",
                           a.itl, a.nco, a.dsz, a.col, a.flg, a.nxt, a.prv);
    }
    inline std::string to_string(const Kdxlnext &a) {
        return fmt::format("KDXLNEXT : nxt: {}", a.nxt);
    }

    inline std::string to_string(const KdxHead &a) {

        return std::visit(
                [](auto &&x) -> std::string {
                    using T = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<T, std::monostate>) {
                        return "KDXHDR: None";
                    } else {
                        return to_string(x);
                    }
                },
                a);
    }

    inline std::string to_string(const Change_1008 &a) {

        return fmt::format("Ch 10.8:\n{}{}\n  row_slots: {}, key_entry_data:{}",
                           a.ktb ? to_string(*a.ktb) : "",
                           "\n  " + to_string(a.hdr),
                           a.row_slots ? a.row_slots->size() : 0,
                           a.key_entry_data ? "\n  " + to_string(*a.key_entry_data): "" );
    }

}