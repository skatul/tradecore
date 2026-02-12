#pragma once

#include <string>
#include <unordered_map>

#include "orders/order.hpp"

namespace tradecore::matching {

struct MatchResult {
    bool matched = false;
    double fill_price = 0.0;
    double fill_quantity = 0.0;
    double remaining_quantity = 0.0;
};

class MatchingEngine {
public:
    /// For MVP: immediately fill market orders at the last known price.
    MatchResult try_match(const orders::Order& order);

    /// Set the "current market price" for a symbol.
    void update_market_price(const std::string& symbol, double price);

    double get_market_price(const std::string& symbol) const;

private:
    std::unordered_map<std::string, double> market_prices_;
};

}  // namespace tradecore::matching
