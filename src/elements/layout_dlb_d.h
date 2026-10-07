#pragma once

#include "tcb/span.hpp"
#include "../coral_decode.h"
#include "../coral_result.h"
#include "layout_common.h"
#include "../coral_show.h"
#include <fmt/ranges.h>

namespace ora {

    using coral::decode_at, coral::Result, coral::err_of;
    using std::optional, std::vector;

    /// 19.1 #1 --- KTBBH --- Transaction Header of Data Block
    struct Ktb_txh {
        uint8_t  block_type;      // 1:DATA, 2:INDEX, maybe~ 8/9: Lob data/index, 0x10/11: Undo hdr/data, 0x20 :secure file
        uint32_t data_obj_id;     // Data Object ID
        uint32_t clean_scn_base;  // Block Cleanout SCN base
        uint16_t clean_scn_wrap;  // Block Cleanout SCN wrap
        uint16_t n_itl;           // ITL count
        uint8_t  fb;              // flag ---
        uint8_t  itl_free_slt;    // ITL Free Slot Index
        uint32_t dba;             // Data Block Address

        static constexpr size_t sz_txh = 24;
        static Result<Ktb_txh> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 19.1 #1 - KDBH --- Block Header - 14 bytes)
    struct Ktb_bdh {
        uint8_t  fb;              // (1 byte, offset 0) 0x40 = Compress
        uint8_t  n_tdr;           // (1 byte, offset 1) number of table directories
        uint16_t n_rdr;           // (2 bytes, offset 2) number of rows
        uint16_t first_free;      // (2 bytes, offset 4) first free row entry index
        uint16_t free_space_start;// (2 bytes, offset 6) offset to free space start
        uint16_t free_space_end;  // (2 bytes, offset 8) offset to free space end
        uint16_t free_space_size; // (2 bytes, offset 10) available space in block
        uint16_t unknown1;        // (2 bytes, offset 12) total available space when all transactions commit ??

        bool is_compressed() const {
            return (fb & 0x40) != 0;
        }

        static constexpr size_t sz_kdbh = 14;
        static Result<Ktb_bdh> decode(tcb::span<const char> buf, bool isLittle, size_t offset);
    };

    /// 19.1 #1 (optional) --- KDC9I (9iR2 Compressed Block Header - 22 bytes)
    struct Kdc9i {
        uint32_t unknown1;       // (4 bytes, offset 0)
        uint32_t unknown2;       // (4 bytes, offset 4)
        uint32_t unknown3;       // (4 bytes, offset 8)
        uint32_t unknown4;       // (4 bytes, offset 12)
        uint16_t unknown5;       // (2 bytes, offset 16)
        uint8_t  unknown6;       // (1 byte, offset 18)
        uint8_t  fcls_9ir2_cnt;  // (1 byte, offset 19) fcls 개수
        uint8_t  perm_9ir2_cnt;  // (1 byte, offset 20) perm 개수
        uint8_t  flag_9ir2;      // (1 byte, offset 21) flag_9ir2

        static constexpr size_t sz_kdc9i = 22;
        static Result<Kdc9i> decode(tcb::span<const char> buf, bool isLittle, size_t offset);
    };

    /// 9iR2 압축 관련 가변 데이터 (fcls, perm)
    struct Kdc9iData {
        Kdc9i header;
        std::vector<uint16_t> fcls; // (2 bytes * fcls_9ir2_cnt) 11g 이전 압축된 컬럼 데이터의 길이
        std::vector<uint8_t>  perm;  // (1 byte * perm_9ir2_cnt)  Block 내 컬럼 재배치 순서

        static Result<Kdc9iData> decode(tcb::span<const char> buf, bool isLittle, size_t start = 0);
    };



    /// KTDIR (Table Directory Entry - 4 bytes)
    ///- Data Header의 cnt_tbl_dir 수만큼 연속 생성 (4 bytes)
    ///- 일반 테이블은 1개 / 압축 테이블은 2개 (첫번째: Symbol 테이블, 두번째: 유저 테이블)
    struct Ktb_tdr {
        uint16_t idx;  // start idx of row-dir. (0-based)
        uint16_t nrow; //

        static constexpr size_t sz_tdr = 4;

        static Result<Ktb_tdr> decode(tcb::span<const char> buf, bool isLittle, size_t offset = 0);
        static Result<vector<Ktb_tdr> > decode_n(tcb::span<const char> buf, uint8_t cnt_dir, bool isLittle, size_t start_offset);
    };

    /** KTRDIR (Row Directory) -- 모든 Table의 row offset을 저장 (Data Header의 cnt_row_dir 개수만큼 존재)
     * 2 bytes * cnt_row_dir
     * Offset 값이 -1 (0xFFFF) 혹은 SFLL(Slot Free Linked List) 연결 값일 경우 유효한 row offset이 아님
     */
    struct Ktb_rdr {
        std::vector<uint16_t> offsets; // offsets of row-data start pos.

        static Result<Ktb_rdr> decode(tcb::span<const char> buf, uint16_t cnt_row_dir, bool isLittle, size_t start = 0);
    };


    /// KTRHD (Row Header - 3 bytes) - Row Offset 위치에서 참조되는 Row 레코드 헤더 (3 bytes)
    /// - flag
    ///   - 0x20: Deleted
    ///   - 0x01: Cluster
    ///   - 0x02: Head
    ///   - 0x04: First Piece
    ///   - 0x08: Last Piece
    struct Ktb_rhd {
        uint8_t flag;    // (1 byte, offset 0) Row Flag (0x20=Deleted, 0x01=Cluster, 0x02=Head, 0x04=First Piece, 0x08=Last Piece 등)
        uint8_t lock;    // (1 byte, offset 1) ITL Slot 번호 (0이면 Lock 없음)
        uint8_t cols;    // (1 byte, offset 2) 해당 Row Piece의 컬럼 개수

        static constexpr size_t sz_ktrdh = 3;
        static Result<Ktb_rhd> decode( tcb::span<const char> buf, bool isLittle, size_t offset = 0);
    };

    struct Kd_col {
        uint16_t len;
        size_t   offset;  // data offset from Data-Header
    };

    struct Kd_row {
        Ktb_rhd hdr;
        vector<Kd_col> cols;

        static Result<Kd_row> decode(tcb::span<const char> buf, size_t data_header_offset, uint16_t row_offset, bool isLittle);
    };

    /// Oracle Data Block
    /// https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-DirectLoadRedo
    struct DLB_D {
        Ktb_txh               header;        // Block Transaction Header
        vector<Ktb_ItlEntry>  itls;          // ITL List
        Ktb_bdh               data_header;   // Data Block Header
        optional<Kdc9iData>   compress_info; // (선택) 9iR2 압축 정보
        vector<Ktb_tdr>       tdr;           // Table Directory
        Ktb_rdr               rdr;           // Row Directory (Row Offsets)
        vector<Kd_row>        rows;          // Rows

        static Result<DLB_D> decode(tcb::span<const char> buf, bool isLittle);
        static bool is_data_block(tcb::span<const char> buf) {
            return buf[0] == 0x01 && buf[1] == 0x00 && buf[2] == 0x00 && buf[3] == 0x00
                   && ((buf[18] & 0x30) == 0x30);
        }
    };


    // --------------------------------------------------------------------------------
    inline Result<Ktb_txh> Ktb_txh::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sz_txh ) {
            return err_of(fmt::format("[Ktb_txh] buf ({}) < {}", buf.size(), sz_txh));
        }

        return Ktb_txh{
            .block_type       = decode_At<uint8_t >(buf, isLittle, 0),
            .data_obj_id      = decode_At<uint32_t>(buf, isLittle, 4),
            .clean_scn_base   = decode_At<uint32_t>(buf, isLittle, 8),
            .clean_scn_wrap   = decode_At<uint16_t>(buf, isLittle, 12),
            .n_itl            = decode_At<uint16_t>(buf, isLittle, 16),
            .fb               = decode_At<uint8_t >(buf, isLittle, 18),
            .itl_free_slt     = decode_At<uint8_t >(buf, isLittle, 19),
            .dba              = decode_At<uint32_t>(buf, isLittle, 20)
        };
    }

    inline Result<Ktb_bdh> Ktb_bdh::decode(tcb::span<const char> buf, bool isLittle, size_t offset) {
        if (buf.size() < offset + sz_kdbh) {
            return err_of(fmt::format("[Kdbh] buf-size ({}) < offset-required ({})",
                                      buf.size(), offset + sz_kdbh));
        }

        return Ktb_bdh{
            .fb              = decode_At<uint8_t  >(buf, isLittle, offset + 0),
            .n_tdr            = decode_At<uint8_t  >(buf, isLittle, offset + 1),
            .n_rdr            = decode_At<uint16_t >(buf, isLittle, offset + 2),
            .first_free       = decode_At<uint16_t >(buf, isLittle, offset + 4),
            .free_space_start = decode_At<uint16_t >(buf, isLittle, offset + 6),
            .free_space_end   = decode_At<uint16_t >(buf, isLittle, offset + 8),
            .free_space_size  = decode_At<uint16_t >(buf, isLittle, offset + 10),
            .unknown1         = decode_At<uint16_t >(buf, isLittle, offset + 12)
        };
    }

    inline Result<Kdc9i> Kdc9i::decode(tcb::span<const char> buf, bool isLittle, size_t offset) {
        if (buf.size() < offset + sz_kdc9i) {
            return err_of(fmt::format("[Kdc9i:h] buf-size ({}) < ({} + {})", buf.size(), offset, sz_kdc9i ));
        }

        return Kdc9i{
            .unknown1      = decode_At<uint32_t>(buf, isLittle, offset + 0),
            .unknown2      = decode_At<uint32_t>(buf, isLittle, offset + 4),
            .unknown3      = decode_At<uint32_t>(buf, isLittle, offset + 8),
            .unknown4      = decode_At<uint32_t>(buf, isLittle, offset + 12),
            .unknown5      = decode_At<uint16_t>(buf, isLittle, offset + 16),
            .unknown6      = decode_At<uint8_t >(buf, isLittle, offset + 18),
            .fcls_9ir2_cnt = decode_At<uint8_t >(buf, isLittle, offset + 19),
            .perm_9ir2_cnt = decode_At<uint8_t >(buf, isLittle, offset + 20),
            .flag_9ir2     = decode_At<uint8_t >(buf, isLittle, offset + 21)
        };
    }

    inline Result<Kdc9iData> Kdc9iData::decode(const tcb::span<const char> buf, const bool isLittle, const size_t start)
    {
        auto h = Kdc9i::decode(buf, isLittle, start);
        if (!h) return tl::make_unexpected(h.error());

        size_t cur = start + Kdc9i::sz_kdc9i;

        const size_t f_need = sizeof(uint16_t) * h->fcls_9ir2_cnt;
        const size_t p_need = sizeof(uint8_t ) * h->perm_9ir2_cnt;
        if (buf.size() < cur + f_need + p_need)
            return err_of(fmt::format("[Kdc9i] buf({}) < ({} + {} + {})", buf.size(), cur, f_need, p_need));

        Kdc9iData out;
        out.header = *h;
        out.fcls.reserve(h->fcls_9ir2_cnt);
        out.perm.reserve(h->perm_9ir2_cnt);

        for (uint8_t i = 0; i < h->fcls_9ir2_cnt; ++i) {
            out.fcls.push_back(decode_At<uint16_t>(buf, isLittle, cur));
            cur += sizeof(uint16_t);
        }

        for (uint8_t i = 0; i < h->perm_9ir2_cnt; ++i) {
            out.perm.push_back(decode_At<uint8_t>(buf, isLittle, cur));
            cur += sizeof(uint8_t);
        }

        return out;
    }

    inline Result<Ktb_tdr> Ktb_tdr::decode(const tcb::span<const char> buf, const bool isLittle, const size_t offset)
    {
        if (buf.size() < offset + sz_tdr)
            return err_of(fmt::format("[Ktb_tdr] buf({}) < ({} + {})", buf.size(), offset, sz_tdr));

        return Ktb_tdr{
            .idx = decode_At<uint16_t>(buf, isLittle, offset + 0),
            .nrow =  decode_At<uint16_t>(buf, isLittle, offset + 2)
        };
    }

    inline Result<vector<Ktb_tdr>> Ktb_tdr::decode_n(const tcb::span<const char> buf,
                                                     const uint8_t cnt_dir,
                                                     const bool isLittle,
                                                     const size_t start_offset)
    {
        const size_t need = sz_tdr * cnt_dir;
        if (buf.size() < start_offset + need)
            return err_of(fmt::format("[Ktb_tdr] buf({}) < ({} + {})", buf.size(), start_offset, need));

        vector<Ktb_tdr> entries;
        entries.reserve(cnt_dir);

        for (uint8_t i = 0; i < cnt_dir; ++i) {
            size_t cur = start_offset + (i * sz_tdr);
            auto elm = decode(buf, isLittle, cur);
            if (!elm) {
                return tl::make_unexpected(elm.error());
            }
            entries.push_back(*elm);
        }

        return entries;
    }

    inline Result<Ktb_rdr> Ktb_rdr::decode(const tcb::span<const char> buf,
                                           const uint16_t cnt_row_dir,
                                           const bool isLittle,
                                           const size_t start)
    {
        const size_t need = sizeof(uint16_t) * cnt_row_dir;
        if (buf.size() < start + need) {
            return err_of(fmt::format("[Ktrdirs] buf-size ({}) < total-required ({})", buf.size(), start + need));
        }

        Ktb_rdr out;
        out.offsets.reserve(cnt_row_dir);


        for (uint16_t i = 0; i < cnt_row_dir; ++i) {
            size_t cur = start + (i * sizeof(uint16_t));
            auto off = decode_At<uint16_t>(buf, isLittle, cur);
            out.offsets.push_back(off);
        }

        return out;
    }

    inline Result<Ktb_rhd> Ktb_rhd::decode(const tcb::span<const char> buf,
                                       const bool isLittle,
                                       const size_t offset)
    {
        if (buf.size() < offset + sz_ktrdh) {
            return err_of(fmt::format("[rhd] buf-size ({}) < offset-required ({})", buf.size(), offset + sz_ktrdh ));
        }

        return Ktb_rhd{
            .flag = decode_At<uint8_t>(buf, isLittle, offset + 0),
            .lock = decode_At<uint8_t>(buf, isLittle, offset + 1),
            .cols = decode_At<uint8_t>(buf, isLittle, offset + 2)
        };
    }

    inline Result<Kd_row> Kd_row::decode(const tcb::span<const char> buf,
                                       const size_t data_header_offset,
                                       const uint16_t row_offset,
                                       const bool isLittle)
    {
        if (row_offset == 0xFFFF) return err_of("[Kdrow] invalid or free row slot (0xFFFF)");

        size_t current = data_header_offset + row_offset;

        auto rhd_res = Ktb_rhd::decode(buf, isLittle, current);
        if (!rhd_res) return tl::make_unexpected(rhd_res.error());

        current += Ktb_rhd::sz_ktrdh;

        Kd_row row;
        row.hdr = *rhd_res;
        row.cols.reserve(rhd_res->cols);

        for (uint8_t i = 0; i < rhd_res->cols; ++i) {
            if (buf.size() < current + 1)
                return err_of(fmt::format("[Kdrow] insufficient buffer at index {}", i));

            uint16_t col_len = decode_at<uint8_t, true>(buf, current);
            current += 1;

            if (col_len == 0xFE) {
                if (buf.size() < current + 2)
                    return err_of(fmt::format("[Kdrow] insufficient buffer 2-byte ext-col at index {}", i));

                col_len = isLittle ? decode_at<uint16_t, true>(buf, current)
                                   : decode_at<uint16_t, false>(buf, current);
                current += 2;
            }

            Kd_col col;
            col.len = col_len;

            if (col_len != 0xFF) {
                if (buf.size() < current + col_len) {
                    return err_of(fmt::format("[Kdrow] insufficient buffer for col-data-payload at index {}", i));
                }
                col.offset = current;
                current += col_len;
            } else {
                col.offset = 0;
            }

            row.cols.push_back(col);
        }
        return row;
    }


    inline Result<DLB_D> DLB_D::decode(tcb::span<const char> buf, bool isLittle) {

        size_t offset = 0;

        // 1. Tx header
        const auto txh = Ktb_txh::decode(buf, isLittle);
        if (!txh) return tl::make_unexpected(txh.error());
        offset += Ktb_txh::sz_txh;      //24

        // 2. ITLs
        const auto itls = decode_ktb_itlEntry24s(buf, txh->n_itl, isLittle, offset);
        if (!itls) return tl::make_unexpected(itls.error());
        offset += Ktb_ItlEntry::sz_itlEntry * txh->n_itl;

        // 2.1 skip (optional) Bitmap
        const auto bitmap_sz = (txh->fb > 0x10) ? 8 : 0;
        offset += bitmap_sz;

        // 3. Data header
        const auto data_header_offset = offset;     ///  Data start

        auto bh = Ktb_bdh::decode(buf, isLittle, offset);
        if (!bh) return tl::make_unexpected(bh.error());
        offset += Ktb_bdh::sz_kdbh;

        // 4. (optional) Compress info
        optional<Kdc9iData> compress_info;
        if (bh->is_compressed()) {
            auto a = Kdc9iData::decode(buf, isLittle, offset);
            if (!a) return tl::make_unexpected(a.error());
            compress_info = *a;
            offset += Kdc9i::sz_kdc9i
                    + (sizeof(uint16_t) * a->header.fcls_9ir2_cnt)
                    + (sizeof(uint8_t) * a->header.perm_9ir2_cnt);
        }

        // 5. Table Directories
        auto tdr = Ktb_tdr::decode_n(buf, bh->n_tdr, isLittle, offset);
        if (!tdr) { return tl::make_unexpected(tdr.error()); }
        offset += Ktb_tdr::sz_tdr * bh->n_tdr;

        // 6. Row Directory
        auto rdr = Ktb_rdr::decode(buf, bh->n_rdr, isLittle, offset);
        if (!rdr) { return tl::make_unexpected(rdr.error()); }

        // 7. Rows
        vector<Kd_row> rows;
        rows.reserve(rdr->offsets.size());

        for (uint16_t row_off : rdr->offsets) {
            const auto row = Kd_row::decode(buf, data_header_offset, row_off, isLittle);
            if (row) {
                rows.push_back(std::move(*row));
            } else {
                rows.push_back(Kd_row{}); // invalid or free (0xFFFF 등)
            }
        }

        // 8. OraBlock
        return DLB_D{
            .header        = std::move(*txh),
            .itls          = std::move(*itls),
            .data_header   = std::move(*bh),
            .compress_info = std::move(compress_info),
            .tdr           = std::move(*tdr),
            .rdr           = std::move(*rdr),
            .rows          = std::move(rows)
        };
    }
    // --------------------------------------------------------------------------------

    inline std::string to_string(const Ktb_txh& a) {
        return fmt::format("TxH : block type: {}, obj: {}, clean scn: 0x{:x}.{:x}, itl#: {}, free itl#{}, fb: {}, dba: 0x{:x}",
            a.block_type, a.data_obj_id, a.clean_scn_wrap, a.clean_scn_base, a.n_itl, a.itl_free_slt, a.fb ,a.dba);
    }

    inline std::string to_string(const Ktb_bdh& a) {
        return fmt::format("BDH : fb: {}, tdr#: {}, rdr#: {}, free: {} ({} ~ {}), free0: {}",
                           a.fb, a.n_tdr, a.n_rdr,
                           a.free_space_size, a.free_space_start, a.free_space_end, a.first_free);
    }

    inline std::string to_string(const Kdc9iData& a) {

        auto f = fmt::format("[{}]", fmt::join(a.fcls, ","));
        auto p = fmt::format("[{}]", fmt::join(a.perm, ","));

        return fmt::format("C9I : fb: {}, fcls: {}, perm: {}", a.header.flag_9ir2, f, p);
    }

    inline std::string to_string(const Ktb_tdr& a) {
        return fmt::format("@{}:{}", a.idx, a.nrow);
    }

    inline std::string to_string(const Ktb_rdr& a) {
        return fmt::format("[{}]", fmt::join(a.offsets, ","));
    }

    inline std::string to_string(const Ktb_rhd& a) {
        return fmt::format("fb:{}, lb:{}, cc:{:2}", a.flag, a.lock, a.cols);
    }

    inline std::string to_string(const Kd_col& a) {
        return fmt::format("@{}:{}", a.offset, a.len);
    }

    inline std::string to_string(const Kd_row& a) {

        auto s = coral::mkString_with(a.cols, [](const Kd_col&a) {return to_string(a);});
        return fmt::format("Row : {} {}", to_string(a.hdr), s);
    }

    inline std::string to_string(const DLB_D& a) {

        auto td = coral::mkString_with(a.tdr, [](const Ktb_tdr&a) { return to_string(a); });
        auto rs = coral::mkString_with(a.rows, [](const Kd_row&a) { return to_string(a); }, "\n   ");

        return fmt::format("DLB_D:\n  {}\n  {}{}"
                           "\n    tdr: {}\n    rdr: {}\n  {}",
            to_string(a.header),
            to_string(a.data_header),
            a.compress_info ? "\n  " + to_string(*a.compress_info) : "",
            td, to_string(a.rdr), rs
        );
    }

}
