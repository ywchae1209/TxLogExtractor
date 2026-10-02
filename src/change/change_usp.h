#pragma once

#include "../coral_decode.h"
#include "../coral_result.h"
#include "tcb/span.hpp"

namespace ora {
    using coral::decode_at, coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional;

    /** KTUSP (KTU Supplemental logging)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Supplementalvector
     * - 해당 DML 레코드의 추가 정보를 기록 (Supplemental Log Vector)
     * - Change 5.1 / 5.6 / 5.11
     *
     * * type : DML 레코드 및 Supplemental 컬럼 데이터의 분할 상태
     *  - Row Chaining/Migration 발생 시 하나의 Logical DML이 여러 DML 레코드로 분할될 수 있음
     *  - Supplemental 컬럼 데이터는 해당 Vector 뒤에 기록되며, 데이터가 긴 경우 여러 Piece로 분할될 수 있음
     *
     *    - 0x08 : DML의 첫 번째 레코드
     *    - 0x04 : DML의 마지막 레코드
     *    - 0x02 : Supplemental 컬럼의 이전 Piece가 존재
     *    - 0x01 : Supplemental 컬럼의 다음 Piece가 존재
     *
     * * kdo_info1
     *   - 0x400  (OSV) : 11.16 Lominer에서 기록, 물리 데이터는 NULL이지만 실제 데이터는 NULL이 아닐 경우( DEFAULT 컬럼 추가 시 발생 )
     *   - 0x800  (HSCFI) : Compress 테이블의 압축된 컬럼 데이터를 기록
     *   - 0x1800 (HSCFI_NSRCI) : Unknown
     *   - 0x1002 (INV_NSRCI) : HSCFI에서 Row chaining/migration 발생 시
     *   - 0x2800 (HSCFI_QMICM) : HSCFI에서 Multi-insert/Multi-delete로 여러 row 데이터를 기록

     * * kdo_info2
     *   - 0x20 (NOSUP) : 기록해야할 Suppelmental 컬럼이 있으나 기록 스킵 (스키마, 테이블, 컬럼명 중 하나가 30자 이상 또는 26ai의 신규 타입)
     *   - 0x40  (NCHG) : Update DML이지만 실제 변경된 데이터가 없을 경우
     */
    struct Ktusp {
        uint8_t type;       //  DML Operation (0x01=UPDATE, 0x02=INSERT, 0x04=DELETE)
        uint8_t fb;         //  DML 레코드 및 Supplemental 컬럼 데이터의 분할 상태
        uint16_t cc;        //  Supplemental 컬럼 개수
        uint16_t objv;      //  테이블의 version
        uint16_t before;    //  Change 5.1의 Undo 컬럼 시작 번호
        uint16_t after;     //  Change 11.n의 Redo 컬럼 시작 번호
        uint16_t kdo_info1; //  kdo_info1
        uint32_t kdo_info2; //  kdo_info2
        uint32_t bdba;      //  Row Chaining 시 Head Row의 DBA
        uint16_t slot;      //  Row Chaining 시 Head Row의 Slot

        static Result<Ktusp> decode(tcb::span<const char> buf, bool isLittle);
    };

    struct Ch_Usp {
        size_t span_count;
        Ktusp usp;
        std::vector<uint16_t> col_ids;
        std::vector<uint16_t> col_sizes;
        RawFlds col_raws;

        static Result<Ch_Usp> parse(SpanCursor &ctx, const std::string_view name);
    };

    // --------------------------------------------------------------------------------
    /// undo-supplemental
    inline Result<Ktusp> Ktusp::decode(tcb::span<const char> buf, bool isLittle) {
        constexpr auto sz_Ktusp = 20;

        if (buf.size() < sz_Ktusp) {
            return err_of(fmt::format("[Ktusp] buf({}) < {}", buf.size(), sz_Ktusp));
        }

        Ktusp out {
            .type       = decode_At<uint8_t > (buf, isLittle, 0),  // type
            .fb         = decode_At<uint8_t > (buf, isLittle, 1),  // fb
            .cc         = decode_At<uint16_t> (buf, isLittle, 2),  // cc
            .objv       = decode_At<uint16_t> (buf, isLittle, 4),
            .before     = decode_At<uint16_t> (buf, isLittle, 6),  // before
            .after      = decode_At<uint16_t> (buf, isLittle, 8),  // after
            .kdo_info1  = decode_At<uint16_t> (buf, isLittle, 12),
            .kdo_info2  = decode_At<uint32_t> (buf, isLittle, 16),
        };
        if (buf.size() >= 26) {
            // when row-chained
            out.bdba    = decode_At<uint32_t> (buf, isLittle, 20);    // bdba
            out.slot    = decode_At<uint16_t> (buf, isLittle, 24);    // slot
        }
        return out;
    }

    inline Result<Ch_Usp> Ch_Usp::parse (SpanCursor &ctx, const std::string_view name) {

        Ch_Usp out{.span_count = ctx.remaining()};
        if (out.span_count == 0) return out;  // empty is ok

        // [#1] Ktusp
        if (auto a = ctx.one_of<Ktusp>(name, Ktusp::decode)) out.usp = std::move(*a);
        else return tl::make_unexpected(a.error());

        if (out.span_count == 1) return out;

        const auto col_cnt = out.usp.cc;

        // [#2] col-indices
        if (auto a = ctx.one_array<uint16_t>("Ch_Usp:#2", col_cnt)) out.col_ids = std::move(*a);
        else return tl::make_unexpected(a.error());

        // [#3] col-sizes
        if (auto a = ctx.one_array<uint16_t>("Ch_Usp:#3", col_cnt)) out.col_sizes = std::move(*a);
        else return tl::make_unexpected(a.error());

        // [#4~ N] col-raws
        if (auto a = ctx.raws_by("Ch_Usp:#4", out.col_sizes)) out.col_raws = std::move(*a);
        else return tl::make_unexpected(a.error());

        return out;
    }
    // --------------------------------------------------------------------------------
    inline std::string to_string(const Ktusp &s) {
        return fmt::format(
            "USP {{type: 0x{:02x}, fb: 0x{:02x}, cc: {}, objv: {}, before: {}, after: {}, "
            "kdo_info1: 0x{:04x}, kdo_info2: 0x{:08x}, bdba: 0x{:08x}, slot: {}}}",
            s.type, s.fb, s.cc, s.objv, s.before, s.after,
            s.kdo_info1, s.kdo_info2, s.bdba, s.slot
        );
    }

    inline std::string to_string(const Ch_Usp &s) {
        if (s.span_count== 0)
            return "SUP: Empty";

        const auto msg = s.usp.cc == s.col_raws.size() ? ""  : "potential-Error";

        return fmt::format(
            "SUP: span#: {}\n    {}\n    RAWS: col#: {}, col_raws#: {} {}\n{}",
            s.span_count, to_string(s.usp),
            s.col_ids.size(), s.col_raws.size(), msg,
            to_string(s.col_raws)
        );
    }

}
