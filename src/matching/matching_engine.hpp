#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "matching/order_book.hpp"
#include "orders/order.hpp"

namespace tradecore::matching {

struct FillEvent {
    std::string order_id;         // aggressor order
    std::string resting_order_id; // resting order consumed
    double fill_price = 0.0;
    double fill_quantity = 0.0;
};

struct MatchResult {
    bool matched = false;
    double fill_price = 0.0;
    double fill_quantity = 0.0;
    double remaining_quantity = 0.0;
    std::vector<FillEvent> fills;
};

class MatchingEngine {
public:
    /// Match an order against the book. For market orders, walks the book.
    /// For limit orders, matches crossable levels and rests the remainder.
    MatchResult try_match(const orders::Order& order);

    /// Set the "current market price" for a symbol (used for auto-seeding).
    void update_market_price(const std::string& symbol, double price);

    double get_market_price(const std::string& symbol) const;

    /// Seed synthetic liquidity around a reference price for backtest simulation.
    void seed_book(const std::string& symbol, double ref_price,
                   double spread_bps = 10.0, int depth_levels = 5,
                   double qty_per_level = 1000.0);

    /// Cancel a resting order from the book.
    bool cancel_order(const std::string& symbol, const std::string& order_id);

    /// Get the order book for a symbol. Returns nullptr if none exists.
    const OrderBook* get_book(const std::string& symbol) const;

private:
    MatchResult match_market_order(const orders::Order& order);
    MatchResult match_limit_order(const orders::Order& order);

    std::unordered_map<std::string, double> market_prices_;
    std::unordered_map<std::string, OrderBook> books_;
};

}  // namespace tradecore::matching
