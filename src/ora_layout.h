#pragma once

#include "coral_decode.h"
#include "coral_show.h"

namespace ora {

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
        uint16_t wrap_high{}; ///< SCN Wrap_High -- ignore ??
    };

    // ================================================================================
#pragma pack(push, 1)
    struct SCN_lo {
        uint32_t minor; // base
        uint16_t major; // wrap ~ wrap_high
        uint16_t major_high; // wrap ~ wrap_high
    };
#pragma pack(pop)

    // in redo header
    inline SCN decode_SCN0(const SCN_lo &raw, const bool isLittle) {
        const uint32_t minor = coral::decode(raw.minor, isLittle);
        const uint16_t major = coral::decode(raw.major, isLittle);
        const uint16_t major_high = coral::decode(raw.major_high, isLittle);

        /* ========================================
            SCN in redo header (low-scn, next-scn)

            11.32.4.0       base(32).wrap(16).____(16)
            12.16.2.0       base(32).wrap(16).____(16)
            18.0.0.0        base(32).8000(16).wrap(16)  <<<<-----
            19.0.0.0        base(32).8000(16).wrap(16)  <<<<-----
                            base(32).wrap(16).ffff(16)
                            base(32).wrap(16).0000(16)
         */

        SCN o = {};
        o.base = minor;

        if (major == 0x8000) {
            o.wrap = major_high;            // & 0x7fff; // MSB is active-flag.
            o.wrap_high = major;

        } else {
            o.wrap = major;            // & 0x7fff; // MSB is active-flag.
            o.wrap_high = major_high;
        }
        return o;
    }


    inline SCN decode_SCN(const SCN_lo &raw, const bool isLittle) {
        const uint32_t minor = coral::decode(raw.minor, isLittle);
        const uint16_t major = coral::decode(raw.major, isLittle);
        const uint16_t major_high = coral::decode(raw.major_high, isLittle);

        SCN o = {};
        o.base = minor;
        o.wrap = major;
        o.wrap_high = major_high;

        return o;
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

    // ================================================================================
    struct OraVer {
        uint32_t raw_val{};
        uint8_t major{};
        uint8_t minor{};
        uint8_t patch{};
        uint8_t extra{};
    };

    // OraVer : 19.3.0.0
    inline std::string toHex(const OraVer &ver) {
        return fmt::format("{}.{}.{}.{}", ver.major, ver.minor, ver.patch, ver.extra);
    }
}
