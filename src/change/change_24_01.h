#pragma once
#include <fmt/format.h>
#include "../coral_combinator.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::Result;
    using namespace combinator;

    /// 24.1 # 1: DDL Identifier Header (24 Bytes)
    struct KrvDDLh {
        uint32_t ddl_version{0};  //
        uint16_t xid_usn{0};      //
        uint16_t xid_slot{0};     //
        uint32_t xid_sqn{0};      //
        uint16_t audit_action{0}; //
        uint16_t flag{0};         // (0:Basic, 1:Chain, 4:Global-Temp-Tbl, 5:Priv-Temp-Tbl, 8:ObjMeta, 9:ColMeta, 10:ChgObj
        uint16_t chain_seq{0};    // Offset 18
        uint16_t total_chains{0}; // Offset 20

        bool is_basic_or_chain() const noexcept { return flag == 0 || flag == 1; }
        bool is_chained() const noexcept { return flag == 1; }
        static Result<KrvDDLh> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// {24, 1, "KRVDDL", "Common portion of DDL"},
    /// - head :: DDL Identifier Header
    /// - raws ::  2 ~ N: Raw Bytes --- todo ::::::
    struct Change_2401 {
        KrvDDLh head; // # 1
        RawFlds raws; // # 2 ~ N

        static Result<Change_2401> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<KrvDDLh> KrvDDLh::decode( const tcb::span<const char> buf, const bool isLittle) {

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
    inline Result<Change_2401> Change_2401::parse( SpanCursor &ctx ) {

        Change_2401 out;

        // [# 1] KrvDDLh
        auto head = ctx.one_of<KrvDDLh>( "Ch24_4:Header", KrvDDLh::decode);
        if (!head) return tl::make_unexpected(head.error());
        out.head = std::move(*head);

        // [# 2 ~ N]
        auto raws = ctx.rest("Ch24_4:RawPayload");
        if (!raws) return out;
        out.raws = std::move(*raws);

        return out;
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const KrvDDLh& a) {
        return fmt::format("KrvDDLh: "
                           "ver: {} xid: 0x{:x}.0x{:x}.{} act: {} flg: {} "
                           "chain: {}/{}",
                           a.ddl_version,
                           a.xid_usn, a.xid_slot, a.xid_sqn,
                           a.audit_action, a.flag,
                           a.chain_seq, a.total_chains);
    }

    static std::string to_string(const Change_2401& a) {
        return fmt::format("Ch 24.1: {}\n"
                           "         {}",
                           to_string(a.head),
                           to_string(a.raws));
    }

}