#include "orders/order_manager.hpp"

#include <iostream>
#include <sstream>

namespace tradecore::orders {

OrderManager::OrderManager(matching::MatchingEngine& matcher,
                           booking::BookKeeper& book_keeper)
    : matcher_(matcher), book_keeper_(book_keeper) {}

std::vector<fix::FixMessage> OrderManager::handle_new_order(
    const fix::FixMessage& msg) {
    std::vector<fix::FixMessage> responses;

    if (!msg.has_new_order_single()) {
        responses.push_back(messaging::make_reject(msg, "Message has no NewOrderSingle body"));
        return responses;
    }

    const auto& nos = msg.new_order_single();

    // Convert FIX NewOrderSingle to internal Order
    Order order;
    try {
        order.cl_ord_id = nos.cl_ord_id();
        order.instrument = instrument::Instrument::from_proto(nos.instrument());
        order.side = (nos.side() == fix::SIDE_BUY) ? Side::Buy : Side::Sell;
        order.quantity = nos.order_qty();
        order.order_type = (nos.ord_type() == fix::ORD_TYPE_LIMIT) ? OrderType::Limit : OrderType::Market;
        order.limit_price = nos.price();
        order.strategy_id = nos.text();

        switch (nos.time_in_force()) {
            case fix::TIF_GTC: order.time_in_force = TimeInForce::GTC; break;
            case fix::TIF_IOC: order.time_in_force = TimeInForce::IOC; break;
            default: order.time_in_force = TimeInForce::Day; break;
        }
    } catch (const std::exception& e) {
        responses.push_back(messaging::make_reject(msg, std::string("Parse error: ") + e.what()));
        return responses;
    }

    // Assign order ID
    order.order_id = next_order_id();

    // Validate
    auto error = validate(order);
    if (!error.empty()) {
        responses.push_back(messaging::make_reject(msg, error));
        return responses;
    }

    // Accept
    order.status = OrderStatus::Accepted;
    std::cout << "[ORDER] Accepted " << order.order_id
              << " | " << side_to_string(order.side)
              << " " << order.quantity << " " << order.instrument.symbol
              << " @ " << order_type_to_string(order.order_type) << std::endl;

    // Try to match
    auto match_result = matcher_.try_match(order);

    if (match_result.matched) {
        order.status = (match_result.remaining_quantity == 0.0)
            ? OrderStatus::Filled
            : OrderStatus::PartiallyFilled;

        auto fill_id = next_fill_id();
        auto trade_id = next_trade_id();
        double commission = match_result.fill_price * match_result.fill_quantity * 0.001;

        // Book the trade
        booking::Trade trade;
        trade.trade_id = trade_id;
        trade.order_id = order.order_id;
        trade.cl_ord_id = order.cl_ord_id;
        trade.symbol = order.instrument.symbol;
        trade.side = side_to_string(order.side);
        trade.quantity = match_result.fill_quantity;
        trade.price = match_result.fill_price;
        trade.commission = commission;
        trade.timestamp = messaging::current_timestamp();
        trade.strategy_id = order.strategy_id;

        book_keeper_.book_trade(trade);

        std::cout << "[FILL]  " << fill_id
                  << " | " << order.instrument.symbol
                  << " " << match_result.fill_quantity
                  << " @ " << match_result.fill_price << std::endl;

        responses.push_back(messaging::make_execution_report_fill(
            msg, order.order_id, fill_id,
            match_result.fill_price, match_result.fill_quantity,
            match_result.remaining_quantity, match_result.fill_quantity,
            commission));
    } else {
        responses.push_back(messaging::make_reject(
            msg, "Could not match order — no market price available"));
    }

    // Store order
    orders_[order.order_id] = std::move(order);

    return responses;
}

std::string OrderManager::validate(const Order& order) const {
    if (order.cl_ord_id.empty()) return "ClOrdID (tag 11) is required";
    if (order.instrument.symbol.empty()) return "Symbol (tag 55) is required";
    if (order.quantity <= 0.0) return "OrderQty (tag 38) must be positive";
    if (order.order_type == OrderType::Limit && order.limit_price <= 0.0) {
        return "Price (tag 44) must be positive for limit orders";
    }
    return "";
}

const Order* OrderManager::find_order(const std::string& order_id) const {
    auto it = orders_.find(order_id);
    return (it != orders_.end()) ? &it->second : nullptr;
}

std::string OrderManager::next_order_id() {
    std::ostringstream ss;
    ss << "TC-" << std::setfill('0') << std::setw(5) << ++order_seq_;
    return ss.str();
}

std::string OrderManager::next_fill_id() {
    std::ostringstream ss;
    ss << "F-" << std::setfill('0') << std::setw(5) << ++fill_seq_;
    return ss.str();
}

std::string OrderManager::next_trade_id() {
    std::ostringstream ss;
    ss << "T-" << std::setfill('0') << std::setw(5) << ++trade_seq_;
    return ss.str();
}

}  // namespace tradecore::orders
