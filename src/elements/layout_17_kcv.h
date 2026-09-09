#pragma once

#include "tcb/span.hpp"
#include "../coral_decode.h"

namespace ora {

    using coral::decode_at;
    using std::optional;


#pragma pack(push, 1)
    /** 17.27 #1
     * KTRTH (Thread Enable Marker: Recover Thread)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ThreadEnableMarker
     * - (RAC 환경) 중단된 노드가 다시 startup 될 경우 해당 정보를 기록
     */
    struct Ktrth {
        uint32_t thread;        // (4 bytes, offset 0) 활성화되는 Redo thread number
        uint32_t logseq;        // (4 bytes, offset 4) 활성화되는 Redo thread의 log sequence number
        uint32_t scn_base;      // (4 bytes, offset 8) 시작 SCN base
        uint16_t scn_wrap;      // (2 bytes, offset 12) 시작 SCN wrap
        uint16_t scn_wrap2;     // (2 bytes, offset 14) Oracle 12.2.0.1 이상 extended SCN wrap
    };
    static_assert(sizeof(Ktrth) == 16, "Ktrth size mismatch");
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktrth decode_ktrth0(tcb::span<const char> buf) {
        return Ktrth{
            .thread    = decode_at<uint32_t, IsLittle>(buf, 0),
            .logseq    = decode_at<uint32_t, IsLittle>(buf, 4),
            .scn_base  = decode_at<uint32_t, IsLittle>(buf, 8),
            .scn_wrap  = decode_at<uint16_t, IsLittle>(buf, 12),
            .scn_wrap2 = decode_at<uint16_t, IsLittle>(buf, 14)
        };
    }

    [[nodiscard]] inline std::optional<Ktrth> decode_ktrth(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktrth)) { // sizeof(Ktrth) == 16
            return std::nullopt;
        }

        return isLittle ? decode_ktrth0<true>(buf)
                        : decode_ktrth0<false>(buf);
    }
}