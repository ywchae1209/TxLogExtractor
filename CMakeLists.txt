cmake_minimum_required(VERSION 3.26)
project(TxLogExtractor)

set(CMAKE_CXX_STANDARD 17)

add_executable(TxLogExtractor
        src/main.cpp
        src/FileHead.cpp
        src/RedoHead.cpp
        src/RecordHead.cpp
        src/Block.cpp
        src/BlockIt.cpp
        src/RecordSource.cpp
        src/OpCodes.cpp
)

################################################################################
# header-only library :: usr/local/include/
################################################################################
# fmt      :: formating for print
# lexy     :: parser combinator ~ for complex decoding
# stx      :: exception handling
# tcb      :: C++17 read-only binary-collection
# range-v3 :: C++17 range

