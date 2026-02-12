#include "booking/book_keeper.hpp"

namespace tradecore::booking {

void BookKeeper::book_trade(const Trade& trade) {
    trades_.push_back(trade);

    auto& pos = positions_[trade.symbol];
    if (pos.symbol.empty()) {
        pos.symbol = trade.symbol;
    }
    pos.apply_fill(trade.side, trade.quantity, trade.price);
}

const Position* BookKeeper::get_position(const std::string& symbol) const {
    auto it = positions_.find(symbol);
    return (it != positions_.end()) ? &it->second : nullptr;
}

std::vector<Position> BookKeeper::get_all_positions() const {
    std::vector<Position> result;
    result.reserve(positions_.size());
    for (const auto& [_, pos] : positions_) {
        result.push_back(pos);
    }
    return result;
}

}  // namespace tradecore::booking
