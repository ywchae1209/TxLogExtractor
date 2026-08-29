#pragma once

#include <memory>
#include <tcb/span.hpp>
#include "Change.h"
#include "ora_opCodes.h"
#include "../blocks/RecordBound.h"

namespace ora {

    // ================================================================================
    struct RecordHead_Base {
        uint32_t len;             ///< Record total length
        uint8_t  vld;             ///< Validity flags
        uint16_t scn_wrap;        ///< SCN wrap
        uint32_t scn_base;        ///< SCN base             --- len ~ scn_base :: verify
        uint16_t sub_scn;         ///< Sub SCN
        uint32_t container_id;    ///< Container ID (PDB ID; over 12c)
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
        size_t size{0};
        RecordHead_Base base;
        std::optional<RecordHead_LWN> lwn; // optional로 하는게 좋을까?

    };

    // ================================================================================
    struct BufferSlice {
        std::shared_ptr<const std::vector<char>> storage;
        tcb::span<const char> span;

        BufferSlice(std::shared_ptr<const std::vector<char>> store, size_t offset, size_t len)
            : storage(std::move(store))
            , span(storage->data() + offset, len) {}
    };

    class RecordPayload {
        std::vector<BufferSlice> slices;
        mutable std::optional<std::vector<char>> cache{std::nullopt};

    public:
        RecordPayload() = default;

        void add_slice(std::shared_ptr<const std::vector<char>> storage,
                       size_t offset,
                       size_t len) {
            slices.emplace_back(std::move(storage), offset, len);
            cache.reset();
        }

        [[nodiscard]] size_t size() const {
            size_t total = 0;
            for (const auto& slice : slices) total += slice.span.size();
            return total;
        }

        [[nodiscard]] tcb::span<const char> span_at(size_t index = 0) const {
            if (slices.empty()) return {};
            return slices[index].span;
        }

        [[nodiscard]] std::vector<char>& asVector() const {
            if (!cache.has_value()) {
                std::vector<char> out;
                out.reserve(size());
                for (const auto& s : slices) {
                    out.insert(out.end(), s.span.begin(), s.span.end());
                }
                cache = std::move(out);
            }
            return *cache;
        }

        [[nodiscard]] tcb::span<const char> asSpan() const {
            if (slices.size() == 1) {
                return slices[0].span;
            }
            const auto& vec = asVector();
            return tcb::span(vec.data(), vec.size());
        }

        [[nodiscard]] size_t slice_count() const { return slices.size(); }
    };

    struct Record {
        RBA rba;
        uint32_t end_block{0};
        uint16_t end_offset{0};

        RecordPayload raw;      // const std::vector<char> raw;
        RecordHead header{};

        bool over12c;
        bool isLittle;
        bool isVoid;

        // --------------------------------------------------------------------------------

        std::optional<std::vector<Change>> cache_changes{};

        std::vector<Change> changes() {
            if (!cache_changes.has_value()) {
                cache_changes = isVoid
                                    ? std::vector<Change>{}
                                    : Changes_of(rba, raw.asSpan().subspan(header.size), over12c, isLittle);
            }
            return *cache_changes;
        }
    };

    Record Record_of(
        const RBA& rba,
        RecordPayload& bytes, // const std::vector<char>& bytes,
        const RecordBound& bound,
        const BlockCtx& ctx );

    // --------------------------------------------------------------------------------
    std::string to_string(const RecordHead &h);
    void show(Record& r, uint8_t showMode = 0, std::ostream &os = std::cout);
}

