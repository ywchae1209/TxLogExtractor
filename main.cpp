#include <fstream>
#include <iostream>
#include <string>

#include "BlockIt.h"
#include "RecordSource.h"

using ora::BlockIt, ora::show, ora::NextRecordOffset;


void shows(const std::string &filename) {

    std::cout << filename << std::endl;
    std::cout << "*****************************************************************" << std::endl;

    // ================================================================================
    auto block_it = std::make_unique<BlockIt>(filename);
    ora::DefaultRecordSource record_source(std::move(block_it));

    auto nr = 0;
    while (auto rec = record_source.getNext()) {
        if ( nr > 100)
            break;
        show(*rec);
        nr++;
    }



    // ================================================================================
    // const auto bi{BlockIt(filename)}; // BlockIt : public BlockSource
    //
    // const auto fh = bi.getFileHead();
    // const auto rh = bi.getRedoHead();
    // // show(fh);
    // // show(rh);
    //
    // size_t n = 1;
    //
    // ora::Block ptr;
    // for (const auto& block : bi) {
    //     if (n > 100) break;
    //     n++;
    //     // ptr = block;
    //     show(block);
    // }

    std::cout << "*****************************************************************" << std::endl;
}

int main() {
    shows("redo05.log");
    shows("redo06.log");
    shows("redo07.log");

    shows("redo11.log");
    shows("redo12.log");
    shows("redo12.log");

    return 0;
}