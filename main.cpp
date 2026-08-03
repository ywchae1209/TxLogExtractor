#include <fstream>
#include <iostream>
#include <string>

#include "BlockIt.h"
#include "RecordSource.h"

using ora::BlockIt, ora::show, ora::RecordBound;

void shows(const std::string &filename) {

    std::cout << filename << std::endl;
    std::cout << "*****************************************************************" << std::endl;

    // ================================================================================
    auto block_it = std::make_unique<BlockIt>(filename);
    // show(block_it->getFileHead());
    show(block_it->getRedoHead());


    ora::DefaultRecordSource record_source(std::move(block_it));

    auto nr = 0;
    while (auto rec = record_source.getNext()) {
        if ( nr > 1000) break;
        show(*rec);
        nr++;
    }

    std::cout << "*****************************************************************" << std::endl;
}

int main() {
    shows("redo05.log");
    // shows("redo06.log");
    // shows("redo07.log");

    // shows("redo11.log");
    // shows("redo12.log");
    // shows("redo12.log");

    return 0;
}