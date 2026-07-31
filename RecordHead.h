#pragma once
#include <cstdint>

namespace ora {

    // 24 byte
    struct Base_lo {
        uint32_t len;             // Record total length
        uint8_t  vld;             // VLD flags
        uint8_t  foo;             // Record type / Layer / Opcode <<< is this right?
        uint16_t scn_wrap;        // SCN wrap
        uint32_t scn_base;        // SCN base
        uint16_t sub_scn;         // Sub SCN
        uint16_t unknown1;        // Unknown / Audit flags
        uint32_t container_id;    // Container ID (PDB ID)
        uint32_t padding;         // Alignment Padding
    };

    struct Lwn_lo {
        uint32_t lwn_nst;            // LWN NST (4 bytes)
        uint32_t lwn_next;           // LWN Next (4 bytes)
        uint32_t lwn_length;         // LWN Length (4 bytes)
        uint32_t unknown2;           // Unknown (4 bytes)
        uint32_t lwn_scn_base;       // LWN SCN base (4 bytes)
        uint16_t lwn_scn_wrap;       // LWN SCN wrap (2 bytes)
        uint16_t unknown3;           // Unknown (2 bytes)
        uint32_t unknown4;           // Unknown (4 bytes)
        uint32_t unknown5;           // Unknown (4 bytes)
        uint32_t lwn_next_scn_base;  // LWN Next SCN base (4 bytes)
        uint16_t lwn_next_scn_wrap;  // LWN Next SCN wrap (12.1) (2 bytes)
        uint16_t lwn_next_scn_wrap2; // LWN Next SCN wrap2 (12.2) (2 bytes)
        uint32_t epoch;              // Epoch Timestamp (4 bytes)
        uint32_t hpux_padding;       // HP-UX 전용 Padding (4 bytes)
    };

    struct RecordHead_lo {
        Base_lo base;
        Lwn_lo lo;
    };

}

