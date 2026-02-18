#pragma once

#include "strategy.hpp"

#include <map>
#include <string>

namespace tradecore::strategy {

// Base class for strategies that trade multiple instruments simultaneously.
// Receives synchronized bars across all instruments via on_bars().
class MultiInstrumentStrategy : public Strategy {
public:
    // Called with a map of symbol -> Bar for each timestamp
    virtual void on_bars(const std::map<std::string, Bar>& bars) = 0;

    // Single-bar dispatch routes to on_bars with a single entry
    void on_bar(const Bar& bar) override {
        std::map<std::string, Bar> bars;
        bars[bar.symbol] = bar;
        on_bars(bars);
    }

    // Position per instrument
    double position_for(const std::string& symbol) const {
        auto it = positions_.find(symbol);
        return (it != positions_.end()) ? it->second : 0.0;
    }

    void set_position_for(const std::string& symbol, double pos) {
        positions_[symbol] = pos;
    }

    const std::map<std::string, double>& positions() const { return positions_; }

private:
    std::map<std::string, double> positions_;
};

}  // namespace tradecore::strategy
