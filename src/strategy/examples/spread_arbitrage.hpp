#pragma once

#include "../multi_instrument_strategy.hpp"

#include <cmath>
#include <string>

namespace tradecore::strategy::examples {

// Cross-venue spread arbitrage: trades when the price spread between
// two venues for the same instrument exceeds a threshold.
// Buys on the cheaper venue, sells on the expensive venue.
class SpreadArbitrage : public MultiInstrumentStrategy {
public:
    SpreadArbitrage(const std::string& symbol_a, const std::string& symbol_b,
                    double spread_threshold = 0.01, double quantity = 1.0)
        : symbol_a_(symbol_a),
          symbol_b_(symbol_b),
          spread_threshold_(spread_threshold),
          quantity_(quantity) {}

    std::string name() const override { return "SpreadArbitrage"; }

    void on_bars(const std::map<std::string, Bar>& bars) override {
        auto it_a = bars.find(symbol_a_);
        auto it_b = bars.find(symbol_b_);
        if (it_a == bars.end() || it_b == bars.end()) return;

        double price_a = it_a->second.close;
        double price_b = it_b->second.close;
        double mid = (price_a + price_b) / 2.0;
        if (mid < 1e-10) return;

        double spread = (price_a - price_b) / mid;
        double pos_a = position_for(symbol_a_);
        double pos_b = position_for(symbol_b_);
        bool flat = (std::abs(pos_a) < 1e-10 && std::abs(pos_b) < 1e-10);

        if (flat && spread > spread_threshold_) {
            // A is expensive, B is cheap: sell A, buy B
            submit_order(symbol_a_, OrderSide::Sell, quantity_, price_a);
            submit_order(symbol_b_, OrderSide::Buy, quantity_, price_b);
        } else if (flat && spread < -spread_threshold_) {
            // B is expensive, A is cheap: buy A, sell B
            submit_order(symbol_a_, OrderSide::Buy, quantity_, price_a);
            submit_order(symbol_b_, OrderSide::Sell, quantity_, price_b);
        } else if (!flat && std::abs(spread) < spread_threshold_ * 0.5) {
            // Spread converged, close positions
            if (pos_a > 0) submit_order(symbol_a_, OrderSide::Sell, std::abs(pos_a), price_a);
            if (pos_a < 0) submit_order(symbol_a_, OrderSide::Buy, std::abs(pos_a), price_a);
            if (pos_b > 0) submit_order(symbol_b_, OrderSide::Sell, std::abs(pos_b), price_b);
            if (pos_b < 0) submit_order(symbol_b_, OrderSide::Buy, std::abs(pos_b), price_b);
        }
    }

    double spread_threshold() const { return spread_threshold_; }
    void set_spread_threshold(double t) { spread_threshold_ = t; }

private:
    std::string symbol_a_;
    std::string symbol_b_;
    double spread_threshold_;
    double quantity_;
};

}  // namespace tradecore::strategy::examples
