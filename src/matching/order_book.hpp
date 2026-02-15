#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>

namespace tradecore::matching {

struct OrderEntry {
    std::string order_id;
    std::string cl_ord_id;
    double price = 0.0;
    double remaining_quantity = 0.0;
    double original_quantity = 0.0;
    uint64_t sequence = 0;
};

struct PriceLevel {
    double price = 0.0;
    std::deque<OrderEntry> orders;

    double total_quantity() const {
        double total = 0.0;
        for (const auto& entry : orders) {
            total += entry.remaining_quantity;
        }
        return total;
    }
};

enum class BookSide { Bid, Ask };

struct DepthEntry {
    double price = 0.0;
    double quantity = 0.0;
    int order_count = 0;
};

class OrderBook {
public:
    void add_order(BookSide side, const OrderEntry& entry);

    bool cancel_order(const std::string& order_id);

    std::optional<double> best_bid() const;
    std::optional<double> best_ask() const;

    std::vector<DepthEntry> get_depth(BookSide side, size_t levels = 5) const;

    /// Walk the bid side consuming liquidity. Returns consumed entries.
    /// Each entry has fill_price and fill_quantity set.
    std::vector<OrderEntry> consume_bids(double quantity);

    /// Walk the ask side consuming liquidity. Returns consumed entries.
    std::vector<OrderEntry> consume_asks(double quantity);

    void cleanup_empty_levels();

    size_t bid_levels() const { return bids_.size(); }
    size_t ask_levels() const { return asks_.size(); }

private:
    // Bids: descending price order (std::greater)
    std::map<double, PriceLevel, std::greater<>> bids_;
    // Asks: ascending price order (default)
    std::map<double, PriceLevel> asks_;
    // O(1) cancel lookup: order_id -> (side, price)
    std::unordered_map<std::string, std::pair<BookSide, double>> order_index_;
    uint64_t sequence_ = 0;
};

}  // namespace tradecore::matching
