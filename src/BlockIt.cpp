#include "BlockIt.h"
#include "coral_result.h"

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

        auto b = Block_of(buf, fh.block_sz, fh.isLittle);

        return Ok_of(std::move(b));
    }

    static Result<RedoHead> read_RedoHead(std::ifstream& in, const Block& b, const FileHead& fh) {
        auto rh = RedoHead_of(b.payload, fh.isLittle);

        if (!rh.isOk() )
            return Err_of<RedoHead> ("[read_RedoHead] Invalid Redo-Head: " + to_string(rh.valid));

        return Ok_of(std::move(rh));
    }

    BlockIt::BlockIt(const std::string &path, const bool showBlock, const size_t buffer_sz)
    :out_buffer_sz { buffer_sz}, showBlock(showBlock)
    {
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
    }

    void BlockIt::drain() {

        const auto low = redoHead.writeInfo.low_scn;
        const auto top = redoHead.writeInfo.next_scn;

        const auto block_sz = fileHead.block_sz;

        read_buf.resize(block_sz);

        for (auto i = 0; i < out_buffer_sz && file; ++i) {
            file.read(read_buf.data(), block_sz);

            if (file.gcount() < block_sz)
                break; // exception. this must not happen

            auto b = Block_of(read_buf, block_sz, fileHead.isLittle, low, top);

            if (b.head.log_seq_no != this->log_seq_no)
                break;

            out_buffer.push_back(b);
        }
    }

    std::optional<Block> BlockIt::getNext() {
        if (out_buffer.empty()) {
            drain();
            if (out_buffer.empty())
                return std::nullopt;
        }
        auto b = out_buffer.front();

        if (showBlock)
            show(b);
        out_buffer.pop_front();
        return b;
    }
}