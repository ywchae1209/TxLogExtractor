#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <tcb/span.hpp>

#include "coral_decode.h"
#include "coral_record.h"

namespace ora {

#pragma pack(push, 1)

    /// 24 byte KTRRH(Redo Record Header)
    struct Base_lo {
        uint32_t len;             ///< * Record total length
        uint8_t  vld;             ///< * Validity flags
        uint8_t  foo;             ///< Record type.(internal?) -- ignore

        uint16_t scn_wrap;        ///< * SCN wrap
        uint32_t scn_base;        ///< * SCN base
        uint16_t sub_scn;         ///< * Sub SCN

        uint16_t flags;           ///< Record Level flags.(internal?) -- ignore
        uint32_t container_id;    ///< Container ID (PDB ID; over 12c) -- ignore
        uint32_t spare;           ///< Spare padding  -- ignore
    };

    /// 44 || 48 byte KTRRLWN(Redo Record LWN)
    struct Lwn_lo {
        uint32_t lwn_nst;            ///< * LWN NST (4 bytes)

        uint32_t lwn_next;           ///< * LWN Next (4 bytes)
        uint32_t lwn_length;         ///< * LWN Length (4 bytes)
        uint32_t unknown;           ///< Unknown (4 bytes)

        SCN_lo lwn_scn;
        // uint32_t lwn_scn_base;       ///< * LWN SCN base (4 bytes)
        // uint16_t lwn_scn_wrap;       ///< * LWN SCN wrap (2 bytes)
        // uint16_t may_scn_wrap_high;  ///< Unknown (2 bytes) ::: wrap_high?

        uint32_t unknown4;           ///< Unknown (4 bytes)
        uint32_t unknown5;           ///< Unknown (4 bytes)

        SCN_lo lwn_next_scn;
        // uint32_t lwn_next_scn_base;  ///< * LWN Next SCN base (4 bytes)
        // uint16_t lwn_next_scn_wrap;  ///< * LWN Next SCN wrap (12.1) (2 bytes)
        // uint16_t lwn_next_scn_wrap2; ///< * LWN Next SCN wrap2 (12.2) (2 bytes) ::: wrap_high?

        uint32_t epoch;              ///< * Epoch Timestamp (4 bytes)
//         uint32_t os_padding;         ///< OS Padding (4 bytes; 64bit OS = 8의 배수)
    };
#pragma pack(pop)

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
        uint32_t unknown;           ///< Unknown (4 bytes)

        SCN lwn_start_scn;
        SCN lwn_next_scn;

        uint32_t epoch;              ///< Epoch Timestamp (4 bytes)
    };

    struct RecordHead {
        RecordHead_Base base;
        std::unique_ptr<RecordHead_LWN> lwn_lo;
    };


    inline RecordHead_Base decode(const Base_lo &raw, const bool isLittle) noexcept {
        using coral::decode;

        RecordHead_Base b;
        b.len          = decode(raw.len, isLittle);
        b.vld          = raw.vld;

        b.scn_wrap     = decode(raw.scn_wrap, isLittle);
        b.scn_base     = decode(raw.scn_base, isLittle);
        b.sub_scn      = decode(raw.sub_scn, isLittle);

        return b;
    }

    inline RecordHead_LWN decode(const Lwn_lo &raw, const bool isLittle) noexcept {
        using coral::decode;

        RecordHead_LWN l;
        l.lwn_nst            = decode(raw.lwn_nst, isLittle);
        l.lwn_next           = decode(raw.lwn_next, isLittle);
        l.lwn_length         = decode(raw.lwn_length, isLittle);
        l.unknown            = decode(raw.unknown, isLittle);

        l.lwn_start_scn      = decode_SCN(raw.lwn_scn, isLittle);
        l.lwn_next_scn       = decode_SCN(raw.lwn_next_scn, isLittle);

        l.epoch              = decode(raw.epoch, isLittle);
        return l;
    }

    inline RecordHead RecordHead_of(const tcb::span<const char>& raw, const bool isLittle) {

        if (raw.size() < sizeof(Base_lo)) {
            throw std::out_of_range("Buffer size is smaller than Base_lo size(24 byte).");
        }

        Base_lo raw_base;
        std::memcpy(&raw_base, raw.data(), sizeof(Base_lo));

        auto base =  decode(raw_base, isLittle);

        if (coral::dependBit_on(base.vld) ) {
            Lwn_lo raw_lwn;
            if (raw.size() < sizeof(Base_lo) + sizeof(Lwn_lo) ) {
                throw std::out_of_range("Buffer size is smaller than Base_lo + LWN_lo size(64 byte).");
            }
            std::memcpy(&raw_lwn, raw.data() + sizeof(Base_lo), sizeof(Lwn_lo));
            auto lwn = decode(raw_lwn, isLittle);
            return RecordHead{ base , std::make_unique<RecordHead_LWN>(lwn)};

        } else {
            return RecordHead {base};
        }
    }

    inline void show(const RecordHead &h, std::ostream &os = std::cout) noexcept {
        using coral::toHex;

        const auto &b = h.base;

        uint64_t base_scn = ((uint64_t)b.scn_wrap << 32) | b.scn_base;

        fmt::print(os,
            "LEN:{}\t"
            "VLD:{} {}\t"
            "SCN: 0x{:04x}.{:08x}:{:03}\n",
            b.len,
            toHex(b.vld), coral::vld_string(b.vld),
            b.scn_wrap, b.scn_base, b.sub_scn
        );

        if (h.lwn_lo) {
            const auto &l = *h.lwn_lo;

            fmt::print(os,
                "  └─ [LWN] NST:{}\tNEXT:{}\tLEN:{} ({})\tSTART_SCN:{})\tNEXT_SCN:{}\tEPOCH:{}\n",
                l.lwn_nst,
                l.lwn_next,
                l.lwn_length, toHex(l.lwn_length),
                toHex(l.lwn_start_scn),
                toHex(l.lwn_next_scn),
                l.epoch
            );
        }
    }

}

