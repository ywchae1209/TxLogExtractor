#pragma once

// depend on glibc
#include <endian.h>   // leXXtoh, beXXtoh
#include <byteswap.h> // bswap16, bswap32, bswap64
#include <cstring>
#include <optional>

#include <fmt/ostream.h>
#include <tcb/span.hpp>
#include "coral_result.h"

namespace coral {
    template <size_t N>
    inline std::array<uint8_t, N> copy_bytes(const uint8_t* src) noexcept {
        std::array<uint8_t, N> arr{};
        std::memcpy(arr.data(), src, N);
        return arr;
    }

    template <size_t N>
    inline std::string to_string_n(tcb::span<const char> buf, size_t offset) {
        const char* ptr = buf.data() + offset;
        return std::string(ptr, strnlen(ptr, N));
    }

    template <typename T, bool IsLittle>
    inline T decode_at(tcb::span<const char> buf, size_t offset) {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

        // Unaligned Safe Read
        T v;
        std::memcpy(&v, &buf[offset], sizeof(T));

        if constexpr (sizeof(T) == 1) {
            return v;
        }
        else if constexpr (sizeof(T) == 2) {
            auto raw = static_cast<uint16_t>(v);
            return static_cast<T>(IsLittle ? le16toh(raw) : be16toh(raw));
        }
        else if constexpr (sizeof(T) == 4) {
            auto raw = static_cast<uint32_t>(v);
            return static_cast<T>(IsLittle ? le32toh(raw) : be32toh(raw));
        }
        else if constexpr (sizeof(T) == 8) {
            auto raw = static_cast<uint64_t>(v);
            return static_cast<T>(IsLittle ? le64toh(raw) : be64toh(raw));
        } else
            static_assert(sizeof(T) == 0, "Unsupported type size for decode_at");
    }

    template <typename T>
    inline T decode_At(tcb::span<const char> buf, bool isLittle, size_t offset) {
        return isLittle
                   ? decode_at<T, true>(buf, offset)
                   : decode_at<T, false>(buf, offset);
    }

    template <typename T>
    inline T decode_At0(tcb::span<const char> buf, bool isLittle) {
        return isLittle
                   ? decode_at<T, true>(buf, 0)
                   : decode_at<T, false>(buf, 0);
    }

    template <typename T>
    inline std::vector<T> decode_array(const tcb::span<const char> buf, bool isLittle, size_t count) {

        std::vector<T> out;
        out.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            out.push_back(coral::decode_At<T>(buf, isLittle, i * sizeof(T)));
        }
        return out;
    }

    [[nodiscard]] inline std::vector<tcb::span<const char>> splits_by(
            const tcb::span<const char> buf,
            const tcb::span<const uint16_t> sizes) {

        std::vector<tcb::span<const char>> splits;
        splits.reserve(sizes.size());

        size_t offset = 0;
        const size_t total_buf_sz = buf.size();

        for (const auto len : sizes) {
            if (offset + len > total_buf_sz) {
                break;
            }
            splits.push_back(buf.subspan(offset, len));
            offset += len;
        }

        return splits;
    }


    template <typename T>
    inline T get_at(tcb::span<const char> s, size_t offset, bool isLittle) {
        return isLittle
                   ? coral::decode_at<T, true>(s, offset)
                   : coral::decode_at<T, false>(s, offset);
    }

    inline constexpr size_t align_up4(const size_t size) noexcept {
        return (size + 3) & ~static_cast<size_t>(3);
    }

    inline Result<void> enough(const tcb::span<const char> buf,
                               const size_t required_size,
                               const std::string_view prefix) {
        if (buf.size() < required_size) {
            return err_of(fmt::format("[{}] buf-size ({}) < required ({})", prefix, buf.size(), required_size));
        }
        return {};
    }

}

