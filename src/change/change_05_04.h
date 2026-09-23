#pragma once
#include <fmt/format.h>
#include <optional>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "tl/expected.hpp"

namespace ora {
    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

#pragma pack(push, 1)

    namespace Ktucm_flag {
        constexpr uint8_t HAS_KTUCF   = 0x02; //
        constexpr uint8_t ROLLED_BACK = 0x04; //
    }

    ///  5.4 #1 * KTUCM (KTU Commit)
    /// - https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktucmvector
    /// -  flag & 2 = Ktucf element가 저장
    /// -  flag & 10 = Unknown (commit) 시간대로 추측
    /// - Transaction ID의 Usn는 Change header의 cls에 기록 → usn = (cls - 15) / 2
    struct Ktucm {
        uint16_t xid_slt; //  XID Slot
        uint32_t xid_sqn; //  XID Sequence Number
        uint8_t srt;      //  SRT
        uint32_t sta;     //  Status
        uint8_t flg;      //  Flag (0x4 = Rollback)

        bool has_ucf() const { return (flg & Ktucm_flag::HAS_KTUCF) != 0; }
        bool is_rolled_back() const { return (flg & Ktucm_flag::ROLLED_BACK) != 0; }

        static Result<Ktucm> decode(tcb::span<const char> buf, bool isLittle);
    };

    ///  5.4 #2 * KTUCF (KTU Commit Free/Space)
    ///- https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktucfvector
    /// -앞 Ktucm의 flag가 0x2일 경우 기록
    struct Ktucf {
        uint32_t uba_dba; //  undo block addr
        uint16_t uba_sqn; //  undo block addr reuse#
        uint8_t uba_rec;  //  undo block addr rec#
        uint16_t ext;     //  extent#
        uint16_t spc;     //  free space
        uint8_t fbi;      //  fbi

        static Result<Ktucf> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// {5, 4, "KTURCM", "Commit transaction (transaction table update) - no undo record"},
    struct Change_0504 {
        Ktucm           ucm; // #1: Commit Header
        optional<Ktucf> ucf; // #2: Commit Free/Space

        bool has_ucf() const noexcept { return ucm.has_ucf() && ucf.has_value(); }
        bool is_rolled_back() const noexcept { return ucm.is_rolled_back(); }

        static Result<Change_0504> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Ktucm> Ktucm::decode(tcb::span<const char> buf, bool isLittle) {
        if ( buf.size() < 20) {
            return err_of(fmt::format("[Ktucm] buf-size ({}) < {}", buf.size(), 20));
        }

        return Ktucm{
            .xid_slt = decode_At<uint16_t>(buf, isLittle, 0),
            .xid_sqn = decode_At<uint32_t>(buf, isLittle, 4),
            .srt     = decode_At<uint8_t >(buf, isLittle, 8),
            .sta     = decode_At<uint32_t>(buf, isLittle, 12),
            .flg     = decode_At<uint8_t >(buf, isLittle, 16),
        };
    }

    inline Result<Ktucf> Ktucf::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 16) {
            return err_of(fmt::format("[Ktucf] buf-size ({}) < {}", buf.size(), 16));
        }

        return Ktucf{
            .uba_dba = decode_At<uint32_t>(buf, isLittle, 0),
            .uba_sqn = decode_At<uint16_t>(buf, isLittle, 4),
            .uba_rec = decode_At<uint8_t >(buf, isLittle, 6),
            .ext     = decode_At<uint16_t>(buf, isLittle, 8),
            .spc     = decode_At<uint16_t>(buf, isLittle, 10),
            .fbi     = decode_At<uint8_t >(buf, isLittle, 12),
        };
    }

    inline Result<Change_0504> Change_0504::parse( SpanCursor &ctx) //, uint16_t usn_hint = 0)
    {
        Change_0504 out;

        // [# 1] ucm
        auto ucm = ctx.one_of<Ktucm>("Ch5_4:ucm", Ktucm::decode);
        if (!ucm) return tl::make_unexpected(ucm.error());
        out.ucm = std::move(*ucm);

        // [# 2] ucf
        if (ucm->has_ucf() && ctx.no_remaining()) return err_of("Ch5_4: ucf flag is set, but no remaining.");

        auto ucf = ctx.one_of<Ktucf>("Ch5_4:ucf", Ktucf::decode);
        if (ucf) out.ucf = std::move(*ucf);

        return out;
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Ktucm& a) {
        return fmt::format("Ktucm : "
                           "slt: 0x{:04x} sqn: 0x{:08x} srt:{} sta: {} flg: 0x{:x}",
                           a.xid_slt, a.xid_sqn, a.srt, a.sta, a.flg);
    }

    static std::string to_string(const Ktucf& a) {
        return fmt::format("Ktucf : "
                           "uba: 0x{:08x}.{:04x}.{:02x} ext:{} spc:{} fbi:{}",
                           a.uba_dba, a.uba_sqn, a.uba_rec,
                           a.ext, a.spc, a.fbi);
    }
    static std::string to_string(const Change_0504& a) {
        return fmt::format("Ch 5.4: {}\n"
                           "        {}",
                           to_string(a.ucm),
                           a.has_ucf() ? to_string(*a.ucf) : "");
    }
}