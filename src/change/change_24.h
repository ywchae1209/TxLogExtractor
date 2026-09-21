#pragma once
#include <fmt/format.h>
#include "../coral_combinator.h"
#include "../elements/layout_24.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::Result;
    using namespace combinator;

    /// 24.1 # 1: DDL Identifier Header (24 Bytes)
    // ----------------------------------------------------------------------------------------------------
    struct KrvDDLh {
        uint32_t ddl_version{0};  //
        uint16_t xid_usn{0};      //
        uint16_t xid_slot{0};     //
        uint32_t xid_sqn{0};      //
        uint16_t audit_action{0}; //
        uint16_t flag{0};         // (0:Basic, 1:Chain, 4:Global-Temp-Tbl, 5:Priv-Temp-Tbl, 8:ObjMeta, 9:ColMeta, 10:ChgObj
        uint16_t chain_seq{0};    // Offset 18
        uint16_t total_chains{0}; // Offset 20

        [[nodiscard]] constexpr bool is_basic_or_chain() const noexcept { return flag == 0 || flag == 1; }

        // if basic-flag DLL size too large(>4000)
        [[nodiscard]] constexpr bool is_chained() const noexcept { return flag == 1; }
    };

    static std::string to_string(const KrvDDLh& a) {
        return fmt::format("KrvDDLh: "
                           "ver: {} xid: 0x{:x}.0x{:x}.{} act: {} flg: {} "
                           "chain: {}/{}",
                           a.ddl_version,
                           a.xid_usn, a.xid_slot, a.xid_sqn,
                           a.audit_action, a.flag,
                           a.chain_seq, a.total_chains);
    }

    [[nodiscard]] inline Result<KrvDDLh> decode_krvddlh(
        const tcb::span<const char> buf,
        const bool isLittle) {

        if (buf.size() < 18) {
            return err_of("[Ch24.1:Header] size < 18");
        }

        return KrvDDLh {
            .ddl_version  = decode_At<uint32_t>(buf, isLittle, 0),
            .xid_usn      = decode_At<uint16_t>(buf, isLittle, 4),
            .xid_slot     = decode_At<uint16_t>(buf, isLittle, 6),
            .xid_sqn      = decode_At<uint32_t>(buf, isLittle, 8),
            .audit_action = decode_At<uint16_t>(buf, isLittle, 12),
            .flag         = decode_At<uint16_t>(buf, isLittle, 16),
            .chain_seq    = (buf.size() >= 20) ? decode_At<uint16_t>(buf, isLittle, 18) : static_cast<uint16_t>(0),
            .total_chains = (buf.size() >= 22) ? decode_At<uint16_t>(buf, isLittle, 20) : static_cast<uint16_t>(0)
        };
    }
    // --------------------------------------------------------------------------------
    /// {24, 1, "KRVDDL", "Common portion of DDL"},
    /// - head :: DDL Identifier Header
    /// - raws ::  2 ~ N: Raw Bytes --- todo ::::::
    struct Change_2401 {
        KrvDDLh head; // # 1
        RawFlds raws; // # 2 ~ N
    };

    static std::string to_string(const Change_2401& a) {
        return fmt::format("Ch 24.1: {}\n"
                           "         {}",
                           to_string(a.head),
                           to_string(a.raws));
    }

    [[nodiscard]] inline Result<Change_2401> parse_2401( SpanCursor &ctx ) {

        Change_2401 out;

        // [# 1] KrvDDLh
        auto head = ctx.one_of<KrvDDLh>( "Ch24_4:Header", decode_krvddlh);
        if (!head) return tl::make_unexpected(head.error());
        out.head = std::move(*head);

        // [# 2 ~ N]
        auto raws = ctx.rest("Ch24_4:RawPayload");
        if (!raws) return out;
        out.raws = std::move(*raws);

        return out;
    }

    // --------------------------------------------------------------------------------


    /** 24.6 #1
     * KTDLR (Direct Load Redo Entry)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Directloadentry
     * - Direct load entry
     * - Direct Path Insert DML의 트랜잭션 정보를 기록
     */
    struct Ktdlr {
        uint32_t objn;         // (4 bytes, offset 0) Object ID
        uint16_t objv;         // (2 bytes, offset 4) Object version
        uint16_t dlrflags;     // (2 bytes, offset 6)
        uint16_t xid_usn;      // (2 bytes, offset 8) Transaction ID undo segment number
        uint16_t xid_slt;      // (2 bytes, offset 10) Transaction ID slot
        uint32_t xid_sqn;      // (4 bytes, offset 12) Transaction ID sequence number
    };

    static std::string to_string(const Ktdlr& a) {
        return fmt::format("Ktdlr: "
                           "objn: {} objv: {} dlrflags: 0x{:04x} "
                           "xid: 0x{:x}.0x{:x}.{}",
                           a.objn, a.objv, a.dlrflags,
                           a.xid_usn, a.xid_slt, a.xid_sqn);
    }

    [[nodiscard]] inline Result<Ktdlr> decode_ktdlr(tcb::span<const char> buf, bool isLittle) {
        if ( buf.size() < 16) {
            return err_of(fmt::format("[Ktdlr] buf ({}) < {}", buf.size(), 16));
        }

        return Ktdlr{
            .objn     = decode_At<uint32_t>(buf, isLittle, 0),
            .objv     = decode_At<uint16_t>(buf, isLittle, 4),
            .dlrflags = decode_At<uint16_t>(buf, isLittle, 6),
            .xid_usn  = decode_At<uint16_t>(buf, isLittle, 8),
            .xid_slt  = decode_At<uint16_t>(buf, isLittle, 10),
            .xid_sqn  = decode_At<uint32_t>(buf, isLittle, 12)
        };
    }

    /// {24, 6, "KRVDLR10", "Direct load redo 10g"},
    /// - dlr : Direct-Load-Entry
    struct Change_2406 {
        Ktdlr dlr;         // # 1
    };

    static std::string to_string(const Change_2406& a) {
        return fmt::format("Ch 24.6: {}", to_string(a.dlr));
    }

    [[nodiscard]] inline Result<Change_2406> parse_2406(SpanCursor &ctx) {

        // [# 1] dlr
        auto dlr = ctx.one_of<Ktdlr>("Ch24_6:dlr", decode_ktdlr);
        if (!dlr) return tl::make_unexpected(dlr.error());

        return Change_2406{ .dlr = std::move(*dlr) };
    }

    // --------------------------------------------------------------------------------
    /* todo :: {24, 8, "KRVXML", "XML redo - doc or diff - opcode"},

        XML 타입 컬럼의 데이터를 기록한다. (자세한 구조 분석 필요)

        Index	Element	Description
        1       uint8
        2       uint32
        3       uint8[8]    트랜잭션 ID
        4       uint32      Object ID
        5       uint16      Object version (schema version)
        6       uint16      XML 컬럼 번호
        7       uint32      Generic flags / Subtype flags
        8       Byte        XML 데이터
    */

}