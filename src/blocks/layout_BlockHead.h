#pragma once

#include "Block.h"
#include "tcb/span.hpp"
#include "../coral_decode.h"

namespace ora {

    // BlockHead
    // ----------------------------------------------------------------------------------------------------
#pragma pack(push, 1)
    struct BlockHead {
        uint8_t  signature[4]; // Block Type (offset 0)
        uint32_t block_no;     // Block number (4 bytes, offset 4)
        uint32_t log_seq_no;   // Log sequence number (4 bytes, offset 8)
        uint16_t offset;       // Record-begin offset (2 bytes, offset 12) & 0x7fff
        uint8_t  crc[2];       // CRC (2 bytes, offset 14)

        // --- ---
        // Block Signature 검증 (일반적으로 Redo Block은 0x01, 0x22 패턴)
        [[nodiscard]] constexpr bool is_valid_signature() const noexcept {
            return (signature[0] == 0x01 && signature[1] == 0x22) ||
                   (signature[0] == 0x22 && signature[1] == 0x01);
        }
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline BlockHead decode_BlockHead0(tcb::span<const char> buf) {
        using coral::decode_at;

        BlockHead res{};

        res.signature[0] = static_cast<uint8_t>(buf[0]);
        res.signature[1] = static_cast<uint8_t>(buf[1]);
        res.signature[2] = static_cast<uint8_t>(buf[2]);
        res.signature[3] = static_cast<uint8_t>(buf[3]);

        res.block_no   = decode_at<uint32_t, IsLittle>(buf, 4);
        res.log_seq_no = decode_at<uint32_t, IsLittle>(buf, 8);
        res.offset     = decode_at<uint16_t, IsLittle>(buf, 12) & 0x7fff;

        res.crc[0]     = static_cast<uint8_t>(buf[14]);
        res.crc[1]     = static_cast<uint8_t>(buf[15]);

        return res;
    }

    [[nodiscard]] inline BlockHead decode_BlockHead(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(BlockHead))  // sizeof(BlockHeadLo) == 16
            return {};

        return isLittle ? decode_BlockHead0<true>(buf)
                        : decode_BlockHead0<false>(buf);
    }
}
