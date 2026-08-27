#include <tcb/span.hpp>

#include "RedoHead.h"
#include "layout_RedoHead.h"

namespace ora {

    RedoHead RedoHead_of(const tcb::span<const char> &raw, const bool isLittle) {
        if (raw.size() < sizeof(RedoHead_lo))
            return RedoHead{RHValid::TooShort};

        const auto lo = reinterpret_cast<const RedoHead_lo *>(raw.data());
        return decode_RedoHead(*lo, isLittle);
    }


    // --------------------------------------------------------------------------------
    using coral::toHex;

    std::string to_string(const SourceInfo& si) {
        const uint64_t mb_in_file = static_cast<uint64_t>(si.blocks_in_file) * si.block_sz / 1024 / 1024;
        return fmt::format("--- [ Source Info ] ---\n"
                           "  Software Version   : {}\n"
                           "  Compatible Version : {}\n"
                           "  Database Name      : {}\n"
                           "  Database ID        : {} ({})\n"
                           "  Activation ID      : {}\n"
                           "  Control Sequence   : {}\n"
                           "* Block Size         : {} bytes\n"
                           "* Blocks in file     : {} ({} MB)\n"
                           "  Group No/ file-type: Group {} / Type {}\n"
                           "* Description        : {}\n\n",
                           toHex(si.software_ver),
                           toHex(si.compat_ver),
                           si.database_name,
                           si.database_id, toHex(si.database_id),
                           toHex(si.activation_id),
                           toHex(si.control_sequence),
                           si.block_sz,
                           si.blocks_in_file, mb_in_file,
                           si.group_no,
                           toHex(si.file_type),
                           si.desc );
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const WriteInfo& wi) {
        return fmt::format("--- [ Write Info ] ---\n"
                           "* NAB (Next Avail Blk): {} ({})\n"
                           "  Resetlogs Count    : {}\n"
                           "  Resetlogs SCN      : {}\n"
                           "  HWS(High-Water Seq): {}\n"
                           "  Thread No          : {}\n"
                           "* Low  SCN / epoch   : {} / epoch: {}\n"
                           "* Next SCN / epoch   : {} / epoch: {}\n\n",
                           wi.nab, toHex(wi.nab),
                           wi.resetlogs_count,
                           toHex(wi.resetlogs_scn),
                           wi.hws,
                           wi.thread_no,
                           toHex(wi.low_scn), wi.low_epoch,
                           toHex(wi.next_scn), wi.next_epoch );
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const ThreadState& ts) {
        return fmt::format("--- [ Thread State ] ---\n"
                           "  EOT Flag           : {} (End Of Thread)\n"
                           "  DIS Flag           : {} (Disabled flag)\n"
                           "  Enabled SCN / epoch: {} / epoch: {}\n"
                           "  Close   SCN / epoch: {} / epoch: {}\n\n",
                           toHex(ts.eot),
                           toHex(ts.dis),
                           toHex(ts.enabled_scn), ts.enabled_epoch,
                           toHex(ts.close_scn), ts.close_epoch );
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const FileState& fs) {
        return fmt::format("--- [ File State ] ---\n"
                           "  Log Format Version : {}\n"
                           "  Flags              : {}\n"
                           "  Terminal SCN/Epoch : {} / Epoch: {}\n\n",
                           fs.log_format_ver,
                           toHex(fs.flags),
                           toHex(fs.terminal_scn),
                           fs.terminal_epoch );
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const TDEKeyInfo& ki) {
        return fmt::format("--- [ TDE Key Info ] ---\n"
                           "* Master Key ID      : {}\n"
                           "* Encrypt Key        : {}\n"
                           "  Key Flag           : {}\n"
                           "=================================================================\n",
                           toHex(ki.master_key_id.data(), ki.master_key_id.size()),
                           toHex(ki.encrypt_key.data(), ki.encrypt_key.size()),
                           toHex(ki.key_flag) );
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const RHValid& val) {
        switch (val) {
            case RHValid::Ok:               return "Ok";
            case RHValid::Empty:            return "Empty";
            case RHValid::TooShort:         return "TooShort";
            case RHValid::InvalidFileType:  return "InvalidFileType";
            case RHValid::InvalidBlockSize: return "InvalidBlockSize";
            case RHValid::InvalidNab:       return "InvalidNab";
            case RHValid::InvalidGroupNo:   return "InvalidGroupNo";
            case RHValid::InvalidThreadNo:  return "InvalidThreadNo";
            case RHValid::ScnLogicMismatch: return "ScnLogicMismatch";
            case RHValid::EpochMismatch:    return "EpochMismatch";
        }
        return "Unknown";
    }

    // --------------------------------------------------------------------------------
    void show(const RedoHead &head, std::ostream &os) {
        using coral::toHex;
        fmt::print(os,
                   "=================================================================\n"
                   "                    ORACLE REDO HEADER INFO                      \n"
                   "=================================================================\n"
                   "[ Validation Status ] : {}\n\n",
                   to_string(head.valid)
        );

        fmt::print(os, "{}", to_string(head.sourceInfo));
        fmt::print(os, "{}", to_string(head.writeInfo));
        fmt::print(os, "{}", to_string(head.threadState));
        fmt::print(os, "{}", to_string(head.fileState));
        fmt::print(os, "{}", to_string(head.keyInfo));
    }
}
