#pragma once

#include "coral_decode.h"
#include "coral_show.h"

namespace ora {

    // ================================================================================
#pragma pack(push, 1)
    struct SCN_lo{
        uint32_t minor;  // base
        uint32_t major;  // wrap ~ wrap_high
    };
#pragma pack(pop)

    /** 오라클 SCN (System Change Number) (64-bit) */
    struct SCN {
        uint32_t base{};        ///< SCN Base
        uint16_t wrap{};        ///< SCN Wrap
        uint16_t wrap_high{};   ///< SCN Wrap_High -- ignore ??
    };

    inline SCN decode_SCN(const SCN_lo& raw, const bool isLittle) {

        const uint32_t minor = coral::decode(raw.minor, isLittle);
        const uint32_t major = coral::decode(raw.major, isLittle);

        const uint16_t upper = major >> 16;
        const uint16_t lower = major & 0xFFFF;

        SCN o = {};
        o.base = minor;
        o.wrap = upper;
        o.wrap_high = lower;

        return o;
    }

    inline uint64_t scn_to64(const SCN& scn) {
        // drop wrap_high
        return (static_cast<uint64_t>(scn.wrap) << 32) | scn.base;
    }

    // SCN : 0x0000.00000000 (0)
    inline std::string toHex(const SCN& scn) {
        if (scn.wrap_high == 0)
            return fmt::format("0x{:04x}.{:08x} ({})", scn.wrap, scn.base, scn_to64(scn));

        return fmt::format("0x{:04x}.{:04x}.{:08x} ({})", scn.wrap_high, scn.wrap, scn.base, scn_to64(scn));
    }

    // ================================================================================
    struct OraVer {
        uint32_t raw_val{};
        uint8_t  major{};
        uint8_t  minor{};
        uint8_t  patch{};
        uint8_t  extra{};
    };

    // OraVer : 19.3.0.0
    inline std::string toHex(const OraVer& ver) {
        return fmt::format("{}.{}.{}.{}", ver.major, ver.minor, ver.patch, ver.extra);
    }
}
