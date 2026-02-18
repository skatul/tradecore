#pragma once

#include "indicator.hpp"

#include <stdexcept>

namespace tradecore::indicators {

// Volume Weighted Average Price (cumulative).
class VWAP : public Indicator {
public:
    // Update with price and volume
    void update(double price, double volume) {
        ++count_;
        cum_pv_ += price * volume;
        cum_vol_ += volume;
        if (cum_vol_ > 0.0) {
            vwap_ = cum_pv_ / cum_vol_;
        }
    }

    // Single-value update assumes volume = 1
    void update(double price) override {
        update(price, 1.0);
    }

    void reset() override {
        count_ = 0;
        cum_pv_ = 0.0;
        cum_vol_ = 0.0;
        vwap_ = 0.0;
    }

    bool ready() const override { return count_ > 0; }

    double value() const override { return vwap_; }

    double cumulative_volume() const { return cum_vol_; }

private:
    double cum_pv_ = 0.0;
    double cum_vol_ = 0.0;
    double vwap_ = 0.0;
};

// On-Balance Volume.
class OBV : public Indicator {
public:
    // Update with close price and volume
    void update(double close, double volume) {
        ++count_;
        if (count_ == 1) {
            prev_close_ = close;
            obv_ = volume;
            return;
        }
        if (close > prev_close_) {
            obv_ += volume;
        } else if (close < prev_close_) {
            obv_ -= volume;
        }
        prev_close_ = close;
    }

    // Single-value update uses close price with volume = 1
    void update(double close) override {
        update(close, 1.0);
    }

    void reset() override {
        count_ = 0;
        obv_ = 0.0;
        prev_close_ = 0.0;
    }

    bool ready() const override { return count_ > 0; }

    double value() const override { return obv_; }

private:
    double obv_ = 0.0;
    double prev_close_ = 0.0;
};

}  // namespace tradecore::indicators
