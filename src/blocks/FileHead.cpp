#include <vector>
#include <optional>

#include "FileHead.h"
#include "layout_FileHead.h"
#include "../coral_decode.h"

namespace ora {
    std::optional<std::string> FileHead::errString() const noexcept {
        if (isOk())
            return std::nullopt;

        return to_string(valid);
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

    inline std::string to_string(const FileType& info) {
        switch (info.type) {
            case FileTypes::RedoLog:     return "redo-file (0x22)";
            case FileTypes::DataFile:    return "data-file (0xA2)";
            case FileTypes::ControlFile: return "ctrl-file (0xC2)";
            case FileTypes::Unknown:
            default:
                return fmt::format("else-file (0x{:02X})", info.rawValue);
        }
    }

    void show(const FileHead &h, std::ostream &os) {
        fmt::print(os, "{}", to_string(h));
    }
}