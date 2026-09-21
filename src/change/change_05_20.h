#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_decode.h"
#include "../coral_combinator.h"
#include "Change_05_19.h"

/// {5, 20, "KTUTSC", "Transaction continue audit log record"},

namespace ora {
    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    // --------------------------------------------------------------------------------

    /// {5, 20, "KTUTSC", "Transaction continue audit log record"},
    /// - alias of Change_0519 (same struct)
    using Change_0520 = Change_0519;

    // --------------------------------------------------------------------------------
    inline Result<Change_0520> parse_0520(SpanCursor &ctx) {

        Change_0520 o;

        auto as_str = [](tcb::span<const char> s) { return std::string_view(s.data(), s.size()); };
        auto as_u32_str = [&](tcb::span<const char> s) {
            return s.size() >= 4
                       ? std::to_string(decode_At<uint32_t>(s, ctx.isLittle, 0))
                       : std::string(s.data(), s.size());
        };

        if (auto s = ctx.next("Ch5_20:f1"); s) decode_session_serial(*s, ctx.isLittle, ctx.over19, o); else return o;
        if (auto s = ctx.next("Ch5_20:f2"); s) o.set(TRANSACTION_NAME, as_str(*s)); else return o;
        if (auto s = ctx.next("Ch5_20:f3"); s) decode_audit_flags(*s, ctx.isLittle, o); else return o;
        if (auto s = ctx.next("Ch5_20:f4"); s) o.set(VERSION, as_u32_str(*s)); else return o;
        if (auto s = ctx.next("Ch5_20:f5"); s) o.set(AUDIT_SESSION_ID, as_u32_str(*s)); else return o;
        if (auto s = ctx.next("Ch5_20:f6"); s) o.set(STREAMS_TAG, as_str(*s)); else return o;
        if (auto s = ctx.next("Ch5_20:f7"); s) o.set(CLIENT_ID, as_str(*s)); else return o;
        if (auto s = ctx.next("Ch5_20:f8"); s) o.set(LOGIN_USER_NAME, as_str(*s));

        return o;
    }
}