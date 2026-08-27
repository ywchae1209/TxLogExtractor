#pragma once

#include "Block.h"
#include "tcb/span.hpp"

namespace ora {

#pragma pack(push, 1)
    struct BlockHead_lo {
        uint8_t signature[4]; // signature = Block Type 0x2201
        uint32_t block_no;
        uint32_t log_seq_no;
        uint16_t offset0; // offset = record-begin offset
        uint8_t crc[2];
    };
#pragma pack(pop)

    inline BlockHead decode(const BlockHead_lo &raw, const bool isLittle) noexcept {
        BlockHead h{};

        h.valid = BHValid::Ok;

        h.signature[0] = raw.signature[0];
        h.signature[1] = raw.signature[1];
        h.signature[2] = raw.signature[2];
        h.signature[3] = raw.signature[3];

        h.block_no = coral::decode(raw.block_no, isLittle);
        h.log_seq_no = coral::decode(raw.log_seq_no, isLittle);

        h.offset = coral::decode(raw.offset0, isLittle) & 0x7fff;

        h.crc[0] = raw.crc[0];
        h.crc[1] = raw.crc[1];

        return h;
    }

    inline BlockHead BlockHead_of(const tcb::span<const char> &raw, const bool isLittle) noexcept {
        if (raw.size() < sizeof(BlockHead_lo))
            return BlockHead{.valid = BHValid::Empty};

        const auto lo = reinterpret_cast<const BlockHead_lo *>(raw.data());
        return decode(*lo, isLittle);
    }
}
