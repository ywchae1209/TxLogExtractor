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
    namespace KdliCode {
        constexpr uint8_t Info       = 0x01;     // in OLR KDLI_CODE_INFO{0x01};
        constexpr uint8_t LoadCommon = 0x02;     // in OLR KDLI_CODE_LOAD_COMMON{0x02};
        constexpr uint8_t LoadData   = 0x04;     // in OLR KDLI_CODE_LOAD_DATA{0x04};
        constexpr uint8_t Zero       = 0x05;     // in OLR KDLI_CODE_ZERO{0x05};
        constexpr uint8_t Fill       = 0x06;     // in OLR KDLI_CODE_FILL{0x06};
        constexpr uint8_t Lmap       = 0x07;     // in OLR KDLI_CODE_LMAP{0x07};
        constexpr uint8_t Lmapx      = 0x08;     // in OLR KDLI_CODE_LMAPX{0x08};
        constexpr uint8_t Suplog     = 0x09;     // in OLR KDLI_CODE_SUPLOG{0x09};
        constexpr uint8_t Gmap       = 0x0A;     // in OLR KDLI_CODE_GMAP{0x0A};
        constexpr uint8_t Fpload     = 0x0B;     // in OLR KDLI_CODE_FPLOAD{0x0B};
        constexpr uint8_t LoadLhb    = 0x0C;     // in OLR KDLI_CODE_LOAD_LHB{0x0C};
        constexpr uint8_t Almap      = 0x0D;     // in OLR KDLI_CODE_ALMAP{0x0D};
        constexpr uint8_t Almapx     = 0x0E;     // in OLR KDLI_CODE_ALMAPX{0x0E};
        constexpr uint8_t LoadItree  = 0x0F;     // in OLR KDLI_CODE_LOAD_ITREE{0x0F};
        constexpr uint8_t Imap       = 0x10;     // in OLR KDLI_CODE_IMAP{0x10};
        constexpr uint8_t Imapx      = 0x11;     // in OLR KDLI_CODE_IMAPX{0x11};
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

        static Result<KdliHead> decode(tcb::span<const char> buf, bool isLittle);
    };

    // LMAP / ALMAP / IMAP -- 8Byte entry
    struct KdliMapEntry {
        uint8_t  num1{0};
        uint8_t  num2{0};
        uint16_t num3{0};
        uint32_t dba{0};
    };

    // LMAPX -- 16Byte entry
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

        static Result<KdliInfo> decode(tcb::span<const char> buf, bool isLittle);
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

        static Result<KdliLoadData> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 0x05: KDLI Zero
    struct KdliZero {
        uint16_t zoff{0};
        uint16_t zsiz{0};

        static Result<KdliZero> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 0x06: KDLI Fill (LOB Data Payload Chunk)
    struct KdliFill {
        uint16_t     lob_offset{0};
        uint16_t     fill_size{0};
        vector<char> payload;

        static Result<KdliFill> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 0x07: KDLI Lmap
    struct KdliLmap {
        uint32_t             asiz{0};
        vector<KdliMapEntry> entries;

        static Result<KdliLmap> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 0x08: KDLI Lmapx
    struct KdliLmapx {
        uint32_t              asiz{0};
        vector<KdliMapxEntry> entries;

        static Result<KdliLmapx> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 0x09: KDLI Suplog (Supplemental Logging Metadata)
    struct KdliSuplog {
        uint16_t xid_usn{0};
        uint16_t xid_slot{0};
        uint32_t xid_sqn{0};
        uint32_t objn{0};
        uint16_t col_no{0};
        uint32_t flag{0};

        static Result<KdliSuplog> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 0x0B: KDLI Fpload (Direct Load Metadata)
    struct KdliFpload {
        uint32_t bsz{0};
        uint16_t xid_usn{0};
        uint16_t xid_slot{0};
        uint32_t xid_sqn{0};
        uint32_t data_obj{0};

        static Result<KdliFpload> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 0x0C: KDLI Load Lhb (LOB Header Block)
    struct KdliLoadLhb {
        std::array<uint8_t, 10> lob_id;
        uint32_t dba0{0};
        uint32_t dba1{0};
        uint32_t dba2{0};
        uint32_t dba3{0};

        static Result<KdliLoadLhb> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 0x0D: KDLI Almap
    struct KdliAlmap {
        uint32_t             nent{0};
        uint32_t             sidx{0};
        vector<KdliMapEntry> entries;

        static Result<KdliAlmap> decode_kdli_almap(tcb::span<const char> buf, bool isLittle);
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

        static Result<KdliLoadItree> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 0x10: KDLI Imap
    struct KdliImap {
        uint32_t             asiz{0};
        vector<KdliMapEntry> entries;

        static Result<KdliImap> decode(tcb::span<const char> buf, bool isLittle);
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
    inline Result<KdliHead> KdliHead::decode(tcb::span<const char> buf, bool isLittle) {
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

    inline Result<KdliInfo> KdliInfo::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 17) return err_of("[KdliInfo] size < 17");
        KdliInfo out;
        std::memcpy(out.lob_id.data(), buf.data() + 1, 10);
        out.block = decode_at<uint32_t, false>(buf, 11); // Big-Endian Read
        out.slot  = decode_at<uint16_t, false>(buf, 15); // Big-Endian Read
        return out;
    }

    inline Result<KdliLoadData> KdliLoadData::decode(tcb::span<const char> buf, bool isLittle) {
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

    inline Result<KdliZero> KdliZero::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 6) return err_of("[KdliZero] size < 6");
        return KdliZero {
            .zoff = decode_At<uint16_t>(buf, isLittle, 2),
            .zsiz = decode_At<uint16_t>(buf, isLittle, 4)
        };
    }

    inline Result<KdliFill> KdliFill::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 8) return err_of("[KdliFill] size < 8");
        KdliFill out;
        out.lob_offset = decode_At<uint16_t>(buf, isLittle, 2);
        out.fill_size  = decode_At<uint16_t>(buf, isLittle, 4);
        uint16_t flen  = decode_At<uint16_t>(buf, isLittle, 6);
        if (buf.size() >= 8 + flen) {
            out.payload.assign(buf.begin() + 8, buf.begin() + 8 + flen);    // lobData
        }
        return out;
    }

    inline Result<KdliLmap> KdliLmap::decode(tcb::span<const char> buf, bool isLittle) {
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

    inline Result<KdliLmapx> KdliLmapx::decode(tcb::span<const char> buf, bool isLittle) {
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

    inline Result<KdliSuplog> KdliSuplog::decode(tcb::span<const char> buf, bool isLittle) {
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

    inline Result<KdliFpload> KdliFpload::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 28) return err_of("[KdliFpload] size < 28");
        return KdliFpload {
            .bsz      = decode_At<uint32_t>(buf, isLittle, 4),
            .xid_usn  = decode_At<uint16_t>(buf, isLittle, 16),
            .xid_slot = decode_At<uint16_t>(buf, isLittle, 18),
            .xid_sqn  = decode_At<uint32_t>(buf, isLittle, 20),
            .data_obj = decode_At<uint32_t>(buf, isLittle, 24)
        };
    }

    inline Result<KdliLoadLhb> KdliLoadLhb::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 112) return err_of("[KdliLoadLhb] size < 112");
        KdliLoadLhb out;
        std::memcpy(out.lob_id.data(), buf.data() + 12, 10);
        out.dba0 = decode_At<uint32_t>(buf, isLittle, 64);
        out.dba1 = decode_At<uint32_t>(buf, isLittle, 68);
        out.dba2 = decode_At<uint32_t>(buf, isLittle, 72);
        out.dba3 = decode_At<uint32_t>(buf, isLittle, 76);
        return out;
    }

    inline Result<KdliAlmap> KdliAlmap::decode_kdli_almap(tcb::span<const char> buf, bool isLittle) {
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

    inline Result<KdliLoadItree> KdliLoadItree::decode(tcb::span<const char> buf, bool isLittle) {
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

    inline Result<KdliImap> KdliImap::decode(tcb::span<const char> buf, bool isLittle) {
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
    inline Result<KdliElem> decode_kdli(tcb::span<const char> buf, bool isLittle) {
        if (buf.empty()) {
            return err_of("[Kdli] empty buffer");
        }

        const auto code = decode_At<uint8_t>(buf, isLittle, 0);

        auto decode_with = [&](auto decode_fn) -> Result<KdliElem> {
            auto r = decode_fn(buf, isLittle);
            return r ? Result<KdliElem>(*r) : tl::make_unexpected(r.error());
        };

        switch (code) {
            case KdliCode::Info:      return decode_with(KdliInfo::decode);
            case KdliCode::LoadData:  return decode_with(KdliLoadData::decode);
            case KdliCode::Zero:      return decode_with(KdliZero::decode);
            case KdliCode::Fill:      return decode_with(KdliFill::decode);
            case KdliCode::Lmap:      return decode_with(KdliLmap::decode);
            case KdliCode::Lmapx:     return decode_with(KdliLmapx::decode);
            case KdliCode::Suplog:    return decode_with(KdliSuplog::decode);
            case KdliCode::Fpload:    return decode_with(KdliFpload::decode);
            case KdliCode::LoadLhb:   return decode_with(KdliLoadLhb::decode);
            case KdliCode::Almap:     return decode_with(KdliAlmap::decode_kdli_almap);
            case KdliCode::LoadItree: return decode_with(KdliLoadItree::decode);
            case KdliCode::Imap:      return decode_with(KdliImap::decode);
            default:
                return KdliRawPayload{.code = code, .raw = vector(buf.begin(), buf.end())};
        }
    }

    // ----------------------------------------------------------------------------------------------------
    inline std::string format_lob_id(const std::array<uint8_t, 10> &id) {
        return fmt::format(
            "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7], id[8], id[9]
        );
    }

    // ----------------------------------------------------------------------------------------------------
    inline std::string to_string(const KdliHead &h) {
        return fmt::format(
            "KdliHead{{opc: {}, type: {}, flg0: 0x{:02x}, flg1: 0x{:02x}, psiz: {}, poff: {}, dba: 0x{:08x}}}",
            h.opc, h.type, h.flg0, h.flg1, h.psiz, h.poff, h.dba
        );
    }

    inline std::string to_string(const KdliMapEntry &e) {
        return fmt::format(
            "KdliMapEntry{{num1: {}, num2: {}, num3: {}, dba: 0x{:08x}}}",
            e.num1, e.num2, e.num3, e.dba
        );
    }

    inline std::string to_string(const KdliMapxEntry &e) {
        return fmt::format(
            "KdliMapxEntry{{num1: {}, num2: {}, num3: {}, dba: 0x{:08x}, num4: {}, num5: {}}}",
            e.num1, e.num2, e.num3, e.dba, e.num4, e.num5
        );
    }
    // ----------------------------------------------------------------------------------------------------

    inline std::string to_string(const KdliInfo &i) {
        return fmt::format(
            "KdliInfo{{lob_id: {}, block: {}, slot: {}}}",
            format_lob_id(i.lob_id), i.block, i.slot
        );
    }

    inline std::string to_string(const KdliLoadData &d) {
        return fmt::format(
            "KdliLoadData{{lob_id: {}, flg0: 0x{:02x}, flg1: 0x{:02x}, rid_slot: {}, rid_page: 0x{:08x}, flg2: 0x{:02x}, flg3: 0x{:02x}, hwm: {}}}",
            format_lob_id(d.lob_id), d.flg0, d.flg1, d.rid_slot, d.rid_page, d.flg2, d.flg3, d.hwm
        );
    }

    inline std::string to_string(const KdliZero &z) {
        return fmt::format("KdliZero{{zoff: {}, zsiz: {}}}", z.zoff, z.zsiz);
    }

    inline std::string to_string(const KdliFill &f) {
        return fmt::format(
            "KdliFill{{lob_offset: {}, fill_size: {}, payload_size: {}}}",
            f.lob_offset, f.fill_size, f.payload.size()
        );
    }

    inline std::string to_string(const KdliLmap &m) {
        return fmt::format("KdliLmap{{asiz: {}, entries_cnt: {}}}", m.asiz, m.entries.size());
    }

    inline std::string to_string(const KdliLmapx &m) {
        return fmt::format("KdliLmapx{{asiz: {}, entries_cnt: {}}}", m.asiz, m.entries.size());
    }

    inline std::string to_string(const KdliSuplog &s) {
        return fmt::format(
            "KdliSuplog{{xid_usn: {}, xid_slot: {}, xid_sqn: {}, objn: {}, col_no: {}, flag: 0x{:08x}}}",
            s.xid_usn, s.xid_slot, s.xid_sqn, s.objn, s.col_no, s.flag
        );
    }

    inline std::string to_string(const KdliFpload &f) {
        return fmt::format(
            "KdliFpload{{bsz: {}, xid_usn: {}, xid_slot: {}, xid_sqn: {}, data_obj: {}}}",
            f.bsz, f.xid_usn, f.xid_slot, f.xid_sqn, f.data_obj
        );
    }

    inline std::string to_string(const KdliLoadLhb &l) {
        return fmt::format(
            "KdliLoadLhb{{lob_id: {}, dba0: 0x{:08x}, dba1: 0x{:08x}, dba2: 0x{:08x}, dba3: 0x{:08x}}}",
            format_lob_id(l.lob_id), l.dba0, l.dba1, l.dba2, l.dba3
        );
    }

    inline std::string to_string(const KdliAlmap &a) {
        return fmt::format(
            "KdliAlmap{{nent: {}, sidx: {}, entries_cnt: {}}}",
            a.nent, a.sidx, a.entries.size()
        );
    }

    inline std::string to_string(const KdliLoadItree &t) {
        return fmt::format(
            "KdliLoadItree{{lob_id: {}, flg0: 0x{:02x}, flg1: 0x{:02x}, rid_slot: {}, rid_page: 0x{:08x}, flg2: 0x{:02x}, flg3: 0x{:02x}, lvl: {}, asiz: {}, hwm: {}, par: {}}}",
            format_lob_id(t.lob_id), t.flg0, t.flg1, t.rid_slot, t.rid_page, t.flg2, t.flg3, t.lvl, t.asiz, t.hwm, t.par
        );
    }

    inline std::string to_string(const KdliImap &m) {
        return fmt::format("KdliImap{{asiz: {}, entries_cnt: {}}}", m.asiz, m.entries.size());
    }

    inline std::string to_string(const KdliRawPayload &r) {
        return fmt::format("KdliRawPayload{{code: 0x{:02x}, raw_size: {}}}", r.code, r.raw.size());
    }

    // ----------------------------------------------------------------------------------------------------
    inline std::string to_string(const KdliElem &elem) {
        return std::visit([](const auto &arg) { return to_string(arg); }, elem);
    }

    inline std::string to_string(const std::vector<KdliElem> &elems) {
        if (elems.empty()) return "[]";

        std::string result = "[\n";
        for (size_t i = 0; i < elems.size(); ++i) {
            result += "  " + to_string(elems[i]);
            if (i + 1 < elems.size()) {
                result += ",\n";
            }
        }
        result += "\n]";
        return result;
    }

}