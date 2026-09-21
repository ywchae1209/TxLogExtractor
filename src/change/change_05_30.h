#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_decode.h"
#include "../coral_combinator.h"
#include "../elements/layout_5.h"

namespace ora {
    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    /** 5.30 #1
      * KTUDX (KTU Distributed Txn)
      *
      * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ChangeDistributeTxnStatevector
      *  - 글로벌 트랜잭션 상태를 변경 (분산 트랜잭션)
     */
    struct Ktudx {
        uint16_t xid_slt;  // (2 bytes, offset 0) Transaction ID의 slot
        uint32_t xid_sqn;  // (4 bytes, offset 4) Transaction ID의 sqn
        uint8_t sta;       // (1 byte, offset 8) 변경할 Transaction status
        // 9 ~ 19 :: unknown
        uint8_t cfl;       // (1 byte, offset 20) cfl
        // 21~ 24 :: unknown
    };

    static std::string to_string(const Ktudx& a) {
        return fmt::format("Ktudx: "
                           "slt: 0x{:x} sqn: {} sta: 0x{:x} cfl: 0x{:02x}",
                           a.xid_slt, a.xid_sqn, a.sta, a.cfl);
    }

    [[nodiscard]] inline Result<Ktudx> decode_ktudx(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 24) {
            return err_of(fmt::format("[Ktudx] buf({}) < {}", buf.size(), 24));
        }

        return Ktudx{
            .xid_slt = decode_At<uint16_t>(buf, isLittle, 0),
            .xid_sqn = decode_At<uint32_t>(buf, isLittle, 4),
            .sta     = decode_At<uint8_t >(buf, isLittle, 8),
            .cfl     = decode_At<uint8_t >(buf, isLittle, 20),
        };
    }
    // --------------------------------------------------------------------------------
    /// {5, 30, "KTURCDTS", "Change on-disk state for a distributed transaction (KTUDX)"},
    struct Change_0530 {
        Ktudx            udx;
        optional<RawFld> ext_redo_meta;
        optional<RawFld> global_tx_state;
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_0530> parse_0530( SpanCursor &ctx ) {

        Change_0530 out;

        // [# 1] udx
        auto udx = ctx.one<Ktudx>("Change5_30:udx", [&](auto s) { return decode_ktudx(s, ctx.isLittle); });
        if (!udx) return tl::make_unexpected(udx.error());
        out.udx = *udx;

        // [# 2, 3] ExtRedoMeta ~ Global Tx State
        if (auto r = ctx.one_raw("Ch10_2:f2"); r) out.ext_redo_meta = std::move(*r); else return out;
        if (auto r = ctx.one_raw("Ch10_2:f3"); r) out.global_tx_state= std::move(*r); else return out;

        return out;
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Change_0530& a) {

        std::string out = fmt::format("Ch 5.30: {}", to_string(a.udx));
        if (a.ext_redo_meta) {
            fmt::format_to(std::back_inserter(out), "\n         ext_redo_meta:\n{}", to_string(*a.ext_redo_meta));
        }
        if (a.global_tx_state) {
            fmt::format_to(std::back_inserter(out), "\n         global_tx_state:\n{}", to_string(*a.global_tx_state));
        }

        return out;
    }

}