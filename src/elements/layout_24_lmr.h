#pragma once

#include "tcb/span.hpp"
#include "../coral_decode.h"

namespace ora {

    using coral::decode_at;
    using std::optional;

#pragma pack(push, 1)
    /** 24.4 #1
     * KTMRM (Media Recovery Marker)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-RecoveryMarker
     * - Media Recovery Marker (LogMiner transaction finalized marker)
     * - Chagne 24.4는 Supplemental logging 설정 시 생성되는 추가 로깅용 Change
     * - 첫 번째 Element의 Recovery Marker에 따라 하위 Element 구성이 달라짐
     *
     * * type
     * - 0x06: 가상 트랜잭션 시작
     * - 0x07: 가상 트랜잭션 종료
     * - 0x0c : Recovery 최소 복구 시점 (분석 안됨)
     * - 0x0e: 트랜잭션 종결 보조
    */
    struct Ktmrm {
        uint32_t unknown1;      // (4 bytes, offset 0) unknown
        uint16_t xid_usn;       // (2 bytes, offset 4) Transaction ID undo segment number
        uint16_t xid_slot;      // (2 bytes, offset 6) Transaction ID slot
        uint32_t xid_sqn;       // (4 bytes, offset 8) Transaction ID sequence number
        uint16_t type;          // (2 bytes, offset 12) Recovery marker type
        uint16_t unknown2;      // (2 bytes, offset 14)
    };
    static_assert(sizeof(Ktmrm) == 16, "Ktmrm size mismatch");
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktmrm decode_ktmrm0(tcb::span<const char> buf) {
        return Ktmrm{
            .unknown1 = decode_at<uint32_t, IsLittle>(buf, 0),
            .xid_usn  = decode_at<uint16_t, IsLittle>(buf, 4),
            .xid_slot = decode_at<uint16_t, IsLittle>(buf, 6),
            .xid_sqn  = decode_at<uint32_t, IsLittle>(buf, 8),
            .type     = decode_at<uint16_t, IsLittle>(buf, 12),
            .unknown2 = decode_at<uint16_t, IsLittle>(buf, 14)
        };
    }

    [[nodiscard]] inline std::optional<Ktmrm> decode_ktmrm(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktmrm)) { // sizeof(Ktmrm) == 16
            return std::nullopt;
        }

        return isLittle ? decode_ktmrm0<true>(buf)
                        : decode_ktmrm0<false>(buf);
    }


#pragma pack(push, 1)
    /** 24.4 #2
     * KTPTX (Pseudo Transaction Marker)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-DpPesudoTx:가상트랜잭션시작/종료(0x06/0x07)
     * - 가상 트랜잭션 시작/종료 (0x06 / 0x07)
     * - Change 24.4 Pseudo Transaction Marker (28 bytes)
     */
    struct Ktptx {
        uint32_t objn;              // (4 bytes, offset 0) 테이블의 고유 식별 번호
        uint32_t objv;              // (4 bytes, offset 4) 테이블 버전 version

        uint16_t pseudo_xid_usn;    // (2 bytes, offset 8) Pseudo Transaction ID undo segment number
        uint16_t pseudo_xid_slot;   // (2 bytes, offset 10) Pseudo Transaction ID slot
        uint32_t pseudo_xid_sqn;    // (4 bytes, offset 12) Pseudo Transaction ID sequence number

        uint16_t parent_xid_usn;    // (2 bytes, offset 16) Parent Transaction ID undo segment number
        uint16_t parent_xid_slot;   // (2 bytes, offset 18) Parent Transaction ID slot
        uint32_t parent_xid_sqn;    // (4 bytes, offset 20) Parent Transaction ID sequence number

        uint32_t unknown;           // (4 bytes, offset 24)
    };
    static_assert(sizeof(Ktptx) == 28, "Ktptx size mismatch");
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktptx decode_ktptx0(tcb::span<const char> buf) {
        return Ktptx{
            .objn            = decode_at<uint32_t, IsLittle>(buf, 0),
            .objv            = decode_at<uint32_t, IsLittle>(buf, 4),
            .pseudo_xid_usn  = decode_at<uint16_t, IsLittle>(buf, 8),
            .pseudo_xid_slot = decode_at<uint16_t, IsLittle>(buf, 10),
            .pseudo_xid_sqn  = decode_at<uint32_t, IsLittle>(buf, 12),
            .parent_xid_usn  = decode_at<uint16_t, IsLittle>(buf, 16),
            .parent_xid_slot = decode_at<uint16_t, IsLittle>(buf, 18),
            .parent_xid_sqn  = decode_at<uint32_t, IsLittle>(buf, 20),
            .unknown         = decode_at<uint32_t, IsLittle>(buf, 24)
        };
    }

    [[nodiscard]] inline std::optional<Ktptx> decode_ktptx(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktptx)) { // sizeof(Ktptx) == 28
            return std::nullopt;
        }

        return isLittle ? decode_ktptx0<true>(buf)
                        : decode_ktptx0<false>(buf);
    }

#pragma pack(push, 1)

    /** 24.4 #2
     * KTTXS (Transaction State Vector)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-TransactionFinailizedMarker
     * - Transaction Finalized Marker
     */
    struct Kttxs {
        uint8_t outcome;          // (1 byte, offset 0) 1 = Transaction end successfully, 2 = rollback
        uint8_t old_state;        // (1 byte, offset 1) Transaction status (9 = Free, 10 = Active)
        uint8_t new_state;        // (1 byte, offset 2) New transaction status
        uint8_t unknown;          // (1 byte, offset 3) unknown
    };
    static_assert(sizeof(Kttxs) == 4, "Kttxs size mismatch");
#pragma pack(pop)

    template <bool IsLittle>
    inline Kttxs decode_kttxs0(tcb::span<const char> buf) {
        return Kttxs{
            .outcome   = decode_at<uint8_t, IsLittle>(buf, 0),
            .old_state = decode_at<uint8_t, IsLittle>(buf, 1),
            .new_state = decode_at<uint8_t, IsLittle>(buf, 2),
            .unknown   = decode_at<uint8_t, IsLittle>(buf, 3)
        };
    }

    [[nodiscard]] inline std::optional<Kttxs> decode_kttxs(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Kttxs)) { // sizeof(Kttxs) == 4
            return std::nullopt;
        }

        return isLittle ? decode_kttxs0<true>(buf)
                        : decode_kttxs0<false>(buf);
    }

#pragma pack(push, 1)
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
    static_assert(sizeof(Ktdlr) == 16, "Ktdlr size mismatch");
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktdlr decode_ktdlr0(tcb::span<const char> buf) {
        return Ktdlr{
            .objn     = decode_at<uint32_t, IsLittle>(buf, 0),
            .objv     = decode_at<uint16_t, IsLittle>(buf, 4),
            .dlrflags = decode_at<uint16_t, IsLittle>(buf, 6),
            .xid_usn  = decode_at<uint16_t, IsLittle>(buf, 8),
            .xid_slt  = decode_at<uint16_t, IsLittle>(buf, 10),
            .xid_sqn  = decode_at<uint32_t, IsLittle>(buf, 12)
        };
    }

    [[nodiscard]] inline std::optional<Ktdlr> decode_ktdlr(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktdlr)) { // sizeof(Ktdlr) == 16
            return std::nullopt;
        }

        return isLittle ? decode_ktdlr0<true>(buf)
                        : decode_ktdlr0<false>(buf);
    }

}