#include <vector>
#include <optional>

#include "FileHead.h"
#include "coral_decode.h"

namespace ora {

    using coral::decode;

    // --------------------------------------------------------------------------------
    enum class FHValid : uint16_t {
        Ok = 0,
        Empty,               ///< 입력 버퍼가 NULL인 경우
        TooShort,
        InvalidEndian,      ///< Unknown endian-magic
        InvalidFileType,    ///< Unknown file-type
        InvalidBlockSize    ///< Block Size가 512B~32KB 아닌 경우
    };

    std::string to_string(FHValid valid) {
        switch (valid) {
            case FHValid::Ok:               return "Ok (0)";
            case FHValid::Empty:            return "Empty Buffer / NULL (1)";
            case FHValid::TooShort:         return "Buffer Too Short (2)";
            case FHValid::InvalidEndian:    return "Unknown Endian-Magic (3)";
            case FHValid::InvalidFileType:  return "Unknown File-Type (4)";
            case FHValid::InvalidBlockSize: return "Invalid Block Size [Not 512B~32KB] (5)";
            default: return "Unknown Error Code (" + std::to_string(static_cast<int>(valid)) + ")";
        }
    }

    bool isOk(const FHValid valid) {
        return valid == FHValid::Ok;

    }
    // --------------------------------------------------------------------------------
    enum class FileTypes : uint8_t {
        RedoLog      = 0x22,
        DataFile     = 0xA2,
        ControlFile  = 0xC2,
        Unknown       = 0x00  // unknown tag
    };

    std::string to_string(const FileType& info) {
        switch (info.type) {
            case FileTypes::RedoLog:     return "redo-file (0x22)";
            case FileTypes::DataFile:    return "data-file (0xA2)";
            case FileTypes::ControlFile: return "ctrl-file (0xC2)";
            case FileTypes::Unknown:
            default:
                return fmt::format("else-file (0x{:02X})", info.rawValue);
        }
    }

    FileType::FileType(uint8_t c) : rawValue(c) {
        switch (c) {
            case 0x22: type = FileTypes::RedoLog; break;
            case 0xA2: type = FileTypes::DataFile; break;
            case 0xC2: type = FileTypes::ControlFile; break;
            default:   type = FileTypes::Unknown; break;
        }
    }

    bool FileType::isUnknown() const noexcept {
        return type == FileTypes::Unknown;
    }

    // --------------------------------------------------------------------------------

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

    static FHValid validate(const FileHead& fh) noexcept {

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

    static std::optional<bool> isLittleEndian(const uint8_t (&bytes)[4] ) {

        if (bytes[0] == 0x7a && bytes[1] == 0x7b &&
            bytes[2] == 0x7c && bytes[3] == 0x7d)
            return false;

        if ( bytes[0] == 0x7d && bytes[1] == 0x7c &&
            bytes[2] == 0x7b && bytes[3] == 0x7a)
            return true;

        return std::nullopt;
    }

    static FileHead decode(const FileHead_lo &raw) noexcept {

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

    FileHead FileHead_of(const std::vector<char> &raw) noexcept{
        if (raw.size() < sizeof(FileHead_lo))
            return FileHead{FHValid::TooShort};

        const auto lo = reinterpret_cast<const FileHead_lo *>(raw.data());

        return decode(*lo);
    }

    std::string to_string(const FileHead& h) {

        using coral::toHex;
        return fmt::format(
            "=================================================================\n"
            "                    ORACLE REDO FILE HEADER INFO                 \n"
            "=================================================================\n"
            "[ Validation Status ] : {}\n\n"
            "  Type      : {}\n"
            "* BlockSz   : {}\n"
            "* BlockCount: {}\n"
            "* isLittle  : {}\n",
            to_string(h.valid),
            to_string(h.file_type),
            h.block_sz,
            h.total_blocks,
            h.isLittle
        );
    }
}