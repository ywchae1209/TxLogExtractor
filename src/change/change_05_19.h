#pragma once
#include <fmt/format.h>
#include <optional>
#include <string>
#include <string_view>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "change_aud.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

/// {5, 19, "KTUTSL", "Transaction start audit log record"},
namespace ora {

    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    /// {5, 19, "KTUTSL", "Transaction start audit log record"},
    struct Change_0519: Change_Aud {
        using Change_Aud::Change_Aud;
        static Result<Change_0519> parse(SpanCursor& ctx);
    };

    // --------------------------------------------------------------------------------
    [[nodiscard]] inline Result<Change_0519> Change_0519::parse(SpanCursor &ctx) {

        Change_0519 o;

        auto as_str = [](tcb::span<const char> s) { return std::string_view(s.data(), s.size()); };
        auto as_u32_str = [&](tcb::span<const char> s) {
            return s.size() >= 4
                       ? std::to_string(decode_At<uint32_t>(s, ctx.isLittle, 0))
                       : std::string(s.data(), s.size());
        };

        if (auto s= ctx.next("Ch5_19:f1"); s) o.decode_session_serial(*s, ctx.isLittle, ctx.over19); else return o;

        if (auto s= ctx.next("Ch5_19:f2"); s) o.set(CURRENT_USER_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f3"); s) o.set(LOGIN_USER_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f4"); s) o.set(CLIENT_INFO, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f5"); s) o.set(OS_USER_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f6"); s) o.set(MACHINE_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f7"); s) o.set(OS_TERMINAL, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f8"); s) o.set(OS_PROCESS_ID, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f9"); s) o.set(OS_PROGRAM_NAME, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f10"); s) o.set(TRANSACTION_NAME, as_str(*s)); else return o;

        if (auto s= ctx.next("Ch5_19:f11"); s) o.decode_audit_flags(*s, ctx.isLittle); else return o;

        if (auto s= ctx.next("Ch5_19:f12",4); s) o.set(VERSION, as_u32_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f13",4); s) o.set(AUDIT_SESSION_ID, as_u32_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f14"); s) o.set(CLIENT_ID, as_str(*s)); else return o;
        if (auto s= ctx.next("Ch5_19:f15"); s) o.set(GLOBAL_TRANSACTION_ID, as_str(*s));   // todo :: need check
        return o;
    }
    // --------------------------------------------------------------------------------

    inline std::string to_string(const Change_0519& a) {
        auto str = a.Change_Aud::to_string();
        return fmt::format("Ch 5.19: \n{}", str);
    }

}