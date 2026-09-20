#pragma once
#include <fmt/format.h>
#include <cstdint>
#include <array>
#include <vector>
#include <variant>
#include <optional>
#include <cstring>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_combinator.h"
#include "../coral_decode.h"

namespace ora {

    using coral::decode_at, coral::decode_At, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::vector, std::variant;

    // ----------------------------------------------------------------------------------------------------
    enum class KdliCode : uint8_t {
        Info       = 0x01,
        LoadCommon = 0x02,
        LoadData   = 0x04,
        Zero       = 0x05,
        Fill       = 0x06,
        Lmap       = 0x07,
        Lmapx      = 0x08,
        Suplog     = 0x09,
        Gmap       = 0x0A,
        Fpload     = 0x0B,
        LoadLhb    = 0x0C,
        Almap      = 0x0D,
        Almapx     = 0x0E,
        LoadItree  = 0x0F,
        Imap       = 0x10,
        Imapx      = 0x11
    };

    // ====================================================================================================
    struct KdliHead {
        uint8_t  opc{0};   // Subtype (0=REDO, 6=BIMG 등)
        uint8_t  type{0};  // LOB Type
        uint8_t  flg0{0};
        uint8_t  flg1{0};
        uint16_t psiz{0};
        uint16_t poff{0};
        uint32_t dba{0};   // Data Block Address
    };

    [[nodiscard]] inline Result<KdliHead> decode_kdli_head(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 12) {
            return err_of(fmt::format("[KdliCommon] buf size {} < 12", buf.size()));
        }
        return KdliHead {
            .opc  = decode_At<uint8_t >(buf, isLittle, 0),
            .type = decode_At<uint8_t >(buf, isLittle, 1),
            .flg0 = decode_At<uint8_t >(buf, isLittle, 2),
            .flg1 = decode_At<uint8_t >(buf, isLittle, 3),
            .psiz = decode_At<uint16_t>(buf, isLittle, 4),
            .poff = decode_At<uint16_t>(buf, isLittle, 6),
            .dba  = decode_At<uint32_t>(buf, isLittle, 8)
        };
    }

    // ====================================================================================================
    // Sub-Element 엔티티 항목 (Map Entry 구조체)
    // ====================================================================================================

    // LMAP / ALMAP / IMAP 공통 8Byte 맵 엔트리
    struct KdliMapEntry {
        uint8_t  num1{0};
        uint8_t  num2{0};
        uint16_t num3{0};
        uint32_t dba{0};
    };

    // LMAPX 16Byte 맵 엔트리
    struct KdliMapxEntry {
        uint8_t  num1{0};
        uint8_t  num2{0};
        uint16_t num3{0};
        uint32_t dba{0};
        int32_t  num4{0};
        int32_t  num5{0};
    };

    // ====================================================================================================
    /// 0x01: KDLI Info
    struct KdliInfo {
        std::array<uint8_t, 10> lob_id; // LOB 식별자 (10 Bytes)
        uint32_t block{0};              // Data Block (Big-Endian)
        uint16_t slot{0};               // Slot (Big-Endian)
    };

    /// 0x04: KDLI Load Data
    struct KdliLoadData {
        std::array<uint8_t, 10> lob_id;
        uint8_t  flg0{0};
        uint8_t  flg1{0};
        uint16_t rid_slot{0};
        uint32_t rid_page{0};
        uint8_t  flg2{0};
        uint8_t  flg3{0};
        uint16_t hwm{0};
    };

    /// 0x05: KDLI Zero
    struct KdliZero {
        uint16_t zoff{0};
        uint16_t zsiz{0};
    };

    /// 0x06: KDLI Fill (LOB Data Payload Chunk)
    struct KdliFill {
        uint16_t     lob_offset{0};
        uint16_t     fill_size{0};
        vector<char> payload;
    };

    /// 0x07: KDLI Lmap
    struct KdliLmap {
        uint32_t             asiz{0};
        vector<KdliMapEntry> entries;
    };

    /// 0x08: KDLI Lmapx
    struct KdliLmapx {
        uint32_t              asiz{0};
        vector<KdliMapxEntry> entries;
    };

    /// 0x09: KDLI Suplog (Supplemental Logging Metadata)
    struct KdliSuplog {
        uint16_t xid_usn{0};
        uint16_t xid_slot{0};
        uint32_t xid_sqn{0};
        uint32_t objn{0};
        uint16_t col_no{0};
        uint32_t flag{0};
    };

    /// 0x0B: KDLI Fpload (Direct Load Metadata)
    struct KdliFpload {
        uint32_t bsz{0};
        uint16_t xid_usn{0};
        uint16_t xid_slot{0};
        uint32_t xid_sqn{0};
        uint32_t data_obj{0};
    };

    /// 0x0C: KDLI Load Lhb (LOB Header Block)
    struct KdliLoadLhb {
        std::array<uint8_t, 10> lob_id;
        uint32_t dba0{0};
        uint32_t dba1{0};
        uint32_t dba2{0};
        uint32_t dba3{0};
    };

    /// 0x0D: KDLI Almap
    struct KdliAlmap {
        uint32_t             nent{0};
        uint32_t             sidx{0};
        vector<KdliMapEntry> entries;
    };

    /// 0x0F: KDLI Load Itree
    struct KdliLoadItree {
        std::array<uint8_t, 10> lob_id;
        uint8_t  flg0{0};
        uint8_t  flg1{0};
        uint16_t rid_slot{0};
        uint32_t rid_page{0};
        uint8_t  flg2{0};
        uint8_t  flg3{0};
        uint16_t lvl{0};
        uint16_t asiz{0};
        uint16_t hwm{0};
        uint32_t par{0};
    };

    /// 0x10: KDLI Imap
    struct KdliImap {
        uint32_t             asiz{0};
        vector<KdliMapEntry> entries;
    };

    // fallback
    struct KdliRawPayload {
        uint8_t      code{0};
        vector<char> raw;
    };

    // ----------------------------------------------------------------------------------------------------
    ///
    using KdliElem = variant<
        KdliInfo,
        KdliLoadData,
        KdliZero,
        KdliFill,
        KdliLmap,
        KdliLmapx,
        KdliSuplog,
        KdliFpload,
        KdliLoadLhb,
        KdliAlmap,
        KdliLoadItree,
        KdliImap,
        KdliRawPayload
    >;

    // ====================================================================================================

    [[nodiscard]] inline Result<KdliInfo> decode_kdli_info(tcb::span<const char> buf, bool /*isLittle*/) {
        if (buf.size() < 17) return err_of("[KdliInfo] size < 17");
        KdliInfo out;
        std::memcpy(out.lob_id.data(), buf.data() + 1, 10);
        out.block = decode_at<uint32_t, false>(buf, 11); // Big-Endian Read
        out.slot  = decode_at<uint16_t, false>(buf, 15); // Big-Endian Read
        return out;
    }

    [[nodiscard]] inline Result<KdliLoadData> decode_kdli_load_data(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 56) return err_of("[KdliLoadData] size < 56");
        KdliLoadData out;
        std::memcpy(out.lob_id.data(), buf.data() + 12, 10);
        out.flg0     = decode_At<uint8_t >(buf, isLittle, 10);
        out.flg1     = decode_At<uint8_t >(buf, isLittle, 11);
        out.rid_slot = decode_At<uint16_t>(buf, isLittle, 22);
        out.rid_page = decode_At<uint32_t>(buf, isLittle, 24);
        out.flg2     = decode_At<uint8_t >(buf, isLittle, 28);
        out.flg3     = decode_At<uint8_t >(buf, isLittle, 29);
        out.hwm      = decode_At<uint16_t>(buf, isLittle, 52);
        return out;
    }

    [[nodiscard]] inline Result<KdliZero> decode_kdli_zero(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 6) return err_of("[KdliZero] size < 6");
        return KdliZero {
            .zoff = decode_At<uint16_t>(buf, isLittle, 2),
            .zsiz = decode_At<uint16_t>(buf, isLittle, 4)
        };
    }

    [[nodiscard]] inline Result<KdliFill> decode_kdli_fill(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 8) return err_of("[KdliFill] size < 8");
        KdliFill out;
        out.lob_offset = decode_At<uint16_t>(buf, isLittle, 2);
        out.fill_size  = decode_At<uint16_t>(buf, isLittle, 4);
        uint16_t flen  = decode_At<uint16_t>(buf, isLittle, 6);
        if (buf.size() >= 8 + flen) {
            out.payload.assign(buf.begin() + 8, buf.begin() + 8 + flen);
        }
        return out;
    }

    [[nodiscard]] inline Result<KdliLmap> decode_kdli_lmap(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 8) return err_of("[KdliLmap] size < 8");
        KdliLmap out;
        out.asiz = decode_At<uint32_t>(buf, isLittle, 4);
        if (buf.size() < 8 + out.asiz * 8) {
            return err_of(fmt::format("[KdliLmap] buf size {} < {}", buf.size(), 8 + out.asiz * 8));
        }
        out.entries.reserve(out.asiz);
        for (uint32_t i = 0; i < out.asiz; ++i) {
            const size_t off = 8 + i * 8;
            out.entries.push_back(KdliMapEntry{
                .num1 = decode_At<uint8_t >(buf, isLittle, off + 0),
                .num2 = decode_At<uint8_t >(buf, isLittle, off + 1),
                .num3 = decode_At<uint16_t>(buf, isLittle, off + 2),
                .dba  = decode_At<uint32_t>(buf, isLittle, off + 4)
            });
        }
        return out;
    }

    [[nodiscard]] inline Result<KdliLmapx> decode_kdli_lmapx(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 8) return err_of("[KdliLmapx] size < 8");
        KdliLmapx out;
        out.asiz = decode_At<uint32_t>(buf, isLittle, 4);
        if (buf.size() < 8 + out.asiz * 16) {
            return err_of(fmt::format("[KdliLmapx] buf size {} < {}", buf.size(), 8 + out.asiz * 16));
        }
        out.entries.reserve(out.asiz);
        for (uint32_t i = 0; i < out.asiz; ++i) {
            const size_t off = 8 + i * 16;
            out.entries.push_back(KdliMapxEntry{
                .num1 = decode_At<uint8_t >(buf, isLittle, off + 0),
                .num2 = decode_At<uint8_t >(buf, isLittle, off + 1),
                .num3 = decode_At<uint16_t>(buf, isLittle, off + 2),
                .dba  = decode_At<uint32_t>(buf, isLittle, off + 4),
                .num4 = decode_At<int32_t >(buf, isLittle, off + 8),
                .num5 = decode_At<int32_t >(buf, isLittle, off + 12)
            });
        }
        return out;
    }

    [[nodiscard]] inline Result<KdliSuplog> decode_kdli_suplog(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 24) return err_of("[KdliSuplog] size < 24");
        return KdliSuplog {
            .xid_usn  = decode_At<uint16_t>(buf, isLittle, 4),
            .xid_slot = decode_At<uint16_t>(buf, isLittle, 6),
            .xid_sqn  = decode_At<uint32_t>(buf, isLittle, 8),
            .objn     = decode_At<uint32_t>(buf, isLittle, 12),
            .col_no   = decode_At<uint16_t>(buf, isLittle, 18),
            .flag     = decode_At<uint32_t>(buf, isLittle, 20)
        };
    }

    [[nodiscard]] inline Result<KdliFpload> decode_kdli_fpload(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 28) return err_of("[KdliFpload] size < 28");
        return KdliFpload {
            .bsz      = decode_At<uint32_t>(buf, isLittle, 4),
            .xid_usn  = decode_At<uint16_t>(buf, isLittle, 16),
            .xid_slot = decode_At<uint16_t>(buf, isLittle, 18),
            .xid_sqn  = decode_At<uint32_t>(buf, isLittle, 20),
            .data_obj = decode_At<uint32_t>(buf, isLittle, 24)
        };
    }

    [[nodiscard]] inline Result<KdliLoadLhb> decode_kdli_load_lhb(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 112) return err_of("[KdliLoadLhb] size < 112");
        KdliLoadLhb out;
        std::memcpy(out.lob_id.data(), buf.data() + 12, 10);
        out.dba0 = decode_At<uint32_t>(buf, isLittle, 64);
        out.dba1 = decode_At<uint32_t>(buf, isLittle, 68);
        out.dba2 = decode_At<uint32_t>(buf, isLittle, 72);
        out.dba3 = decode_At<uint32_t>(buf, isLittle, 76);
        return out;
    }

    [[nodiscard]] inline Result<KdliAlmap> decode_kdli_almap(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 12) return err_of("[KdliAlmap] size < 12");
        KdliAlmap out;
        out.nent = decode_At<uint32_t>(buf, isLittle, 4);
        out.sidx = decode_At<uint32_t>(buf, isLittle, 8);
        if (buf.size() < 12 + out.nent * 8) {
            return err_of(fmt::format("[KdliAlmap] buf size {} < {}", buf.size(), 12 + out.nent * 8));
        }
        out.entries.reserve(out.nent);
        for (uint32_t i = 0; i < out.nent; ++i) {
            const size_t off = 12 + i * 8;
            out.entries.push_back(KdliMapEntry{
                .num1 = decode_At<uint8_t >(buf, isLittle, off + 0),
                .num2 = decode_At<uint8_t >(buf, isLittle, off + 1),
                .num3 = decode_At<uint16_t>(buf, isLittle, off + 2),
                .dba  = decode_At<uint32_t>(buf, isLittle, off + 4)
            });
        }
        return out;
    }

    [[nodiscard]] inline Result<KdliLoadItree> decode_kdli_load_itree(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 40) return err_of("[KdliLoadItree] size < 40");
        KdliLoadItree out;
        std::memcpy(out.lob_id.data(), buf.data() + 12, 10);
        out.flg0     = decode_At<uint8_t >(buf, isLittle, 10);
        out.flg1     = decode_At<uint8_t >(buf, isLittle, 11);
        out.rid_slot = decode_At<uint16_t>(buf, isLittle, 22);
        out.rid_page = decode_At<uint32_t>(buf, isLittle, 24);
        out.flg2     = decode_At<uint8_t >(buf, isLittle, 28);
        out.flg3     = decode_At<uint8_t >(buf, isLittle, 29);
        out.lvl      = decode_At<uint16_t>(buf, isLittle, 30);
        out.asiz     = decode_At<uint16_t>(buf, isLittle, 32);
        out.hwm      = decode_At<uint16_t>(buf, isLittle, 34);
        out.par      = decode_At<uint32_t>(buf, isLittle, 36);
        return out;
    }

    [[nodiscard]] inline Result<KdliImap> decode_kdli_imap(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 8) return err_of("[KdliImap] size < 8");
        KdliImap out;
        out.asiz = decode_At<uint32_t>(buf, isLittle, 4);
        if (buf.size() < 8 + out.asiz * 8) {
            return err_of(fmt::format("[KdliImap] buf size {} < {}", buf.size(), 8 + out.asiz * 8));
        }
        out.entries.reserve(out.asiz);
        for (uint32_t i = 0; i < out.asiz; ++i) {
            const size_t off = 8 + i * 8;
            out.entries.push_back(KdliMapEntry{
                .num1 = decode_At<uint8_t >(buf, isLittle, off + 0),
                .num2 = decode_At<uint8_t >(buf, isLittle, off + 1),
                .num3 = decode_At<uint16_t>(buf, isLittle, off + 2),
                .dba  = decode_At<uint32_t>(buf, isLittle, off + 4)
            });
        }
        return out;
    }

    // ----------------------------------------------------------------------------------------------------
    [[nodiscard]] inline Result<KdliElem> decode_kdli(tcb::span<const char> buf, bool isLittle) {
        if (buf.empty()) {
            return err_of("[Kdli] empty buffer");
        }

        const auto code = static_cast<KdliCode>(decode_At<uint8_t>(buf, isLittle, 0));

        switch (code) {
            case KdliCode::Info: {
                auto r = decode_kdli_info(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::LoadData: {
                auto r = decode_kdli_load_data(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::Zero: {
                auto r = decode_kdli_zero(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::Fill: {
                auto r = decode_kdli_fill(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::Lmap: {
                auto r = decode_kdli_lmap(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::Lmapx: {
                auto r = decode_kdli_lmapx(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::Suplog: {
                auto r = decode_kdli_suplog(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::Fpload: {
                auto r = decode_kdli_fpload(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::LoadLhb: {
                auto r = decode_kdli_load_lhb(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::Almap: {
                auto r = decode_kdli_almap(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::LoadItree: {
                auto r = decode_kdli_load_itree(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            case KdliCode::Imap: {
                auto r = decode_kdli_imap(buf, isLittle);
                if (!r) return tl::make_unexpected(r.error());
                return *r;
            }
            default:
                return KdliRawPayload{.code = static_cast<uint8_t>(code), .raw = vector(buf.begin(), buf.end())};
        }
    }

}