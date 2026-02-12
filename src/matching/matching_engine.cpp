#include "matching/matching_engine.hpp"

namespace tradecore::matching {

MatchResult MatchingEngine::try_match(const orders::Order& order) {
    MatchResult result;

    if (order.order_type == orders::OrderType::Market) {
        // MVP: fill at last known price, or limit_price as fallback
        double price = 0.0;
        auto it = market_prices_.find(order.instrument.symbol);
        if (it != market_prices_.end()) {
            price = it->second;
        } else if (order.limit_price > 0.0) {
            price = order.limit_price;
        } else {
            // No price available — can't fill
            return result;
        }

        result.matched = true;
        result.fill_price = price;
        result.fill_quantity = order.quantity;
        result.remaining_quantity = 0.0;
    } else if (order.order_type == orders::OrderType::Limit) {
        // MVP: fill limit orders immediately at limit price
        if (order.limit_price > 0.0) {
            result.matched = true;
            result.fill_price = order.limit_price;
            result.fill_quantity = order.quantity;
            result.remaining_quantity = 0.0;
        }
    }

    return result;
}

void MatchingEngine::update_market_price(const std::string& symbol, double price) {
    market_prices_[symbol] = price;
}

double MatchingEngine::get_market_price(const std::string& symbol) const {
    auto it = market_prices_.find(symbol);
    return (it != market_prices_.end()) ? it->second : 0.0;
}

}  // namespace tradecore::matching
