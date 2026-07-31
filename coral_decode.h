#pragma once

#include <endian.h>   // leXXtoh, beXXtoh
#include <byteswap.h> // bswap16, bswap32, bswap64


// depend on glibc
namespace coral {
    template<typename T>
    T decode(const T &value, bool isLittle);

    template<>
    inline uint8_t decode<uint8_t>(const uint8_t &v, bool isLittle) {
        return v;
    }

    template<>
    inline int8_t decode<int8_t>(const int8_t &v, bool isLittle) {
        return v;
    }

    template<>
    inline uint16_t decode<uint16_t>(const uint16_t &v, bool isLittle) {
        return isLittle ? le16toh(v) : be16toh(v);
    }

    template<>
    inline uint32_t decode<uint32_t>(const uint32_t &v, bool isLittle) {
        return isLittle ? le32toh(v) : be32toh(v);
    }

    template<>
    inline uint64_t decode<uint64_t>(const uint64_t &v, bool isLittle) {
        return isLittle ? le64toh(v) : be64toh(v);
    }

    template<>
    inline int16_t decode<int16_t>(const int16_t &v, bool isLittle) {
        return static_cast<int16_t>(
            decode<uint16_t>(static_cast<const uint16_t &>(v), isLittle)
        );
    }

    template<>
    inline int32_t decode<int32_t>(const int32_t &v, bool isLittle) {
        return static_cast<int32_t>(
            decode<uint32_t>(static_cast<const uint32_t &>(v), isLittle)
        );
    }

    template<>
    inline int64_t decode<int64_t>(const int64_t &v, bool isLittle) {
        return static_cast<int64_t>(
            decode<uint64_t>(static_cast<const uint64_t &>(v), isLittle)
        );
    }
}

