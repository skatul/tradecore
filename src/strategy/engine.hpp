#pragma once

#include "bar.hpp"
#include "multi_instrument_strategy.hpp"
#include "strategy.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace tradecore::strategy {

struct EquityPoint {
    int64_t timestamp = 0;
    double equity = 0.0;
};

struct BacktestResult {
    double initial_capital = 0.0;
    double final_equity = 0.0;
    double total_return = 0.0;
    double max_drawdown = 0.0;
    int total_trades = 0;
    std::vector<EquityPoint> equity_curve;
};

// Backtest engine that feeds bars to a strategy and tracks equity.
class BacktestEngine {
public:
    explicit BacktestEngine(double initial_capital = 100000.0);

    void set_strategy(std::shared_ptr<Strategy> strategy);

    // Add bars for a single instrument (sequential mode)
    void add_bars(const std::vector<Bar>& bars);

    // Add bars for multiple instruments (synchronized mode)
    void add_multi_bars(const std::string& symbol, const std::vector<Bar>& bars);

    // Run the backtest
    BacktestResult run();

    double initial_capital() const { return initial_capital_; }

private:
    void process_orders(const std::vector<StrategyOrder>& orders, double current_price);
    void record_equity(int64_t timestamp);

    double initial_capital_;
    double cash_;
    std::shared_ptr<Strategy> strategy_;
    std::vector<Bar> single_bars_;
    std::map<std::string, std::vector<Bar>> multi_bars_;
    std::vector<EquityPoint> equity_curve_;
    std::map<std::string, double> positions_;
    std::map<std::string, double> last_prices_;
    int total_trades_ = 0;
};

}  // namespace tradecore::strategy
