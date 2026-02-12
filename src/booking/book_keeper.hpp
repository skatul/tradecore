#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "booking/position.hpp"
#include "booking/trade.hpp"

namespace tradecore::booking {

class BookKeeper {
public:
    /// Record a trade and update positions.
    void book_trade(const Trade& trade);

    /// Get current position for a symbol. Returns nullptr if no position.
    const Position* get_position(const std::string& symbol) const;

    /// Get all positions.
    std::vector<Position> get_all_positions() const;

    /// Get trade history (book of records).
    const std::vector<Trade>& get_trades() const { return trades_; }

    /// Get number of trades booked.
    size_t trade_count() const { return trades_.size(); }

private:
    std::vector<Trade> trades_;
    std::unordered_map<std::string, Position> positions_;
};

}  // namespace tradecore::booking
