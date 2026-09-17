#pragma once
#include <fmt/format.h>
#include "../coral_combinator.h"
#include "../elements/layout_24.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::Result;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// {24, 1, "KRVDDL", "Common portion of DDL"},
    /// - head :: DDL Identifier Header
    /// - raws ::  2 ~ N: Raw Bytes --- todo ::::::
    struct Change_2401 {
        KrvDDLh head; // # 1
        RawFlds raws; // # 2 ~ N
    };

    [[nodiscard]] inline Result<Change_2401> parse_2401( SpanCursor &ctx ) {

        Change_2401 out;

        // [# 1] KrvDDLh
        auto head = ctx.one<KrvDDLh>( "Ch24_4:Header", [&](auto s) { return decode_krv_ddl_header(s, ctx.isLittle); });
        if (!head) return tl::make_unexpected(head.error());
        out.head = std::move(*head);

        // [# 2 ~ N]
        auto raws = ctx.rest("Ch24_4:RawPayload");
        if (!raws) return out;
        out.raws = std::move(*raws);

        return out;
    }

    // --------------------------------------------------------------------------------
    /// {24, 6, "KRVDLR10", "Direct load redo 10g"},
    /// - dlr : Direct-Load-Entry
    struct Change_2406 {
        Ktdlr dlr;         // # 1
    };

    [[nodiscard]] inline Result<Change_2406> parse_2406(SpanCursor &ctx) {

        // [# 1] dlr
        auto dlr = ctx.one<Ktdlr>("Ch24_6:dlr", [&](auto s) { return decode_ktdlr(s, ctx.isLittle); });
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