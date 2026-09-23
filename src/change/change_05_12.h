#pragma once
#include <optional>
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_decode.h"
#include "../coral_combinator.h"
#include "../elements/layout_5.h"

namespace ora {
    using coral::decode_At, coral::decode_At0, coral::Result;
    using std::optional;
    using namespace combinator;

    /** 5.12 #1 KTUST (KTU Status)
      * - https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Ktustvector
      * - 로컬 트랜잭션 상태를 변경 (분산 트랜잭션)
      * - Change 5.12의 첫 번째 Element (24 bytes)
    */
    struct Ktust {
        uint16_t xid_slt;       // Transaction ID slot
        uint16_t unknown0;      //
        uint32_t xid_sqn;       //  Transaction ID sqn
        uint32_t sta;           //  변경할 Transaction status
        uint8_t cflg;           //
        static Result<Ktust> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// {5, 12, "KTURST", "Change transaction state (in transaction table entry)"},
    struct Change_0512 {
        Ktust              ust;     // #1: Transaction Status Update Info
        optional<uint32_t> unknown;  // #2: Unknown / Status Flag ??? (4 Bytes, optional)
        static Result<Change_0512> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------

    inline Result<Ktust> Ktust::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 24) {
            return err_of(fmt::format("[Ktust] buf ({}) < {}", buf.size(), 24));
        }
        return Ktust{

            .xid_slt = decode_At<uint16_t>(buf, isLittle, 0),
            .xid_sqn = decode_At<uint32_t>(buf, isLittle, 4),
            .sta     = decode_At<uint32_t>(buf, isLittle, 8),
            .cflg    = decode_At<uint8_t >(buf, isLittle, 20),
        };
    }

    // --------------------------------------------------------------------------------
    inline Result<Change_0512> Change_0512::parse(SpanCursor &ctx) {

        Change_0512 out;

        // [# 1] ust
        auto ust = ctx.one_of<Ktust>("Ch5_12:ust", Ktust::decode);
        if (!ust) return tl::make_unexpected(ust.error());
        out.ust = std::move(*ust);

        // [# 2] uint32
        auto state = ctx.one_of<uint32_t>("Ch5_12:status", 4, decode_At0<uint32_t>);

        if (!state) return out;
        out.unknown = std::move(*state);
        return out;

    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Ktust& a) {
        return fmt::format("Ktust : "
                           "slt: 0x{:x} sqn: {} sta: 0x{:x} cflg: 0x{:02x}",
                           a.xid_slt, a.xid_sqn, a.sta, a.cflg);
    }

    static std::string to_string(const Change_0512& a) {
        return fmt::format("Ch 5.12: {}\n"
                           "         {}",
                           to_string(a.ust),
                           a.unknown ? fmt::format("unknown: 0x{:x}", *a.unknown) : "");
    }
}