#pragma once
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_dlb_d.h"

namespace ora {
    using coral::Result;
    using namespace combinator;

    using coral::decode_At, coral::Result, coral::err_of;
    using std::vector, std::array;

    ///  from OLR
    ///  Lob블록인 경우
    struct DLB_L {
        uint32_t           objd{0};        // Offset 0 : Data Object ID
        array<uint8_t, 10> lob_id{};       // Offset 4 : 10-Byte LOB ID
        uint32_t           v2{0};          // Offset 16: Version 2
        uint16_t           v1{0};          // Offset 20: Version 1
        uint32_t           lob_page_no{0}; // Offset 24: LOB Page Number
        uint32_t           pdba{0};        // Offset 28: Physical DBA ?? Previous DBA?
        size_t             lob_data_sz{0};
        vector<char>       lob_data;       // Offset 36~: Direct Payload Data

        static Result<DLB_L> decode( tcb::span<const char> buf, bool isLittle);
    };

    // ----------------------------------------------------------------------------------------------------
    inline Result<DLB_L> DLB_L::decode( tcb::span<const char> buf, bool isLittle) {

        if (buf.size() < 36) {
            return err_of(fmt::format("[DirectLoader19_1] buf size {} < 36", buf.size()));
        }

        DLB_L out;

        out.objd        = decode_At<uint32_t>(buf, isLittle, 0);
        std::memcpy(out.lob_id.data(), buf.data() + 4, 10);
        out.v2          = decode_At<uint32_t>(buf, isLittle, 16);
        out.v1          = decode_At<uint16_t>(buf, isLittle, 20);
        out.lob_page_no = decode_At<uint32_t>(buf, isLittle, 24);
        out.pdba        = decode_At<uint32_t>(buf, isLittle, 28);

        // 36~
        if (buf.size() > 36) {
            out.lob_data.assign(buf.begin() + 36, buf.end());
        }
        out.lob_data_sz = buf.size() - 36;

        return out;
    }

    inline std::string to_string(const DLB_L& a) {

        std::string lob_id_str;
        lob_id_str.reserve(20);
        for (uint8_t b : a.lob_id) {
            lob_id_str += fmt::format("{:02x}", b);
        }

        std::string out = fmt::format(
            "DLB_LF: objd: 0x{:08x}, LobId: 0x{}, page: {}, version: 0x{:04x}.{:08x}, pdba: 0x{:08x}, sz: {}\n    ",
            a.objd, lob_id_str, a.lob_page_no, a.v1, a.v2, a.pdba, a.lob_data_sz );

        // 36 ~
        if (!a.lob_data.empty()) {
            auto min = (16 * 6) < a.lob_data.size() ? 16 * 6 : a.lob_data.size();
            for (size_t j = 0; j < min; ++j) {
                out += fmt::format("{:02x} ", static_cast<uint8_t>(a.lob_data[j]));
                if ((j % 32) == 31 && j != a.lob_data.size() - 1) {
                    out += "\n    ";
                }
            }
            out += '\n';
        }

        return out;
    }

}
