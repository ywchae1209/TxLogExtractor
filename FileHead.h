#pragma once

#include <string>
#include <vector>

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
    // redo file head (in 1st block)
    struct FileHead {
        FHValid   valid{FHValid::Empty};
        bool      isLittle{false};
        uint8_t   file_type{0};
        uint32_t  block_sz{0};
        uint32_t  total_blocks{0};
    };

    inline std::string fileTypeOr(const uint8_t c) {
        switch(c) {
            case 0x22: return "redo log";
            case 0xA2: return "data file";
            case 0xC2: return "control file";
            default:   return "unknown";
        }
    }

    FileHead FileHead_of(const std::vector<char> &raw);

    void show(const FileHead& h, std::ostream &os = std::cout);
}
