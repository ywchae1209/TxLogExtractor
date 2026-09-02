#pragma once

#include <array>
#include <iostream>
#include <string>
#include <tcb/span.hpp>

#include "Block.h"
#include "../coral_show.h"
#include "../coral_decode.h"
#include "../ora_layout.h"
#include "../coral_result.h"

namespace ora {

    /** 데이터베이스 및 리두 로그 메타데이터 기본 정보 */
    struct SourceInfo {
        OraVer   software_ver{};     ///< 오라클 소프트웨어 버전
        OraVer   compat_ver{};       ///< DB 호환성 레벨 버전
        uint32_t database_id{};      ///< DB 고유 식별자
        std::string database_name{}; ///< DB 이름 (최대 8자 + NULL 종결)
        uint32_t control_sequence{}; ///< 컨트롤 파일 시퀀스 번호
        uint32_t blocks_in_file{};   ///< 파일 내 전체 블록 수
        uint32_t block_sz{};         ///< 리두 로그 블록 크기
        uint16_t group_no{};         ///< 리두 로그 그룹 번호
        uint16_t file_type{};        ///< 오라클 내부 파일 유형 코드
        uint32_t activation_id{};    ///< RESETLOGS 실행 시 생성되는 DB 생애 주기 ID
        std::string desc{};          ///< 설명 텍스트
    };

    /** 리두 로그 쓰기 상태 및 SCN/타임스탬프 구간 정보 */
    struct WriteInfo {
        uint32_t nab{};              ///< Next Available Block
        uint32_t resetlogs_count{};  ///< RESETLOGS 실행 횟수
        SCN      resetlogs_scn{};   ///
        uint32_t hws{};              ///< High-Water-Mark Sequence
        uint16_t thread_no{};        ///< RAC 쓰레드 번호
        SCN      low_scn{};         // MSB is flag.
        uint32_t low_epoch{};
        SCN      next_scn{};        // MSB is flag.
        uint32_t next_epoch{};
    };

    /** RAC 쓰레드 활성화 및 상태 정보 */
    struct ThreadState {
        uint8_t  eot{};              ///< End of Thread 플래그
        uint8_t  dis{};              ///< Thread Disabled 플래그
        SCN      enabled_scn{};
        uint32_t enabled_epoch{};
        SCN      close_scn{};       // MSB is flag.
        uint32_t close_epoch{};
    };

    /** 파일 포맷 및 Standby/Data Guard 제어 정보 */
    struct FileState {
        uint32_t log_format_ver{};
        uint32_t flags{};
        SCN      terminal_scn{};        // MSB is flag.
        uint32_t terminal_epoch{};
    };

    /** TDE (Transparent Data Encryption) 암호화 */
    struct TDEKeyInfo {
        std::array<uint8_t, 16> encrypt_key{};
        std::array<uint8_t, 16> master_key_id{};
        uint16_t key_flag{};
    };

    // --------------------------------------------------------------------------------
    enum class RHValid {
        Ok = 0,
        Empty,
        TooShort,
        InvalidFileType,
        InvalidBlockSize,
        InvalidNab,
        InvalidGroupNo,
        InvalidThreadNo,
        ScnLogicMismatch,
        EpochMismatch
    };

    // --------------------------------------------------------------------------------
    struct RedoHead {
        SourceInfo  sourceInfo{};
        WriteInfo   writeInfo{};
        ThreadState threadState{};
        FileState   fileState{};
        TDEKeyInfo  keyInfo{};

        uint32_t log_seq_no;
    };

    coral::Result<RedoHead> RedoHead_of(const Block&raw, bool isLittle);

    // --------------------------------------------------------------------------------
    void show(const RedoHead &head, std::ostream &os = std::cout) ;
    std::string to_string(const RHValid& val);
    std::string to_string(const SourceInfo& si);
    std::string to_string(const WriteInfo& wi);
    std::string to_string(const ThreadState& ts);
    std::string to_string(const FileState& fs);
    std::string to_string(const TDEKeyInfo& ki);

    inline static RHValid validate(const RedoHead& head) {

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
        if (wi.nab != 0xFFFFFFFF && (wi.nab == 0 || wi.nab > si.blocks_in_file + 1)) {
            fmt::println("InvalidNab : nab {} > blocks {} + 1", wi.nab, si.blocks_in_file);
            return RHValid::InvalidNab;
        }

        const uint64_t low_scn = scn_to64(wi.low_scn);
        const uint64_t nxt_scn = scn_to64(wi.next_scn);
        const uint64_t rst_scn = scn_to64(wi.resetlogs_scn);

        return RHValid::Ok;
    }

}
