#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "../coral_show.h"
#include "../coral_result.h"

namespace ora {
    // --------------------------------------------------------------------------------
    enum class FHValid : uint16_t {
        Ok = 0,
        TooShort,
        InvalidEndian,      ///< Unknown endian-magic
        InvalidFileType,    ///< Unknown file-type
        InvalidBlockSize    ///< Block Size가 512B~32KB 아닌 경우
    };

    // --------------------------------------------------------------------------------
    enum class FileTypes : uint8_t {
        RedoLog      = 0x22,
        DataFile     = 0xA2,
        ControlFile  = 0xC2,
        Unknown      = 0x00  // unknown tag
    };

    inline FileTypes FileTypes_of(uint8_t c) {
        switch (c) {
            case 0x22: return FileTypes::RedoLog;
            case 0xA2: return FileTypes::DataFile;
            case 0xC2: return FileTypes::ControlFile;
            default:   return FileTypes::Unknown;
        }
    }

    struct FileType {
        FileTypes type;
        uint8_t rawValue;

        explicit FileType(uint8_t c): type(FileTypes_of(c)), rawValue(c) {};
        bool isUnknown() const noexcept { return type == FileTypes::Unknown; }
    };

    // --------------------------------------------------------------------------------
    // redo file head (in 1st block)
    struct FileHead {

        FHValid   valid{};
        FileType  file_type{FileType{0}};
        bool      isLittle{};
        uint16_t  block_sz{};
        uint32_t  total_blocks{};             ////< total written_blocks or total_blocks

        [[nodiscard]] bool isOk() const noexcept { return valid == FHValid::Ok; }
        [[nodiscard]] std::optional<std::string> errString() const noexcept ;
    };

    // --------------------------------------------------------------------------------
    coral::Result<FileHead> FileHead_of(const std::vector<char> &raw) noexcept;

    std::string to_string(FHValid valid);
    std::string to_string(const FileHead& h);
    std::string to_string(const FileType& info);
    void show(const FileHead &h, std::ostream &os = std::cout);

}
