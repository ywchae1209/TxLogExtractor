#pragma once

#include <fmt/format.h>
#include <optional>
#include <variant>
#include <vector>
#include "../coral_combinator.h"
#include "../elements/layout_kdl.h"
#include "../elements/layout_kdx.h"
#include "../elements/layout_ktubu.h"
#include "change_kdo.h"
#include "change_usp.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

namespace ora {
    using coral::decode_at, coral::decode_At, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::vector, std::variant;
    using namespace combinator;

    /// 5.1 #1 KTUDB (KTU Undo Block) --- Undo 레코드 정보를 기록
    ///- https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Ktudb
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

    // ================================================================================

    /// KDO Undo (Before Image & Supplemental Logging)
    struct KdoUndo {
        Change_kdo       ckdo; // ktb, kdo, ...
        optional<Ch_Usp> uspl;

        static Result<KdoUndo> parse(SpanCursor &ctx);
    };

    /// LOB undo
    struct LobUndo {
        KtbVector ktb;    // # 1: Transaction Layer Redo
        KdliHead  head;   // # 2: LOB Common Header
        KdliElem  elem;   // # 3 ~ N

        RawFlds rest;
        static Result<LobUndo> parse(SpanCursor &ctx);
    };

    /// Index undo
    struct IdxUndo {
        KtbVector       ktb;       // #1: Transaction Layer Redo
        Kdxlk           kdi;       // #2: Index Leaf Header & Key Sizes
        vector<uint8_t> key;       // #3 (0x050103): indKey
        vector<uint8_t> key_data;  // #4 (0x050104): indKeyData / Bitmap
        vector<uint8_t> self_lock; // #5 (0x050105): self_lock
        vector<uint8_t> bitmap;    // #6 (0x050106): bitmap

        static Result<IdxUndo> parse(SpanCursor &ctx);
    };

    /// Truncate Undo
    struct TrnUndo {
        std::optional<uint64_t> newobjd;
    };

    /// skip on purpose
    struct SkipUndo {
        uint16_t op;
        std::string_view desc;
    };

    struct OtherUndo {
        uint16_t op;
        RawFlds rest;
    };

    /// Before Image & Supplemental Logging
    using KtuBody = std::variant<
                                 KdoUndo,        // Kdo
                                 LobUndo,        // Lob
                                 IdxUndo,        // Lob
                                 TrnUndo,        // Truncate
                                 SkipUndo,       // Ignore
                                 OtherUndo >;

    /// {5, 1, "KTUUNDO", "Transaction Undo Change Vector"}
    struct Change_0501 {
        Ktudb udb;        // # 1: KTU Undo Block Header (contain xid)
        Ktubu ubu;        // # 2: KTU Block Header
        KtuBody before{}; //

        uint32_t objn() const { return ubu.header.objn; }
        uint32_t objd() const { return ubu.header.objd; }

        static Result<Change_0501> parse( SpanCursor& ctx );
    };

    // --------------------------------------------------------------------------------
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

    // --------------------------------------------------------------------------------
    inline Result<KdoUndo> KdoUndo::parse(SpanCursor &ctx) {

        KdoUndo undo{};

        if (auto a = parse_kdop(ctx, "0x0B01:ktb/kdo")) undo.ckdo = std::move(*a);
        else return tl::make_unexpected(a.error());

        if (auto a = Ch_Usp::parse(ctx, "0x0B01:uspl")) undo.uspl = std::move(*a);

        return undo;
    }

    inline Result<IdxUndo> IdxUndo::parse( SpanCursor& ctx) {

        IdxUndo out;

        // [# 1] Ktb
        if (auto a = ctx.one_of<KtbVector>("Op0A16:ktb", decode_ktb)) out.ktb = *a;
        else return tl::make_unexpected(a.error());

        // [# 2] Kdilk
        if (auto a = ctx.one_of<Kdxlk>("Op0A16:Kdilk", Kdxlk::decode)) out.kdi = *a;
        else return tl::make_unexpected(a.error());

        // [# 3] (Optional): indKey
        if (auto s = ctx.next("Op0A16:indKey", 1)) out.key.assign(s->begin(), s->end()); else return out;

        // [# 4] (Optional): indKeyData
        if (auto s = ctx.next("Op0A16:indKeyData", 1)) out.key_data.assign(s->begin(), s->end()); else return out;

        // [# 5] (Optional): selflock
        if (auto s = ctx.next("Op0A16:selflock", 1)) out.self_lock.assign(s->begin(), s->end()); else return out;

        // [# 6] (Optional): bitmap
        if (auto s = ctx.next("Op0A16:bitmap, 1")) out.bitmap.assign(s->begin(), s->end());

        return out;
    }

    inline Result<LobUndo> LobUndo::parse(SpanCursor &ctx) {

        LobUndo undo{};
        // [# 1] Ktb
        if (auto a = ctx.one_of<KtbVector>("0x1A01:ktb", decode_ktb)) undo.ktb = *a;
        else return tl::make_unexpected(a.error());

        // [# 2] KdliHead
        if (auto a = ctx.one_of<KdliHead>("0x1A01:head", KdliHead::decode)) undo.head = *a;
        else return tl::make_unexpected(a.error());

        // [# 3] KdliElem
        if (auto a = ctx.one_of<KdliElem>("0x1A01:elem", decode_kdli)) undo.elem = *a;
        else return tl::make_unexpected(a.error());

        if (auto a = ctx.rest("0x1A01:rest"); a) undo.rest = std::move(*a);

        return undo;
    }

    // --------------------------------------------------------------------------------
    /// Ktudb ~ Ktub(ubl/ubu) ~ Ktdo(Ktb ~ KdoHead ~< KdoBody (~ ...)) ~ Ktspl ~ ...
    inline Result<Change_0501> Change_0501::parse( SpanCursor& ctx ) {

        // [# 1] udb (Undo Header)
        auto udb = ctx.one_of<Ktudb>( "Ch5_1:udb", Ktudb::decode);
        if (!udb) return tl::make_unexpected(udb.error());

        // [# 2] ubu (Undo Block Header)
        auto ubu = ctx.one<Ktubu>("Ch5_1:ub", [&](auto s) { return Ktubu::decode(s, ctx.isLittle, false); });
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
        switch (op) { // [# 3 ~ ]
            // 11.1 --> KDO Undo (Row Before Image)
            case 0x0B01: {
                if (auto undo = KdoUndo::parse(ctx)) out.before = std::move(*undo);
                else return tl::make_unexpected(undo.error());
                return out;
            }

            // 26.1 --> LOB Undo :::
            case 0x1A01: {
                if (auto undo = LobUndo::parse(ctx)) out.before = std::move(*undo);
                else return tl::make_unexpected(undo.error());
                return out;
            }

            // 10.22 --> Index Undo
            case 0x0A16: {
                if (auto undo = IdxUndo::parse(ctx)) out.before = std::move(*undo);
                else return tl::make_unexpected(undo.error());
                return out;
            }

            // Undo to ignore --- 1_310_1199112171.dbf
            case 0x0507: out.before = SkipUndo{ .op = op, .desc = "Tx Table update(Tx begin)" }; return out;
            case 0x0A15: out.before = SkipUndo{ .op = op, .desc = "Undo above cache" }; return out;
            case 0x0D09: out.before = SkipUndo{ .op = op, .desc = "Undo for linking block" }; return out;
            case 0x0D17: out.before = SkipUndo{ .op = op, .desc = "Undo for L1 BMB" }; return out;
            case 0x0D19: out.before = SkipUndo{ .op = op, .desc = "Undo for L2 BMB" }; return out;
            case 0x0D1B: out.before = SkipUndo{ .op = op, .desc = "Undo for L3 BMB" }; return out;
            case 0x0D1D: out.before = SkipUndo{ .op = op, .desc = "Undo for pg-tbl seg hdr block" }; return out;
            case 0x0D32: out.before = SkipUndo{ .op = op, .desc = "Undo for NGLOB hash bucket" }; return out;
            case 0x0D34: out.before = SkipUndo{ .op = op, .desc = "Undo for NGLOB free-space" }; return out;
            case 0x0D36: out.before = SkipUndo{ .op = op, .desc = "Undo for NGLOB persistent" }; return out;
            case 0x0D38: out.before = SkipUndo{ .op = op, .desc = "Undo for NGLOB seg hdr" }; return out;
            case 0x0E05: out.before = SkipUndo{ .op = op, .desc = "Undo for Extent op" }; return out;
            case 0x0E08: out.before = SkipUndo{ .op = op, .desc = "Undo for truncate ops, flush object" }; return out;
            case 0x1603: out.before = SkipUndo{ .op = op, .desc = "Undo for space hdr" }; return out;

            default: {
                OtherUndo undo{ .op = op };
                if (auto r = ctx.rest(""); r) undo.rest = std::move(*r);
                out.before = std::move(undo);
                return out;
            }
        }
    }

    // --------------------------------------------------------------------------------
    inline std::string to_string(const Ktudb &u) {
        return fmt::format(
            "UDB {{size: {}, spc: {}, flag: 0x{:04x}, xid: {}.{}.{}, seq: {}, rec: {}}}",
            u.size, u.spc, u.flag, u.xid_usn, u.xid_slt, u.xid_sqn, u.seq, u.rec
        );
    }

    inline std::string to_string(const Ch_Usp &s) {
        return fmt::format(
            "Sup {{spl: {}, col_cnt: {}, col_raws_cnt: {}}}",
            to_string(s.spl), s.col_ids.size(), s.col_raws.size()
        );
    }

    inline std::string to_string(const KdoUndo &u) {
        std::string uspl_str = u.uspl.has_value() ? "\n  " + to_string(u.uspl.value()) : "";
        return fmt::format("{}{}", to_string(u.ckdo), uspl_str);
    }

    inline std::string to_string(const LobUndo &u) {
        return fmt::format(
            "LobUndo {{\n{}\n  {}\n  {}\n  rest_cnt: {}\n  }}",
            to_string(u.ktb), to_string(u.head), to_string(u.elem), u.rest.size()
        );
    }

    inline std::string to_string(const IdxUndo &u) {
        return fmt::format(
            "IdxUndo {{\n{}\n  {}\n  key#: {}, key_data#: {}, self_lock#: {}, bitmap#: {}\n  }}",
            to_string(u.ktb), to_string(u.kdi), u.key.size(), u.key_data.size(), u.self_lock.size(), u.bitmap.size());
    }

    inline std::string to_string(const TrnUndo &u) {
        std::string objd_str = u.newobjd.has_value() ? fmt::format("{}", u.newobjd.value()) : "";
        return fmt::format("TrnUndo {{newobjd: {}}}", objd_str);
    }

    inline std::string to_string(const SkipUndo &u) {
        return fmt::format("Skip :(op: {:04x}) : {}", u.op,  u.desc);
    }

    inline std::string to_string(const OtherUndo &u) {
        return fmt::format("Other {{ op: {:04x} rest_cnt: {}}}", u.op,  u.rest.size());
    }

    inline std::string to_string(const KtuBody &body) {
        return std::visit([](const auto &arg) -> std::string { return to_string(arg); }, body);
    }

    inline std::string to_string(const Change_0501 &c) {
        return fmt::format(
            "Ch 5.1:\n"
            "  {}\n"
            "  {}\n"
            "  --- Before --- {}\n",
            to_string(c.udb),
            to_string(c.ubu),
            to_string(c.before)
        );
    }
}
