#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "booking/book_keeper.hpp"
#include "matching/matching_engine.hpp"
#include "messaging/protocol.hpp"
#include "orders/order.hpp"

namespace tradecore::orders {

class OrderManager {
public:
    OrderManager(matching::MatchingEngine& matcher, booking::BookKeeper& book_keeper);

    /// Process an incoming new_order message. Returns response messages.
    std::vector<messaging::Message> handle_new_order(const messaging::Message& msg);

    /// Validate order fields. Returns empty string if valid, error otherwise.
    std::string validate(const Order& order) const;

    const Order* find_order(const std::string& order_id) const;

    size_t order_count() const { return orders_.size(); }

private:
    std::string next_order_id();
    std::string next_fill_id();
    std::string next_trade_id();

    matching::MatchingEngine& matcher_;
    booking::BookKeeper& book_keeper_;
    std::unordered_map<std::string, Order> orders_;
    uint64_t order_seq_ = 0;
    uint64_t fill_seq_ = 0;
    uint64_t trade_seq_ = 0;
};

}  // namespace tradecore::orders
