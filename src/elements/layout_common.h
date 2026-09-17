#pragma once

#include "../coral_decode.h"
#include "../coral_result.h"

namespace ora {

    using coral::decode_at, coral::decode_At, coral::Result, coral::err_of;
    using std::optional, std::vector;

#pragma pack(push, 1)

    struct Ktb_scn6 {
        uint16_t wrap; // (2 bytes, offset 0)
        uint32_t base; // (4 bytes, offset 2) Transaction ID sequence number

        uint64_t to_int64() const { return (static_cast<uint64_t>(wrap) << 32) | static_cast<uint64_t>(base); }
    };

    struct Ktb_xid8 {
        uint16_t usn; // (2 bytes, offset 0) Transaction ID undo segment number
        uint16_t slt; // (2 bytes, offset 2) Transaction ID slot
        uint32_t sqn; // (4 bytes, offset 4) Transaction ID sequence number
    };
    static_assert(sizeof(Ktb_xid8) == 8, "Ktb_xid8 size mismatch");

    struct Ktb_uba7 {
        uint32_t dba;
        uint16_t sqn;
        uint8_t rec;
    };
    static_assert(sizeof(Ktb_uba7) == 7, "Ktb_uba7 size mismatch");

    struct Ktb_ItlEntry {
        Ktb_xid8 xid;       // Offset  0 ~ 7  (8 Bytes: usn 2B, slt 2B, sqn 4B)
        Ktb_uba7 uba;       // Offset  8 ~ 14 (7 Bytes: dba 4B, sqn 2B, rec 1B)
        uint8_t pad;        // Offset 15      (1 Byte : spare/alignment)
        uint16_t flags_lck; // Offset 16 ~ 17 (2 Bytes: flags & lock count)
        Ktb_scn6 scn_fsc;

        [[nodiscard]]
        bool is_committed() const noexcept {
            return (flags_lck & 0x8000) != 0;
        }

        [[nodiscard]] std::string flag_string() const {

            char flagStr[5]{"----"};
            if ((flags_lck & 0x1000) != 0) flagStr[3] = 'T';
            if ((flags_lck & 0x2000) != 0) flagStr[2] = 'U';
            if ((flags_lck & 0x4000) != 0) flagStr[1] = 'B';
            if ((flags_lck & 0x8000) != 0) flagStr[0] = 'C';

            return flagStr;
        }

        [[nodiscard]]
        std::optional<uint64_t> committed_scn() const noexcept {
            if (!is_committed()) return std::nullopt;
            return scn_fsc.to_int64();
        }

        [[nodiscard]] std::optional<uint32_t> free_space_credit() const noexcept {
            if (is_committed()) return std::nullopt;
            return scn_fsc.base;
        }
    };
    static_assert(sizeof(Ktb_ItlEntry) == 24, "Ktb_ItlEntry size mismatch");



#pragma pack(pop)

    // --------------------------------------------------------------------------------
    template<bool IsLittle>
    inline Ktb_scn6 decode_ktb_scn6(const tcb::span<const char> b,
                                    const size_t sp) {
        return Ktb_scn6{
            .wrap = decode_at<uint16_t, IsLittle>(b, sp    ),
            .base = decode_at<uint32_t, IsLittle>(b, sp + 2),
        };
    }

    inline Ktb_scn6 decode_ktb_scn6(const tcb::span<const char> b,
                                    const bool isLittle,
                                    const size_t sp) {
        return isLittle
                   ? decode_ktb_scn6<true>(b, sp)
                   : decode_ktb_scn6<false>(b, sp);
    }

    template<bool IsLittle>
    inline Ktb_uba7 decode_ktb_uba7(const tcb::span<const char> b,
                                   const size_t sp) {
        return Ktb_uba7{
            .dba = decode_at<uint32_t, IsLittle>(b, sp),
            .sqn = decode_at<uint16_t, IsLittle>(b, sp + 4),
            .rec = decode_at<uint8_t , IsLittle>(b, sp + 6)
        };
    }

    inline Ktb_uba7 decode_ktb_uba7(const tcb::span<const char> b,
                                   const bool isLittle,
                                   const size_t sp) {
        return isLittle
                   ? decode_ktb_uba7<true>(b, sp)
                   : decode_ktb_uba7<false>(b, sp);
    }

    template<bool IsLittle>
    inline Ktb_xid8 decode_ktb_xid8(const tcb::span<const char> b,
                                   const size_t sp) {
        return Ktb_xid8{
            .usn = decode_at<uint16_t, IsLittle>(b, sp),
            .slt = decode_at<uint16_t, IsLittle>(b, sp + 2),
            .sqn = decode_at<uint32_t, IsLittle>(b, sp + 4)
        };
    }

    inline Ktb_xid8 decode_ktb_xid8(const tcb::span<const char> b,
                                   const bool isLittle,
                                   const size_t sp) {
        return isLittle
                   ? decode_ktb_xid8<true>(b, sp)
                   : decode_ktb_xid8<false>(b, sp);
    }

    template<bool IsLittle>
    inline Ktb_ItlEntry decode_ktb_itlEntry24(const tcb::span<const char> b,
                                        const size_t sp) {
        return Ktb_ItlEntry{
            .xid       = decode_ktb_xid8<   IsLittle>(b, sp),
            .uba       = decode_ktb_uba7<   IsLittle>(b, sp + 8),
            .pad       = decode_at<uint8_t ,IsLittle>(b, sp + 15),
            .flags_lck = decode_at<uint16_t,IsLittle>(b, sp + 16),
            .scn_fsc   = decode_ktb_scn6<   IsLittle>(b, sp + 24),
        };

    }

    inline Ktb_ItlEntry decode_ktb_itlEntry24(const tcb::span<const char> b,
                                        const bool isLittle,
                                        const size_t offset) {
        return isLittle
                   ? decode_ktb_itlEntry24<true>(b, offset)
                   : decode_ktb_itlEntry24<false>(b, offset);

    }


    inline Result<vector<Ktb_ItlEntry> > decode_ktb_itl(const tcb::span<const char> buf,
                                                        const bool isLittle,
                                                        const size_t sp,
                                                        const uint16_t itl_cnt) {

        const auto all = sizeof(Ktb_ItlEntry) * itl_cnt;
        if (buf.size() < sp + all) {
            return err_of(fmt::format("[Ktb_itl] buf-size ({}) < total-required ({})",
                                      buf.size(), sp + all));
        }

        vector<Ktb_ItlEntry> out;
        out.reserve(itl_cnt);

        for (uint16_t i = 0; i < itl_cnt; ++i) {
            const auto cur = sp + i * sizeof(Ktb_ItlEntry);
            out.push_back( decode_ktb_itlEntry24(buf, isLittle, cur));
        }
        return out;
    }


}