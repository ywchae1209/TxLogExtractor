#include <fstream>
#include <iostream>
#include <string>

#include "BlockSource.h"
#include "RecordSource.h"

using ora::FileBlockSource, ora::RecordBound;

void shows(const std::string &filename,
           const bool showBlock,
           const bool showDump,
           const bool onlyBlock,
           const bool only1000) {

    std::cout << filename << std::endl;
    std::cout << "*****************************************************************" << std::endl;

    // ================================================================================
    auto block_it = std::make_unique<FileBlockSource>(filename, showBlock || onlyBlock); // <<<<<

    ora::show(block_it->getFileHead());
    ora::show(block_it->getRedoHead());

    ora::DefaultRecordSource record_source(std::move(block_it));                 // <<<<<

    auto nr = 0;
    while (auto rec = record_source.getNext()) {
        if (only1000 && (nr > 1000) ) break;
        if (!onlyBlock)
            show(*rec, showDump);                                               // <<<<<
        nr++;
    }

    std::cout << "*****************************************************************" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage: " << argv[0]
        << "--d --b --ob --s <filename1> <filename2> ...\n"
        << "--d :: show dump of Record\n"
           "--s :: short (only 1000 Records)\n"
           "--b :: show Block\n"
           "--ob :: show only Block\n"
        ;
        return 1;
    }

    bool showBlock = false;
    bool showDump = false;
    bool onlyBlock = false;
    bool only1000 = false;

    for (int i = 1; i < argc; ++i) {

        const std::string opt = argv[i];
        if (showDump  || opt == "--d"){ showDump = true; }
        if (showBlock || opt == "--b") { showBlock = true;}
        if (onlyBlock || opt == "--ob"){ onlyBlock = true; }
        if (only1000 || opt == "--s"){ only1000 = true; }
    }

   // ----------------------------------------
    for (int i = 1; i < argc; ++i) {

        const std::string opt = argv[i];
        if (opt.rfind("--", 0) == 0) continue;

        shows(opt, showBlock, showDump, onlyBlock, only1000);
    }

    return 0;
}