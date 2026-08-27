#pragma once
#include <string>

namespace ora {
    std::string opCode_string(uint8_t layer, uint8_t code);
    std::string cType_string(uint8_t layer, uint8_t code, uint8_t ctype);
}
