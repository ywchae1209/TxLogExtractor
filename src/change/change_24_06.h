#pragma once
#include <fmt/format.h>
#include "../coral_combinator.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::Result;
    using namespace combinator;

    /** 24.6 #1
     * KTDLR (Direct Load Redo Entry)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Directloadentry
     * - Direct load entry
     * - Direct Path Insert DML의 트랜잭션 정보를 기록
     */
    struct Ktdlr {
        uint32_t objn;         // Object ID
        uint16_t objv;         // Object version
        uint16_t dlrflags;     //
        uint16_t xid_usn;      // Transaction ID undo segment number
        uint16_t xid_slt;      // Transaction ID slot
        uint32_t xid_sqn;      // Transaction ID sequence number

        static Result<Ktdlr> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// {24, 6, "KRVDLR10", "Direct load redo 10g"},
    /// - dlr : Direct-Load-Entry
    struct Change_2406 {
        Ktdlr dlr;         // # 1
        static Result<Change_2406> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------

    inline Result<Ktdlr> Ktdlr::decode(tcb::span<const char> buf, bool isLittle) {
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

    inline Result<Change_2406> Change_2406::parse(SpanCursor &ctx) {

        Change_2406 out;

        // [# 1] dlr
        auto dlr = ctx.one_of<Ktdlr>("Ch24_6:dlr", Ktdlr::decode);
        if (!dlr) return tl::make_unexpected(dlr.error());
        out.dlr = std::move(*dlr);

        return out;
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Ktdlr& a) {
        return fmt::format("Ktdlr: "
                           "objn: {} objv: {} dlrflags: 0x{:04x} "
                           "xid: 0x{:x}.0x{:x}.{}",
                           a.objn, a.objv, a.dlrflags,
                           a.xid_usn, a.xid_slt, a.xid_sqn);
    }

    static std::string to_string(const Change_2406& a) {
        return fmt::format("Ch 24.6: {}", to_string(a.dlr));
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
