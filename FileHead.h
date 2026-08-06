#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "coral_show.h"

namespace ora {

    // --------------------------------------------------------------------------------
    enum class FHValid {
        Ok = 0,
        Empty,               ///< 입력 버퍼가 NULL인 경우
        TooShort,
        InvalidEndian,      ///< Unknown endian-magic
        InvalidFileType,    ///< Unknown file-type
        InvalidBlockSize    ///< Block Size가 512B~32KB 아닌 경우
    };

    inline std::string to_string(FHValid valid) {
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

    // --------------------------------------------------------------------------------
    enum class FHType : uint8_t {
        RedoLog      = 0x22,
        DataFile     = 0xA2,
        ControlFile  = 0xC2,
        Unknown       = 0x00  // unknown tag
    };

    struct FileType {
        FHType type;
        uint8_t rawValue;

        explicit FileType(uint8_t c) : rawValue(c) {
            switch (c) {
                case 0x22: type = FHType::RedoLog; break;
                case 0xA2: type = FHType::DataFile; break;
                case 0xC2: type = FHType::ControlFile; break;
                default:   type = FHType::Unknown; break;
            }
        }
        bool isUnknown() const noexcept { return type == FHType::Unknown; }
    };

    inline std::string to_string(const FileType& info) {
        switch (info.type) {
            case FHType::RedoLog:     return "redo-file (0x22)";
            case FHType::DataFile:    return "data-file (0xA2)";
            case FHType::ControlFile: return "ctrl-file (0xC2)";
            case FHType::Unknown:
            default:
                return fmt::format("else-file (0x{:02X})", info.rawValue);
        }
    }

    // --------------------------------------------------------------------------------
    // redo file head (in 1st block)
    struct FileHead {
        FHValid   valid{FHValid::Empty};
        FileType  file_type{FileType{0}};
        bool      isLittle;
        uint16_t  block_sz;
        uint32_t  total_blocks;

        bool isOk() const noexcept { return valid == FHValid::Ok; }

        std::optional<std::string> errString() const noexcept {
            if (isOk())
                return std::nullopt;
            return to_string(valid);
        }
    };

    inline std::string to_string(const FileHead& h) {

        using coral::toHex;
        return fmt::format(
            "=================================================================\n"
            "                    ORACLE REDO FILE HEADER INFO                 \n"
            "=================================================================\n"
            "[ Validation Status ] : {}\n\n"
            "  Type      : {}\n"
            "  BlockSz   : {}\n"
            "  BlockCount: {}\n"
            "  isLittle  : {}\n",
            to_string(h.valid),
            to_string(h.file_type),
            h.block_sz,
            h.total_blocks,
            h.isLittle
        );
    }

    inline void show(const FileHead& h, std::ostream &os = std::cout) {
        using coral::toHex;
        fmt::print(os, "{}", to_string(h) );
    }

    FileHead FileHead_of(const std::vector<char> &raw) noexcept;

}
