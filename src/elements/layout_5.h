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
    /** 5.2 #1
     * KTUDH (KTU Undo Header)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Ktudh
     *  - Transaction 시작 시 할당받은 Undo segment 정보를 기록
     *  - Transactoin ID의 Usn는 Change header의 cls에 기록
     *  → usn = (cls - 15) / 2
    */
    struct Ktudh {
        uint16_t xid_slt;  // (2 bytes, offset 0) XID Slot
        uint16_t unknown0; // (2 bytes, offset 2)
        uint32_t xid_sqn;  // (4 bytes, offset 4) XID Sequence Number
        uint32_t uba_dba;  // (4 bytes, offset 8) Undo Block Address: DBA
        uint16_t uba_sqn;  // (2 bytes, offset 12) Undo Block Address: Sequence
        uint8_t uba_rec;   // (1 byte, offset 14) Undo Block Address: Record#
        uint8_t unknown1;  // (1 byte, offset 15)
        uint16_t flag;     // (2 bytes, offset 16) Transaction Flag
        uint16_t size;     // (2 bytes, offset 18) Size
        uint16_t fbi;      // (2 bytes, offset 20) FBI
        uint16_t unknown2; // (2 bytes, offset 22)
        uint16_t pxid_maj; // (2 bytes, offset 24) Parent XID Major (when Distributed Tx)
        uint16_t pxid_min; // (2 bytes, offset 26) Parent XID Minor
        uint32_t pxid_mic; // (4 bytes, offset 28) Parent XID Micro
    };

    static_assert(sizeof(Ktudh) == 32, "Ktudh size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktudh decode_ktudh0(tcb::span<const char> buf) {
        return Ktudh{
            .xid_slt  = decode_at<uint16_t, IsLittle>(buf, 0),
            .unknown0 = decode_at<uint16_t, IsLittle>(buf, 2),
            .xid_sqn  = decode_at<uint32_t, IsLittle>(buf, 4),
            .uba_dba  = decode_at<uint32_t, IsLittle>(buf, 8),
            .uba_sqn  = decode_at<uint16_t, IsLittle>(buf, 12),
            .uba_rec  = decode_at<uint8_t,  IsLittle>(buf, 14),
            .unknown1 = decode_at<uint8_t,  IsLittle>(buf, 15),
            .flag     = decode_at<uint16_t, IsLittle>(buf, 16),
            .size     = decode_at<uint16_t, IsLittle>(buf, 18),
            .fbi      = decode_at<uint16_t, IsLittle>(buf, 20),
            .unknown2 = decode_at<uint16_t, IsLittle>(buf, 22),
            .pxid_maj = decode_at<uint16_t, IsLittle>(buf, 24),
            .pxid_min = decode_at<uint16_t, IsLittle>(buf, 26),
            .pxid_mic = decode_at<uint32_t, IsLittle>(buf, 28)
        };
    }

    [[nodiscard]] inline Result<Ktudh> decode_ktudh(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(Ktudh); buf.size() < sz) {
            return err_of(fmt::format("[Ktudh] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_ktudh0<true>(buf)
                   : decode_ktudh0<false>(buf);
    }

#pragma pack(push, 1)

    namespace Ktucm_flag {
        constexpr uint8_t HAS_KTUCF   = 0x02; //
        constexpr uint8_t ROLLED_BACK = 0x04; //
    }

    /**  5.4 #1
     * KTUCM (KTU Commit)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktucmvector
     *
     * - 트랜잭션 커밋 시 Undo 세그먼트의 트랜잭션 정보를 변경
     *
     * * flag에 따라 다음 Elements의 구성에 변화 있음
     *   -  flag & 2 = Ktucf element가 저장
     *   -  flag & 10 = Unknown (commit) 시간대로 추측
     *
     * - Transaction ID의 Usn는 Change header의 cls에 기록 → usn = (cls - 15) / 2
    */
    struct Ktucm {
        uint16_t xid_slt;        // (2 bytes, offset 0) XID Slot
        uint16_t unknown;        // (2 bytes, offset 2) Reserved
        uint32_t xid_sqn;        // (4 bytes, offset 4) XID Sequence Number
        uint8_t  srt;            // (1 byte, offset 8) SRT
        uint8_t  unknown1[3];    // (3 bytes, offset 9) Reserved
        uint32_t status;         // (4 bytes, offset 12) Status

        uint8_t  flag;           // (1 byte, offset 16) Flag (0x4 = Rollback)
        uint8_t  unknown2[3];    // (3 bytes, offset 17) Reserved


        [[nodiscard]] constexpr bool has_ucf() const { return (flag & Ktucm_flag::HAS_KTUCF) != 0; }
        [[nodiscard]] constexpr bool is_rolled_back() const { return (flag & Ktucm_flag::ROLLED_BACK) != 0; }
    };

    static_assert(sizeof(Ktucm) == 20, "Ktcum size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktucm decode_ktucm0(tcb::span<const char> buf) {
        return Ktucm{
            .xid_slt = decode_at<uint16_t, IsLittle>(buf, 0),
            .unknown = decode_at<uint16_t, IsLittle>(buf, 2),
            .xid_sqn = decode_at<uint32_t, IsLittle>(buf, 4),
            .srt = decode_at<uint8_t, IsLittle>(buf, 8),
            .unknown1 = {
                decode_at<uint8_t, IsLittle>(buf, 9),
                decode_at<uint8_t, IsLittle>(buf, 10),
                decode_at<uint8_t, IsLittle>(buf, 11)
            },
            .status = decode_at<uint32_t, IsLittle>(buf, 12),
            .flag = decode_at<uint8_t, IsLittle>(buf, 16),
            .unknown2 = {
                decode_at<uint8_t, IsLittle>(buf, 17),
                decode_at<uint8_t, IsLittle>(buf, 18),
                decode_at<uint8_t, IsLittle>(buf, 19)
            }
        };
    }

    [[nodiscard]] inline Result<Ktucm> decode_ktucm(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(Ktucm); buf.size() < sz) {
            return err_of(fmt::format("[Ktucm] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_ktucm0<true>(buf)
                   : decode_ktucm0<false>(buf);
    }

#pragma pack(push, 1)
    /**  5.4 #2
     * KTUCF (KTU Commit Free/Space)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktucfvector
     *  - 커밋 후 Undo의 남은 공간에 대한 정보를 기록 (추측)
     *  - Change 5.4의 두 번째 Element
     *  -앞 Ktucm의 flag가 0x2일 경우 기록
     */
    struct Ktucf {
        uint32_t uba_dba;        // (4 bytes, offset 0) undo block addr
        uint16_t uba_sqn;        // (2 bytes, offset 4) undo block addr reuse#
        uint8_t  uba_rec;        // (1 byte, offset 6) undo block addr rec#
        uint8_t  unknown;        // (1 byte, offset 7)
        uint16_t ext;            // (2 bytes, offset 8) extent#
        uint16_t spc;            // (2 bytes, offset 10) free space
        uint8_t  fbi;            // (1 byte, offset 12) fbi
        uint8_t  unknown1[3];    // (3 bytes, offset 13~15)
    };

    static_assert(sizeof(Ktucf) == 16, "Ktucf size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktucf decode_ktucf0(tcb::span<const char> buf) {
        return Ktucf{
            .uba_dba = decode_at<uint32_t, IsLittle>(buf, 0),
            .uba_sqn = decode_at<uint16_t, IsLittle>(buf, 4),
            .uba_rec = decode_at<uint8_t, IsLittle>(buf, 6),
            .unknown = decode_at<uint8_t, IsLittle>(buf, 7),
            .ext     = decode_at<uint16_t, IsLittle>(buf, 8),
            .spc     = decode_at<uint16_t, IsLittle>(buf, 10),
            .fbi     = decode_at<uint8_t, IsLittle>(buf, 12),
            .unknown1= {
                decode_at<uint8_t, IsLittle>(buf, 13),
                decode_at<uint8_t, IsLittle>(buf, 14),
                decode_at<uint8_t, IsLittle>(buf, 15)
            }
        };
    }

    [[nodiscard]] inline Result<Ktucf> decode_ktucf(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(Ktucf); buf.size() < sz) {
            return err_of(fmt::format("[Ktucf] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_ktucf0<true>(buf)
                   : decode_ktucf0<false>(buf);
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
    /** 5.12 #1
      * KTUST (KTU Status)
      *
      * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Ktustvector
      * - 로컬 트랜잭션 상태를 변경 (분산 트랜잭션)
      * - Change 5.12의 첫 번째 Element (24 bytes)
    */
    struct Ktust {
        uint16_t xid_slt;       // (2 bytes, offset 0) Transaction ID slot
        uint16_t unknown0;      // (2 bytes, offset 2)
        uint32_t xid_sqn;       // (4 bytes, offset 4) Transaction ID sqn
        uint32_t sta;           // (4 bytes, offset 8) 변경할 Transaction status
        uint32_t unknown1;      // (4 bytes, offset 12)
        uint32_t unknown2;      // (4 bytes, offset 16)
        uint8_t cflg;           // (1 byte, offset 20)
        uint8_t unknown3[3];    // (3 bytes, offset 21)
    };

    static_assert(sizeof(Ktust) == 24, "Ktust size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktust decode_ktust0(tcb::span<const char> buf) {
        return Ktust{
            .xid_slt = decode_at<uint16_t, IsLittle>(buf, 0),
            .unknown0 = decode_at<uint16_t, IsLittle>(buf, 2),
            .xid_sqn = decode_at<uint32_t, IsLittle>(buf, 4),
            .sta = decode_at<uint32_t, IsLittle>(buf, 8),
            .unknown1 = decode_at<uint32_t, IsLittle>(buf, 12),
            .unknown2 = decode_at<uint32_t, IsLittle>(buf, 16),
            .cflg = decode_at<uint8_t, IsLittle>(buf, 20),
            .unknown3 = {
                decode_at<uint8_t, IsLittle>(buf, 21),
                decode_at<uint8_t, IsLittle>(buf, 22),
                decode_at<uint8_t, IsLittle>(buf, 23)
            }
        };
    }

    [[nodiscard]] inline Result<Ktust> decode_ktust(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(Ktust); buf.size() < sz) {
            return err_of(fmt::format("[Ktust] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_ktust0<true>(buf)
                   : decode_ktust0<false>(buf);
    }

#pragma pack(push, 1)
    /** 5.30 #1
      * KTUDX (KTU Distributed Txn)
      *
      * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ChangeDistributeTxnStatevector
      *  - 글로벌 트랜잭션 상태를 변경 (분산 트랜잭션)
     */
    struct Ktudx {
        uint16_t xid_slt;  // (2 bytes, offset 0) Transaction ID의 slot
        uint16_t unknown0; // (2 bytes, offset 2)
        uint32_t xid_sqn;  // (4 bytes, offset 4) Transaction ID의 sqn
        uint8_t sta;       // (1 byte, offset 8) 변경할 Transaction status
        uint8_t unknown1;  // (1 byte, offset 9)
        uint32_t unknown2; // (4 bytes, offset 10)
        uint32_t unknown3; // (4 bytes, offset 14)
        uint16_t unknown4; // (2 bytes, offset 18)
        uint8_t cfl;       // (1 byte, offset 20) cfl
        uint8_t unknown5;  // (1 byte, offset 21) unknown5
        uint16_t unknown6; // (2 bytes, offset 22) unknown6
    };

    static_assert(sizeof(Ktudx) == 24, "Ktudx size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktudx decode_ktudx0(tcb::span<const char> buf) {
        return Ktudx{
            .xid_slt = decode_at<uint16_t, IsLittle>(buf, 0),
            .unknown0 = decode_at<uint16_t, IsLittle>(buf, 2),
            .xid_sqn = decode_at<uint32_t, IsLittle>(buf, 4),
            .sta = decode_at<uint8_t, IsLittle>(buf, 8),
            .unknown1 = decode_at<uint8_t, IsLittle>(buf, 9),
            .unknown2 = decode_at<uint32_t, IsLittle>(buf, 10),
            .unknown3 = decode_at<uint32_t, IsLittle>(buf, 14),
            .unknown4 = decode_at<uint16_t, IsLittle>(buf, 18),
            .cfl = decode_at<uint8_t, IsLittle>(buf, 20),
            .unknown5 = decode_at<uint8_t, IsLittle>(buf, 21),
            .unknown6 = decode_at<uint16_t, IsLittle>(buf, 22)
        };
    }

    [[nodiscard]] inline Result<Ktudx> decode_ktudx(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(Ktudx); buf.size() < sz) {
            return err_of(fmt::format("[Ktudx] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_ktudx0<true>(buf)
                   : decode_ktudx0<false>(buf);
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

#pragma pack(push, 1)

    /** 5.1 / 5.6 / 5.11 sup
     * KTUSP (KTU Supplemental logging)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Supplementalvector
     * - 해당 DML 레코드의 추가 정보를 기록 (Supplemental Log Vector)
     * - Change 5.1 / 5.6 / 5.11 (고정 헤더 28 bytes + 가변 데이터)
     *
     * * flag : DML 레코드 및 Supplemental 컬럼 데이터의 분할 상태
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
        uint8_t op;         // (1 byte, offset 0) DML Operation (0x01=UPDATE, 0x02=INSERT, 0x04=DELETE)
        uint8_t flag;       // (1 byte, offset 1) DML 레코드 및 Supplemental 컬럼 데이터의 분할 상태
        uint16_t col_cnt;   // (2 bytes, offset 2) Supplemental 컬럼 개수

        uint16_t obj_ver;   // (2 bytes, offset 4) 테이블의 version
        uint16_t undo_col;  // (2 bytes, offset 6) Change 5.1의 Undo 컬럼 시작 번호

        uint16_t redo_col;  // (2 bytes, offset 8) Change 11.n의 Redo 컬럼 시작 번호
        uint16_t unknown1;  // (2 bytes, offset 10)

        uint16_t kdo_info1; // (2 bytes, offset 12) kdo_info1
        uint16_t unknown2;  // (2 bytes, offset 14)

        uint32_t kdo_info2; // (4 bytes, offset 16) kdo_info2

        uint32_t head_dba;  // (4 bytes, offset 20) Row Chaining 시 Head Row의 DBA

        uint16_t head_slot; // (2 bytes, offset 24) Row Chaining 시 Head Row의 Slot
        uint16_t unknown3;  // (2 bytes, offset 26) Padding
    };

    static_assert(sizeof(Ktusp) == 28, "Ktusp size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktusp decode_ktusp0(tcb::span<const char> buf) {
        return Ktusp{
            .op        = decode_at<uint8_t, IsLittle>(buf, 0),
            .flag      = decode_at<uint8_t, IsLittle>(buf, 1),      // fb
            .col_cnt   = decode_at<uint16_t, IsLittle>(buf, 2),     // cc
            .obj_ver   = decode_at<uint16_t, IsLittle>(buf, 4),
            .undo_col  = decode_at<uint16_t, IsLittle>(buf, 6),     // before
            .redo_col  = decode_at<uint16_t, IsLittle>(buf, 8),     // after
            .unknown1  = decode_at<uint16_t, IsLittle>(buf, 10),
            .kdo_info1 = decode_at<uint16_t, IsLittle>(buf, 12),
            .unknown2  = decode_at<uint16_t, IsLittle>(buf, 14),
            .kdo_info2 = decode_at<uint32_t, IsLittle>(buf, 16),
            .head_dba  = decode_at<uint32_t, IsLittle>(buf, 20),    // bdba
            .head_slot = decode_at<uint16_t, IsLittle>(buf, 24),    // slot
            .unknown3  = decode_at<uint16_t, IsLittle>(buf, 26)
        };
    }

    /// undo-supplemental
    [[nodiscard]] inline Result<Ktusp> decode_ktusp(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(Ktusp); buf.size() < sz) {
            return err_of(fmt::format("[Ktusp] buf-size ({}) < {}", buf.size(), sz));
        }

        return isLittle
                   ? decode_ktusp0<true>(buf)
                   : decode_ktusp0<false>(buf);
    }

    /// 5.2 #2 KTEOP (Extent Map Redo)
    struct kteop {
        uint32_t ext;       // Offset  4 ~ 7  : Extent Number (ext#)
        uint32_t ext_size;  // Offset 12 ~ 15 : Extent Size (ext size)
        uint32_t highwater; // Offset 16 ~ 19 : High Water Mark (SETHWM)
        uint32_t offset;    // Offset 24 ~ 27 : Map Offset

        // TODO: 향후 오라클 바이너리 버전별로 위치 파악 시 확장 가능한 필드
        uint32_t blk = 0;             // Block Number (blk#)
        uint32_t blocks_freelist = 0; // #blocks in seg. hdr's freelists
        uint32_t blocks_below = 0;    // #blocks below
        uint32_t mapblk = 0;          // Map Block Address
    };

    // --------------------------------------------------------------------------------
    [[nodiscard]] inline Result<kteop> decode_kteop(tcb::span<const char> buf, bool isLittle) {
        if (auto check = enough(buf, 36, "KTEOP:Redo"); !check) {
            return tl::make_unexpected(check.error());
        }

        return kteop{
            .ext = decode_At<uint32_t>(buf, isLittle, 4),
            .ext_size = decode_At<uint32_t>(buf, isLittle, 12),
            .highwater = decode_At<uint32_t>(buf, isLittle, 16),
            .offset = decode_At<uint32_t>(buf, isLittle, 24)
        };
    }

    inline std::string to_string(const kteop &kte) {
        return fmt::format(
            "kteop redo - redo operation on extent map\n"
            "   SETHWM:       Highwater:: 0x{:08x} ext#: {:<6} blk#: {:<6} ext size: {:<6}\n"
            "  #blocks in seg. hdr's freelists: {}\n"
            "  #blocks below: {:<6}\n"
            "  mapblk  0x{:08x} offset: {:<6}\n",
            kte.highwater, kte.ext, kte.blk, kte.ext_size,
            kte.blocks_freelist,
            kte.blocks_below,
            kte.mapblk, kte.offset
        );
    }

    /// 5.2 #2 PDB Information (4 Bytes)
    struct PdbInfo {
        uint32_t db_id; // PDB DB id
    };

    inline Result<PdbInfo> decode_pdb(tcb::span<const char> buf, bool isLittle) {
        if (auto check = enough(buf, 4, "PDB"); !check) {
            return tl::make_unexpected(check.error());
        }

        return PdbInfo{
            .db_id = decode_At<uint32_t>(buf, isLittle, 0)
        };
    }


    /// 5.6 32 KTUXVOFF (8 Bytes)
    struct Ktuxvoff {
        uint16_t off; // Offset 0: Rollback Record Offset ??
        uint16_t flg; // Offset 4: Flags
    };

    inline Result<Ktuxvoff> decode_ktuxvoff(tcb::span<const char> buf, bool isLittle) {
        if (auto check = enough(buf, 8, "KTUXVOFF"); !check) {
            return tl::make_unexpected(check.error());
        }

        return Ktuxvoff{
            .off = decode_At<uint16_t>(buf, isLittle, 0),
            .flg = decode_At<uint16_t>(buf, isLittle, 4)
        };
    }

    /// 5.11 #2 Optional Field: Rollback DBA Information (8 Bytes) -- todo :: need ckeck
    struct Ktubrb {
        uint32_t prev_dba; // Offset 0: Previous Rollback Block DBA
        uint16_t wrp;      // Offset 4: Wrap Sequence
        uint16_t rec_flg;  // Offset 6: Record Index / Flag
    };

    inline Result<Ktubrb> decode_ktubrb(tcb::span<const char> buf, bool isLittle) {
        if (auto check = enough(buf, 8, "KTUBRB:Field2"); !check) {
            return tl::make_unexpected(check.error());
        }

        return Ktubrb{
            .prev_dba = decode_At<uint32_t>(buf, isLittle, 0),
            .wrp      = decode_At<uint16_t>(buf, isLittle, 4),
            .rec_flg  = decode_At<uint16_t>(buf, isLittle, 6)
        };
    }

}
