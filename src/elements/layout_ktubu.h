#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <optional>
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_decode.h"
#include "../coral_result.h"
#include "layout_common.h"

namespace ora {

    using coral::decode_at, coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::variant, std::monostate;

    // --- (from: OLR) ---
    namespace Ktub_Flag {
        constexpr uint16_t MBU_HEAD       = 0x0001;
        constexpr uint16_t MBU_TAIL       = 0x0002;
        constexpr uint16_t LAST_SPLIT     = 0x0004;
        constexpr uint16_t BEGIN_TRANS    = 0x0008;
        constexpr uint16_t USER_DONE      = 0x0010;
        constexpr uint16_t TEMP_OBJECT    = 0x0020;
        constexpr uint16_t USER_ONLY      = 0x0040;
        constexpr uint16_t TS_UNDO        = 0x0080;
        constexpr uint16_t MBU_MID        = 0x0100;
        constexpr uint16_t BU_EXT         = 0x0800;
    }

    // --- 1. ~24
    struct Ktubu_base {
        uint32_t objn;     // Object ID
        uint32_t objd;     // Data Object ID
        uint32_t tsn;      // Tablespace ID
        uint32_t prev_dba; // undo-dba or Prev DBA(when over19c)
        uint16_t opc;      // Opcode (Op1 << 8 | Op2)
        uint8_t slt;       // Slot Number -- Tx ID Slot ??
        uint8_t rci;       // Rollback Change Index -- Record Chain Index ??
        uint16_t flg;      // Flags         -- read when over19c in OLR
        uint16_t wrp;      // Wrap Sequence -- read when over19c in OLR

        bool is_begin_trans() const noexcept { return (flg & Ktub_Flag::BEGIN_TRANS) != 0; }
        bool is_bu_ext() const noexcept { return (flg & Ktub_Flag::BU_EXT) != 0; }

        bool is_mbu_head() const noexcept { return (flg & Ktub_Flag::MBU_HEAD) != 0; }
        bool is_mbu_tail() const noexcept { return (flg & Ktub_Flag::MBU_TAIL) != 0; }
        bool is_mbu_mid() const noexcept { return (flg & Ktub_Flag::MBU_MID) != 0; }
        bool is_regular() const noexcept { return !(is_mbu_head() && is_mbu_tail() && is_mbu_mid()); }

        bool is_lastSplit() const noexcept { return (flg & Ktub_Flag::LAST_SPLIT) != 0; }
        bool is_userUndoDone() const noexcept { return (flg & Ktub_Flag::USER_DONE) != 0; }
        bool is_tempObject() const noexcept { return (flg & Ktub_Flag::TEMP_OBJECT) != 0; }

        static Result<Ktubu_base> decode(tcb::span<const char> buf, bool isLittle);
    };

    // --- 2. ~ 28
    struct Ktubu_ext {
        uint16_t flg2;      // Offset 24 ~ 25 : Secondary Flags
        int16_t  buext_idx; // Offset 26 ~ 27 : BuExt Index
    };

    // --- 3. ~76바이트 이상 (Begin Trans) ---
    struct Ktubl_ext {
        Ktb_uba7 prev_ctl_uba;         // Offset 28 ~ 34 (7 Bytes: UBA)
        uint64_t prev_ctl_max_cmt_scn; // Offset 36 ~ 43 (8 Bytes: SCN)
        uint64_t prev_tx_cmt_scn;      // Offset 44 ~ 51 (8 Bytes: SCN)
        uint64_t tx_start_scn;         // Offset 56 ~ 63 (8 Bytes: SCN)
        uint32_t prev_brb;             // Offset 64 ~ 67 : Prev Block Rollback Block
        uint32_t prev_bcl;             // Offset 68 ~ 71 : Prev Block Cleanout
        uint32_t logon_user;           // Offset 72 ~ 75 : Logon User ID
    };

    // --------------------------------------------------------------------------------
    /// Ktubu | Ktubl
    struct Ktubu {
        Ktubu_base          header;
        optional<Ktubu_ext> ext0;             // 28 Bytes Extension
        optional<Ktubl_ext> ext1;             // 76 Bytes Full Extension

        bool is_begin_trans() const noexcept { return (header.flg & Ktub_Flag::BEGIN_TRANS) != 0; }

        bool is_mbu_head() const noexcept { return (header.flg & Ktub_Flag::MBU_HEAD) != 0; }
        bool is_mbu_tail() const noexcept { return (header.flg & Ktub_Flag::MBU_TAIL) != 0; }
        bool is_mbu_mid() const noexcept { return (header.flg & Ktub_Flag::MBU_MID) != 0; }
        bool is_regular() const noexcept { return !(is_mbu_head() && is_mbu_tail() && is_mbu_mid()); }

        bool is_lastSplit() const noexcept { return (header.flg & Ktub_Flag::LAST_SPLIT) != 0; }
        bool is_userUndoDone() const noexcept { return (header.flg & Ktub_Flag::USER_DONE) != 0; }
        bool is_tempObject() const noexcept { return (header.flg & Ktub_Flag::TEMP_OBJECT) != 0; }

        bool has_ubu_ext() const noexcept { return ext0.has_value(); }
        bool has_ubl_ext() const noexcept { return ext1.has_value(); }

        static Result<Ktubu> decode(tcb::span<const char> buf, bool isLittle, bool hint);
    };

    // --------------------------------------------------------------------------------
    inline Result<Ktubu_base> Ktubu_base::decode(tcb::span<const char> buf, bool isLittle) {

        if ( buf.size() < 24)
            return err_of(fmt::format("[Ktubu:base] buf({}) < {}", buf.size(), 24));

        return Ktubu_base {
            .objn     = decode_At<uint32_t>(buf, isLittle, 0),
            .objd     = decode_At<uint32_t>(buf, isLittle, 4),
            .tsn      = decode_At<uint32_t>(buf, isLittle, 8),
            .prev_dba = decode_At<uint32_t>(buf, isLittle, 12),    // previous DBA
            .opc      = static_cast<uint16_t>((decode_At<uint8_t>(buf, isLittle, 16) << 8) |
                                               decode_At<uint8_t>(buf, isLittle, 17)),
            .slt      = decode_At<uint8_t >(buf, isLittle, 18),
            .rci      = decode_At<uint8_t >(buf, isLittle, 19),
            .flg      = decode_At<uint16_t>(buf, isLittle, 20),
            .wrp      = decode_At<uint16_t>(buf, isLittle, 22)
        };
    }

    inline Result<Ktubu> Ktubu::decode(tcb::span<const char> buf, bool isLittle, bool hint) {

        auto base = Ktubu_base::decode(buf, isLittle);
        if (!base) return tl::make_unexpected(base.error());

        const size_t sz = buf.size();

        optional<Ktubu_ext> ext0;
        // ----------------------------------------
        if (sz >= 28 || base->is_bu_ext()) {
            if (enough(buf, 28, "KTUB:ext0")) {
                ext0 = Ktubu_ext{
                    .flg2      = decode_At<uint16_t>(buf, isLittle, 24),
                    .buext_idx = static_cast<int16_t>(decode_At<uint16_t>(buf, isLittle, 26))
                };
            }
        }

        optional<Ktubl_ext> ext1;
        // ----------------------------------------
        const bool is_ktubl = base->is_begin_trans() && hint;
        if (is_ktubl && sz >= 76) {
            ext1 = Ktubl_ext{
                .prev_ctl_uba         = decode_ktb_uba7(buf, isLittle, 28),
                .prev_ctl_max_cmt_scn = decode_ktb_scn8(buf, isLittle, 36),
                .prev_tx_cmt_scn      = decode_ktb_scn8(buf, isLittle, 44),
                .tx_start_scn         = decode_ktb_scn8(buf, isLittle, 56),
                .prev_brb             = decode_At<uint32_t>(buf, isLittle, 64),
                .prev_bcl             = decode_At<uint32_t>(buf, isLittle, 68),
                .logon_user           = decode_At<uint32_t>(buf, isLittle, 72)
            };
        }

        return Ktubu{
            .header   = std::move(*base),
            .ext0     = ext0,
            .ext1     = ext1
        };
    }

    // ----------------------------------------------------------------------------------------------------

    inline std::string to_string(const Ktubu_base &b) {
        return fmt::format(
            "flg: 0x{:04x}, begin_tx: {}, lastSplit: {}, userDone: {}, is_temp: {}\n",
            b.flg, (b.flg & Ktub_Flag::BEGIN_TRANS) != 0, b.is_lastSplit(), b.is_userUndoDone(), b.is_tempObject()
        );
    }

    inline std::string to_string(const Ktubu_ext &e) {
        return fmt::format(
            "flg2: 0x{:04x}, buext_idx: {}\n",
            e.flg2, e.buext_idx
        );
    }

    inline std::string to_string(const Ktubl_ext &e) {
        return fmt::format(
            "prev_ctl_uba: {}, prev_ctl_max_cmt_scn: {}, prev_tx_cmt_scn: {}, tx_start_scn: {}, prev_brb: 0x{:08x}, prev_bcl: 0x{:08x}, logon_user: {}\n",
            to_string(e.prev_ctl_uba), e.prev_ctl_max_cmt_scn, e.prev_tx_cmt_scn, e.tx_start_scn, e.prev_brb, e.prev_bcl, e.logon_user
        );
    }

    inline std::string to_string(const Ktubu &u) {
        std::string ext0_str = u.ext0.has_value() ? "    " + to_string(u.ext0.value()) : "";
        std::string ext1_str = u.ext1.has_value() ? "    " + to_string(u.ext1.value()) : "";

        return fmt::format(
            "UBU {{\n"
            "    {}{}{}  }}",
            to_string(u.header), ext0_str, ext1_str
        );
    }
}