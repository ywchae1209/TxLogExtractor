#include <fstream>
#include <iostream>
#include <string>

#include "BlockSource.h"
#include "RecordSource.h"

using ora::FileBlockSource, ora::RecordBound;

void shows(const std::string &filename,
           const bool showInfo,
           const bool showBlock,
           const bool showDump,
           const bool onlyBlock,
           const bool only1000) {

    std::cout << filename << std::endl;
    std::cout << "*****************************************************************" << std::endl;

    // ================================================================================
    auto block_it = std::make_unique<FileBlockSource>(filename, showBlock || onlyBlock);

    ora::show(block_it->getFileHead());
    ora::show(block_it->getRedoHead());

    ora::DefaultRecordSource record_source(std::move(block_it));   // moved

    auto nr = 0;

    uint8_t showMode = (showDump? 2: 0) + (showInfo? 1 : 0);

    while (auto rec = record_source.getNext()) {
        if (only1000 && (nr > 1000) ) break;
        if (!onlyBlock)
            show(*rec, showMode);
        nr++;
    }

    std::cout << "*****************************************************************" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage: " << argv[0]
        << "--i --d --b --ob --s <filename1> <filename2> ...\n"
        <<
           "--i :: info of change head\n"
           "--d :: show dump of Record\n"
           "--s :: short (only 1000 Records)\n"
           "--b :: show Block\n"
           "--ob :: show only Block\n"
        ;
        return 1;
    }

    bool showInfo = false;
    bool showDump = false;
    bool showBlock = false;
    bool onlyBlock = false;
    bool only1000 = false;

    for (int i = 1; i < argc; ++i) {

        const std::string opt = argv[i];
        if (showInfo  || opt == "--i"){ showInfo = true; }
        if (showDump  || opt == "--d"){ showDump = true; }
        if (showBlock || opt == "--b"){ showBlock = true;}
        if (only1000  || opt == "--s"){ only1000 = true; }
        if (onlyBlock || opt == "--ob"){ onlyBlock = true; }
    }

   // ----------------------------------------
    for (int i = 1; i < argc; ++i) {

        const std::string opt = argv[i];
        if (opt.rfind("--", 0) == 0) continue;

        shows(opt, showInfo, showBlock, showDump, onlyBlock, only1000);
    }

    return 0;
}