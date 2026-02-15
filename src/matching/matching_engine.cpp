#include "matching/matching_engine.hpp"

#include <cmath>
#include <sstream>

namespace tradecore::matching {

MatchResult MatchingEngine::try_match(const orders::Order& order) {
    const auto& symbol = order.instrument.symbol;

    // Backward compatibility: auto-seed book if market_prices_ has a price but no book
    auto price_it = market_prices_.find(symbol);
    if (books_.find(symbol) == books_.end() && price_it != market_prices_.end()) {
        seed_book(symbol, price_it->second);
    }

    if (order.order_type == orders::OrderType::Market) {
        return match_market_order(order);
    } else if (order.order_type == orders::OrderType::Limit) {
        return match_limit_order(order);
    }

    return {};
}

MatchResult MatchingEngine::match_market_order(const orders::Order& order) {
    MatchResult result;
    auto book_it = books_.find(order.instrument.symbol);

    if (book_it == books_.end()) {
        // Fallback: use limit_price if available (backward compat)
        if (order.limit_price > 0.0) {
            result.matched = true;
            result.fill_price = order.limit_price;
            result.fill_quantity = order.quantity;
            result.remaining_quantity = 0.0;
            FillEvent fe;
            fe.order_id = order.order_id;
            fe.fill_price = order.limit_price;
            fe.fill_quantity = order.quantity;
            result.fills.push_back(std::move(fe));
        }
        return result;
    }

    auto& book = book_it->second;

    // Buy market order: consume asks (ascending price)
    // Sell market order: consume bids (descending price)
    std::vector<OrderEntry> consumed;
    if (order.side == orders::Side::Buy) {
        consumed = book.consume_asks(order.quantity);
    } else {
        consumed = book.consume_bids(order.quantity);
    }

    if (consumed.empty()) return result;

    double total_qty = 0.0;
    double total_notional = 0.0;

    for (const auto& entry : consumed) {
        double qty = entry.remaining_quantity;  // fill qty stored here
        total_qty += qty;
        total_notional += qty * entry.price;

        FillEvent fe;
        fe.order_id = order.order_id;
        fe.resting_order_id = entry.order_id;
        fe.fill_price = entry.price;
        fe.fill_quantity = qty;
        result.fills.push_back(std::move(fe));
    }

    result.matched = true;
    result.fill_quantity = total_qty;
    result.fill_price = total_notional / total_qty;  // VWAP
    result.remaining_quantity = order.quantity - total_qty;

    return result;
}

MatchResult MatchingEngine::match_limit_order(const orders::Order& order) {
    MatchResult result;
    auto& book = books_[order.instrument.symbol];

    double remaining = order.quantity;
    double total_qty = 0.0;
    double total_notional = 0.0;

    // Match crossable levels
    if (order.side == orders::Side::Buy) {
        // Buy limit: match against asks where ask_price <= limit_price
        auto best = book.best_ask();
        while (best.has_value() && best.value() <= order.limit_price && remaining > 0.0) {
            auto consumed = book.consume_asks(remaining);
            for (const auto& entry : consumed) {
                if (entry.price > order.limit_price) break;
                double qty = entry.remaining_quantity;
                total_qty += qty;
                total_notional += qty * entry.price;
                remaining -= qty;

                FillEvent fe;
                fe.order_id = order.order_id;
                fe.resting_order_id = entry.order_id;
                fe.fill_price = entry.price;
                fe.fill_quantity = qty;
                result.fills.push_back(std::move(fe));
            }
            best = book.best_ask();
        }
    } else {
        // Sell limit: match against bids where bid_price >= limit_price
        auto best = book.best_bid();
        while (best.has_value() && best.value() >= order.limit_price && remaining > 0.0) {
            auto consumed = book.consume_bids(remaining);
            for (const auto& entry : consumed) {
                if (entry.price < order.limit_price) break;
                double qty = entry.remaining_quantity;
                total_qty += qty;
                total_notional += qty * entry.price;
                remaining -= qty;

                FillEvent fe;
                fe.order_id = order.order_id;
                fe.resting_order_id = entry.order_id;
                fe.fill_price = entry.price;
                fe.fill_quantity = qty;
                result.fills.push_back(std::move(fe));
            }
            best = book.best_bid();
        }
    }

    if (total_qty > 0.0) {
        result.matched = true;
        result.fill_quantity = total_qty;
        result.fill_price = total_notional / total_qty;
        result.remaining_quantity = remaining;
    }

    // Rest remainder in the book
    if (remaining > 0.0) {
        OrderEntry entry;
        entry.order_id = order.order_id;
        entry.cl_ord_id = order.cl_ord_id;
        entry.price = order.limit_price;
        entry.remaining_quantity = remaining;
        entry.original_quantity = order.quantity;

        BookSide side = (order.side == orders::Side::Buy) ? BookSide::Bid : BookSide::Ask;
        book.add_order(side, entry);

        // If nothing matched, still indicate remaining
        if (!result.matched) {
            result.remaining_quantity = remaining;
        }
    }

    return result;
}

void MatchingEngine::update_market_price(const std::string& symbol, double price) {
    market_prices_[symbol] = price;
}

double MatchingEngine::get_market_price(const std::string& symbol) const {
    auto it = market_prices_.find(symbol);
    return (it != market_prices_.end()) ? it->second : 0.0;
}

void MatchingEngine::seed_book(const std::string& symbol, double ref_price,
                                double spread_bps, int depth_levels,
                                double qty_per_level) {
    auto& book = books_[symbol];

    double half_spread = ref_price * spread_bps / 20000.0;  // half-spread in price
    double tick = half_spread;  // use half-spread as tick size for levels
    if (tick <= 0.0) tick = 0.01;

    uint64_t seq = 0;
    for (int i = 0; i < depth_levels; ++i) {
        double bid_price = ref_price - half_spread - i * tick;
        double ask_price = ref_price + half_spread + i * tick;

        std::ostringstream bid_id, ask_id;
        bid_id << "SEED-B-" << symbol << "-" << i;
        ask_id << "SEED-A-" << symbol << "-" << i;

        OrderEntry bid_entry;
        bid_entry.order_id = bid_id.str();
        bid_entry.cl_ord_id = bid_id.str();
        bid_entry.price = bid_price;
        bid_entry.remaining_quantity = qty_per_level;
        bid_entry.original_quantity = qty_per_level;
        book.add_order(BookSide::Bid, bid_entry);

        OrderEntry ask_entry;
        ask_entry.order_id = ask_id.str();
        ask_entry.cl_ord_id = ask_id.str();
        ask_entry.price = ask_price;
        ask_entry.remaining_quantity = qty_per_level;
        ask_entry.original_quantity = qty_per_level;
        book.add_order(BookSide::Ask, ask_entry);
    }
}

bool MatchingEngine::cancel_order(const std::string& symbol, const std::string& order_id) {
    auto it = books_.find(symbol);
    if (it == books_.end()) return false;
    return it->second.cancel_order(order_id);
}

const OrderBook* MatchingEngine::get_book(const std::string& symbol) const {
    auto it = books_.find(symbol);
    return (it != books_.end()) ? &it->second : nullptr;
}

}  // namespace tradecore::matching
