#pragma once

#include "../multi_instrument_strategy.hpp"

#include <cmath>
#include <deque>
#include <string>

namespace tradecore::strategy::examples {

// Statistical pair trading strategy based on z-score of the spread
// between two instruments. Enters when z-score exceeds entry threshold,
// exits when it reverts below exit threshold.
class PairTrading : public MultiInstrumentStrategy {
public:
    PairTrading(const std::string& symbol_a, const std::string& symbol_b,
                size_t lookback = 20, double entry_z = 2.0,
                double exit_z = 0.5, double quantity = 1.0)
        : symbol_a_(symbol_a),
          symbol_b_(symbol_b),
          lookback_(lookback),
          entry_z_(entry_z),
          exit_z_(exit_z),
          quantity_(quantity) {}

    std::string name() const override { return "PairTrading"; }

    void on_bars(const std::map<std::string, Bar>& bars) override {
        auto it_a = bars.find(symbol_a_);
        auto it_b = bars.find(symbol_b_);
        if (it_a == bars.end() || it_b == bars.end()) return;

        double price_a = it_a->second.close;
        double price_b = it_b->second.close;
        double spread = price_a - price_b;

        spread_history_.push_back(spread);
        if (spread_history_.size() > lookback_) {
            spread_history_.pop_front();
        }

        if (spread_history_.size() < lookback_) return;

        // Calculate z-score of current spread
        double mean = 0.0;
        for (double s : spread_history_) mean += s;
        mean /= static_cast<double>(lookback_);

        double var = 0.0;
        for (double s : spread_history_) var += (s - mean) * (s - mean);
        var /= static_cast<double>(lookback_);
        double std_dev = std::sqrt(var);

        if (std_dev < 1e-10) return;

        double z_score = (spread - mean) / std_dev;

        double pos_a = position_for(symbol_a_);
        double pos_b = position_for(symbol_b_);
        bool flat = (std::abs(pos_a) < 1e-10 && std::abs(pos_b) < 1e-10);

        if (flat && z_score > entry_z_) {
            // Spread is wide: sell A, buy B (expect mean reversion)
            submit_order(symbol_a_, OrderSide::Sell, quantity_, price_a);
            submit_order(symbol_b_, OrderSide::Buy, quantity_, price_b);
        } else if (flat && z_score < -entry_z_) {
            // Spread is narrow: buy A, sell B
            submit_order(symbol_a_, OrderSide::Buy, quantity_, price_a);
            submit_order(symbol_b_, OrderSide::Sell, quantity_, price_b);
        } else if (!flat && std::abs(z_score) < exit_z_) {
            // Mean reversion, close positions
            if (pos_a > 0) submit_order(symbol_a_, OrderSide::Sell, std::abs(pos_a), price_a);
            if (pos_a < 0) submit_order(symbol_a_, OrderSide::Buy, std::abs(pos_a), price_a);
            if (pos_b > 0) submit_order(symbol_b_, OrderSide::Sell, std::abs(pos_b), price_b);
            if (pos_b < 0) submit_order(symbol_b_, OrderSide::Buy, std::abs(pos_b), price_b);
        }
    }

    double entry_z() const { return entry_z_; }
    double exit_z() const { return exit_z_; }
    void set_entry_z(double z) { entry_z_ = z; }
    void set_exit_z(double z) { exit_z_ = z; }

private:
    std::string symbol_a_;
    std::string symbol_b_;
    size_t lookback_;
    double entry_z_;
    double exit_z_;
    double quantity_;
    std::deque<double> spread_history_;
};

}  // namespace tradecore::strategy::examples
