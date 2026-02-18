#include "engine.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace tradecore::strategy {

BacktestEngine::BacktestEngine(double initial_capital)
    : initial_capital_(initial_capital), cash_(initial_capital) {}

void BacktestEngine::set_strategy(std::shared_ptr<Strategy> strategy) {
    strategy_ = std::move(strategy);
}

void BacktestEngine::add_bars(const std::vector<Bar>& bars) {
    single_bars_.insert(single_bars_.end(), bars.begin(), bars.end());
}

void BacktestEngine::add_multi_bars(const std::string& symbol, const std::vector<Bar>& bars) {
    multi_bars_[symbol] = bars;
}

BacktestResult BacktestEngine::run() {
    if (!strategy_) return {};

    cash_ = initial_capital_;
    positions_.clear();
    last_prices_.clear();
    equity_curve_.clear();
    total_trades_ = 0;

    strategy_->on_init();

    auto* multi_strat = dynamic_cast<MultiInstrumentStrategy*>(strategy_.get());

    if (!multi_bars_.empty() && multi_strat) {
        // Synchronized multi-instrument mode
        // Collect all unique timestamps across instruments
        std::set<int64_t> timestamps;
        for (const auto& [sym, bars] : multi_bars_) {
            for (const auto& bar : bars) {
                timestamps.insert(bar.timestamp);
            }
        }

        // Build index maps for quick lookup
        std::map<std::string, std::map<int64_t, size_t>> index_maps;
        for (const auto& [sym, bars] : multi_bars_) {
            for (size_t i = 0; i < bars.size(); ++i) {
                index_maps[sym][bars[i].timestamp] = i;
            }
        }

        for (int64_t ts : timestamps) {
            std::map<std::string, Bar> bar_map;
            for (const auto& [sym, bars] : multi_bars_) {
                auto it = index_maps[sym].find(ts);
                if (it != index_maps[sym].end()) {
                    bar_map[sym] = bars[it->second];
                    last_prices_[sym] = bars[it->second].close;
                }
            }

            // Update positions on the strategy
            for (const auto& [sym, pos] : positions_) {
                multi_strat->set_position_for(sym, pos);
            }

            multi_strat->on_bars(bar_map);

            auto orders = strategy_->take_orders();
            for (const auto& order : orders) {
                auto price_it = last_prices_.find(order.symbol);
                if (price_it != last_prices_.end()) {
                    process_orders({order}, price_it->second);
                }
            }

            record_equity(ts);
        }
    } else if (!single_bars_.empty()) {
        // Sequential single-instrument mode
        for (const auto& bar : single_bars_) {
            last_prices_[bar.symbol] = bar.close;
            strategy_->set_position(positions_[bar.symbol]);
            strategy_->on_bar(bar);

            auto orders = strategy_->take_orders();
            process_orders(orders, bar.close);

            record_equity(bar.timestamp);
        }
    }

    // Compute result
    BacktestResult result;
    result.initial_capital = initial_capital_;

    double final_eq = cash_;
    for (const auto& [sym, pos] : positions_) {
        auto price_it = last_prices_.find(sym);
        if (price_it != last_prices_.end()) {
            final_eq += pos * price_it->second;
        }
    }
    result.final_equity = final_eq;
    result.total_return = (final_eq - initial_capital_) / initial_capital_;
    result.total_trades = total_trades_;
    result.equity_curve = equity_curve_;

    // Compute max drawdown
    double peak = 0.0;
    double max_dd = 0.0;
    for (const auto& pt : equity_curve_) {
        if (pt.equity > peak) peak = pt.equity;
        double dd = (peak > 0.0) ? (peak - pt.equity) / peak : 0.0;
        if (dd > max_dd) max_dd = dd;
    }
    result.max_drawdown = max_dd;

    return result;
}

void BacktestEngine::process_orders(const std::vector<StrategyOrder>& orders, double current_price) {
    for (const auto& order : orders) {
        double fill_price = (order.price > 0.0) ? order.price : current_price;
        double signed_qty = (order.side == OrderSide::Buy) ? order.quantity : -order.quantity;

        cash_ -= signed_qty * fill_price;
        positions_[order.symbol] += signed_qty;
        ++total_trades_;
    }
}

void BacktestEngine::record_equity(int64_t timestamp) {
    double equity = cash_;
    for (const auto& [sym, pos] : positions_) {
        auto price_it = last_prices_.find(sym);
        if (price_it != last_prices_.end()) {
            equity += pos * price_it->second;
        }
    }
    equity_curve_.push_back({timestamp, equity});
}

}  // namespace tradecore::strategy
