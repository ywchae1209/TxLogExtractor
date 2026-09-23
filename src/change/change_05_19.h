#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_decode.h"
#include "../coral_combinator.h"

/// {5, 19, "KTUTSL", "Transaction start audit log record"},
namespace ora {

    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    enum class TxK : uint8_t {
        VERSION,
        AUDIT_SESSION_ID,
        SESSION_NUMBER,
        SERIAL_NUMBER,
        CURRENT_USER_NAME,
        LOGIN_USER_NAME,
        CLIENT_INFO,
        OS_USER_NAME,
        MACHINE_NAME,
        OS_TERMINAL,
        OS_PROCESS_ID,
        OS_PROGRAM_NAME,
        TRANSACTION_NAME,
        CLIENT_ID,
        GLOBAL_TRANSACTION_ID, // todo check ::: Global Tx id (GTRID) 또는 PDB Global Service Name / Shard Key Context
        STREAMS_TAG,           // DBMS_STREAMS Tag (Raw/Hex string)

        // audit flag
        DDL_TRANSACTION,
        SPACE_MANAGEMENT_TRANSACTION,
        RECURSIVE_TRANSACTION,
        LOGMINER_INTERNAL_TRANSACTION,
        DB_OPEN_IN_MIGRATE_MODE,
        LSBY_IGNORE,
        LOGMINER_NO_TX_CHUNKING,
        LOGMINER_STEALTH_TRANSACTION,
        LSBY_PRESERVE,
        LOGMINER_MARKER_TRANSACTION,
        TRANSACTION_IN_PRAGMAED_PLSQL,
        DISABLED_LOGICAL_REPLICATION_TRANSACTION,
        DATAPUMP_IMPORT_TRANSACTION,
        TRANSACTION_AUDIT_CV_FLAGS_UNDEFINED,
        FEDERATION_PDB_REPLAY,
        PDB_DDL_REPLAY,
        LOGMINER_SKIP_TRANSACTION,
        SEQ_UPDATE_TRANSACTION
    };

    constexpr auto VERSION                                   = TxK::VERSION;
    constexpr auto AUDIT_SESSION_ID                          = TxK::AUDIT_SESSION_ID;
    constexpr auto SESSION_NUMBER                            = TxK::SESSION_NUMBER;
    constexpr auto SERIAL_NUMBER                             = TxK::SERIAL_NUMBER;
    constexpr auto CURRENT_USER_NAME                         = TxK::CURRENT_USER_NAME;
    constexpr auto LOGIN_USER_NAME                           = TxK::LOGIN_USER_NAME;
    constexpr auto CLIENT_INFO                               = TxK::CLIENT_INFO;
    constexpr auto OS_USER_NAME                              = TxK::OS_USER_NAME;
    constexpr auto MACHINE_NAME                              = TxK::MACHINE_NAME;
    constexpr auto OS_TERMINAL                               = TxK::OS_TERMINAL;
    constexpr auto OS_PROCESS_ID                             = TxK::OS_PROCESS_ID;
    constexpr auto OS_PROGRAM_NAME                           = TxK::OS_PROGRAM_NAME;
    constexpr auto TRANSACTION_NAME                          = TxK::TRANSACTION_NAME;
    constexpr auto CLIENT_ID                                 = TxK::CLIENT_ID;
    constexpr auto GLOBAL_TRANSACTION_ID                     = TxK::GLOBAL_TRANSACTION_ID;
    constexpr auto STREAMS_TAG                               = TxK::STREAMS_TAG;
    constexpr auto DDL_TRANSACTION                           = TxK::DDL_TRANSACTION;
    constexpr auto SPACE_MANAGEMENT_TRANSACTION              = TxK::SPACE_MANAGEMENT_TRANSACTION;
    constexpr auto RECURSIVE_TRANSACTION                     = TxK::RECURSIVE_TRANSACTION;
    constexpr auto LOGMINER_INTERNAL_TRANSACTION             = TxK::LOGMINER_INTERNAL_TRANSACTION;
    constexpr auto DB_OPEN_IN_MIGRATE_MODE                   = TxK::DB_OPEN_IN_MIGRATE_MODE;
    constexpr auto LSBY_IGNORE                               = TxK::LSBY_IGNORE;
    constexpr auto LOGMINER_NO_TX_CHUNKING                   = TxK::LOGMINER_NO_TX_CHUNKING;
    constexpr auto LOGMINER_STEALTH_TRANSACTION              = TxK::LOGMINER_STEALTH_TRANSACTION;
    constexpr auto LSBY_PRESERVE                             = TxK::LSBY_PRESERVE;
    constexpr auto LOGMINER_MARKER_TRANSACTION               = TxK::LOGMINER_MARKER_TRANSACTION;
    constexpr auto TRANSACTION_IN_PRAGMAED_PLSQL             = TxK::TRANSACTION_IN_PRAGMAED_PLSQL;
    constexpr auto DISABLED_LOGICAL_REPLICATION_TRANSACTION  = TxK::DISABLED_LOGICAL_REPLICATION_TRANSACTION;
    constexpr auto DATAPUMP_IMPORT_TRANSACTION               = TxK::DATAPUMP_IMPORT_TRANSACTION;
    constexpr auto TRANSACTION_AUDIT_CV_FLAGS_UNDEFINED      = TxK::TRANSACTION_AUDIT_CV_FLAGS_UNDEFINED;
    constexpr auto FEDERATION_PDB_REPLAY                     = TxK::FEDERATION_PDB_REPLAY;
    constexpr auto PDB_DDL_REPLAY                            = TxK::PDB_DDL_REPLAY;
    constexpr auto LOGMINER_SKIP_TRANSACTION                 = TxK::LOGMINER_SKIP_TRANSACTION;
    constexpr auto SEQ_UPDATE_TRANSACTION                    = TxK::SEQ_UPDATE_TRANSACTION;

    // --------------------------------------------------------------------------------
    /// {5, 19, "KTUTSL", "Transaction start audit log record"},
    struct Change_0519 {
        std::unordered_map<TxK, std::string> attributes;

        void set(TxK key, std::string_view v) {
            if (!v.empty()) attributes.insert_or_assign(key, std::string(v));
        }

        void set_flag(TxK key, bool cond = true) {
            if (cond) attributes.insert_or_assign(key, "true");
        }

        optional<std::string_view> get(TxK key) const noexcept {
            auto it = attributes.find(key);
            if (it != attributes.end()) return std::string_view(it->second);
            return nullopt;
        }

        bool has_flag(TxK key) const noexcept {
            auto it = attributes.find(key);
            return (it != attributes.end() && it->second == "true");
        }
    };

    // --------------------------------------------------------------------------------
    // [#1] Session & Serial
    inline void decode_session_serial(tcb::span<const char> buf,
                                          bool isLittle,
                                          bool over19,
                                          Change_0519 &out) {
        if (buf.size() < 4) return;

        const uint16_t serial_number = decode_At<uint16_t>(buf, isLittle, 2);
        uint32_t session_number = 0;

        if (over19) {
            if (buf.size() < 8) return;
            session_number = decode_At<uint32_t>(buf, isLittle, 4);
        } else {
            session_number = decode_At<uint16_t>(buf, isLittle, 0);
        }

        out.set(SESSION_NUMBER, std::to_string(session_number));
        out.set(SERIAL_NUMBER, std::to_string(serial_number));
    }

    // [#11] Transaction Audit Flags
    inline void decode_audit_flags(
        const tcb::span<const char> buf,
        const bool isLittle,
        Change_0519 &o)
    {
        if (buf.size() < 2) return;

        const uint16_t flags = decode_At<uint16_t>(buf, isLittle, 0);

        o.set_flag(DDL_TRANSACTION,                         (flags & 0x0001) != 0);
        o.set_flag(SPACE_MANAGEMENT_TRANSACTION,            (flags & 0x0002) != 0);
        o.set_flag(RECURSIVE_TRANSACTION,                   (flags & 0x0004) != 0);
        o.set_flag(LOGMINER_INTERNAL_TRANSACTION,           (flags & 0x0008) != 0);
        o.set_flag(DB_OPEN_IN_MIGRATE_MODE,                 (flags & 0x0010) != 0);
        o.set_flag(LSBY_IGNORE,                             (flags & 0x0020) != 0);
        o.set_flag(LOGMINER_NO_TX_CHUNKING,                 (flags & 0x0040) != 0);
        o.set_flag(LOGMINER_STEALTH_TRANSACTION,            (flags & 0x0080) != 0);
        o.set_flag(LSBY_PRESERVE,                           (flags & 0x0100) != 0);
        o.set_flag(LOGMINER_MARKER_TRANSACTION,             (flags & 0x0200) != 0);
        o.set_flag(TRANSACTION_IN_PRAGMAED_PLSQL,           (flags & 0x0400) != 0);
        o.set_flag(DISABLED_LOGICAL_REPLICATION_TRANSACTION,(flags & 0x0800) != 0);
        o.set_flag(DATAPUMP_IMPORT_TRANSACTION,             (flags & 0x1000) != 0);
        o.set_flag(TRANSACTION_AUDIT_CV_FLAGS_UNDEFINED,    (flags & 0x8000) != 0);

        if (buf.size() >= 6) {
            const uint16_t flags2 = decode_At<uint16_t>(buf, isLittle, 4);
            o.set_flag(FEDERATION_PDB_REPLAY,     (flags2 & 0x0001) != 0);
            o.set_flag(PDB_DDL_REPLAY,            (flags2 & 0x0002) != 0);
            o.set_flag(LOGMINER_SKIP_TRANSACTION, (flags2 & 0x0004) != 0);
            o.set_flag(SEQ_UPDATE_TRANSACTION,    (flags2 & 0x0008) != 0);
        }
    }

    // --------------------------------------------------------------------------------
    [[nodiscard]] inline Result<Change_0519> parse_0519(SpanCursor &ctx) {

        Change_0519 o;

        auto as_str = [](tcb::span<const char> s) { return std::string_view(s.data(), s.size()); };
        auto as_u32_str = [&](tcb::span<const char> s) {
            return s.size() >= 4
                       ? std::to_string(decode_At<uint32_t>(s, ctx.isLittle, 0))
                       : std::string(s.data(), s.size());
        };

        if (auto s= ctx.next("Ch5_19:f1"); s) decode_session_serial(*s, ctx.isLittle, ctx.over19, o); else return o;

        if (auto s= ctx.next("Ch5_19:f2"); s) o.set(CURRENT_USER_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f3"); s) o.set(LOGIN_USER_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f4"); s) o.set(CLIENT_INFO, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f5"); s) o.set(OS_USER_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f6"); s) o.set(MACHINE_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f7"); s) o.set(OS_TERMINAL, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f8"); s) o.set(OS_PROCESS_ID, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f9"); s) o.set(OS_PROGRAM_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f10"); s) o.set(TRANSACTION_NAME, as_str(*s)); else return o;

        if (auto s= ctx.next("Ch5_19:f11"); s) decode_audit_flags(*s, ctx.isLittle, o); else return o;

        if (auto s= ctx.next("Ch5_19:f12",4); s) o.set(VERSION, as_u32_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f13",4); s) o.set(AUDIT_SESSION_ID, as_u32_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f14"); s) o.set(CLIENT_ID, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f15"); s) o.set(GLOBAL_TRANSACTION_ID, as_str(*s));   // todo :: need check
        return o;
    }

    // --------------------------------------------------------------------------------
   constexpr std::string_view to_st_view(TxK key) noexcept {
        switch (key) {
            case VERSION:                                  return "version";
            case AUDIT_SESSION_ID:                         return "audit session id";
            case SESSION_NUMBER:                           return "session number";
            case SERIAL_NUMBER:                            return "serial number";
            case CURRENT_USER_NAME:                        return "current user name";
            case LOGIN_USER_NAME:                          return "login username";
            case CLIENT_INFO:                              return "client info";
            case OS_USER_NAME:                             return "os username";
            case MACHINE_NAME:                             return "machine name";
            case OS_TERMINAL:                              return "os terminal";
            case OS_PROCESS_ID:                            return "os process id";
            case OS_PROGRAM_NAME:                          return "os program name";
            case TRANSACTION_NAME:                         return "transaction name";
            case CLIENT_ID:                                return "client id";
            case GLOBAL_TRANSACTION_ID:                    return "global transaction id";
            case STREAMS_TAG:                              return "streams tag";

            // Audit Flags
            case DDL_TRANSACTION:                          return "DDL transaction";
            case SPACE_MANAGEMENT_TRANSACTION:             return "Space Management transaction";
            case RECURSIVE_TRANSACTION:                    return "Recursive transaction";
            case LOGMINER_INTERNAL_TRANSACTION:            return "LogMiner Internal transaction";
            case DB_OPEN_IN_MIGRATE_MODE:                  return "DB Open in Migrate Mode";
            case LSBY_IGNORE:                              return "LSBY ignore";
            case LOGMINER_NO_TX_CHUNKING:                  return "LogMiner no tx chunking";
            case LOGMINER_STEALTH_TRANSACTION:             return "LogMiner Stealth transaction";
            case LSBY_PRESERVE:                            return "LSBY preserve";
            case LOGMINER_MARKER_TRANSACTION:              return "LogMiner Marker transaction";
            case TRANSACTION_IN_PRAGMAED_PLSQL:            return "Transaction in pragma'ed plsql";
            case DISABLED_LOGICAL_REPLICATION_TRANSACTION: return "Disabled Logical Repln. txn.";
            case DATAPUMP_IMPORT_TRANSACTION:              return "Datapump import txn";
            case TRANSACTION_AUDIT_CV_FLAGS_UNDEFINED:     return "Tx audit CV flags undefined";
            case FEDERATION_PDB_REPLAY:                    return "Federation PDB replay";
            case PDB_DDL_REPLAY:                           return "PDB DDL replay";
            case LOGMINER_SKIP_TRANSACTION:                return "LogMiner SKIP transaction";
            case SEQ_UPDATE_TRANSACTION:                   return "SEQ update transaction";
        }
        return "unknown";
    }

    inline std::string to_string(const Change_0519& ch519, bool multiline = true) {

        std::string out;
        out.reserve(256);

        for (const auto& [key, val] : ch519.attributes) {
            std::string_view key_str = to_st_view(key);
            if (val == "true") {
                if (multiline) fmt::format_to(std::back_inserter(out), "{}\n", key_str);
                else           fmt::format_to(std::back_inserter(out), "[{}] ", key_str);
            } else {
                if (multiline) fmt::format_to(std::back_inserter(out), "{:<20} = {}\n", key_str, val);
                else           fmt::format_to(std::back_inserter(out), "{}={} ", key_str, val);
            }
        }
        return out;
    }
}