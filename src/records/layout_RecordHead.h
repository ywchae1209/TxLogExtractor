#pragma once

#include "Record.h"

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

        uint16_t flags;           ///< Record Level flags.(internal?)  -- ignore
        uint32_t container_id;    ///< Container ID (PDB ID; over 12c) -- ignore
        uint32_t spare;           ///< Spare padding  -- ignore
    };

    /// 44 || 48 byte KTRRLWN(Redo Record LWN)
    struct Lwn_lo {
        uint32_t lwn_nst;            ///< * LWN NST (4 bytes)         LwnNum:2 ??
        uint32_t lwn_next;           ///< * LWN Next (4 bytes)        LwnSize:
        uint32_t lwn_length;         ///< * LWN Length (4 bytes)
        uint32_t unknown;            ///< Unknown (4 bytes)
        SCN_lo lwn_scn;              ///< base(4) wrap(2) high(2) in little-Endian
        uint32_t unknown4;           ///< Unknown (4 bytes)
        uint32_t unknown5;           ///< Unknown (4 bytes)
        SCN_lo lwn_next_scn;

        uint32_t epoch;              ///< * Epoch Timestamp (4 bytes)
        //uint32_t os_padding;         ///< todo check :: OS Padding (4 bytes; 64bit OS = 8의 배수)
    };
#pragma pack(pop)

    using coral::decode;
    static RecordHead_Base decode(const Base_lo &raw, const bool isLittle) noexcept {

        RecordHead_Base b{};

        b.len          = decode(raw.len, isLittle);
        b.vld          = raw.vld;

        b.scn_wrap     = decode(raw.scn_wrap, isLittle);
        b.scn_base     = decode(raw.scn_base, isLittle);
        b.sub_scn      = decode(raw.sub_scn, isLittle);
        b.container_id = decode(raw.container_id, isLittle);

        return b;
    }

    static RecordHead_LWN decode(const Lwn_lo &raw, const bool isLittle) noexcept {

        RecordHead_LWN l{};

        l.lwn_nst       = decode(raw.lwn_nst, isLittle);
        l.lwn_next      = decode(raw.lwn_next, isLittle);
        l.lwn_length    = decode(raw.lwn_length, isLittle);

        l.lwn_start_scn = decode_SCN(raw.lwn_scn, isLittle);
        l.lwn_next_scn  = decode_SCN(raw.lwn_next_scn, isLittle);

        l.epoch         = decode(raw.epoch, isLittle);

        return l;
    }
}