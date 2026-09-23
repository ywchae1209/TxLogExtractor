#pragma once

#include <fmt/format.h>
#include <optional>
#include <variant>
#include <vector>
#include "../coral_combinator.h"
#include "../elements/layout_kdli.h"
#include "../elements/layout_ktubu.h"
#include "../elements/layout_5.h"
#include "change_kdo.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

/// {5, 1, "KTUUNDO", "Transaction Undo Change Vector"}, (0x0501 == Opcode 5.1)

namespace ora {
    using coral::decode_at, coral::decode_At, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::vector, std::variant;
    using namespace combinator;

    using coral::Result, coral::err_of;
    using namespace combinator;

    /// 5.1 #1 KTUDB (KTU Undo Block)
    ///- https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Ktudb
    ///- Undo 레코드 정보를 기록
    struct Ktudb {
        uint16_t size;    //  Undo record size
        uint16_t spc;     //  free space(?)
        uint16_t flag;    //
        uint16_t xid_usn; //  xid undo segment num
        uint16_t xid_slt; // xid slot
        uint32_t xid_sqn; // xid sequence number
        uint16_t seq;     // undo block's sqn
        uint8_t rec;      // record# in undo block

        static Result<Ktudb> decode(tcb::span<const char> buf, bool isLittle);
    };

    inline Result<Ktudb> Ktudb::decode(tcb::span<const char> buf, const bool isLittle) {
        if ( buf.size() < 20) {
            return err_of(fmt::format("[Ktudb] buf({}) < {}", buf.size(), 20));
        }

        return Ktudb{
            .size    = decode_At<uint16_t>(buf, isLittle, 0),
            .spc     = decode_At<uint16_t>(buf, isLittle, 2),
            .flag    = decode_At<uint16_t>(buf, isLittle, 4),
            .xid_usn = decode_At<uint16_t>(buf, isLittle, 8),
            .xid_slt = decode_At<uint16_t>(buf, isLittle, 10),
            .xid_sqn = decode_At<uint32_t>(buf, isLittle, 12),
            .seq     = decode_At<uint16_t>(buf, isLittle, 16),
            .rec     = decode_At<uint8_t >(buf, isLittle, 18),
        };
    }

    struct Ch_sup {
        Ktusp spl;
        std::vector<uint16_t> col_ids;
        std::vector<uint16_t> col_sizes;
        RawFlds col_raws;
    };

    /// todo ::: not finished
    inline Result<Ch_sup> parse_ksup (SpanCursor &ctx, const std::string_view name, const bool isLittle) {

        // [#1] Ktusp
        auto usp = ctx.one_of<Ktusp>(name, decode_ktusp);
        if (!usp) return tl::make_unexpected(usp.error());

        const auto col_cnt = usp->cc;

        // [#2] col-indices
        auto col_ids = ctx.one_array<uint16_t>(name, col_cnt);
        if (!col_ids) return tl::make_unexpected(usp.error());

        // [#3] col-sizes
        auto col_sizes = ctx.one_array<uint16_t>(name, col_cnt);
        if (!col_sizes) return tl::make_unexpected(usp.error());

        // [#4~ N] col-raws
        auto col_raws = ctx.raws_by(name, *col_sizes);
        if (!col_raws) return tl::make_unexpected(usp.error());

        return Ch_sup{
            .spl = std::move(*usp),
            .col_ids = std::move(*col_ids),
            .col_sizes = std::move(*col_sizes),
            .col_raws = std::move(*col_raws)
        };
    }
    // --------------------------------------------------------------------------------
    /// KDO Undo (Before Image & Supplemental Logging)
    struct KdoUndo {
        Ch_ktdo          ktdo; // ktb, kdo
        optional<Ch_sup> uspl;
    };

    struct KliUndo {
        KtbVector ktb;    // # 1: Transaction Layer Redo
        KdliHead  head;   // # 2: LOB Common Header
        KdliElem  elem;  // # 3 ~ N

        RawFlds rest;
    };

    /// Truncate Undo
    struct TrnUndo {
        std::optional<uint64_t> newobjd;
    };

    struct OtherUndo {
        RawFlds rest;
    };

    /// Before Image & Supplemental Logging
    using KtuBody = std::variant<std::monostate, //
                                 KdoUndo,        // Kdo
                                 KliUndo,        // Lob
                                 TrnUndo,        // Truncate
                                 OtherUndo >;

    // --------------------------------------------------------------------------------
    struct Change_0501 {
        Ktudb udb;                   // # 1: KTU Undo Block Header (contain xid)
        Ktubu  ubu;                    // # 2: KTU Block Header

        KtuBody before{};

        uint32_t objn() const { return ubu.header.objn; }
        uint32_t objd() const { return ubu.header.objd; }
    };

    // --------------------------------------------------------------------------------
    /// Ktudb ~ Ktub(ubl/ubu) ~ Ktdo(Ktb ~ KdoHead ~< KdoBody (~ ...)) ~ Ktspl ~ ...
    [[nodiscard]] inline Result<Change_0501> parse_0501( SpanCursor& ctx ) {

        // [# 1] udb (Undo Header)
        auto udb = ctx.one_of<Ktudb>( "Ch5_1:udb", Ktudb::decode);
        if (!udb) return tl::make_unexpected(udb.error());

        // [# 2] ubu (Undo Block Header)
        auto ubu = ctx.one<Ktubu>("Ch5_1:ub", [&](auto s) { return decode_ktub(s, ctx.isLittle, false); });
        if (!ubu) return tl::make_unexpected(ubu.error());

        Change_0501 out {
            .udb = *udb,
            .ubu = *ubu
        };

        // Incomplete ctx: don't analyze further :: in OLR
        if ((out.ubu.header.flg & (Ktub_Flag::MBU_HEAD | Ktub_Flag::MBU_TAIL | Ktub_Flag::MBU_MID)) != 0) {
            return out;
        }

        const uint16_t op = out.ubu.header.opc;
        switch (op) {
            // 11.1 --> KDO Undo (Row Before Image)
            case 0x0B01: {
                // [# 3 ~ ] (Ktb ~ Kdo ~ ...) ~ (Ksup ~ ...)
                KdoUndo undo{};
                if (auto kdo = parse_kdop(ctx, "Ch5_1:ktdo", ctx.isLittle)) undo.ktdo = std::move(*kdo);
                if (auto sup = parse_ksup(ctx, "Ch5_1:uspl", ctx.isLittle)) undo.uspl = std::move(*sup);
                out.before = std::move(undo);
                return out;
            }

            // 26.1 --> LOB Undo :::
            case 0x1A01: {
            // todo ktb ~ KdliH ~ KdliE

                KliUndo o{};

                // [# 3] Ktb
                auto ktb = ctx.one_of<KtbVector>("Ch5_1:26.1:ktb", decode_ktb);
                if (!ktb) return tl::make_unexpected(ktb.error());
                o.ktb = *ktb;

                // [# 4] KdliHead
                auto head = ctx.one_of<KdliHead>("Ch5_1:26.1:head", decode_kdli_head);
                if (!head) return tl::make_unexpected(head.error());
                o.head = *head;

                // [# 5] KdliElem
                auto elm = ctx.one_of<KdliElem>("Ch5_1:26.1:elem", decode_kdli);
                if (!elm) return tl::make_unexpected(elm.error());;
                o.elem = *elm;

                if (auto r = ctx.rest("Ch5_1:26.1:rest"); r) o.rest = std::move(*r);

                out.before = std::move(o);
                return out;
            }

            // 10.22 --> Key Undo
            // todo ktb ~ KdilK ~ keys ~ keydata/bitmapVec ~ selflock ~ bitmap
            case 0x0A16: {
                KliUndo undo{};
                if (auto r = ctx.rest(""); r) undo.rest = std::move(*r);
                out.before = std::move(undo);
                return out;
            }

            // 14.8 --> Truncate Undo
            case 0x0E08:
                out.before = TrnUndo{
                    .newobjd = ctx.one_scn8_if("Ch5_1:trn:newobjd", true)
                };

            default: {
                OtherUndo undo{};
                if (auto r = ctx.rest(""); r) undo.rest = std::move(*r);
                out.before = std::move(undo);
                return out;
            }
        }
    }
}
