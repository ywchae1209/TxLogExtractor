#include <cstdint>
#include <cstring>
#include <tcb/span.hpp>

#include "coral_decode.h"
#include "RedoHead.h"

namespace ora {

#pragma pack(push, 1)
    struct SCN_lo{
        uint32_t minor;            // base
        uint16_t major;            // wrap
        uint16_t major_high;       // wrap_high. (before ora.11g = 0)
    };

    struct SourceInfo_lo{
        uint32_t software_ver;     //
        uint32_t compat_ver;       //
        uint32_t database_id;      // database ID !!
        char     database_name[8]; // ex: 'ORACLE'
        uint32_t control_sequence; // ora internal. 로그 생성시점의 컨트롤파일시퀀스번호
        uint32_t blocks_in_file;   // 블록갯수 ex) 40K => 40K X block(0.5K) = 20MB
        uint32_t block_sz;         //
        uint16_t group_no;         // 그룹번호 in redo-log-group !!
        uint16_t file_type;        // 2 == LOG
        uint32_t activation_id;    // reset RESETLOGS !!
        uint8_t  pad[36];          //
        char     desc[64];         //"Thread 0001, S 0000003344, SCN 0x000000004344ae53-0xffffffffffffffff"
    };

    struct WriteInfo_lo{
        uint32_t nab;             // 파일내의 Next Available Block번호
        uint32_t resetlogs_count; // RESETLOGS 횟수(또는 timestamp)
        SCN_lo   resetlogs_scn;   // RESETLOGS scn
        uint32_t hws;             // Oracle internal. High-Water-Mark Sequence.
        uint16_t thread_no;       // single = 1, RAC = 1, 2, ... !!
        uint8_t  pad[2];          //
        SCN_lo   low_scn;         // low(start) scn in this log-file
        uint32_t low_epoch;       //
        SCN_lo   next_scn;        // next scn (when CURRENT, 0xffff.ffffffff 01/01/1988 00:00:00)
        uint32_t next_epoch;      //
    };

    struct ThreadState_lo{
        uint8_t  eot;             // flag ~ End of thread. (1 = last log of current-thread)
        uint8_t  dis;             // flag ~ Thread disabled.
        uint8_t  pad[2];          //
        SCN_lo   enabled_scn;     // scn-epoch at thread enabled
        uint32_t enabled_epoch;   //
        SCN_lo   close_scn;       // scn-epoch at thread closed/offline normally.
        uint32_t close_epoch;     //
    };

    struct FileState_lo{
        uint32_t log_format_ver;  //
        uint32_t flags;           // flag ~ log-file state. (cleared, archived, ...)
        SCN_lo   terminal_scn;    // scn-epoch for failover/switch-over control in standby-DB/ Data-Guard
        uint32_t terminal_epoch;  //
        uint8_t  pad[4];          //
        uint8_t  unknown[192];    //
    };

    struct TDEKey_lo{
        uint32_t pad_aix;           // todo :: check BigEndian OS :: AIX,
        uint8_t  encrypt_key[16];   //
        uint8_t  master_key_id[16]; //
        uint8_t  key_flag[2];       //
    };

    struct RedoHead_lo{
        SourceInfo_lo  source_info;
        WriteInfo_lo   write_info;
        ThreadState_lo threadState;
        FileState_lo   fileState;
        TDEKey_lo      keyInfo;
    };

#pragma pack(pop)
    SCN decode_SCN(const SCN_lo& raw, const bool isLittle) {

        const uint32_t minor      = coral::decode(raw.minor, isLittle);
        const uint32_t major      = coral::decode(raw.major, isLittle);
        const uint32_t major_high = coral::decode(raw.major_high, isLittle);

        SCN o = {};
        o.base = minor;
        // o.wrap = (major_high << 16) | major; // major_high may be padding garbage.
        o.wrap = major;

        return o;
    }

    OraVer decode_OraVer(const uint32_t n, const bool isLittle) {

        const uint32_t o = coral::decode(n, isLittle);
        return OraVer{
            o,
            static_cast<uint8_t>((o >> 24) & 0xFF),
            static_cast<uint8_t>((o >> 16) & 0xFF),
            static_cast<uint8_t>((o >> 8)  & 0xFF),
            static_cast<uint8_t>(o & 0xFF)
        };
    }

    SourceInfo decode_SourceInfo(const SourceInfo_lo& raw, const bool isLittle) {
        SourceInfo o{};

        o.software_ver     = decode_OraVer(raw.software_ver, isLittle);
        o.compat_ver       = decode_OraVer(raw.compat_ver, isLittle);
        o.database_id      = coral::decode(raw.database_id, isLittle);
        o.control_sequence = coral::decode(raw.control_sequence, isLittle);
        o.blocks_in_file   = coral::decode(raw.blocks_in_file, isLittle);
        o.block_sz         = coral::decode(raw.block_sz, isLittle);
        o.activation_id    = coral::decode(raw.activation_id, isLittle);
        o.group_no         = coral::decode(raw.group_no, isLittle);
        o.file_type        = coral::decode(raw.file_type, isLittle);

        std::memcpy(o.database_name, raw.database_name, 8);
        o.database_name[8] = '\0';

        std::memcpy(o.desc, raw.desc, 64);
        o.desc[64] = '\0';

        return o;
    }

    WriteInfo decode_WriteInfo(const WriteInfo_lo& raw, const bool isLittle) {
        WriteInfo o{};

        o.nab              = coral::decode(raw.nab, isLittle);
        o.resetlogs_count  = coral::decode(raw.resetlogs_count, isLittle);
        o.resetlogs_scn    = decode_SCN(raw.resetlogs_scn, isLittle);
        o.hws              = coral::decode(raw.hws, isLittle);
        o.thread_no        = coral::decode(raw.thread_no, isLittle);
        o.low_scn          = decode_SCN(raw.low_scn, isLittle);
        o.low_epoch        = coral::decode(raw.low_epoch, isLittle);
        o.next_scn         = decode_SCN(raw.next_scn, isLittle);
        o.next_epoch       = coral::decode(raw.next_epoch, isLittle);

        return o;
    }

    ThreadState decode_ThreadState(const ThreadState_lo& raw, const bool isLittle) {
        ThreadState o{};

        o.eot           = raw.eot;
        o.dis           = raw.dis;
        o.enabled_scn   = decode_SCN(raw.enabled_scn, isLittle);
        o.enabled_epoch = coral::decode(raw.enabled_epoch, isLittle);
        o.close_scn     = decode_SCN(raw.close_scn, isLittle);
        o.close_epoch   = coral::decode(raw.close_epoch, isLittle);

        return o;
    }

    FileState decode_FileState(const FileState_lo& raw, const bool isLittle) {
        FileState o{};

        o.log_format_ver = coral::decode(raw.log_format_ver, isLittle);
        o.flags          = coral::decode(raw.flags, isLittle);
        o.terminal_scn   = decode_SCN(raw.terminal_scn, isLittle);
        o.terminal_epoch = coral::decode(raw.terminal_epoch, isLittle);
        return o;
    }

    TDEKeyInfo decode_KeyInfo(const TDEKey_lo& raw, const bool isLittle) {

        TDEKeyInfo o{};

        // todo :: check -- Solaris(SPARC-based), HP-UX( PA-RISC, Itanium), AIX (POWER, PowerPC)
        // Big-Endian(AIX 등)인 경우 패딩 4바이트를 건너뛰고 오프셋 +4 지점에서 복사
        // Solaris(SPARC-based), HP-UX( PA-RISC, Itanium), AIX (POWER, PowerPC)
        const uint8_t* ptr = (isLittle || (raw.pad_aix != 0))
                             ? reinterpret_cast<const uint8_t*>(&raw)
                             : reinterpret_cast<const uint8_t*>(&raw) + 4;

        std::memcpy(o.encrypt_key.data(),   ptr,      16);
        std::memcpy(o.master_key_id.data(), ptr + 16, 16);
        o.key_flag = coral::decode(*reinterpret_cast<const uint16_t*>(ptr + 32), isLittle);

        return o;
    }

    RHValid validate(const RedoHead& head) {

        constexpr int MAX_RAC   = 1024;
        constexpr int MAX_GROUP = 1024;

        const SourceInfo& si = head.sourceInfo;
        const WriteInfo&  wi = head.writeInfo;

        // 1. File Type: redo-log == 2
        if (si.file_type != 2) return RHValid::InvalidFileType;

        // 2. Block Size: 512 || 1024 || 4096
        if (si.block_sz != 512 && si.block_sz != 1024 && si.block_sz != 4096)
            return RHValid::InvalidBlockSize;

        // 3. Group Number: 1 <= group_no <= 1024
        if (si.group_no == 0 || si.group_no > MAX_GROUP)
            return RHValid::InvalidGroupNo;

        // 4. Thread Number: 1 <= thread_no <= 1024
        if (wi.thread_no == 0 || wi.thread_no > MAX_RAC)
            return RHValid::InvalidThreadNo;

        // 5. NAB (Next Available Block)
        if (wi.nab == 0 || wi.nab > (si.blocks_in_file + 1))
            return RHValid::InvalidNab;

        const uint64_t low_scn = scn_to64(wi.low_scn);
        const uint64_t nxt_scn = scn_to64(wi.next_scn);
        const uint64_t rst_scn = scn_to64(wi.resetlogs_scn);

        // Resetlogs-SCN < Low-SCN
        if (rst_scn > low_scn) return RHValid::ScnLogicMismatch;

        // Low-SCN < Next-SCN
        if (nxt_scn != 0xFFFFFFFFFFFFFFFFULL) {
            if (low_scn > nxt_scn) return RHValid::ScnLogicMismatch;
            if (wi.low_epoch > wi.next_epoch) return RHValid::EpochMismatch;
        }

        return RHValid::Ok;
    }

    RedoHead decode_RedoHead(const RedoHead_lo& raw, const bool isLittle) {

        RedoHead o {
            RHValid::Ok,
            decode_SourceInfo(raw.source_info, isLittle),
            decode_WriteInfo(raw.write_info, isLittle),
            decode_ThreadState(raw.threadState, isLittle),
            decode_FileState(raw.fileState, isLittle),
            decode_KeyInfo(raw.keyInfo, isLittle)
        };

        o.valid = validate(o);

        return o;
    }

    // function to export
    RedoHead RedoHead_of(const tcb::span<const char> &raw, const bool isLittle) {
        if (raw.size() < sizeof(RedoHead_lo))
            return RedoHead{RHValid::TooShort};

        const auto lo = reinterpret_cast<const RedoHead_lo *>(raw.data());

        return decode_RedoHead(*lo, isLittle);
    }

    void show(const RedoHead &head, std::ostream &os) {
        using coral::toHex;
        fmt::print(os,
                   "=================================================================\n"
                   "                    ORACLE REDO HEADER INFO                      \n"
                   "=================================================================\n"
                   "[ Validation Status ] : {}\n\n",
                   to_string(head.valid)
        );

        // 1. Source Info
        // -----------------------------------------------------------------
        const auto &si = head.sourceInfo;
        const uint64_t mb_in_file = static_cast<uint64_t>(si.blocks_in_file) * si.block_sz / 1024 / 1024;

        fmt::print(os,
                   "--- [ Source Info ] ---\n"
                   "  Software Version   : {}\n"
                   "  Compatible Version : {}\n"
                   "  Database Name      : {}\n"
                   "  Database ID        : {} ({})\n"
                   "  Activation ID      : {}\n"
                   "  Control Sequence   : {}\n"
                   "* Block Size         : {} bytes\n"
                   "* Blocks in file     : {} ({} MB)\n"
                   "  Group No/ file-type: Group {} / Type {}\n"
                   "  Description        : {}\n\n",
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
                   si.desc
        );

        // 2. Write Info
        // -----------------------------------------------------------------
        const auto &wi = head.writeInfo;
        fmt::print(os,
                   "--- [ Write Info ] ---\n"
                   "* NAB (Next Avail Blk): {}\n"
                   "  Resetlogs Count    : {}\n"
                   "  Resetlogs SCN      : {}\n"
                   "  HWS(High-Water Seq): {}\n"
                   "  Thread No          : {}\n"
                   "  Low  SCN / epoch   : {} / epoch: {}\n"
                   "  Next SCN / epoch   : {} / epoch: {}\n\n",
                   wi.nab,
                   wi.resetlogs_count,
                   toHex(wi.resetlogs_scn),
                   wi.hws,
                   wi.thread_no,
                   toHex(wi.low_scn), wi.low_epoch,
                   toHex(wi.next_scn), wi.next_epoch
        );

        // 3. Thread State
        // -----------------------------------------------------------------
        const auto &ts = head.threadState;
        fmt::print(os,
                   "--- [ Thread State ] ---\n"
                   "  EOT Flag           : {} (End Of Thread)\n"
                   "  DIS Flag           : {} (Disabled flag)\n"
                   "  Enabled SCN / epoch: {} / epoch: {}\n"
                   "  Close   SCN / epoch: {} / epoch: {}\n\n",
                   toHex(ts.eot),
                   toHex(ts.dis),
                   toHex(ts.enabled_scn), ts.enabled_epoch,
                   toHex(ts.close_scn), ts.close_epoch
        );

        // 4. File State
        // -----------------------------------------------------------------
        const auto &fs = head.fileState;
        fmt::print(os,
                   "--- [ File State ] ---\n"
                   "  Log Format Version : {}\n"
                   "  Flags              : {}\n"
                   "  Terminal SCN/Epoch : {} / Epoch: {}\n\n",
                   fs.log_format_ver,
                   toHex(fs.flags),
                   toHex(fs.terminal_scn),
                   fs.terminal_epoch
        );

        // 5. TDE Key Info
        // -----------------------------------------------------------------
        const auto &ki = head.keyInfo;
        fmt::print(os,
                   "--- [ TDE Key Info ] ---\n"
                   "  Master Key ID      : {}\n"
                   "  Encrypt Key        : {}\n"
                   "  Key Flag           : {}\n"
                   "=================================================================\n",
                   toHex(ki.master_key_id.data(), ki.master_key_id.size()),
                   toHex(ki.encrypt_key.data(), ki.encrypt_key.size()),
                   toHex(ki.key_flag)
        );
    }
}
