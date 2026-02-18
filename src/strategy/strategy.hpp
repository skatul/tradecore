#pragma once

#include "bar.hpp"

#include <string>
#include <vector>

namespace tradecore::strategy {

enum class OrderSide { Buy, Sell };

struct StrategyOrder {
    std::string symbol;
    OrderSide side;
    double quantity = 0.0;
    double price = 0.0;  // 0 = market order
};

// Abstract base class for single-instrument strategies.
class Strategy {
public:
    virtual ~Strategy() = default;

    virtual std::string name() const = 0;

    // Called once before the first bar
    virtual void on_init() {}

    // Called on each new bar
    virtual void on_bar(const Bar& bar) = 0;

    // Retrieve and clear pending orders
    std::vector<StrategyOrder> take_orders() {
        auto orders = std::move(pending_orders_);
        pending_orders_.clear();
        return orders;
    }

    // Current position for the strategy (tracked externally by the engine)
    double position() const { return position_; }
    void set_position(double pos) { position_ = pos; }

protected:
    void submit_order(const std::string& symbol, OrderSide side, double qty, double price = 0.0) {
        pending_orders_.push_back({symbol, side, qty, price});
    }

private:
    std::vector<StrategyOrder> pending_orders_;
    double position_ = 0.0;
};

}  // namespace tradecore::strategy
