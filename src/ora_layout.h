#pragma once

#include "coral_decode.h"
#include "coral_show.h"

namespace ora {

    // --------------------------------------------------------------------------------
    struct OraVer {
        uint32_t raw_val{};
        uint8_t major{};
        uint8_t minor{};
        uint8_t patch{};
        uint8_t extra{};
    };

    inline static OraVer to_OraVer(const uint32_t n) {
        return OraVer{
            n,
            static_cast<uint8_t>((n >> 24) & 0xFF),
            static_cast<uint8_t>((n >> 16) & 0xFF),
            static_cast<uint8_t>((n >> 8)  & 0xFF),
            static_cast<uint8_t>(n & 0xFF)
        };
    }

    // OraVer : 19.3.0.0
    inline std::string toHex(const OraVer &ver) {
        return fmt::format("{}.{}.{}.{}", ver.major, ver.minor, ver.patch, ver.extra);
    }
    // --------------------------------------------------------------------------------

    struct RBA {
        uint32_t log_seq_no{0}; // Redo-Log Sequence Number
        uint32_t block_no{0};   // Block Number
        uint16_t offset{0};     // start-offset
    };

    inline std::string to_string(const RBA &r) {
        return fmt::format("{:>7}.@{:<3}", r.block_no, r.offset);
    }

    // --------------------------------------------------------------------------------
    /** 오라클 SCN (System Change Number) (64-bit) */
    struct SCN {
        uint32_t base{}; ///< SCN Base
        uint16_t wrap{}; ///< SCN Wrap
        uint16_t wrap_high{}; ///< SCN Wrap_High -- ignore ?? or use lower
    };

    using coral::decode_at;

    template <bool IsLittle>
    inline SCN decode_scn0l( tcb::span<const char>buf) {
        return SCN{
            .base     = decode_at<uint32_t, IsLittle>(buf, 0),
            .wrap     = decode_at<uint16_t, IsLittle>(buf, 4),
            .wrap_high= decode_at<uint16_t, IsLittle>(buf, 6)
        };
    }

    template <bool IsLittle>
    inline SCN decode_scn0s( tcb::span<const char>buf) {
        return SCN{
           .base     = decode_at<uint32_t, IsLittle>(buf, 0),
           .wrap     = decode_at<uint16_t, IsLittle>(buf, 4),
           .wrap_high= 0
        };
    }

    template <bool IsLittle>
    inline SCN decode_scn0s_at(tcb::span<const char> buf, size_t offset) {
        return decode_scn0s<IsLittle>(buf.subspan(offset));
    }

    template <bool IsLittle>
    inline SCN decode_scn0l_at(tcb::span<const char> buf, size_t offset) {
        return decode_scn0l<IsLittle>(buf.subspan(offset));
    }

    template <bool IsLittle>
    inline SCN decode_redo_scn0l( tcb::span<const char>buf) {
        /* ========================================
            SCN in redo header (low-scn, next-scn)

            11.32.4.0       base(32).wrap(16).____(16)
            12.16.2.0       base(32).wrap(16).____(16)
            18.0.0.0        base(32).8000(16).wrap(16)  <<<<-----
            19.0.0.0        base(32).8000(16).wrap(16)  <<<<-----
                            base(32).wrap(16).ffff(16)
                            base(32).wrap(16).0000(16)
         */
        auto o = decode_scn0l<IsLittle>(buf);

        if (o.wrap == 0x8000) {
            fmt::println(std::cerr,
                         fmt::format("redo-scn-swap : wrap(0x{:0x}) <-> wrap_high(0x{:04x})",
                             o.wrap, o.wrap_high));
            auto w = o.wrap;
            o.wrap = o.wrap_high;
            o.wrap_high = w;
        }
        return o;
    }

    template <bool IsLittle>
    inline SCN decode_redo_scn0_at( tcb::span<const char>buf, size_t offset) {
        return decode_redo_scn0l<IsLittle>(buf.subspan(offset));
    }

    // --------------------------------------------------------------------------------
    // include wrap_high
    inline SCN decode_scnL( tcb::span<const char>buf, bool isLittle) {
        return isLittle
                   ? decode_scn0l<true>(buf)
                   : decode_scn0l<false>(buf);
    }

    // ignore wrap_high
    inline SCN decode_scn( tcb::span<const char>buf, bool isLittle) {
        return isLittle
                   ? decode_scn0s<true>(buf)
                   : decode_scn0s<false>(buf);
    }

    inline SCN decode_scn_at( tcb::span<const char>buf, size_t offset, bool isLittle) {
        return isLittle
                   ? decode_scn0s<true>(buf.subspan(offset))
                   : decode_scn0s<false>(buf.subspan(offset));
    }

    inline SCN decode_redo_scn_at(tcb::span<const char> buf, size_t offset, bool isLittle) {
        return isLittle
                   ? decode_redo_scn0l<true>(buf.subspan(offset))
                   : decode_redo_scn0l<false>(buf.subspan(offset));
    }

    //// ignore wrap_high
    inline uint64_t scn_to64(const uint16_t wrap, const uint32_t base) {
        return (static_cast<uint64_t>(wrap) << 32) | base;
    }

    inline uint64_t scn_to64(const SCN &scn) {
        return (static_cast<uint64_t>(scn.wrap) << 32) | scn.base;
    }

    // SCN : 0x0000.00000000 (0)
    inline std::string toHex(const SCN &scn) {
        // return fmt::format("0x{:04x}.{:08x} ({})", scn.wrap, scn.base, scn_to64(scn));
        return fmt::format("0x{:04x}.{:04x}.{:08x} ({})", scn.wrap_high, scn.wrap, scn.base, scn_to64(scn));
    }

    inline std::string toHex0(const SCN &scn) {
        return fmt::format("0x{:04x}.{:08x}", scn.wrap, scn.base);
    }
}
