#pragma once

#include <array>
#include <iostream>
#include <string>
#include <tcb/span.hpp>

#include "coral_show.h"

namespace ora {

    /** 오라클 SCN (System Change Number) (64-bit) */
    struct SCN {
        uint32_t base{};  ///< SCN Base (하위 32비트, SCN_BASE / Minor)
        uint32_t wrap{};  ///< SCN Wrap (상위 32비트, Major_High 16b + Major 16b)
    };

    inline uint64_t scn_to64(const SCN& scn) {
        return (static_cast<uint64_t>(scn.wrap) << 32) | scn.base;
    }

    // SCN : 0x0000.00000000 (0)
    inline std::string toHex(const SCN& scn) {
        return fmt::format("0x{:04x}.{:08x} ({})", scn.wrap, scn.base, scn_to64(scn));
    }

    /** 오라클 버전 */
    struct OraVer {
        uint32_t raw_val{};
        uint8_t  major{};
        uint8_t  minor{};
        uint8_t  patch{};
        uint8_t  extra{};
    };

    // OraVer : 19.3.0.0
    inline std::string toHex(const OraVer& ver) {
        return fmt::format("{}.{}.{}.{}", ver.major, ver.minor, ver.patch, ver.extra);
    }

    /** 데이터베이스 및 리두 로그 메타데이터 기본 정보 */
    struct SourceInfo {
        OraVer   software_ver{};     ///< 오라클 소프트웨어 버전
        OraVer   compat_ver{};       ///< DB 호환성 레벨 버전
        uint32_t database_id{};      ///< DB 고유 식별자
        char     database_name[9]{}; ///< DB 이름 (최대 8자 + NULL 종결)
        uint32_t control_sequence{}; ///< 컨트롤 파일 시퀀스 번호
        uint32_t blocks_in_file{};   ///< 파일 내 전체 블록 수
        uint32_t block_sz{};         ///< 리두 로그 블록 크기
        uint16_t group_no{};         ///< 리두 로그 그룹 번호
        uint16_t file_type{};        ///< 오라클 내부 파일 유형 코드
        uint32_t activation_id{};    ///< RESETLOGS 실행 시 생성되는 DB 생애 주기 ID
        char     desc[65]{};         ///< 설명 텍스트
    };

    /** 리두 로그 쓰기 상태 및 SCN/타임스탬프 구간 정보 */
    struct WriteInfo {
        uint32_t nab{};              ///< Next Available Block
        uint32_t resetlogs_count{};  ///< RESETLOGS 실행 횟수
        SCN      resetlogs_scn{};
        uint32_t hws{};              ///< High-Water-Mark Sequence
        uint16_t thread_no{};        ///< RAC 쓰레드 번호
        SCN      low_scn{};
        uint32_t low_epoch{};
        SCN      next_scn{};
        uint32_t next_epoch{};
    };

    /** RAC 쓰레드 활성화 및 상태 정보 */
    struct ThreadState {
        uint8_t  eot{};              ///< End of Thread 플래그
        uint8_t  dis{};              ///< Thread Disabled 플래그
        SCN      enabled_scn{};
        uint32_t enabled_epoch{};
        SCN      close_scn{};
        uint32_t close_epoch{};
    };

    /** 파일 포맷 및 Standby/Data Guard 제어 정보 */
    struct FileState {
        uint32_t log_format_ver{};
        uint32_t flags{};
        SCN      terminal_scn{};
        uint32_t terminal_epoch{};
    };

    /** TDE (Transparent Data Encryption) 암호화 */
    struct TDEKeyInfo {
        std::array<uint8_t, 16> encrypt_key{};
        std::array<uint8_t, 16> master_key_id{};
        uint16_t key_flag{};
    };

    /** 리두 로그 헤더 검증 상태 코드 */
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

    inline std::string to_string(RHValid val) noexcept {
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
    /** 리두 로그 헤더 전체 구조체 */
    struct RedoHead {
        RHValid     valid{ RHValid::Empty};
        SourceInfo  sourceInfo{};
        WriteInfo   writeInfo{};
        ThreadState threadState{};
        FileState   fileState{};
        TDEKeyInfo  keyInfo{};
    };

    RedoHead RedoHead_of(const tcb::span<const char> &raw, bool isLittle);

    void show(const RedoHead &head, std::ostream &os = std::cout) ;

}
