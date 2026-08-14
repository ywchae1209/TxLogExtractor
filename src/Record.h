#pragma once
#include <tcb/span.hpp>

#include "Block.h"
#include "ChangeHead.h"
#include "coral_record.h"
#include "oara_opCodes.h"

namespace ora {

    struct RecordHead_Base {
        uint32_t len;             ///< Record total length
        uint8_t  vld;             ///< Validity flags
        uint16_t scn_wrap;        ///< SCN wrap
        uint32_t scn_base;        ///< SCN base             --- len ~ scn_base :: verify
        uint16_t sub_scn;         ///< Sub SCN
    };

    struct RecordHead_LWN {
        uint32_t lwn_nst;            ///< LWN NST
        uint32_t lwn_next;           ///< LWN Next
        uint32_t lwn_length;         ///< LWN Length

        SCN lwn_start_scn;
        SCN lwn_next_scn;

        uint32_t epoch;              ///< Epoch Timestamp (4 bytes)
    };

    struct RecordHead {
        RecordHead_Base base;
        size_t offset;
        std::optional<RecordHead_LWN> lwn; // optional로 하는게 좋을까?
    };

    RecordHead RecordHead_of(const tcb::span<const char>& raw, bool isLittle);

    std::string to_string(const RecordHead &h);

    // ================================================================================
    struct RBA {
        uint32_t log_seq_no{0}; // Redo-Log Sequence Number
        uint32_t block_no{0};   // Block Number
        uint16_t offset{0};     // start-offset
    };

    // ================================================================================
    struct Record {
        RBA rba;
        uint32_t end_block{0};
        uint16_t end_offset{0};

        const std::vector<char> raw;

        RecordHead head{};
        ChangeHead change_head{};

        // ------------------------------------------------------------

        Record() = default;

        explicit Record(
            RBA rba,
            std::vector<char> &bytes,
            uint32_t e_block,
            uint16_t e_offset,
            uint16_t block_sz,
            bool isLittle,
            bool over12c );

        void set_Head( bool isLittle, bool over12c);
    };

    void show(Record& r, uint8_t showMode = 0, std::ostream &os = std::cout);
}

