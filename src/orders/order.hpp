#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "instrument/instrument.hpp"

namespace tradecore::orders {

enum class Side { Buy, Sell };
enum class OrderType { Market, Limit };
enum class TimeInForce { Day, GTC, IOC };
enum class OrderStatus { Pending, Accepted, Filled, PartiallyFilled, Rejected, Cancelled };

inline Side side_from_string(const std::string& s) {
    if (s == "buy") return Side::Buy;
    if (s == "sell") return Side::Sell;
    throw std::invalid_argument("Unknown side: " + s);
}

inline std::string side_to_string(Side s) {
    switch (s) {
        case Side::Buy:  return "buy";
        case Side::Sell: return "sell";
    }
    return "unknown";
}

inline OrderType order_type_from_string(const std::string& s) {
    if (s == "market") return OrderType::Market;
    if (s == "limit") return OrderType::Limit;
    throw std::invalid_argument("Unknown order type: " + s);
}

inline std::string order_type_to_string(OrderType ot) {
    switch (ot) {
        case OrderType::Market: return "market";
        case OrderType::Limit:  return "limit";
    }
    return "unknown";
}

inline TimeInForce tif_from_string(const std::string& s) {
    if (s == "day") return TimeInForce::Day;
    if (s == "gtc") return TimeInForce::GTC;
    if (s == "ioc") return TimeInForce::IOC;
    throw std::invalid_argument("Unknown time in force: " + s);
}

inline std::string tif_to_string(TimeInForce tif) {
    switch (tif) {
        case TimeInForce::Day: return "day";
        case TimeInForce::GTC: return "gtc";
        case TimeInForce::IOC: return "ioc";
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

    static Order from_json(const nlohmann::json& payload) {
        Order order;
        order.cl_ord_id = payload.at("cl_ord_id").get<std::string>();
        order.instrument = instrument::Instrument::from_json(payload.at("instrument"));
        order.side = side_from_string(payload.at("side").get<std::string>());
        order.quantity = payload.at("quantity").get<double>();
        order.order_type = order_type_from_string(payload.at("order_type").get<std::string>());

        if (payload.contains("limit_price") && !payload["limit_price"].is_null()) {
            order.limit_price = payload["limit_price"].get<double>();
        }
        if (payload.contains("time_in_force")) {
            order.time_in_force = tif_from_string(payload["time_in_force"].get<std::string>());
        }
        if (payload.contains("strategy_id")) {
            order.strategy_id = payload["strategy_id"].get<std::string>();
        }

        return order;
    }
};

}  // namespace tradecore::orders
