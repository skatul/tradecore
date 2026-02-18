#pragma once

#include "indicator.hpp"

#include <deque>
#include <stdexcept>

namespace tradecore::indicators {

// Simple Moving Average using a sliding window (deque-based).
class SMA : public Indicator {
public:
    explicit SMA(size_t period) : period_(period) {
        if (period == 0) throw std::invalid_argument("SMA period must be > 0");
    }

    void update(double value) override {
        ++count_;
        window_.push_back(value);
        sum_ += value;
        if (window_.size() > period_) {
            sum_ -= window_.front();
            window_.pop_front();
        }
    }

    void reset() override {
        count_ = 0;
        sum_ = 0.0;
        window_.clear();
    }

    bool ready() const override { return window_.size() == period_; }

    double value() const override { return sum_ / static_cast<double>(window_.size()); }

    size_t period() const { return period_; }

private:
    size_t period_;
    double sum_ = 0.0;
    std::deque<double> window_;
};

// Exponential Moving Average using Wilder smoothing (alpha = 1/period).
// Can also be constructed with a custom multiplier for standard EMA (alpha = 2/(period+1)).
class EMA : public Indicator {
public:
    explicit EMA(size_t period, bool wilder = true) : period_(period) {
        if (period == 0) throw std::invalid_argument("EMA period must be > 0");
        multiplier_ = wilder ? 1.0 / static_cast<double>(period)
                             : 2.0 / (static_cast<double>(period) + 1.0);
    }

    void update(double value) override {
        ++count_;
        if (count_ == 1) {
            ema_ = value;
        } else {
            ema_ = (value - ema_) * multiplier_ + ema_;
        }
    }

    void reset() override {
        count_ = 0;
        ema_ = 0.0;
    }

    bool ready() const override { return count_ >= period_; }

    double value() const override { return ema_; }

    size_t period() const { return period_; }

private:
    size_t period_;
    double multiplier_;
    double ema_ = 0.0;
};

}  // namespace tradecore::indicators
