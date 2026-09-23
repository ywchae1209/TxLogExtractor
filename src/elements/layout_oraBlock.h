#pragma once

#include "tcb/span.hpp"
#include "../coral_decode.h"
#include "../coral_result.h"
#include "layout_common.h"

/** Direct Load Redo
 *
 * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-DirectLoadRedo
 */

namespace ora {

    using coral::decode_at, coral::Result, coral::err_of;
    using std::optional, std::vector;

#pragma pack(push, 1)
    /** 19.1 #1
     * KTBBH (Block Transaction Header)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-BlockHeader
     * - Direct path insert / LOB 데이터를 기록
     * - Oracle Data block을 그대로 Element에 기록
     */
    struct Ktbbh {
        uint8_t  block_type;        // (1 byte, offset 0) 1=DATA, 2=INDEX
        uint8_t  spare1;            // (1 byte, offset 1)
        uint8_t  spare2;            // (1 byte, offset 2)
        uint8_t  spare3;            // (1 byte, offset 3)
        uint32_t data_obj_id;       // (4 bytes, offset 4) Data Object ID
        uint32_t clean_scn_base;    // (4 bytes, offset 8) Block Cleanout SCN base
        uint16_t clean_scn_wrap;    // (2 bytes, offset 12) Block Cleanout SCN wrap
        uint8_t  spare4;            // (1 byte, offset 14)
        uint8_t  spare5;            // (1 byte, offset 15)
        uint16_t itl_cnt;           // (2 bytes, offset 16) ITL count
        uint8_t  ktbbh_flg;         // (1 byte, offset 18)
        uint8_t  itl_free_slt;      // (1 byte, offset 19) ITL Free Slot Index
        uint32_t dba;               // (4 bytes, offset 20) Data Block Address
    };
    static_assert(sizeof(Ktbbh) == 24, "Ktbbh size mismatch");
#pragma pack(pop)

    template<bool IsLittle>
    inline Ktbbh decode_ktbbh0(tcb::span<const char> buf) {
        return Ktbbh{
            .block_type       = decode_at<uint8_t,  IsLittle>(buf, 0),
            .spare1           = decode_at<uint8_t,  IsLittle>(buf, 1),
            .spare2           = decode_at<uint8_t,  IsLittle>(buf, 2),
            .spare3           = decode_at<uint8_t,  IsLittle>(buf, 3),
            .data_obj_id      = decode_at<uint32_t, IsLittle>(buf, 4),
            .clean_scn_base   = decode_at<uint32_t, IsLittle>(buf, 8),
            .clean_scn_wrap   = decode_at<uint16_t, IsLittle>(buf, 12),
            .spare4           = decode_at<uint8_t,  IsLittle>(buf, 14),
            .spare5           = decode_at<uint8_t,  IsLittle>(buf, 15),
            .itl_cnt          = decode_at<uint16_t, IsLittle>(buf, 16),
            .ktbbh_flg        = decode_at<uint8_t,  IsLittle>(buf, 18),
            .itl_free_slt     = decode_at<uint8_t,  IsLittle>(buf, 19),
            .dba              = decode_at<uint32_t, IsLittle>(buf, 20)
        };
    }

    [[nodiscard]] inline Result<Ktbbh> decode_ktbbh(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(Ktbbh)) {
            return err_of(fmt::format("[Ktbbh] buf-size ({}) < sizeof(Ktbbh) ({})", buf.size(), sizeof(Ktbbh)));
        }

        return isLittle ? decode_ktbbh0<true>(buf)
                        : decode_ktbbh0<false>(buf);
    }

#pragma pack(push, 1)
    /** 19.1 #1
     * KTBIT (Interested Transaction List Entry - 24 bytes)
     */
    struct Ktbit {
        Ktb_xid8  xid;
        Ktb_uba7  uba;
        uint8_t  spare1;          // (1 byte, offset 15) spare1

        uint16_t flags;           // (2 bytes, offset 16) flags
        uint16_t ktbitun;         // (2 bytes, offset 18) ktbitun Interested Transaction Undo Num:: scn.wrap
        uint32_t base;            // (4 bytes, offset 20) base
    };
    static_assert(sizeof(Ktbit) == 24, "Ktbit size mismatch");
#pragma pack(pop)


#pragma pack(push, 1)
    /** 19.1 #1
     * KDBH (Kernel Data Block Header - 14 bytes)
     */
    struct Kdbh {
        uint8_t  flags;           // (1 byte, offset 0) 0x40 = Compress
        uint8_t  cnt_tbl_dir;     // (1 byte, offset 1) number of table directories
        uint16_t cnt_row_dir;     // (2 bytes, offset 2) number of rows
        uint16_t first_free;      // (2 bytes, offset 4) first free row entry index
        uint16_t free_space_start;// (2 bytes, offset 6) offset to free space start
        uint16_t free_space_end;  // (2 bytes, offset 8) offset to free space end
        uint16_t free_space_size; // (2 bytes, offset 10) available space in block
        uint16_t unknown1;        // (2 bytes, offset 12) total available space when all transactions commit ??
    };
    static_assert(sizeof(Kdbh) == 14, "Kdbh size mismatch");
#pragma pack(pop)

    template <bool IsLittle>
    inline Kdbh decode_kdbh0(tcb::span<const char> buf, size_t offset = 0) {
        return Kdbh{
            .flags            = decode_at<uint8_t,  IsLittle>(buf, offset + 0),
            .cnt_tbl_dir      = decode_at<uint8_t,  IsLittle>(buf, offset + 1),
            .cnt_row_dir      = decode_at<uint16_t, IsLittle>(buf, offset + 2),
            .first_free       = decode_at<uint16_t, IsLittle>(buf, offset + 4),
            .free_space_start = decode_at<uint16_t, IsLittle>(buf, offset + 6),
            .free_space_end   = decode_at<uint16_t, IsLittle>(buf, offset + 8),
            .free_space_size  = decode_at<uint16_t, IsLittle>(buf, offset + 10),
            .unknown1         = decode_at<uint16_t, IsLittle>(buf, offset + 12)
        };
    }

    [[nodiscard]] inline Result<Kdbh> decode_kdbh(tcb::span<const char> buf, bool isLittle, size_t offset = 0) {
        if (buf.size() < offset + sizeof(Kdbh)) {
            return err_of(fmt::format("[Kdbh] buf-size ({}) < offset-required ({})",
                                      buf.size(), offset + sizeof(Kdbh)));
        }

        return isLittle ? decode_kdbh0<true>(buf, offset)
                        : decode_kdbh0<false>(buf, offset);
    }

#pragma pack(push, 1)
    /** 19.1 #1 (optional)
     * KDC9I (9iR2 Compressed Block Header - 22 bytes)
     */
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
    };
    static_assert(sizeof(Kdc9i) == 22, "Kdc9i size mismatch");
#pragma pack(pop)

    /** 9iR2 압축 관련 가변 데이터 (fcls, perm) */
    struct Kdc9iData {
        Kdc9i header;
        std::vector<uint16_t> fcls; // (2 bytes * fcls_9ir2_cnt) 11g 이전 압축된 컬럼 데이터의 길이
        std::vector<uint8_t>  perm; // (1 byte * perm_9ir2_cnt)  Block 내 컬럼 재배치 순서
    };

    template <bool IsLittle>
    inline Kdc9i decode_kdc9i0(tcb::span<const char> buf, size_t offset = 0) {
        return Kdc9i{
            .unknown1      = decode_at<uint32_t, IsLittle>(buf, offset + 0),
            .unknown2      = decode_at<uint32_t, IsLittle>(buf, offset + 4),
            .unknown3      = decode_at<uint32_t, IsLittle>(buf, offset + 8),
            .unknown4      = decode_at<uint32_t, IsLittle>(buf, offset + 12),
            .unknown5      = decode_at<uint16_t, IsLittle>(buf, offset + 16),
            .unknown6      = decode_at<uint8_t,  IsLittle>(buf, offset + 18),
            .fcls_9ir2_cnt = decode_at<uint8_t,  IsLittle>(buf, offset + 19),
            .perm_9ir2_cnt = decode_at<uint8_t,  IsLittle>(buf, offset + 20),
            .flag_9ir2     = decode_at<uint8_t,  IsLittle>(buf, offset + 21)
        };
    }

    [[nodiscard]] inline Result<Kdc9i> decode_kdc9i(tcb::span<const char> buf, bool isLittle, size_t offset = 0) {
        if (buf.size() < offset + sizeof(Kdc9i)) {
            return err_of(fmt::format("[Kdc9i] buf-size ({}) < offset-required ({})",
                                      buf.size(), offset + sizeof(Kdc9i) ));
        }

        return isLittle ? decode_kdc9i0<true>(buf, offset)
                        : decode_kdc9i0<false>(buf, offset);
    }

    [[nodiscard]] inline Result<Kdc9iData> decode_kdc9i_data(
        const tcb::span<const char> buf,
        const bool isLittle,
        const size_t start = 0)
    {
        auto header_res = decode_kdc9i(buf, isLittle, start);
        if (!header_res) {
            return tl::make_unexpected(header_res.error());
        }

        size_t current = start + sizeof(Kdc9i);
        const size_t fcls_bytes = sizeof(uint16_t) * header_res->fcls_9ir2_cnt;
        const size_t perm_bytes = sizeof(uint8_t) * header_res->perm_9ir2_cnt;

        if (buf.size() < current + fcls_bytes + perm_bytes) {
            return err_of(fmt::format("[Kdc9iData] buf-size ({}) < total-required ({})",
                                      buf.size(), current + fcls_bytes + perm_bytes ));
        }

        Kdc9iData out;
        out.header = *header_res;

        out.fcls.reserve(header_res->fcls_9ir2_cnt);
        for (uint8_t i = 0; i < header_res->fcls_9ir2_cnt; ++i) {
            uint16_t val = isLittle ? decode_at<uint16_t, true>(buf, current)
                                    : decode_at<uint16_t, false>(buf, current);
            out.fcls.push_back(val);
            current += sizeof(uint16_t);
        }

        out.perm.reserve(header_res->perm_9ir2_cnt);
        for (uint8_t i = 0; i < header_res->perm_9ir2_cnt; ++i) {
            uint8_t val = isLittle ? decode_at<uint8_t, true>(buf, current)
                                   : decode_at<uint8_t, false>(buf, current);
            out.perm.push_back(val);
            current += sizeof(uint8_t);
        }

        return out;
    }

#pragma pack(push, 1)
    /** 19.1 #1
     * KTDIR (Table Directory Entry - 4 bytes)
     *
     * https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-TableDirectory
     * - Data Header의 cnt_tbl_dir 수만큼 연속 생성 (4 bytes)
     * - 일반 테이블은 1개 / 압축 테이블은 2개 (첫번째: Symbol 테이블, 두번째: 유저 테이블)
     */
    struct Ktdir {
        uint16_t offset;       // (2 bytes, offset 0) Row Directory에서 자신의 row offset의 start Index
        uint16_t num_of_rows;  // (2 bytes, offset 2) Row의 개수
    };
    static_assert(sizeof(Ktdir) == 4, "Ktdir size mismatch");
#pragma pack(pop)

    template <bool IsLittle>
    inline Ktdir decode_ktdir0(const tcb::span<const char> buf, const size_t offset) {
        return Ktdir{
            .offset      = decode_at<uint16_t, IsLittle>(buf, offset + 0),
            .num_of_rows = decode_at<uint16_t, IsLittle>(buf, offset + 2)
        };
    }

    [[nodiscard]] inline Result<Ktdir> decode_ktdir(
        const tcb::span<const char> buf,
        const bool isLittle,
        const size_t offset = 0)
    {
        if (buf.size() < offset + sizeof(Ktdir)) {
            return err_of(fmt::format("[Ktdir] buf-size ({}) < offset-required ({})",
                                      buf.size(), offset + sizeof(Ktdir)));
        }

        return isLittle ? decode_ktdir0<true>(buf, offset)
                        : decode_ktdir0<false>(buf, offset);
    }

    [[nodiscard]] inline Result<vector<Ktdir>> decode_ktdirs(
        const tcb::span<const char> buf,
        const uint8_t cnt_dir,
        const bool isLittle,
        const size_t start_offset)
    {
        const size_t total_size = sizeof(Ktdir) * cnt_dir;
        if (buf.size() < start_offset + total_size) {
            return err_of(fmt::format("[Ktdirs] buf-size ({}) < total-required ({})",
                                      buf.size(), start_offset + total_size));
        }

        vector<Ktdir> entries;
        entries.reserve(cnt_dir);

        for (uint8_t i = 0; i < cnt_dir; ++i) {
            size_t current = start_offset + (i * sizeof(Ktdir));
            auto entry_res = decode_ktdir(buf, isLittle, current);
            if (!entry_res) {
                return tl::make_unexpected(entry_res.error());
            }
            entries.push_back(*entry_res);
        }

        return entries;
    }

    /** KTRDIR (Row Directory)
     * 모든 Table의 row offset을 저장 (Data Header의 cnt_row_dir 개수만큼 존재)
     * 2 bytes * cnt_row_dir
     * Offset 값이 -1 (0xFFFF) 혹은 SFLL(Slot Free Linked List) 연결 값일 경우 유효한 row offset이 아님
     */
    using Ktrdir = uint16_t; // (2 bytes) Row 데이터의 시작 위치 offset

    template <bool IsLittle>
    inline uint16_t decode_ktrdir0(tcb::span<const char> buf, size_t offset) {
        return decode_at<uint16_t, IsLittle>(buf, offset);
    }

    [[nodiscard]] inline Result<uint16_t> decode_ktrdir(tcb::span<const char> buf, bool isLittle, size_t offset = 0) {
        if (buf.size() < offset + sizeof(uint16_t)) {
            return err_of(fmt::format("[Ktrdir] buf-size ({}) < offset-required ({})",
                                                  buf.size(), offset + sizeof(uint16_t)));
        }

        return isLittle ? decode_ktrdir0<true>(buf, offset)
                        : decode_ktrdir0<false>(buf, offset);
    }

    [[nodiscard]] inline Result<vector<uint16_t>> decode_ktrdirs(
        const tcb::span<const char> buf,
        const uint16_t cnt_row_dir,
        const bool isLittle,
        const size_t start = 0)
    {
        const size_t total_size = sizeof(uint16_t) * cnt_row_dir;
        if (buf.size() < start + total_size) {
            return err_of(fmt::format("[Ktrdirs] buf-size ({}) < total-required ({})",
                                      buf.size(), start+ total_size));
        }

        vector<uint16_t> row_offsets;
        row_offsets.reserve(cnt_row_dir);

        for (uint16_t i = 0; i < cnt_row_dir; ++i) {
            size_t current_offset = start + (i * sizeof(uint16_t));
            auto row_offset_res = decode_ktrdir(buf, isLittle, current_offset);
            if (!row_offset_res) {
                return tl::make_unexpected(row_offset_res.error());
            }
            row_offsets.push_back(*row_offset_res);
        }

        return row_offsets;
    }

#pragma pack(push, 1)
    /**
     * KTRHD (Row Header - 3 bytes)
     * - Row Offset 위치에서 참조되는 Row 레코드 헤더 (3 bytes)
     *
     * flag
     * - 0x20: Deleted
     * - 0x01: Cluster
     * - 0x02: Head
     * - 0x04: First Piece
     * - 0x08: Last Piece
     */
    struct Ktrhd {
        uint8_t flag;    // (1 byte, offset 0) Row Flag (0x20=Deleted, 0x01=Cluster, 0x02=Head, 0x04=First Piece, 0x08=Last Piece 등)
        uint8_t lock;    // (1 byte, offset 1) ITL Slot 번호 (0이면 Lock 없음)
        uint8_t cols;    // (1 byte, offset 2) 해당 Row Piece의 컬럼 개수
    };
    static_assert(sizeof(Ktrhd) == 3, "Ktrhd size mismatch");
#pragma pack(pop)

    struct Kdcol {
        uint16_t len;
        size_t   data_offset;
    };

    struct Kdrow {
        Ktrhd header;
        vector<Kdcol> cols;
    };

    template <bool IsLittle>
    inline Ktrhd decode_ktrhd0(tcb::span<const char> buf, size_t offset) {
        return Ktrhd{
            .flag = decode_at<uint8_t, IsLittle>(buf, offset + 0),
            .lock = decode_at<uint8_t, IsLittle>(buf, offset + 1),
            .cols = decode_at<uint8_t, IsLittle>(buf, offset + 2)
        };
    }

    [[nodiscard]] inline Result<Ktrhd> decode_ktrhd(
        const tcb::span<const char> buf,
        const bool isLittle,
        const size_t offset = 0)
    {
        if (buf.size() < offset + sizeof(Ktrhd)) {
            return err_of(fmt::format("[Ktrhd] buf-size ({}) < offset-required ({})",
                                      buf.size(), offset + sizeof(Ktrhd)));
        }

        return isLittle ? decode_ktrhd0<true>(buf, offset)
                        : decode_ktrhd0<false>(buf, offset);
    }

    [[nodiscard]] inline Result<Kdrow> decode_kdrow(
        const tcb::span<const char> buf,
        const size_t data_header_offset,
        const uint16_t row_offset,
        const bool isLittle)
    {
        if (row_offset == 0xFFFF) return err_of("[Kdrow] invalid or free row slot (0xFFFF)");

        size_t current = data_header_offset + row_offset;

        auto rhd_res = decode_ktrhd(buf, isLittle, current);
        if (!rhd_res) return tl::make_unexpected(rhd_res.error());

        current += sizeof(Ktrhd);

        Kdrow row;
        row.header = *rhd_res;
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

            Kdcol col;
            col.len = col_len;

            if (col_len != 0xFF) {
                if (buf.size() < current + col_len) {
                    return err_of(fmt::format("[Kdrow] insufficient buffer for col-data-payload at index {}", i));
                }
                col.data_offset = current;
                current += col_len;
            } else {
                col.data_offset = 0;
            }

            row.cols.push_back(col);
        }
        return row;
    }

    /** Oracle Data Block */
    struct OraBlock {
        Ktbbh               header;        // Block Transaction Header
        vector<Ktb_ItlEntry>itls;          // ITL List
        Kdbh                data_header;   // Data Block Header
        optional<Kdc9iData> compress_info; // (선택) 9iR2 압축 정보
        vector<Ktdir>       table_dirs;    // Table Directory
        vector<uint16_t>    row_dir;       // Row Directory (Row Offsets)
        vector<Kdrow>       rows;          // Rows
    };

    [[nodiscard]] inline Result<OraBlock> decode_ora_block(tcb::span<const char> buf, bool isLittle) {

        size_t offset = 0;

        // 1. Tx header
        const auto hd_tx = decode_ktbbh(buf, isLittle);
        if (!hd_tx) return tl::make_unexpected(hd_tx.error());
        offset += sizeof(Ktbbh);

        // 2. ITLs
        const auto itls = decode_ktb_itl(buf, hd_tx->itl_cnt, isLittle, offset);
        if (!itls) return tl::make_unexpected(itls.error());
        offset += sizeof(Ktbit) * hd_tx->itl_cnt;

        // 2.1 skip (optional) Bitmap
        const auto bitmap_sz = (hd_tx->ktbbh_flg > 0x10) ? 8 : 0;
        offset += bitmap_sz;

        // 3. Data header
        const size_t data_header_offset = offset;

        auto hd_data = decode_kdbh(buf, isLittle, offset);
        if (!hd_data) return tl::make_unexpected(hd_data.error());
        offset += sizeof(Kdbh);

        // 4. (optional) Compress info
        optional<Kdc9iData> compress_info;
        if ((hd_data->flags & 0x40) != 0) {
            auto cdata_res = decode_kdc9i_data(buf, isLittle, offset);
            if (!cdata_res) return tl::make_unexpected(cdata_res.error());
            compress_info = *cdata_res;
            offset += sizeof(Kdc9i)
                    + (sizeof(uint16_t) * cdata_res->header.fcls_9ir2_cnt)
                    + (sizeof(uint8_t) * cdata_res->header.perm_9ir2_cnt);
        }

        // 5. Table Directories
        auto table_dirs = decode_ktdirs(buf, hd_data->cnt_tbl_dir, isLittle, offset);
        if (!table_dirs) { return tl::make_unexpected(table_dirs.error()); }
        offset += sizeof(Ktdir) * hd_data->cnt_tbl_dir;

        // 6. Row Directory
        auto row_dir = decode_ktrdirs(buf, hd_data->cnt_row_dir, isLittle, offset);
        if (!row_dir) { return tl::make_unexpected(row_dir.error()); }

        // 7. Rows
        vector<Kdrow> rows;
        rows.reserve(row_dir->size());

        for (uint16_t row_off : *row_dir) {
            const auto row = decode_kdrow(buf, data_header_offset, row_off, isLittle);
            if (row) {
                rows.push_back(std::move(*row));
            } else {
                rows.push_back(Kdrow{}); // invalid or free (0xFFFF 등)
            }
        }

        // 8. OraBlock
        return OraBlock{
            .header        = *hd_tx,
            .itls          = std::move(*itls),
            .data_header   = *hd_data,
            .compress_info = std::move(compress_info),
            .table_dirs    = std::move(*table_dirs),
            .row_dir       = std::move(*row_dir),
            .rows          = std::move(rows)
        };
    }

}
