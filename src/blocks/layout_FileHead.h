#pragma once

#include <optional>

#include "FileHead.h"
#include "../coral_decode.h"

namespace ora {

#pragma pack(push, 1)
    struct FileHead_lo {
        uint8_t  p0;
        uint8_t  fileType;
        uint8_t  p1[14];

        uint16_t crc;
        uint16_t p2;
        uint32_t block_sz;
        uint32_t total_blocks;
        uint8_t  endian_magic[4]; // 0x7a7b7c7d (big-endian)
    };
#pragma pack(pop)

    inline FHValid validate(const FileHead& fh) noexcept {

        if ( fh.file_type.isUnknown())
            return FHValid::InvalidFileType;

        if (fh.block_sz< 256) return FHValid::InvalidBlockSize;

        switch(fh.block_sz) {
            case 512:
            case 1024:
            case 2048:
            case 4096:
            case 8192:
            case 16384:
            case 32768:
                break;
            default:
                return FHValid::InvalidBlockSize;
        }
        return FHValid::Ok;
    }

    inline std::optional<bool> isLittleEndian(const uint8_t (&bytes)[4] ) {

        if (bytes[0] == 0x7a && bytes[1] == 0x7b &&
            bytes[2] == 0x7c && bytes[3] == 0x7d)
            return false;

        if ( bytes[0] == 0x7d && bytes[1] == 0x7c &&
            bytes[2] == 0x7b && bytes[3] == 0x7a)
            return true;

        return std::nullopt;
    }

    inline FileHead decode(const FileHead_lo &raw) noexcept {
        using coral::decode;

        const auto mayLittle = isLittleEndian(raw.endian_magic);

        if (!mayLittle.has_value())
            return FileHead{FHValid::InvalidEndian};

        FileHead o;
        o.isLittle  = mayLittle.value();
        o.file_type = FileType{decode(raw.fileType, o.isLittle)};
        o.block_sz  = decode(raw.block_sz, o.isLittle);
        o.total_blocks = decode(raw.total_blocks, o.isLittle);

        o.valid = validate(o);

        return o;
    }

}
