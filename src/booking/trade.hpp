#pragma once

#include <string>

namespace tradecore::booking {

struct Trade {
    std::string trade_id;
    std::string order_id;
    std::string cl_ord_id;
    std::string symbol;
    std::string side;  // "buy" / "sell"
    double quantity = 0.0;
    double price = 0.0;
    double commission = 0.0;
    std::string timestamp;
    std::string strategy_id;
};

}  // namespace tradecore::booking
