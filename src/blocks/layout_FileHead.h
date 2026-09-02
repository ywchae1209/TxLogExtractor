#pragma once

#include <optional>
#include "FileHead.h"
#include "../coral_decode.h"
#include "../coral_result.h"

namespace ora {

#pragma pack(push, 1)
    constexpr size_t RedoFileHead_SZ = 32;

    struct FileHead_lo {
        uint8_t  zero;               // (1 byte, offset 0)
        uint8_t  fileType;           // (1 byte, offset 1)
        uint16_t empty0;             // (2 bytes, offset 2)

        uint32_t unknown[3];         // (12 bytes, offset 4)

        uint16_t crc;                // (2 bytes, offset 16)
        uint8_t  empty1[2];          // (2 bytes, offset 18)
        uint32_t block_size;         // Redo block size (4 bytes, offset 20)
        uint32_t total_block_count;  // Total block count (4 bytes, offset 24)
        uint32_t endian_magic;       // Endian Magic (4 bytes, offset 28)

        // -- added ---
        bool isLittle;
    };
#pragma pack(pop)
    using coral::decode_at;

    template <bool IsLittle>
    inline FileHead_lo decode_FileHead_lo0(tcb::span<const char> buf) {
        return FileHead_lo{
            .zero              = decode_at<uint8_t,  IsLittle>(buf, 0),
            .fileType          = decode_at<uint8_t,  IsLittle>(buf, 1),
            .empty0            = decode_at<uint16_t, IsLittle>(buf, 2),
            .unknown           = {
                decode_at<uint32_t, IsLittle>(buf, 4),
                decode_at<uint32_t, IsLittle>(buf, 8),
                decode_at<uint32_t, IsLittle>(buf, 12)
            },
            .crc               = decode_at<uint16_t, IsLittle>(buf, 16),
            .empty1            = {
                decode_at<uint8_t,  IsLittle>(buf, 18),
                decode_at<uint8_t,  IsLittle>(buf, 19)
            },
            .block_size        = decode_at<uint32_t, IsLittle>(buf, 20),
            .total_block_count = decode_at<uint32_t, IsLittle>(buf, 24),
            .endian_magic      = decode_at<uint32_t, IsLittle>(buf, 28),
            .isLittle           = IsLittle
        };
    }

    [[nodiscard]] inline coral::Result<FileHead_lo> decode_FileHead_lo(tcb::span<const char> buf) {
        if (buf.size() < RedoFileHead_SZ)
            return coral::err_of("redo-file head: not enough bytes");

        const auto* magic_bytes = reinterpret_cast<const uint8_t*>(buf.data() + 28);

        // 0x7A, 0x7B, 0x7C, 0x7D ::: Big-Endian
        if (magic_bytes[0] == 0x7A && magic_bytes[1] == 0x7B &&
            magic_bytes[2] == 0x7C && magic_bytes[3] == 0x7D) {
            return decode_FileHead_lo0<false>(buf);
        }

        // 0x7D, 0x7C, 0x7B, 0x7A ::: Little-Endian
        if (magic_bytes[0] == 0x7D && magic_bytes[1] == 0x7C &&
            magic_bytes[2] == 0x7B && magic_bytes[3] == 0x7A) {
            return decode_FileHead_lo0<true>(buf);
        }

        return coral::err_of("redo-file-head: invalid magic-endian");
    }
}
