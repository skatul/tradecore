#pragma once

#include <string>

#include "instrument/instrument.hpp"

namespace tradecore::orders {

enum class Side { Buy, Sell };
enum class OrderType { Market, Limit };
enum class TimeInForce { Day, GTC, IOC };
enum class OrderStatus { Pending, Accepted, Filled, PartiallyFilled, Rejected, Cancelled };

inline std::string side_to_string(Side s) {
    switch (s) {
        case Side::Buy:  return "buy";
        case Side::Sell: return "sell";
    }
    return "unknown";
}

inline std::string order_type_to_string(OrderType ot) {
    switch (ot) {
        case OrderType::Market: return "market";
        case OrderType::Limit:  return "limit";
    }
    return "unknown";
}

inline std::string status_to_string(OrderStatus s) {
    switch (s) {
        case OrderStatus::Pending:          return "pending";
        case OrderStatus::Accepted:         return "accepted";
        case OrderStatus::Filled:           return "filled";
        case OrderStatus::PartiallyFilled:  return "partially_filled";
        case OrderStatus::Rejected:         return "rejected";
        case OrderStatus::Cancelled:        return "cancelled";
    }
    return "unknown";
}

struct Order {
    std::string cl_ord_id;
    std::string order_id;
    instrument::Instrument instrument;
    Side side = Side::Buy;
    double quantity = 0.0;
    OrderType order_type = OrderType::Market;
    double limit_price = 0.0;
    TimeInForce time_in_force = TimeInForce::Day;
    std::string strategy_id;
    OrderStatus status = OrderStatus::Pending;
};

}  // namespace tradecore::orders
