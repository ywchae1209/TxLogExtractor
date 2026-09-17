#pragma once

#include <cstdint>
#include <fmt/format.h>
#include <string>
#include <vector>

#include "coral_decode.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

#include "coral_result.h"
#include "elements/layout_ktb.h"

namespace ora::combinator {

    using coral::Result, coral::err_of;
    using namespace combinator;

    struct RawFld {
        std::vector<char> bytes;

        /// decode as uint16_t array
        std::vector<uint16_t> as_array(bool isLittle) {
            const auto n = bytes.size() / sizeof(uint16_t);
            if (n == 0) return {};
            return coral::decode_array<uint16_t>(bytes, isLittle, n);

        }
    };
    struct RawFlds {
        std::vector<RawFld> elems;
    };

    struct SpanCursor {
        tcb::span<const tcb::span<const char>> spans;  // span of span
        bool isLittle;
        uint32_t redo_version;

        constexpr uint32_t REDO_VERSION_19_0 = 0x13000000;
        const bool over19 = (redo_version >= REDO_VERSION_19_0);

        size_t index = 0;

        [[nodiscard]] Result<tcb::span<const char>> next(std::string_view name) {
            if (index >= spans.size()) {
                return err_of(fmt::format("[{}] span index out of bounds: index ({}) >= size ({})", 
                                          name, index, spans.size()));
            }
            return spans[index++];
        }

        [[nodiscard]] Result<tcb::span<const char>> next(std::string_view name, size_t min_size) {
            if (index >= spans.size()) {
                return err_of(fmt::format("[{}] span index out of bounds: index ({}) >= size ({})",
                                          name, index, spans.size()));
            }
            auto out = spans[index];
            if ( out.size() < min_size)
                return err_of(fmt::format("[{}] buf size {} < min_size {} at index {}",
                                          name, out.size(), min_size, index ));
            index++;
            return out;
        }

        // -------------------------------------------------------------------------------
        template<typename T, typename Func>
        Result<T> one(std::string_view name, Func &&decoder) {
            auto s = next(name);
            if (!s) return tl::make_unexpected(s.error());
            return decoder(*s);
        }

        template<typename T, typename Func>
        Result<T> one(std::string_view name, size_t min_size, Func &&decoder) {
            auto s = next(name, min_size);
            if (!s) return tl::make_unexpected(s.error());
            return decoder(*s);
        }

        // -------------------------------------------------------------------------------
        Result<uint64_t> one_scn8(std::string_view name, bool isLittle) {
            auto s = next(name);
            if (!s) return tl::make_unexpected(s.error());

            return decode_ktb_scn8(*s, isLittle, 0);
        }
        // -------------------------------------------------------------------------------
        Result<RawFld> raw(std::string_view name) {
            auto s = next(name);
            if (!s) return tl::make_unexpected(s.error());

            return RawFld{ std::vector(s->begin(), s->end())};
        }

        // -------------------------------------------------------------------------------
        Result<RawFlds> n_raws(std::string_view name, size_t n) {

            const auto cnt = remaining();
            if (n < cnt) return err_of(fmt::format("{} n_raw: {} < ctx.remaining()", name, n));

            RawFlds out;

            for (auto i = 0; i < cnt; i++) {
                auto r = raw(name);
                if (!r) break;
                out.elems.push_back(std::move(*r));
            }
            return out;
        }

        Result<RawFlds> raws_by( std::string_view name, tcb::span<uint16_t> sizes) {

            const auto cnt = remaining();
            if (sizes.size() < cnt)
                return err_of(fmt::format("{} raw_by: {} < ctx.remaining()", name, sizes.size()));

            RawFlds out;

            // todo :: size check
            for (auto i = 0; i < cnt; i++) {
                auto r = raw(name);
                if (!r) break;
                out.elems.push_back(std::move(*r));
            }
            return out;
        }

        // -------------------------------------------------------------------------------
        Result<RawFlds> rest(std::string_view name) {

            const auto cnt = remaining();
            RawFlds out;

            for (auto i = 0; i < cnt; i++) {
                auto raw = raw(name);
                if (!raw) break;
                out.elems.push_back(std::move(*raw));
            }
            return out;
        }
        // -------------------------------------------------------------------------------
        template <typename T>
        Result<std::vector<T>> one_array(const std::string_view name, const size_t count, bool isLittle) {

            static_assert(std::is_integral_v<T>, "T must be an integral type");

            const size_t need = count * sizeof(T);

            const auto span = next(name, need);
            if (!span) return tl::make_unexpected(span.error());

            const auto buf = *span;

            return coral::decode_array<T>(buf, isLittle, count);
        }

        // -------------------------------------------------------------------------------
        [[nodiscard]] bool has_remaining() const { return index < spans.size(); }
        [[nodiscard]] bool no_remaining() const { return !has_remaining(); }
        [[nodiscard]] size_t remaining() const { return spans.size() - index; }
    };
}