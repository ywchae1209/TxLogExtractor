#pragma once

#include "RedoHead.h"
#include "../coral_decode.h"

namespace ora {
#pragma pack(push, 1)
    // 140
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

    // 48
    struct WriteInfo_lo{
        uint32_t nab;             // 파일내의 Next Available Block번호
        uint32_t resetlogs_count; // RESETLOGS 횟수(또는 timestamp)
        SCN      resetlogs_scn;   // RESETLOGS scn
        uint32_t hws;             // Oracle internal. High-Water-Mark Sequence.
        uint16_t thread_no;       // single = 1, RAC = 1, 2, ... !!
        uint8_t  pad[2];          //
        SCN      low_scn;         // low(start) scn in this log-file
        uint32_t low_epoch;       //
        SCN      next_scn;        // next scn (when CURRENT, 0xffff.ffffffff 01/01/1988 00:00:00)
        uint32_t next_epoch;      //
    };

    // 28
    struct ThreadState_lo{
        uint8_t  eot;             // flag ~ End of thread. (1 = last log of current-thread)
        uint8_t  dis;             // flag ~ Thread disabled.
        uint8_t  pad[2];          //
        SCN      enabled_scn;     // scn-epoch at thread enabled
        uint32_t enabled_epoch;   //
        SCN      close_scn;       // scn-epoch at thread closed/offline normally.
        uint32_t close_epoch;     //
    };

    // 216
    struct FileState_lo{
        uint32_t log_format_ver;  //
        uint32_t flags;           // flag ~ log-file state. (cleared, archived, ...)
        SCN      terminal_scn;    // scn-epoch for failover/switch-over control in standby-DB/ Data-Guard
        uint32_t terminal_epoch;  //
        uint8_t  pad[4];          //
        uint8_t  unknown[192];    //
    };

    // 38
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

    using coral::decode_at, coral::to_string_n;
    template <bool IsLittle>
    inline SourceInfo decode_source_info(tcb::span<const char> buf) noexcept {
        return SourceInfo{
            .software_ver     = to_OraVer(decode_at<uint32_t, IsLittle>(buf, 0)),
            .compat_ver       = to_OraVer(decode_at<uint32_t, IsLittle>(buf, 4)),
            .database_id      = decode_at<uint32_t, IsLittle>(buf, 8),
            .database_name    = to_string_n<8>(buf, 12),
            .control_sequence = decode_at<uint32_t, IsLittle>(buf, 20),
            .blocks_in_file   = decode_at<uint32_t, IsLittle>(buf, 24),
            .block_sz         = decode_at<uint32_t, IsLittle>(buf, 28),
            .group_no         = decode_at<uint16_t, IsLittle>(buf, 32),
            .file_type        = decode_at<uint16_t, IsLittle>(buf, 34),
            .activation_id    = decode_at<uint32_t, IsLittle>(buf, 36),
            // pad 36 (offset 40..75)
            .desc             = to_string_n<64>(buf, 76)
        };
    }

    template <bool IsLittle>
    inline WriteInfo decode_write_info(tcb::span<const char> buf) noexcept {
        return WriteInfo{
            .nab              = decode_at<uint32_t, IsLittle>(buf, 0),
            .resetlogs_count  = decode_at<uint32_t, IsLittle>(buf, 4),
            .resetlogs_scn    = decode_scn0l_at   <IsLittle>(buf, 8),
            .hws              = decode_at<uint32_t, IsLittle>(buf, 16),
            .thread_no        = decode_at<uint16_t, IsLittle>(buf, 20),
            // pad 2 (offset 22..23)
            .low_scn          = decode_redo_scn0_at<IsLittle>(buf, 24),
            .low_epoch        = decode_at<uint32_t, IsLittle>(buf, 32),
            .next_scn         = decode_redo_scn0_at<IsLittle>(buf, 36),
            .next_epoch       = decode_at<uint32_t, IsLittle>(buf, 44)
        };
    }
    template <bool IsLittle>
    inline ThreadState decode_thread_state(tcb::span<const char> buf) noexcept {
        return ThreadState{
            .eot              = decode_at<uint8_t,  IsLittle>(buf, 0),
            .dis              = decode_at<uint8_t,  IsLittle>(buf, 1),
            // pad 2 (offset 2..3)
            .enabled_scn      = decode_scn0l_at   <IsLittle>(buf, 4),
            .enabled_epoch    = decode_at<uint32_t, IsLittle>(buf, 12),
            .close_scn        = decode_scn0l_at   <IsLittle>(buf, 16),
            .close_epoch      = decode_at<uint32_t, IsLittle>(buf, 24)
        };
    }

    template <bool IsLittle>
    inline FileState decode_file_state(tcb::span<const char> buf) noexcept {
        return FileState{
            .log_format_ver   = decode_at<uint32_t, IsLittle>(buf, 0),
            .flags            = decode_at<uint32_t, IsLittle>(buf, 4),
            .terminal_scn     = decode_scn0l_at   <IsLittle>(buf, 8),
            .terminal_epoch   = decode_at<uint32_t, IsLittle>(buf, 16)
        };
    }

    template <bool IsLittle>
    inline TDEKeyInfo decode_tde_key(tcb::span<const char> buf) noexcept {

        TDEKeyInfo o{};

        // todo :: check -- Solaris(SPARC-based), HP-UX( PA-RISC, Itanium), AIX (POWER, PowerPC)
        // Big-Endian(AIX 등)인 경우 패딩 4바이트를 건너뛰고 오프셋 +4 지점에서 복사
        // Solaris(SPARC-based), HP-UX( PA-RISC, Itanium), AIX (POWER, PowerPC)

        auto first4 = decode_at<uint32_t, IsLittle>(buf, 0); // pad_aix
        const size_t offset = (!IsLittle && (first4 == 0)) ? 4 : 0;
        const uint8_t* ptr  = reinterpret_cast<const uint8_t*>(buf.data() + offset);

        return TDEKeyInfo{
            .encrypt_key   = coral::copy_bytes<16>(ptr),
            .master_key_id = coral::copy_bytes<16>(ptr + 16),
            .key_flag      = coral::decode_at<uint16_t, IsLittle>(buf, offset + 32)
        };
    }

    template<bool IsLittle>
    inline RedoHead decode_redo_head0(tcb::span<const char> buf) noexcept {
        return RedoHead{
            .sourceInfo = decode_source_info<IsLittle>(buf.subspan(0)),   // (140, offset 0)
            .writeInfo  = decode_write_info<IsLittle>(buf.subspan(140)),  // (48, offset 140)
            .threadState= decode_thread_state<IsLittle>(buf.subspan(188)),// (28, offset 188)
            .fileState  = decode_file_state<IsLittle>(buf.subspan(216)),  // (216, offset 216)
            .keyInfo    = decode_tde_key<IsLittle>(buf.subspan(432))      // (38,offset 432)
        };
    }

    inline coral::Result<RedoHead> decode_redo_head(const Block &block,
                                                    bool isLittle) noexcept {

        // don't need check size :: payload().size > (512-16)
        const tcb::span buf = block.payload();
        auto out = isLittle
                       ? decode_redo_head0<true>(buf)
                       : decode_redo_head0<false>(buf);

        out.log_seq_no = block.log_seq_no();

        return out;
    }
}