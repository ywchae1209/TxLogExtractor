#pragma once
#include <iostream>
#include <tcb/span.hpp>

#include "coral_record.h"

namespace ora {

    struct RecordHead_Base {
        uint32_t len;             ///< Record total length
        uint8_t  vld;             ///< Validity flags
        uint16_t scn_wrap;        ///< SCN wrap
        uint32_t scn_base;        ///< SCN base             --- len ~ scn_base :: verify
        uint16_t sub_scn;         ///< Sub SCN
    };

    struct RecordHead_LWN {
        uint32_t lwn_nst;            ///< LWN NST (4 bytes)
        uint32_t lwn_next;           ///< LWN Next (4 bytes)
        uint32_t lwn_length;         ///< LWN Length (4 bytes)

        SCN lwn_start_scn;
        SCN lwn_next_scn;

        uint32_t epoch;              ///< Epoch Timestamp (4 bytes)
    };

    struct RecordHead {
        RecordHead_Base base;
        size_t offset;
        std::unique_ptr<RecordHead_LWN> lwn;

    };

    RecordHead RecordHead_of(const tcb::span<const char>& raw, bool isLittle);

    inline std::string to_string(const RecordHead &h) {
        using coral::toHex;
        const auto &b = h.base;

        std::string s = fmt::format(
            "LEN:{:<6} "
            "SCN: 0x{:04x}.{:08x}:{:03} "
            "-- {:3} -- "
            "VLD:{} {}",
            b.len,
            b.scn_wrap, b.scn_base, b.sub_scn,
            h.offset,
            toHex(b.vld), coral::vld_string(b.vld)
        );

        if (h.lwn) {
            const auto &l = *h.lwn;
            s += fmt::format(
                "  -- [LWN] nst:{} next:{:3} len:{:3} start_scn:{} next_scn:{} Epoch:{}",
                l.lwn_nst,
                l.lwn_next,
                l.lwn_length,
                toHex(l.lwn_start_scn),
                toHex(l.lwn_next_scn),
                l.epoch
            );
        }
        return s;
    }

}

