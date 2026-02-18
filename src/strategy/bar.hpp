#pragma once

#include <cstdint>
#include <string>

namespace tradecore::strategy {

struct Bar {
    int64_t timestamp = 0;  // Unix epoch seconds
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double volume = 0.0;
    std::string symbol;
};

}  // namespace tradecore::strategy
