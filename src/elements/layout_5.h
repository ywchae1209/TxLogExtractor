#pragma once

#include "../coral_decode.h"
#include "../coral_result.h"
#include "tcb/span.hpp"

namespace ora {
    using coral::decode_at, coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional;

#pragma pack(push, 1)
    /** 5.1 #1
     * KTUDB (KTU Undo Block)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Ktudb
     * - Undo 레코드 정보를 기록
     */
    struct Ktudb {
        uint16_t size;     // (2 bytes, offset 0) Undo record size
        uint16_t spc;      // (2 bytes, offset 2) free space(?)
        uint16_t flag;     // (2 bytes, offset 4)
        uint16_t unknown0; // (2 bytes, offset 6)
        uint16_t xid_usn;  // (2 bytes, offset 8) xid undo segment num
        uint16_t xid_slt;  // (2 bytes, offset 10) xid slot
        uint32_t xid_sqn;  // (4 bytes, offset 12) xid sequence number
        uint16_t seq;      // (2 bytes, offset 16) undo block's sqn
        uint8_t rec;       // (1 byte, offset 18) record# in undo block
        uint8_t unknown1;  // (1 byte, offset 19)
    };

    static_assert(sizeof(Ktudb) == 20, "Ktudb size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktudb decode_ktudb0(tcb::span<const char> buf) {
        return Ktudb{
            .size    = decode_at<uint16_t, IsLittle>(buf, 0),
            .spc     = decode_at<uint16_t, IsLittle>(buf, 2),
            .flag    = decode_at<uint16_t, IsLittle>(buf, 4),
            .unknown0= decode_at<uint16_t, IsLittle>(buf, 6),
            .xid_usn = decode_at<uint16_t, IsLittle>(buf, 8),
            .xid_slt = decode_at<uint16_t, IsLittle>(buf, 10),
            .xid_sqn = decode_at<uint32_t, IsLittle>(buf, 12),
            .seq     = decode_at<uint16_t, IsLittle>(buf, 16),
            .rec     = decode_at<uint8_t,  IsLittle>(buf, 18),
            .unknown1= decode_at<uint8_t,  IsLittle>(buf, 19)
        };
    }

    [[nodiscard]] inline Result<Ktudb> decode_ktudb(tcb::span<const char> buf, const bool isLittle) {
        if (auto sz = sizeof(Ktudb); buf.size() < sz) {
            return err_of(fmt::format("[Ktudb] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_ktudb0<true>(buf)
                   : decode_ktudb0<false>(buf);
    }



#pragma pack(push, 1)
    /** 5.1 #2
      * KTUBL (KTU Block/Undo Log)
      *
      * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktublvector
      * Undo Record Identifier Vector
      *
      * * op:
      * - 11.1 = Table DML undo
      * - 10.22 = Index Undo
      * - 13.9 = Segment undo
      * - 5.7 = Transaction undo
      *
      * * flag
      * - MBU (Multi BLock Undo) flag
      * - 하나의 Undo 데이터(elements)를 여러 개의 Change 5.1에 분할하여 저장
      * - TAIL → MID → HEAD 순으로 분할 저장
      *    - 0x1 : HEAD
      *    - 0x2 : TAIL
      *    - 0x100 : MID
      */
    struct Ktubl {
        uint32_t objn; // (4 bytes, offset 0) object number: object 고유식별값
        uint32_t objd; // (4 bytes, offset 4) data object number: Object의 실제 데이터가 저장된 세그먼트의 식별자
        uint32_t ts_num; // (4 bytes, offset 8) tablespace number
        uint32_t ts_undo; // (4 bytes, offset 12) undo tablespace
        uint8_t op_maj; // (1 byte, offset 16)
        uint8_t op_min; // (1 byte, offset 17)
        uint8_t slot; // (1 byte, offset 18) slot
        uint8_t rci; // (1 byte, offset 19) record chain idx
        uint16_t mbu_flag; // (2 bytes, offset 20) MBU(Multi-Block-Undo) flag
    };

    static_assert(sizeof(Ktubl) == 22, "Ktubl size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktubl decode_ktubl0(tcb::span<const char> buf) {
        return Ktubl{
            .objn = decode_at<uint32_t, IsLittle>(buf, 0),
            .objd = decode_at<uint32_t, IsLittle>(buf, 4),
            .ts_num = decode_at<uint32_t, IsLittle>(buf, 8),
            .ts_undo = decode_at<uint32_t, IsLittle>(buf, 12),
            .op_maj = decode_at<uint8_t, IsLittle>(buf, 16),
            .op_min = decode_at<uint8_t, IsLittle>(buf, 17),
            .slot = decode_at<uint8_t, IsLittle>(buf, 18),
            .rci = decode_at<uint8_t, IsLittle>(buf, 19),
            .mbu_flag = decode_at<uint16_t, IsLittle>(buf, 20)
        };
    }

    [[nodiscard]] inline Result<Ktubl> decode_ktubl(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(Ktubl); buf.size() < sz) {
            return err_of(fmt::format("[Ktubl] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_ktubl0<true>(buf)
                   : decode_ktubl0<false>(buf);
    }

#pragma pack(push, 1)
    /** 5.1 #2
     * KTUBU (KTU Undo)
     *
     *  https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktubuvector
     * - Undo 레코드의 식별 정보를 기록
     * - Change 5.1의 두 번째 Element (24 bytes)
    */
    struct Ktubu {
        uint32_t objn;     // (4 bytes, offset 0) object number
        uint32_t objd;     // (4 bytes, offset 4) data object number
        uint32_t ts_num;   // (4 bytes, offset 8)
        uint32_t ts_undo;  // (4 bytes, offset 12)
        uint8_t op_maj;    // (1 byte, offset 16)
        uint8_t op_min;    // (1 byte, offset 17)
        uint8_t slot;      // (1 byte, offset 18)
        uint8_t rci;       // (1 byte, offset 19)
        uint16_t mbu_flag; // (2 bytes, offset 20)
        uint16_t unknown;  // (2 bytes, offset 22)
    };

    static_assert(sizeof(Ktubu) == 24, "Ktubu size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktubu decode_ktubu0(tcb::span<const char> buf) {
        return Ktubu{
            .objn = decode_at<uint32_t, IsLittle>(buf, 0),
            .objd = decode_at<uint32_t, IsLittle>(buf, 4),
            .ts_num = decode_at<uint32_t, IsLittle>(buf, 8),
            .ts_undo = decode_at<uint32_t, IsLittle>(buf, 12),
            .op_maj = decode_at<uint8_t, IsLittle>(buf, 16),
            .op_min = decode_at<uint8_t, IsLittle>(buf, 17),
            .slot = decode_at<uint8_t, IsLittle>(buf, 18),
            .rci = decode_at<uint8_t, IsLittle>(buf, 19),
            .mbu_flag = decode_at<uint16_t, IsLittle>(buf, 20),
            .unknown = decode_at<uint16_t, IsLittle>(buf, 22)
        };
    }

    [[nodiscard]] inline Result<Ktubu> decode_ktubu(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(Ktubu); buf.size() < sz) {
            return err_of(fmt::format("[Ktubu] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_ktubu0<true>(buf)
                   : decode_ktubu0<false>(buf);
    }



#pragma pack(push, 1)
    /** * 5.6 #1, 5.11 #1
      * KTURB (KTU Rollback)
      *
      * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-KtubuRollbackvector
      * - Rollback 해야할 DML, Index 등의 트랜잭션 정보를 기록 (LogMiner support)
      * - Change 5.6 #1, Change 5.11 #1 (26 bytes)
      */
    struct Kturb {
        uint32_t objn;    // (4 bytes, offset 0) Object의 고유 식별값
        uint32_t objd;    // (4 bytes, offset 4) Object의 실제 데이터가 저장된 세그먼트의 식별자
        uint32_t ts_num;  // (4 bytes, offset 8) Object가 저장된 Tablespace의 번호
        uint32_t ts_undo; // (4 bytes, offset 12) Tablespace undo
        uint8_t op_maj;   // (1 byte, offset 16) Operation, DML 롤백일 경우 11.1
        uint8_t op_min;   // (1 byte, offset 17) Operation minor
        uint8_t xid_slt;  // (1 byte, offset 18) Transaction ID slot
        uint8_t rci;      // (1 byte, offset 19) Record chain index
        uint16_t flg;     // (2 bytes, offset 20) Flag
        uint16_t xid_sqn; // (2 bytes, offset 22) Transaction ID sqn (사용 불가, sqn: 4바이트, 하위 2바이트만 저장됨)
        uint32_t unknown; // (4 bytes, offset 24) unknown4
    };

    static_assert(sizeof(Kturb) == 28, "Kturb size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Kturb decode_kturb0(tcb::span<const char> buf) {
        return Kturb{
            .objn = decode_at<uint32_t, IsLittle>(buf, 0),
            .objd = decode_at<uint32_t, IsLittle>(buf, 4),
            .ts_num = decode_at<uint32_t, IsLittle>(buf, 8),
            .ts_undo = decode_at<uint32_t, IsLittle>(buf, 12),
            .op_maj = decode_at<uint8_t, IsLittle>(buf, 16),
            .op_min = decode_at<uint8_t, IsLittle>(buf, 17),
            .xid_slt = decode_at<uint8_t, IsLittle>(buf, 18),
            .rci = decode_at<uint8_t, IsLittle>(buf, 19),
            .flg = decode_at<uint16_t, IsLittle>(buf, 20),
            .xid_sqn = decode_at<uint16_t, IsLittle>(buf, 22),
            .unknown = decode_at<uint32_t, IsLittle>(buf, 24)
        };
    }

    [[nodiscard]] inline Result<Kturb> decode_kturb(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(Kturb); buf.size() < sz) {
            return err_of(fmt::format("[Kturb] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_kturb0<true>(buf)
                   : decode_kturb0<false>(buf);
    }

    /** 5.1 / 5.6 / 5.11 sup
     * KTUSP (KTU Supplemental logging)
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
    };

    /// undo-supplemental
    [[nodiscard]] inline Result<Ktusp> decode_ktusp(tcb::span<const char> buf, bool isLittle) {
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
            out.bdba    = decode_At<uint32_t> (buf, isLittle, 20);    // bdba
            out.slot    = decode_At<uint16_t> (buf, isLittle, 24);    // slot
        }

        return out;
    }




}
