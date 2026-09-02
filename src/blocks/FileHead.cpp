#include <vector>
#include <optional>

#include "FileHead.h"
#include "layout_FileHead.h"
#include "../coral_decode.h"

namespace ora {

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

    std::optional<std::string> FileHead::errString() const noexcept {
        if (isOk())
            return std::nullopt;

        return to_string(valid);
    }

    using coral::Result;
    Result<FileHead> FileHead_of(const std::vector<char> &raw) noexcept{
        const Result<FileHead_lo> fh = decode_FileHead_lo(raw);

        if (!fh)
            return coral::err_of(fh.error());

        FileHead o;
        o.isLittle  = fh->isLittle;
        o.block_sz  = fh->block_size;
        o.total_blocks = fh->total_block_count;
        o.file_type = FileType(fh->fileType);

        o.valid = validate(o);

        if (o.valid != FHValid::Ok) {
            return coral::err_of( to_string(o.valid));
        }

        return o;
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