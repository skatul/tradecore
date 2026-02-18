#pragma once

#include "indicator.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <stdexcept>

namespace tradecore::indicators {

// Bollinger Bands: SMA center with standard deviation bands.
class BollingerBands : public Indicator {
public:
    BollingerBands(size_t period = 20, double num_std = 2.0)
        : period_(period), num_std_(num_std) {
        if (period == 0) throw std::invalid_argument("BollingerBands period must be > 0");
    }

    void update(double price) override {
        ++count_;
        window_.push_back(price);
        sum_ += price;
        sum_sq_ += price * price;
        if (window_.size() > period_) {
            double old = window_.front();
            sum_ -= old;
            sum_sq_ -= old * old;
            window_.pop_front();
        }

        if (window_.size() == period_) {
            double n = static_cast<double>(period_);
            middle_ = sum_ / n;
            double variance = (sum_sq_ / n) - (middle_ * middle_);
            std_dev_ = std::sqrt(std::max(variance, 0.0));
            upper_ = middle_ + num_std_ * std_dev_;
            lower_ = middle_ - num_std_ * std_dev_;
        }
    }

    void reset() override {
        count_ = 0;
        sum_ = 0.0;
        sum_sq_ = 0.0;
        middle_ = 0.0;
        upper_ = 0.0;
        lower_ = 0.0;
        std_dev_ = 0.0;
        window_.clear();
    }

    bool ready() const override { return window_.size() == period_; }

    // Returns the middle band (SMA)
    double value() const override { return middle_; }

    double upper() const { return upper_; }
    double lower() const { return lower_; }
    double bandwidth() const { return (middle_ > 0.0) ? (upper_ - lower_) / middle_ : 0.0; }

private:
    size_t period_;
    double num_std_;
    double sum_ = 0.0;
    double sum_sq_ = 0.0;
    double middle_ = 0.0;
    double upper_ = 0.0;
    double lower_ = 0.0;
    double std_dev_ = 0.0;
    std::deque<double> window_;
};

// Average True Range with Wilder smoothing.
class ATR : public Indicator {
public:
    explicit ATR(size_t period = 14) : period_(period) {
        if (period == 0) throw std::invalid_argument("ATR period must be > 0");
    }

    // Update with high, low, close values
    void update(double high, double low, double close) {
        ++count_;
        double tr;
        if (count_ == 1) {
            tr = high - low;
        } else {
            double hl = high - low;
            double hc = std::abs(high - prev_close_);
            double lc = std::abs(low - prev_close_);
            tr = std::max({hl, hc, lc});
        }
        prev_close_ = close;

        if (count_ <= period_) {
            tr_sum_ += tr;
            if (count_ == period_) {
                atr_ = tr_sum_ / static_cast<double>(period_);
            }
        } else {
            // Wilder smoothing
            atr_ = (atr_ * (period_ - 1) + tr) / static_cast<double>(period_);
        }
    }

    // Single-value update uses value as both high, low, and close
    void update(double value) override {
        update(value, value, value);
    }

    void reset() override {
        count_ = 0;
        atr_ = 0.0;
        tr_sum_ = 0.0;
        prev_close_ = 0.0;
    }

    bool ready() const override { return count_ >= period_; }

    double value() const override { return atr_; }

    size_t period() const { return period_; }

private:
    size_t period_;
    double atr_ = 0.0;
    double tr_sum_ = 0.0;
    double prev_close_ = 0.0;
};

}  // namespace tradecore::indicators
