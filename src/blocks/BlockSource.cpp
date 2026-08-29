#include "BlockSource.h"
#include "../coral_result.h"

namespace ora {

    using ora::FileHead;
    using ora::RedoHead;
    using ora::BlockHead, ora::Block;
    using coral::Ok_of, coral::Err_of, coral::Result;

    static Result<FileHead> read_FileHead(std::ifstream& in) {

        std::vector<char> buf(256);

        in.read(buf.data(), buf.size());

        if (in.gcount() != 256)
            return Err_of<FileHead>( "[read_FileHead] Fail to read 256 bytes, but " + std::to_string(in.gcount()) );

        auto h = FileHead_of(buf);
        if (const auto e = h.errString(); e.has_value())
            return Err_of<FileHead>( "[read_FileHead] Invalid File-Head: " + *e);

        in.seekg(h.block_sz - 256 , std::ios::cur);

        return Ok_of(std::move(h));  // move
    }

    static Result<Block> read_RedoHeadBlock(std::ifstream& in, const FileHead& fh) {
        std::vector<char> buf(fh.block_sz);

        in.read(buf.data(), fh.block_sz);
        if (in.gcount() != fh.block_sz)
            return Err_of<Block>("[read_RedoHead] Fail to read block_sz,  but " + std::to_string(in.gcount()) );

        auto b = Block_of(buf, fh.isLittle);

        return Ok_of(std::move(b));
    }

    static Result<RedoHead> read_RedoHead(std::ifstream& in, const Block& b, const FileHead& fh) {
        auto rh = RedoHead_of(b.payload(), fh.isLittle);

        if (!rh.isOk() )
            return Err_of<RedoHead> ("[read_RedoHead] Invalid Redo-Head: " + to_string(rh.valid));

        return Ok_of(std::move(rh));
    }

    FileBlockSource::FileBlockSource(const std::string &path, const uint8_t showBlock)
        : showMode(showBlock) {
        auto f = std::ifstream(path, std::ios::binary);
        if (!f) throw std::runtime_error("Cannot open file: " + path);

        auto fh = read_FileHead(f);
        if (!fh.ok()) throw std::runtime_error(fh.error);

        auto rb = read_RedoHeadBlock(f, fh.get());
        if (!rb.ok()) throw std::runtime_error(rb.error);

        auto rh = read_RedoHead(f, rb.get(), fh.get());
        if (!rh.ok()) throw std::runtime_error(rh.error);

        // ----------------------------------------
        file = std::move(f);
        log_seq_no = rb.get().head.log_seq_no;
        fileHead = std::move(fh.get());
        redoHead = std::move(rh.get());
        ctx = BlockCtx{
            fileHead.isLittle,
            redoHead.sourceInfo.compat_ver.major >= 12,
            fileHead.block_sz,
            redoHead.writeInfo.low_scn,
            redoHead.writeInfo.next_scn
        };
    }

    void FileBlockSource::drain() {

        const auto &ctx = getCtx();
        const auto showReason = (showMode & 2) == 2;

        const size_t CHUNK_SIZE = BLOCKS_PER_READ() * ctx.block_sz;

        if (read_buf.size() < CHUNK_SIZE) {
            read_buf.resize(CHUNK_SIZE);
        }

        if (!file) return;

        file.read(read_buf.data(), CHUNK_SIZE);
        const auto bytes_read = static_cast<size_t>(file.gcount());

        if (bytes_read == 0) {
            fmt::println("-- End of file");
            return;
        }

        if (bytes_read % ctx.block_sz != 0) {
            throw std::runtime_error(fmt::format(
                "-- Invalid read size: {} bytes is not a multiple of block_sz ({})",
                bytes_read, ctx.block_sz
            ));
        }

        const size_t blocks_in_chunk = bytes_read / ctx.block_sz;

        for (size_t i = 0; i < blocks_in_chunk; ++i) {
            const size_t offset = i * ctx.block_sz;

            auto b = Block_of(
                tcb::span<const char>{
                    read_buf.data() + offset,
                    static_cast<size_t>(ctx.block_sz)
                },
                ctx,
                showReason
            );

            if (b.head.log_seq_no != this->log_seq_no) {
                fmt::println(std::cerr, "-- end of same LSN({}) blocks. block #{} (LSN:{})",
                             log_seq_no, b.head.block_no, b.head.log_seq_no);
                break;
            }

            out_buffer.push_back(std::move(b));
        }
    }

    std::optional<Block> FileBlockSource::getNext() {

        if (out_buffer.empty()) {
            drain();
            if (out_buffer.empty())
                return std::nullopt;
        }

        auto b = std::move(out_buffer.front());

        if ((showMode & 1) == 1)
            show(b);

        out_buffer.pop_front();
        return b;
    }
}