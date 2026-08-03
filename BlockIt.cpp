#include "BlockIt.h"

namespace ora {

    using ora::FileHead, ora::FileHead_of;
    using ora::RedoHead, ora::RedoHead_of;
    using ora::BlockHead, ora::BlockHead_of, ora::Block;

    BlockIt::BlockIt(const std::string &path, const size_t len)
        : file(path, std::ios::binary), len_drain(len)
    {
        if (!file) throw std::runtime_error("Cannot open file: " + path);
        read_FileHead();
        read_RedoHead();
    }

    void BlockIt::validate(Block& b) const noexcept {
        if (b.head.log_seq_no != log_seq_no) {
            b.head.valid = BHValid::Log_sqn_mismatch;
        }
    }

    void BlockIt::drain() {

        const auto low = redoHead.writeInfo.low_scn;
        const auto top = redoHead.writeInfo.next_scn;

        buf_drain.clear();
        for (size_t i = 0; i < len_drain && file; ++i) {
            file.read(buf.data(), block_sz);

            if (file.gcount() < block_sz) break;

            auto b = make_Block(buf, block_sz, fileHead.isLittle, low, top);
            validate(b);

            if (b.head.valid != BHValid::Ok)
                break;

            buf_drain.push_back(b);
        }
    }

    void BlockIt::read_FileHead() {

        buf.resize(256);
        file.read(buf.data(), static_cast<std::streamsize>(buf.size()));

        fileHead = FileHead_of(buf);
        if (fileHead.valid != FHValid::Ok) {
            throw std::runtime_error("Invalid file header: " + to_string(fileHead.valid));
        }

        block_sz = static_cast<std::streamsize>(fileHead.block_sz);
        if (block_sz < 256) throw std::runtime_error("Block size must be >= 256");

        buf.resize(block_sz);
        file.seekg(block_sz, std::ios::beg);
    }

    void BlockIt::read_RedoHead() {
        buf.resize(block_sz);

        file.read(buf.data(), block_sz);
        if (file.gcount() < block_sz) throw std::runtime_error("Failed to read Redo Header block");

        const auto b = make_Block(buf, block_sz, fileHead.isLittle);
        show(b.head);
        log_seq_no = b.head.log_seq_no;

        redoHead = RedoHead_of(b.payload, fileHead.isLittle);
        if (redoHead.valid != RHValid::Ok) {
            throw std::runtime_error("Invalid redo header: " + to_string(redoHead.valid));
        }
    }

    std::optional<Block> BlockIt::getNext() {
        if (buf_drain.empty()) {
            drain();
            if (buf_drain.empty())
                return std::nullopt;
        }
        auto b = buf_drain.front();
        // show(b);
        buf_drain.pop_front();
        return b;
    }
}