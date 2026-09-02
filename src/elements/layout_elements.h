#pragma once
#include <cstdint>

#include "tcb/span.hpp"
#include "../coral_decode.h"

namespace ora {

    using coral::decode_at;
    using std::optional;

#pragma pack(push, 1)
    //// https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktudbvector
    //// 20
    struct Ktudb {
        uint16_t size;     // size (2 bytes, offset 0)
        uint16_t spc;      // spc (2 bytes, offset 2)
        uint16_t flag;     // flag (2 bytes, offset 4)
        uint16_t unknown;  // unknown (2 bytes, offset 6)
        uint16_t xid_usn;  // xid undo segment num (2 bytes, offset 8)
        uint16_t xid_slt;  // xid slot (2 bytes, offset 10)
        uint32_t xid_sqn;  // xid sequence number (4 bytes, offset 12)
        uint16_t seq;      // seq (2 bytes, offset 16)
        uint8_t  rec;      // rec (1 byte, offset 18)
        uint8_t  unknown1; // unknown (1 byte, offset 19)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktudb decode_ktudb0(tcb::span<const char> buf) {
        return Ktudb{
            .size     = decode_at<uint16_t, IsLittle>(buf, 0),
            .spc      = decode_at<uint16_t, IsLittle>(buf, 2),
            .flag     = decode_at<uint16_t, IsLittle>(buf, 4),
            .unknown  = decode_at<uint16_t, IsLittle>(buf, 6),
            .xid_usn  = decode_at<uint16_t, IsLittle>(buf, 8),
            .xid_slt  = decode_at<uint16_t, IsLittle>(buf, 10),
            .xid_sqn  = decode_at<uint32_t, IsLittle>(buf, 12),
            .seq      = decode_at<uint16_t, IsLittle>(buf, 16),
            .rec      = decode_at<uint8_t,  IsLittle>(buf, 18),
            .unknown1 = decode_at<uint8_t,  IsLittle>(buf, 19)
        };
    }

    [[nodiscard]] inline std::optional<Ktudb> decode_ktudb(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktudb)) {
            return std::nullopt;
        }

        return isLittle ? decode_ktudb0<true>(buf)
                        : decode_ktudb0<false>(buf);
    }

#pragma pack(push, 1)
    //// https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktudhvector
    //// Transaction 시작 시 undo segement를 할당받는데, 할당된 위치와 트랜잭션 정보를 저장하는 vector이다.
    struct Ktudh {
        uint16_t xid_slt;      // xid slot (2 bytes, offset 0)
        uint16_t unknown;      // unknown (2 bytes, offset 2)
        uint32_t xid_sqn;      // xid sequence number (4 bytes, offset 4)
        uint32_t uba_maj;      // undo block addr major (4 bytes, offset 8)
        uint16_t uba_min;      // undo block addr minor (2 bytes, offset 12)
        uint8_t  uba_mic;      // undo block addr micro (1 byte, offset 14)
        uint8_t  unknown1;     // unknown (1 byte, offset 15)
        uint16_t flag;         // flag (2 bytes, offset 16)
        uint16_t size;         // size (2 bytes, offset 18)
        uint16_t fbi;          // fbi (2 bytes, offset 20)
        uint16_t unknown2;     // unknown[3] (2 bytes, offset 22)
        uint16_t pxid_maj;     // parent xid major (2 bytes, offset 24)
        uint16_t pxid_min;     // parent xid minor (2 bytes, offset 26)
        uint32_t pxid_mic;     // parent xid micro (4 bytes, offset 28)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktudh decode_ktudh0(tcb::span<const char> buf) {
        return Ktudh{
            .xid_slt  = decode_at<uint16_t, IsLittle>(buf, 0),
            .unknown  = decode_at<uint16_t, IsLittle>(buf, 2),
            .xid_sqn  = decode_at<uint32_t, IsLittle>(buf, 4),
            .uba_maj  = decode_at<uint32_t, IsLittle>(buf, 8),
            .uba_min  = decode_at<uint16_t, IsLittle>(buf, 12),
            .uba_mic  = decode_at<uint8_t,  IsLittle>(buf, 14),
            .unknown1 = decode_at<uint8_t,  IsLittle>(buf, 15),
            .flag     = decode_at<uint16_t, IsLittle>(buf, 16),
            .size     = decode_at<uint16_t, IsLittle>(buf, 18),
            .fbi      = decode_at<uint16_t, IsLittle>(buf, 20),
            .unknown2 = decode_at<uint16_t, IsLittle>(buf, 22),
            .pxid_maj = decode_at<uint16_t, IsLittle>(buf, 24),
            .pxid_min = decode_at<uint16_t, IsLittle>(buf, 26),
            .pxid_mic = decode_at<uint32_t, IsLittle>(buf, 28)
        };
    }

    [[nodiscard]] inline std::optional<Ktudh> decode_ktudh(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktudh)) { // sizeof(Ktudh) == 32
            return std::nullopt;
        }
        return isLittle ? decode_ktudh0<true>(buf)
                        : decode_ktudh0<false>(buf);
    }

#pragma pack(push, 1)
    //// https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktucmvector
    //// Transaction commit 할 때 저장되는 vector이다. Rollback tranasction 일 경우 flag에 0x4를 저장한다.
    struct Ktcum {
        uint16_t xid_slt;      // xid slot (2 bytes, offset 0)
        uint16_t unknown;      // unknown (2 bytes, offset 2)
        uint32_t xid_sqn;      // xid sequence number (4 bytes, offset 4)
        uint8_t  srt;          // srt (1 byte, offset 8)
        uint8_t  unknown1[3];  // unknown[3] (3 bytes, offset 9~11)
        uint32_t status;       // status (4 bytes, offset 12)
        uint8_t  flag;         // flag (1 byte, offset 16)
        uint8_t  unknown2[3];  // unknown[3] (3 bytes, offset 17~19)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktcum decode_ktcum0(tcb::span<const char> buf) {
        return Ktcum{
            .xid_slt  = decode_at<uint16_t, IsLittle>(buf, 0),
            .unknown  = decode_at<uint16_t, IsLittle>(buf, 2),
            .xid_sqn  = decode_at<uint32_t, IsLittle>(buf, 4),
            .srt      = decode_at<uint8_t,  IsLittle>(buf, 8),
            .unknown1 = {
                decode_at<uint8_t, IsLittle>(buf, 9),
                decode_at<uint8_t, IsLittle>(buf, 10),
                decode_at<uint8_t, IsLittle>(buf, 11)
            },
            .status   = decode_at<uint32_t, IsLittle>(buf, 12),
            .flag     = decode_at<uint8_t,  IsLittle>(buf, 16),
            .unknown2 = {
                decode_at<uint8_t, IsLittle>(buf, 17),
                decode_at<uint8_t, IsLittle>(buf, 18),
                decode_at<uint8_t, IsLittle>(buf, 19)
            }
        };
    }

    [[nodiscard]] inline std::optional<Ktcum> decode_ktcum(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktcum)) { // sizeof(Ktcum) == 20
            return std::nullopt;
        }

        return isLittle ? decode_ktcum0<true>(buf)
                        : decode_ktcum0<false>(buf);
    }

#pragma pack(push, 1)
    //// https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktucfvector
    struct Ktucf {
        uint32_t uba_maj;      // undo block addr major (4 bytes, offset 0)
        uint16_t uba_min;      // undo block addr minor (2 bytes, offset 4)
        uint8_t  uba_mic;      // undo block addr micro (1 byte, offset 6)
        uint8_t  unknown;      // unknown (1 byte, offset 7)
        uint16_t ext;          // ext (2 bytes, offset 8)
        uint16_t spc;          // spc (2 bytes, offset 10)
        uint8_t  fbi;          // fbi (1 byte, offset 12)
        uint8_t  unknown1[3];  // unknown[3] (3 bytes, offset 13~15)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktucf decode_ktucf0(tcb::span<const char> buf) {
        return Ktucf{
            .uba_maj  = decode_at<uint32_t, IsLittle>(buf, 0),
            .uba_min  = decode_at<uint16_t, IsLittle>(buf, 4),
            .uba_mic  = decode_at<uint8_t,  IsLittle>(buf, 6),
            .unknown  = decode_at<uint8_t,  IsLittle>(buf, 7),
            .ext      = decode_at<uint16_t, IsLittle>(buf, 8),
            .spc      = decode_at<uint16_t, IsLittle>(buf, 10),
            .fbi      = decode_at<uint8_t,  IsLittle>(buf, 12),
            .unknown1 = {
                decode_at<uint8_t, IsLittle>(buf, 13),
                decode_at<uint8_t, IsLittle>(buf, 14),
                decode_at<uint8_t, IsLittle>(buf, 15)
            }
        };
    }

    [[nodiscard]] inline std::optional<Ktucf> decode_ktucf(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktucf)) { // sizeof(Ktucf) == 16
            return std::nullopt;
        }

        return isLittle ? decode_ktucf0<true>(buf)
                        : decode_ktucf0<false>(buf);
    }


#pragma pack(push, 1)
    //// https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ktubuvector
    struct Ktubu {
        uint32_t obj_id;      // object id (4 bytes, offset 0)
        uint32_t data_obj_id; // data object id (4 bytes, offset 4)
        uint32_t ts_num;      // tablespace number (4 bytes, offset 8)
        uint32_t ts_undo;     // tablespace undo (4 bytes, offset 12)
        uint8_t  op_maj;      // op code (major) (1 byte, offset 16)
        uint8_t  op_min;      // op code (minor) (1 byte, offset 17)
        uint8_t  slot;        // slot (1 byte, offset 18)
        uint8_t  rci;         // rci (1 byte, offset 19)
        uint16_t flag;        // flag (2 bytes, offset 20)
        uint16_t trf0;        // trf0 (2 bytes, offset 22)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktubu decode_ktubu0(tcb::span<const char> buf) {
        return Ktubu{
            .obj_id      = decode_at<uint32_t, IsLittle>(buf, 0),
            .data_obj_id = decode_at<uint32_t, IsLittle>(buf, 4),
            .ts_num      = decode_at<uint32_t, IsLittle>(buf, 8),
            .ts_undo     = decode_at<uint32_t, IsLittle>(buf, 12),
            .op_maj      = decode_at<uint8_t,  IsLittle>(buf, 16),
            .op_min      = decode_at<uint8_t,  IsLittle>(buf, 17),
            .slot        = decode_at<uint8_t,  IsLittle>(buf, 18),
            .rci         = decode_at<uint8_t,  IsLittle>(buf, 19),
            .flag        = decode_at<uint16_t, IsLittle>(buf, 20),
            .trf0        = decode_at<uint16_t, IsLittle>(buf, 22)
        };
    }

    [[nodiscard]] inline std::optional<Ktubu> decode_ktubu(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktubu)) { // sizeof(Ktubu) == 24
            return std::nullopt;
        }

        return isLittle ? decode_ktubu0<true>(buf)
                        : decode_ktubu0<false>(buf);
    }

}
