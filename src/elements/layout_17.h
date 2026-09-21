#pragma once

#include "tcb/span.hpp"
#include "../coral_decode.h"
#include "../coral_result.h"

namespace ora {

    using coral::decode_At, coral::Result, coral::err_of;
    using std::optional;

    /** 17.27 #1
     * KTRTH (Thread Enable Marker: Recover Thread)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-ThreadEnableMarker
     * - (RAC 환경) 중단된 노드가 다시 startup 될 경우 해당 정보를 기록
     */
    struct Ktrth {
        uint32_t thread;        //  활성화되는 Redo thread number
        uint32_t logseq;        //  활성화되는 Redo thread의 log sequence number
        uint32_t scn_base;      //  시작 SCN base
        uint16_t scn_wrap;      //  시작 SCN wrap
        uint16_t scn_wrap2;     //  Oracle 12.2.0.1 이상 extended SCN wrap
    };

    [[nodiscard]] inline Result<Ktrth> decode_ktrth(tcb::span<const char> buf, bool isLittle) {
        constexpr auto sz_Ktrth = 16;

        if ( buf.size() < sz_Ktrth) {
            return err_of(fmt::format("[Ktrth] buf-size ({}) < {}", buf.size(), sz_Ktrth));
        }

        return Ktrth{
            .thread    = decode_At<uint32_t>(buf, isLittle, 0),
            .logseq    = decode_At<uint32_t>(buf, isLittle, 4),
            .scn_base  = decode_At<uint32_t>(buf, isLittle, 8),
            .scn_wrap  = decode_At<uint16_t>(buf, isLittle, 12),
            .scn_wrap2 = decode_At<uint16_t>(buf, isLittle, 14)
        };
    }

    inline std::string to_string(Ktrth &a) {
        return fmt::format("Ktrth thread: {} lsqn: {}, "
                           "scn: 0x{:04x}.{:08x}/{:04x} ",
                           a.thread, a.logseq,
                           a.scn_wrap, a.scn_base, a.scn_wrap2);
    }


}