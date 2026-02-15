#include "matching/order_book.hpp"

#include <algorithm>

namespace tradecore::matching {

void OrderBook::add_order(BookSide side, const OrderEntry& entry) {
    OrderEntry e = entry;
    e.sequence = ++sequence_;

    order_index_[e.order_id] = {side, e.price};

    if (side == BookSide::Bid) {
        auto& level = bids_[e.price];
        level.price = e.price;
        level.orders.push_back(std::move(e));
    } else {
        auto& level = asks_[e.price];
        level.price = e.price;
        level.orders.push_back(std::move(e));
    }
}

bool OrderBook::cancel_order(const std::string& order_id) {
    auto it = order_index_.find(order_id);
    if (it == order_index_.end()) return false;

    auto [side, price] = it->second;
    order_index_.erase(it);

    if (side == BookSide::Bid) {
        auto level_it = bids_.find(price);
        if (level_it != bids_.end()) {
            auto& orders = level_it->second.orders;
            orders.erase(
                std::remove_if(orders.begin(), orders.end(),
                    [&](const OrderEntry& e) { return e.order_id == order_id; }),
                orders.end());
            if (orders.empty()) bids_.erase(level_it);
        }
    } else {
        auto level_it = asks_.find(price);
        if (level_it != asks_.end()) {
            auto& orders = level_it->second.orders;
            orders.erase(
                std::remove_if(orders.begin(), orders.end(),
                    [&](const OrderEntry& e) { return e.order_id == order_id; }),
                orders.end());
            if (orders.empty()) asks_.erase(level_it);
        }
    }

    return true;
}

std::optional<double> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<double> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::vector<DepthEntry> OrderBook::get_depth(BookSide side, size_t levels) const {
    std::vector<DepthEntry> result;
    result.reserve(levels);

    if (side == BookSide::Bid) {
        for (auto it = bids_.begin(); it != bids_.end() && result.size() < levels; ++it) {
            DepthEntry d;
            d.price = it->first;
            d.quantity = it->second.total_quantity();
            d.order_count = static_cast<int>(it->second.orders.size());
            result.push_back(d);
        }
    } else {
        for (auto it = asks_.begin(); it != asks_.end() && result.size() < levels; ++it) {
            DepthEntry d;
            d.price = it->first;
            d.quantity = it->second.total_quantity();
            d.order_count = static_cast<int>(it->second.orders.size());
            result.push_back(d);
        }
    }

    return result;
}

std::vector<OrderEntry> OrderBook::consume_bids(double quantity) {
    std::vector<OrderEntry> fills;
    double remaining = quantity;

    auto it = bids_.begin();
    while (it != bids_.end() && remaining > 0.0) {
        auto& level = it->second;
        while (!level.orders.empty() && remaining > 0.0) {
            auto& front = level.orders.front();
            double fill_qty = std::min(remaining, front.remaining_quantity);

            OrderEntry fill;
            fill.order_id = front.order_id;
            fill.cl_ord_id = front.cl_ord_id;
            fill.price = front.price;
            fill.remaining_quantity = fill_qty;  // used as fill_quantity here

            remaining -= fill_qty;
            front.remaining_quantity -= fill_qty;

            if (front.remaining_quantity <= 0.0) {
                order_index_.erase(front.order_id);
                level.orders.pop_front();
            }

            fills.push_back(std::move(fill));
        }

        if (level.orders.empty()) {
            it = bids_.erase(it);
        } else {
            ++it;
        }
    }

    return fills;
}

std::vector<OrderEntry> OrderBook::consume_asks(double quantity) {
    std::vector<OrderEntry> fills;
    double remaining = quantity;

    auto it = asks_.begin();
    while (it != asks_.end() && remaining > 0.0) {
        auto& level = it->second;
        while (!level.orders.empty() && remaining > 0.0) {
            auto& front = level.orders.front();
            double fill_qty = std::min(remaining, front.remaining_quantity);

            OrderEntry fill;
            fill.order_id = front.order_id;
            fill.cl_ord_id = front.cl_ord_id;
            fill.price = front.price;
            fill.remaining_quantity = fill_qty;

            remaining -= fill_qty;
            front.remaining_quantity -= fill_qty;

            if (front.remaining_quantity <= 0.0) {
                order_index_.erase(front.order_id);
                level.orders.pop_front();
            }

            fills.push_back(std::move(fill));
        }

        if (level.orders.empty()) {
            it = asks_.erase(it);
        } else {
            ++it;
        }
    }

    return fills;
}

void OrderBook::cleanup_empty_levels() {
    for (auto it = bids_.begin(); it != bids_.end();) {
        if (it->second.orders.empty()) {
            it = bids_.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = asks_.begin(); it != asks_.end();) {
        if (it->second.orders.empty()) {
            it = asks_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace tradecore::matching
