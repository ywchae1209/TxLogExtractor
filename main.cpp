#include <fstream>
#include <iostream>
#include <string>

#include "BlockIt.h"

using ora::BlockIt, ora::show, ora::NextRecordOffset;

void shows(const std::string &filename) {

    std::cout << filename << std::endl;
    std::cout << "*****************************************************************" << std::endl;

    const auto bi{BlockIt(filename)};

    const auto fh = bi.getFileHead();
    const auto rh = bi.getRedoHead();
    show(fh);
    show(rh);

    size_t n = 1;

    for (const auto& block : bi) {
        n++;
        show(block);
    }

    std::cout << "*****************************************************************" << std::endl;
}

int main() {
    shows("redo05.log");
    // shows("redo06.log");
    // shows("redo07.log");
    //
    // shows("redo11.log");
    // shows("redo12.log");
    // shows("redo12.log");

    return 0;
}