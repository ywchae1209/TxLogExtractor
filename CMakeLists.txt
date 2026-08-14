cmake_minimum_required(VERSION 3.26)

project(TxLogExtractor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

################################################################################
#add_library(TxLogCore STATIC       #or SHARED
#        src/FileHead.cpp
#        src/RedoHead.cpp
#        src/RecordHead.cpp
#        src/Block.cpp
#        src/BlockSource.cpp
#        src/RecordSource.cpp
#        src/OpCodes.cpp
#)
#
#target_include_directories(TxLogCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/include"
#)

add_executable(TxLogExtractor
        src/main.cpp
        src/FileHead.cpp
        src/RedoHead.cpp
        src/Block.cpp
        src/BlockSource.cpp
        src/Record.cpp
        src/ChangeHead.cpp
        src/RecordSource.cpp
        src/ora_opCodes.cpp
)

#target_link_libraries(TxLogExtractor PRIVATE TxLogCore)

target_include_directories(TxLogExtractor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/include")

################################################################################
# header-only library :: usr/local/include/
################################################################################
# fmt      :: formating for print
# lexy     :: parser combinator ~ for complex decoding
# stx      :: exception handling
# tcb      :: C++17 read-only binary-collection
# range-v3 :: C++17 range

