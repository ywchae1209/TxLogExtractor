#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "coral_show.h"

namespace ora {

    // --------------------------------------------------------------------------------
    enum class FHValid : uint16_t;

    std::string to_string(FHValid valid);
    bool isOk(FHValid valid);

    // --------------------------------------------------------------------------------
    enum class FileTypes : uint8_t;

    struct FileType {
        FileTypes type;
        uint8_t rawValue;

        explicit FileType(uint8_t c);
        bool isUnknown() const noexcept;
    };

    std::string to_string(const FileType& info);
    // --------------------------------------------------------------------------------
    // redo file head (in 1st block)
    struct FileHead {
        FHValid   valid{};
        FileType  file_type{FileType{0}};
        bool      isLittle;
        uint16_t  block_sz;
        uint32_t  total_blocks;

        bool isOk() const noexcept { return ora::isOk(valid); }

        std::optional<std::string> errString() const noexcept {
            if (isOk())
                return std::nullopt;
            return to_string(valid);
        }
    };
    std::string to_string(const FileHead& h);

    inline void show(const FileHead& h, std::ostream &os = std::cout) {
        fmt::print(os, "{}", to_string(h) );
    }

    FileHead FileHead_of(const std::vector<char> &raw) noexcept;
}
