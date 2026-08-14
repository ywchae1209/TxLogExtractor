#include "Record.h"

namespace ora {

#pragma pack(push, 1)

    /// 24 byte KTRRH(Redo Record Header)
    struct Base_lo {
        uint32_t len;             ///< * Record total length
        uint8_t  vld;             ///< * Validity flags
        uint8_t  foo;             ///< Record type.(internal?) -- ignore

        uint16_t scn_wrap;        ///< * SCN wrap
        uint32_t scn_base;        ///< * SCN base
        uint16_t sub_scn;         ///< * Sub SCN

        uint16_t flags;           ///< Record Level flags.(internal?)  -- ignore
        uint32_t container_id;    ///< Container ID (PDB ID; over 12c) -- ignore
        uint32_t spare;           ///< Spare padding  -- ignore
    };

    /// 44 || 48 byte KTRRLWN(Redo Record LWN)
    struct Lwn_lo {
        uint32_t lwn_nst;            ///< * LWN NST (4 bytes)

        uint32_t lwn_next;           ///< * LWN Next (4 bytes)
        uint32_t lwn_length;         ///< * LWN Length (4 bytes)
        uint32_t unknown;            ///< Unknown (4 bytes)
        SCN_lo lwn_scn;              ///< base(4) wrap(2) high(2) in little-Endian
        uint32_t unknown4;           ///< Unknown (4 bytes)
        uint32_t unknown5;           ///< Unknown (4 bytes)
        SCN_lo lwn_next_scn;

        uint32_t epoch;              ///< * Epoch Timestamp (4 bytes)
        //uint32_t os_padding;         ///< todo check :: OS Padding (4 bytes; 64bit OS = 8의 배수)
    };
#pragma pack(pop)

    static RecordHead_Base decode(const Base_lo &raw, const bool isLittle) noexcept {
        using coral::decode;

        RecordHead_Base b{};

        b.len          = decode(raw.len, isLittle);
        b.vld          = raw.vld;

        b.scn_wrap     = decode(raw.scn_wrap, isLittle);
        b.scn_base     = decode(raw.scn_base, isLittle);
        b.sub_scn      = decode(raw.sub_scn, isLittle);

        return b;
    }

    static RecordHead_LWN decode(const Lwn_lo &raw, const bool isLittle) noexcept {
        using coral::decode;

        RecordHead_LWN l{};

        l.lwn_nst       = decode(raw.lwn_nst, isLittle);
        l.lwn_next      = decode(raw.lwn_next, isLittle);
        l.lwn_length    = decode(raw.lwn_length, isLittle);

        l.lwn_start_scn = decode_SCN(raw.lwn_scn, isLittle);
        l.lwn_next_scn  = decode_SCN(raw.lwn_next_scn, isLittle);

        l.epoch         = decode(raw.epoch, isLittle);

        return l;
    }

    RecordHead RecordHead_of(const tcb::span<const char>& raw, const bool isLittle) {

        constexpr auto base_sz = sizeof(Base_lo);
        constexpr auto lwn_sz = sizeof(Lwn_lo);

        if (raw.size() < base_sz) {
            throw std::out_of_range("Buffer size is smaller than Base_lo size(24 byte).");
        }

        Base_lo raw_base{};
        std::memcpy(&raw_base, raw.data(), base_sz);
        auto base =  decode(raw_base, isLittle);

        if (!coral::dependBit_on(base.vld) )
            return RecordHead{ base, base_sz};

        if (raw.size() < base_sz + lwn_sz) {
            throw std::out_of_range("Buffer size is smaller than Base_lo + LWN_lo size(64 byte).");
        }

        Lwn_lo raw_lwn{};
        std::memcpy(&raw_lwn, raw.data() + base_sz, lwn_sz);

        return RecordHead{ base, base_sz + lwn_sz, decode(raw_lwn, isLittle)};
    }

    std::string to_string(const RecordHead &h) {
        using coral::toHex;
        const auto &b = h.base;

        std::string s = fmt::format(
            "LEN:{:<6} "
            "SCN: 0x{:04x}.{:08x}:{:03} "
            "-- {:3} -- "
            "VLD:{} {}",
            b.len,
            b.scn_wrap, b.scn_base, b.sub_scn,
            h.offset,
            toHex(b.vld), coral::vld_string(b.vld)
        );

        if (h.lwn) {
            const auto &l = *h.lwn;
            s += fmt::format(
                "  -- [LWN] nst:{} next:{:3} len:{:3} start_scn:{} next_scn:{} Epoch:{}",
                l.lwn_nst,
                l.lwn_next,
                l.lwn_length,
                toHex(l.lwn_start_scn),
                toHex(l.lwn_next_scn),
                l.epoch
            );
        }

        return s;
    }

    Record::Record(
        RBA rba,
        std::vector<char> &bytes,
        const uint32_t e_block,
        const uint16_t e_offset,
        const uint16_t block_sz,
        const bool isLittle,
        const bool over12c)
        : rba(rba),
          end_block(e_offset != block_sz ? e_block : e_block + 1),
          end_offset(e_offset != block_sz ? e_offset : 16),
          raw{std::move(bytes)},
          head{RecordHead_of(raw, isLittle)},
          change_head{ChangeHead_of(tcb::span(raw).subspan(head.offset), over12c, isLittle)} {
    }

    void Record::set_Head(bool isLittle, bool over12c) {

        const auto view = tcb::span(raw);

        head = RecordHead_of(raw, isLittle);

        const auto body = view.subspan(head.offset);
        change_head = ChangeHead_of( body, over12c, isLittle);
    }

    // ================================================================================
    void show(Record& r, uint8_t showMode, std::ostream &os) {

        using coral::toHex;

        fmt::println(os,
            "R {}{:>7}.@{:<3} ~{:>7}.@{:<3}{} | "
            "{}",
            coral::key_color,
            r.rba.block_no, r.rba.offset, r.end_block, r.end_offset,
            coral::reset_color,
            to_string(r.head)
        );

        if ((showMode & 1) == 1)
            fmt::println(os, "{}", to_string(r.change_head));

        if ((showMode & 2) == 2) {

            const auto view = tcb::span(r.raw);
            const auto body = view.subspan(r.head.offset);

            coral::show_HexDump(view.subspan(0, r.head.offset));
            coral::show_HexDump(body);
        }
    }

}