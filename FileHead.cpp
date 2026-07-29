#include <iostream>
#include "FileHead.h"

#include <vector>

#include "coral_decode.h"

namespace ora {

    using coral::decode;

    #pragma pack(push, 1)
    struct FileHead_lo {
        uint8_t  p0;
        uint8_t  fileType;
        uint8_t  p1[14];

        uint16_t crc;
        uint16_t p2;
        uint32_t block_sz;
        uint32_t total_blocks;
        uint8_t  endian_magic[4]; // 0x7a7b7c7d
    };
    #pragma pack(pop)

    inline FHValid validate(const FileHead& fh) {

        // 1. File Type
        if (fh.file_type != 0x22 &&
            fh.file_type != 0xA2 &&
            fh.file_type != 0xC2) {
            return FHValid::InvalidFileType;
        }

        // 2. Block Size
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

    FileHead decode(const FileHead_lo& raw) {

        FileHead o{};
        auto bytes= raw.endian_magic;
        if ( bytes[0] == 0x7a &&
             bytes[1] == 0x7b &&
             bytes[2] == 0x7c &&
             bytes[3] == 0x7d) {
            o.isLittle = false;
        } else if (
            bytes[0] == 0x7d &&
            bytes[1] == 0x7c &&
            bytes[2] == 0x7b &&
            bytes[3] == 0x7a ) {
            o.isLittle = true;
        } else
            return FileHead{FHValid::InvalidEndian};

        o.file_type    = decode(raw.fileType, o.isLittle);
        o.block_sz     = decode(raw.block_sz, o.isLittle);
        o.total_blocks = decode(raw.total_blocks, o.isLittle);

        o.valid = validate(o);

        return o;
    }

    FileHead FileHead_of(const std::vector<char> &raw) {
        if (raw.size() < sizeof(FileHead_lo))
            return FileHead{FHValid::TooShort};

        const auto lo = reinterpret_cast<const FileHead_lo *>(raw.data());

        return decode(*lo);
    }

    void show(const FileHead& h, std::ostream &os) {

        using coral::toHex;

        fmt::print(os,
            "=================================================================\n"
            "                    ORACLE REDO FILE HEADER INFO                 \n"
            "=================================================================\n"
            "[ Validation Status ] : {}\n\n"
            "  Type      : {}\n"
            "  BlockSz   : {}\n"
            "  BlockCount: {}\n"
            "  isLittle  : {}\n"
            "  type      : {}\n",
            to_string(h.valid),
            toHex(h.file_type),
            h.block_sz,
            h.total_blocks,
            h.isLittle,
            fileTypeOr(h.file_type)
        );
    }
}
