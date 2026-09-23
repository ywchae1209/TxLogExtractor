#pragma once
#include <fmt/format.h>
#include <optional>
#include <variant>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::decode_at, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::variant;

    using namespace combinator;

    /** 24.4 #1 KTMRM (Media Recovery Marker)
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
        uint16_t xid_usn;       //  Transaction ID undo segment number
        uint16_t xid_slot;      //  Transaction ID slot
        uint32_t xid_sqn;       //  Transaction ID sequence number
        uint16_t type;          //  Recovery marker type

        static Result<Ktmrm> decode(tcb::span<const char> buf, bool isLittle);
    };

    /** 24.4 #2 * KTPTX (Pseudo Transaction Marker)
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

        static Result<Ktptx> decode(tcb::span<const char> buf, bool isLittle);
    };

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
    };


    // --------------------------------------------------------------------------------
    /// Type 0x06 / 0x07 (Pseudo Transaction)
    struct PseudoTx {
        Ktptx ptxh;                     // # 2: Pseudo Transaction Header (28 Bytes)
    };

    /// Type 0x0E (Min Active Transaction)
    struct MinActiveTx {
        uint64_t min_act_tx_scn;         // # 2: MinActTxScn (8 Bytes SCN)
    };

    /// Type 0x0C (Transaction Finalized)
    struct FinalizedTx {
        Kttxs txs;                    // # 2: Transaction State Vector (4 Bytes)
        RawFld unknown;                 // # 3: Unknown / Padding (4 Bytes)
        optional<uint64_t> tx_start_scn;// # 4: TxStartScn (8 Bytes SCN)
    };

    // fallback
    struct Unknown {
        RawFlds raws;
    };

    using MrmBody = variant<            //
            PseudoTx,                   // Type 0x06 / 0x07 (Pseudo Transaction)
            FinalizedTx,                // Type 0x0C (Transaction Finalized)
            MinActiveTx,                // Type 0x0E (Min Active Transaction)
            Unknown// fallback
            >;

    // --------------------------------------------------------------------------------
    /// {24, 4, "KRVMISC", "Miscellaneous info"},
    struct Change_2404 {
        Ktmrm marker; // # 1: Recovery Marker Header (16 Bytes, 필수)
        MrmBody body; // # 2~
    };


    // ================================================================================
    inline Result<Ktmrm> Ktmrm::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 16) {
            return err_of(fmt::format("[Ktmrm] buf-size ({}) < {}", buf.size(), 16));
        }

        return Ktmrm{
            .xid_usn  = decode_At<uint16_t>(buf, isLittle, 4),
            .xid_slot = decode_At<uint16_t>(buf, isLittle, 6),
            .xid_sqn  = decode_At<uint32_t>(buf, isLittle, 8),
            .type     = decode_At<uint16_t>(buf, isLittle, 12),
        };
    }


    inline Result<Ktptx> Ktptx::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 28) {
            return err_of(fmt::format("[Ktptx] buf-size ({}) < {}", buf.size(), 28));
        }

        return Ktptx{
            .objn            = decode_At<uint32_t>(buf, isLittle, 0),
            .objv            = decode_At<uint32_t>(buf, isLittle, 4),
            .pseudo_xid_usn  = decode_At<uint16_t>(buf, isLittle, 8),
            .pseudo_xid_slot = decode_At<uint16_t>(buf, isLittle, 10),
            .pseudo_xid_sqn  = decode_At<uint32_t>(buf, isLittle, 12),
            .parent_xid_usn  = decode_At<uint16_t>(buf, isLittle, 16),
            .parent_xid_slot = decode_At<uint16_t>(buf, isLittle, 18),
            .parent_xid_sqn  = decode_At<uint32_t>(buf, isLittle, 20),
        };
    }

    [[nodiscard]] inline Result<Kttxs> decode_kttxs(tcb::span<const char> buf, bool isLittle) {
        if ( buf.size() < 4) {
            return err_of(fmt::format("[Kttxs] buf-size ({}) < {}", buf.size(), 4));
        }

        return Kttxs{
            .outcome   = decode_At<uint8_t>(buf, isLittle, 0),
            .old_state = decode_At<uint8_t>(buf, isLittle, 1),
            .new_state = decode_At<uint8_t>(buf, isLittle, 2),
        };
    }

    [[nodiscard]] inline Result<Change_2404> parse_2404( SpanCursor& ctx) {

        Change_2404 out;

        // [# 1] rmh (Media Recovery Marker)
        auto rmh = ctx.one_of<Ktmrm>("Ch24_4:mrm", Ktmrm::decode);
        if (!rmh) return tl::make_unexpected(rmh.error());
        out.marker = *rmh;

        if (!ctx.has_remaining()) return out;

        // ----------------------------
        switch (out.marker.type) {
            case 0x06:
            case 0x07: {

                // [# 2] Pseudo Transaction Marker
                auto o_ktptx = ctx.one_of<Ktptx>("Ch24_4:ktptx", Ktptx::decode);
                if (!o_ktptx) return tl::make_unexpected(o_ktptx.error());
                out.body = PseudoTx{ .ptxh = *o_ktptx };
                return out;
            }

            case 0x0C: {
                FinalizedTx fin;
                // [# 2] Transaction State Vector
                auto o_kttxs = ctx.one_of<Kttxs>("Ch24_4:kttxs", decode_kttxs);
                if (!o_kttxs) return tl::make_unexpected(o_kttxs.error());
                fin.txs = *o_kttxs;

                // [# 3] Unknown
                auto uk = ctx.one_raw("Ch24_4:Unk3");
                if (!uk) return tl::make_unexpected(uk.error());

                // [# 4] TxStartScn (8 Bytes SCN, Optional)
                auto o_scn = ctx.one_scn8("Ch24_4:TxStartScn", ctx.isLittle);
                if (o_scn) fin.tx_start_scn = *o_scn;

                out.body = fin;
                return out;
            }

            case 0x0E: {
                // [# 2] MinActTxScn (8 Bytes SCN)
                auto o_scn = ctx.one_scn8("Ch24_4:MinActTxScn", ctx.isLittle);
                if (!o_scn) return tl::make_unexpected(o_scn.error());

                out.body = MinActiveTx{ .min_act_tx_scn = *o_scn };
                return out;
            }

            default: {
                // [# 2~ ] Unknowns
                if (auto rs = ctx.rest("Ch24_4:unknown-type"))
                    out.body = Unknown{ std::move(*rs) };
                return out;
            }
        }
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Ktmrm& a) {
        return fmt::format("Ktmrm: "
                           "xid: 0x{:x}.0x{:x}.{} type: 0x{:02x}",
                           a.xid_usn, a.xid_slot, a.xid_sqn, a.type);
    }

    static std::string to_string(const Ktptx& a) {
        return fmt::format("Ktptx: "
                           "objn: {} objv: {} "
                           "pseudo_xid: 0x{:x}.0x{:x}.{} "
                           "parent_xid: 0x{:x}.0x{:x}.{}",
                           a.objn, a.objv,
                           a.pseudo_xid_usn, a.pseudo_xid_slot, a.pseudo_xid_sqn,
                           a.parent_xid_usn, a.parent_xid_slot, a.parent_xid_sqn);
    }
    static std::string to_string(const Kttxs& a) {
        return fmt::format("Kttxs: "
                           "outcome: {} old_state: {} new_state: {}",
                           a.outcome, a.old_state, a.new_state);
    }
    static std::string to_string(const PseudoTx& a) { return to_string(a.ptxh); }

    static std::string to_string(const MinActiveTx& a) {
        return fmt::format("MinActiveTx: min_act_tx_scn: 0x{:016x}", a.min_act_tx_scn);
    }

    static std::string to_string(const FinalizedTx& a) {
        std::string result = to_string(a.txs);
        if (a.tx_start_scn) {
            fmt::format_to(std::back_inserter(result), " tx_start_scn: 0x{:016x}", *a.tx_start_scn);
        }
        return result;
    }

    static std::string to_string(const Unknown& a) { return to_string(a.raws); }

    static std::string to_string(const MrmBody& body) {
        return std::visit(
            [](const auto& arg) { return to_string(arg); },
            body);
    }

    static std::string to_string(const Change_2404& a) {
        return fmt::format("Ch 24.4: {}\n"
                           "         {}",
                           to_string(a.marker),
                           to_string(a.body));
    }
}