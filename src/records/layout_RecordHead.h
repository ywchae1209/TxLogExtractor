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
        SCN      lwn_start_scn;      ///< base(4) wrap(2) high(2) in little-Endian
        uint32_t unknown4;           ///< Unknown (4 bytes)
        uint32_t unknown5;           ///< Unknown (4 bytes)
        SCN      lwn_next_scn;

        uint32_t epoch;              ///< * Epoch Timestamp (4 bytes)
        //uint32_t os_padding;       ///< todo check :: OS Padding (4 bytes; 64bit OS = 8의 배수)
    };
#pragma pack(pop)

    using coral::decode_at;
    template <bool IsLittle>
    inline RecordHead_Base decode_record_head_base0(tcb::span<const char> buf) noexcept {
        return RecordHead_Base{
            .len          = decode_at<uint32_t, IsLittle>(buf, 0),
            .vld          = decode_at<uint8_t,  IsLittle>(buf, 4),
         // .foo          = decode_at<uint8_t,  IsLittle>(buf, 5),
            .scn_wrap     = decode_at<uint16_t, IsLittle>(buf, 6),
            .scn_base     = decode_at<uint32_t, IsLittle>(buf, 8),
            .sub_scn      = decode_at<uint16_t, IsLittle>(buf, 12),
         // .flags        = decode_at<uint16_t, IsLittle>(buf, 14),
            .container_id = decode_at<uint32_t, IsLittle>(buf, 16),
         // .spare        = decode_at<uint32_t, IsLittle>(buf, 20)
        };
    }

    inline RecordHead_Base decode_record_head_base(tcb::span<const char> buf, bool isLittle) noexcept {
        return isLittle
                   ? decode_record_head_base0<true>(buf)
                   : decode_record_head_base0<false>(buf);
    }

    template <bool IsLittle>
    inline RecordHead_LWN decode_record_head_lwn0(tcb::span<const char> buf) noexcept {
        return RecordHead_LWN{
            .lwn_nst      = decode_at<uint32_t, IsLittle>(buf, 0),
            .lwn_next     = decode_at<uint32_t, IsLittle>(buf, 4),
            .lwn_length   = decode_at<uint32_t, IsLittle>(buf, 8),
         // .unknown      = decode_at<uint32_t, IsLittle>(buf, 12),
            .lwn_start_scn= decode_scn0s_at   <IsLittle>(buf, 16),
         // .unknown4     = decode_at<uint32_t, IsLittle>(buf, 24),
         // .unknown5     = decode_at<uint32_t, IsLittle>(buf, 28),
            .lwn_next_scn = decode_scn0s_at   <IsLittle>(buf, 32),
            .epoch        = decode_at<uint32_t, IsLittle>(buf, 40)
        };
    }
    inline RecordHead_LWN decode_record_head_lwn(tcb::span<const char> buf, bool isLittle) noexcept {
        return isLittle
                   ? decode_record_head_lwn0<true>(buf)
                   : decode_record_head_lwn0<false>(buf);
    }
}