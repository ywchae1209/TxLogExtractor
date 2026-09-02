// #include <execution>
#include <algorithm>
#include <tcb/span.hpp>

#include "BlockSource.h"
#include "../coral_result.h"
#include "../coral_show.h"

namespace ora {

    using ora::FileHead;
    using ora::RedoHead;
    using ora::BlockHead, ora::Block;
    using coral::Result, coral::err_of;
    using fmt::format;

    static Result<FileHead> read_FileHead(std::ifstream& in) {
        constexpr auto size = 256;

        std::vector<char> buf(size);

        in.read(buf.data(), size);
        if (const auto r = in.gcount(); r != size)
            return err_of(format("[FileHead] need at least 256 bytes, but {}", r));

        auto out = FileHead_of(buf);
        if (!out) return err_of(out.error());

        // consume rest
        in.seekg(out->block_sz - size, std::ios::cur);

        return out;
    }

    static Result<RedoHead> read_RedoHead(std::ifstream& in, const FileHead& f) {

        std::vector<char> buf(f.block_sz);

        in.read(buf.data(), f.block_sz);
        if (const auto r = in.gcount(); r != f.block_sz)
            return err_of(format("[RedoHead] need {} bytes,  but {}", f.block_sz, r));

        const auto b = Block_of(buf, f.isLittle);
        if (b.isEmpty())
            return err_of("[RedoHead] empty block");

        return RedoHead_of(b, f.isLittle);
    }

    FileBlockSource::FileBlockSource(const std::string &path, const uint8_t showMode)
        : showMode(showMode) {

        auto f = std::ifstream(path, std::ios::binary);
        if (!f) throw std::runtime_error("Cannot open file: " + path);

        auto fh = read_FileHead(f);
        if (!fh) throw std::runtime_error(fh.error());

        auto rh = read_RedoHead(f, *fh);
        if (!rh) throw std::runtime_error(rh.error());

        // ----------------------------------------
        file = std::move(f);
        log_seq_no = rh->log_seq_no;
        fileHead = *fh;
        redoHead = *rh;
        ctx = BlockCtx{
            fileHead.isLittle,
            redoHead.sourceInfo.compat_ver.major >= 12,
            fileHead.block_sz,
            redoHead.writeInfo.low_scn,
            redoHead.writeInfo.next_scn
        };
    }

    bool FileBlockSource::drain() {

        const auto &ctx = getCtx();
        const auto showReason = (showMode & 2) == 2;

        const auto CHUNK_SIZE = BLOCKS_PER_READ() * ctx.block_sz;

        //std::vector<char> read_buf;
        if (read_buf.size() < CHUNK_SIZE) {
            read_buf.resize(CHUNK_SIZE);
        }

        if (!file) return false;

        file.read(read_buf.data(), CHUNK_SIZE);
        const auto bytes_read = static_cast<size_t>(file.gcount());

        if (bytes_read == 0) { fmt::println("-- End of file"); return false; }

        if (bytes_read % ctx.block_sz != 0) {
            throw std::runtime_error(format(
                "-- Invalid read size: {} bytes is not a multiple of block_sz ({})",
                bytes_read, ctx.block_sz
            ));
        }

        const size_t blocks_in_chunk = bytes_read / ctx.block_sz;

        // fmt::println("-- Blocks in chunk {}", blocks_in_chunk);

        const auto sp = read_buf.begin();
        for (size_t i = 0; i < blocks_in_chunk; ++i) {
            const size_t offset = i * ctx.block_sz;
            const auto from = sp + offset;
            const auto to = from + ctx.block_sz;

            auto b = Block_of( std::vector(from, to), ctx.isLittle);
            b.set_bounds(ctx, showReason);

            if (b.log_seq_no() != this->log_seq_no) {
                fmt::println(std::cerr, "-- end of same LSN({}) blocks. block #{} (LSN:{})",
                             log_seq_no, b.block_no(), b.log_seq_no());
                return i != 0;
            }
            out_buffer.push_back(std::move(b));
        }
        return true;
    }

    std::optional<Block> FileBlockSource::getNext() {

        if (out_buffer.empty()) {
            if (!drain())
                return std::nullopt;
        }

        auto b = std::move(out_buffer.front());

        if ((showMode & 1) == 1)
            show(b);

        out_buffer.pop_front();
        return b;
    }
}