#pragma once

#include <vector>
#include <optional>
#include <tcb/span.hpp>

#include "../coral_show.h"
#include "RedoHead.h"

namespace ora {

    struct Block;

    // ================================================================================
    struct BlockCtx {
        bool isLittle;
        bool over12c;
        uint16_t block_sz;
        SCN low_scn;
        SCN nxt_scn;

        uint64_t low() const { return scn_to64(low_scn);}
        uint64_t nxt() const { return scn_to64(nxt_scn);}
    };

    struct LwnCtx {
        SCN l_low;
        SCN l_nxt;

        uint64_t low() const { return scn_to64(l_low);}
        uint64_t nxt() const { return scn_to64(l_nxt);}
    };

    // Bound check info
    // --------------------------------------------------------------------------------
    struct BoundInfo {
        uint32_t len;
        SCN scn;
        uint8_t vld;
        uint8_t foo;

        bool dependOn() const { return (vld & 4) == 4;}
        bool isPad() const { return vld == 0;}
        uint64_t scn64() const { return scn_to64(scn);}
    };

    // Block has  record-bounds
    // --------------------------------------------------------------------------------
    struct RecordBound {
        uint32_t len;          // Record length (from before-offset to next_offset)
        uint32_t next_blocks;  // (0 == 현재 블록)
        uint16_t next_offset;  //

        BoundInfo boundInfo;
    };

    // --------------------------------------------------------------------------------
    [[nodiscard]] bool dependBit_on(uint8_t vld);
    [[nodiscard]] std::optional<bool> choose_new_vld( uint8_t vld0, uint8_t vld1);
    [[nodiscard]] std::string vld_string(uint8_t vld);
    [[nodiscard]] std::string foo_string(uint8_t foo);

    std::vector<RecordBound> fairBounds(const Block& block,
                                              const std::optional<LwnCtx>& lwn,
                                              const bool showReason) ;


    bool fairBound(const RecordBound& b, const std::optional<LwnCtx> &lwn, bool showReason, const RBA& rba);

    std::vector<RecordBound> bound_candidates(tcb::span<const char> view,
                                               uint16_t start_offset,
                                               uint32_t block_no,
                                               const BlockCtx &ctx,
                                               bool showReason);
}
