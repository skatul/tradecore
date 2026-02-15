#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <fix_messages.pb.h>
#include "booking/book_keeper.hpp"
#include "matching/matching_engine.hpp"
#include "messaging/protocol.hpp"
#include "orders/order.hpp"

namespace tradecore::orders {

class OrderManager {
public:
    OrderManager(matching::MatchingEngine& matcher, booking::BookKeeper& book_keeper,
                 double commission_rate = 0.001);

    /// Process an incoming NewOrderSingle. Returns response FixMessages.
    std::vector<fix::FixMessage> handle_new_order(const fix::FixMessage& msg);

    /// Process an incoming OrderCancelRequest. Returns response FixMessages.
    std::vector<fix::FixMessage> handle_cancel_request(const fix::FixMessage& msg);

    /// Validate order fields. Returns empty string if valid, error otherwise.
    std::string validate(const Order& order) const;

    const Order* find_order(const std::string& order_id) const;

    /// Find order by cl_ord_id
    const Order* find_order_by_cl_ord_id(const std::string& cl_ord_id) const;

    size_t order_count() const { return orders_.size(); }

private:
    std::string next_order_id();
    std::string next_fill_id();
    std::string next_trade_id();

    matching::MatchingEngine& matcher_;
    booking::BookKeeper& book_keeper_;
    double commission_rate_;
    std::unordered_map<std::string, Order> orders_;
    std::unordered_map<std::string, std::string> cl_ord_to_order_id_;
    uint64_t order_seq_ = 0;
    uint64_t fill_seq_ = 0;
    uint64_t trade_seq_ = 0;
};

}  // namespace tradecore::orders
