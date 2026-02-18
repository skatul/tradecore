#pragma once

#include "indicator.hpp"

#include <algorithm>
#include <deque>
#include <stdexcept>

namespace tradecore::indicators {

// Relative Strength Index with Wilder smoothing.
class RSI : public Indicator {
public:
    explicit RSI(size_t period = 14) : period_(period) {
        if (period == 0) throw std::invalid_argument("RSI period must be > 0");
    }

    void update(double price) override {
        ++count_;
        if (count_ == 1) {
            prev_ = price;
            return;
        }

        double change = price - prev_;
        double gain = std::max(change, 0.0);
        double loss = std::max(-change, 0.0);
        prev_ = price;

        if (count_ <= period_ + 1) {
            // Accumulate initial sums
            gain_sum_ += gain;
            loss_sum_ += loss;
            if (count_ == period_ + 1) {
                avg_gain_ = gain_sum_ / static_cast<double>(period_);
                avg_loss_ = loss_sum_ / static_cast<double>(period_);
            }
        } else {
            // Wilder smoothing
            avg_gain_ = (avg_gain_ * (period_ - 1) + gain) / static_cast<double>(period_);
            avg_loss_ = (avg_loss_ * (period_ - 1) + loss) / static_cast<double>(period_);
        }
    }

    void reset() override {
        count_ = 0;
        prev_ = 0.0;
        gain_sum_ = 0.0;
        loss_sum_ = 0.0;
        avg_gain_ = 0.0;
        avg_loss_ = 0.0;
    }

    bool ready() const override { return count_ > period_; }

    double value() const override {
        if (avg_loss_ == 0.0) return 100.0;
        double rs = avg_gain_ / avg_loss_;
        return 100.0 - (100.0 / (1.0 + rs));
    }

    size_t period() const { return period_; }

private:
    size_t period_;
    double prev_ = 0.0;
    double gain_sum_ = 0.0;
    double loss_sum_ = 0.0;
    double avg_gain_ = 0.0;
    double avg_loss_ = 0.0;
};

// MACD (Moving Average Convergence Divergence).
// Uses standard EMA (alpha = 2/(period+1)) for fast, slow, and signal lines.
class MACD : public Indicator {
public:
    MACD(size_t fast_period = 12, size_t slow_period = 26, size_t signal_period = 9)
        : fast_period_(fast_period),
          slow_period_(slow_period),
          signal_period_(signal_period) {
        if (fast_period == 0 || slow_period == 0 || signal_period == 0)
            throw std::invalid_argument("MACD periods must be > 0");
        fast_mult_ = 2.0 / (static_cast<double>(fast_period) + 1.0);
        slow_mult_ = 2.0 / (static_cast<double>(slow_period) + 1.0);
        signal_mult_ = 2.0 / (static_cast<double>(signal_period) + 1.0);
    }

    void update(double price) override {
        ++count_;
        if (count_ == 1) {
            fast_ema_ = price;
            slow_ema_ = price;
            macd_line_ = 0.0;
            signal_line_ = 0.0;
        } else {
            fast_ema_ = (price - fast_ema_) * fast_mult_ + fast_ema_;
            slow_ema_ = (price - slow_ema_) * slow_mult_ + slow_ema_;
            macd_line_ = fast_ema_ - slow_ema_;
            if (count_ == slow_period_) {
                signal_line_ = macd_line_;
            } else if (count_ > slow_period_) {
                signal_line_ = (macd_line_ - signal_line_) * signal_mult_ + signal_line_;
            }
        }
    }

    void reset() override {
        count_ = 0;
        fast_ema_ = 0.0;
        slow_ema_ = 0.0;
        macd_line_ = 0.0;
        signal_line_ = 0.0;
    }

    bool ready() const override { return count_ >= slow_period_ + signal_period_; }

    // Returns the MACD line (fast EMA - slow EMA)
    double value() const override { return macd_line_; }

    double signal() const { return signal_line_; }
    double histogram() const { return macd_line_ - signal_line_; }

private:
    size_t fast_period_, slow_period_, signal_period_;
    double fast_mult_, slow_mult_, signal_mult_;
    double fast_ema_ = 0.0;
    double slow_ema_ = 0.0;
    double macd_line_ = 0.0;
    double signal_line_ = 0.0;
};

// Stochastic Oscillator (%K and %D).
class Stochastic : public Indicator {
public:
    Stochastic(size_t k_period = 14, size_t d_period = 3)
        : k_period_(k_period), d_period_(d_period) {
        if (k_period == 0 || d_period == 0)
            throw std::invalid_argument("Stochastic periods must be > 0");
    }

    void update(double close) override {
        ++count_;
        highs_.push_back(close);
        lows_.push_back(close);
        if (highs_.size() > k_period_) highs_.pop_front();
        if (lows_.size() > k_period_) lows_.pop_front();

        if (highs_.size() == k_period_) {
            double highest = *std::max_element(highs_.begin(), highs_.end());
            double lowest = *std::min_element(lows_.begin(), lows_.end());
            double range = highest - lowest;
            k_value_ = (range > 0.0) ? ((close - lowest) / range * 100.0) : 50.0;

            k_history_.push_back(k_value_);
            if (k_history_.size() > d_period_) k_history_.pop_front();

            if (k_history_.size() == d_period_) {
                double sum = 0.0;
                for (double v : k_history_) sum += v;
                d_value_ = sum / static_cast<double>(d_period_);
            }
        }
    }

    // Update with separate high, low, close values for more accurate calculation
    void update(double high, double low, double close) {
        ++count_;
        highs_.push_back(high);
        lows_.push_back(low);
        if (highs_.size() > k_period_) highs_.pop_front();
        if (lows_.size() > k_period_) lows_.pop_front();

        if (highs_.size() == k_period_) {
            double highest = *std::max_element(highs_.begin(), highs_.end());
            double lowest = *std::min_element(lows_.begin(), lows_.end());
            double range = highest - lowest;
            k_value_ = (range > 0.0) ? ((close - lowest) / range * 100.0) : 50.0;

            k_history_.push_back(k_value_);
            if (k_history_.size() > d_period_) k_history_.pop_front();

            if (k_history_.size() == d_period_) {
                double sum = 0.0;
                for (double v : k_history_) sum += v;
                d_value_ = sum / static_cast<double>(d_period_);
            }
        }
    }

    void reset() override {
        count_ = 0;
        k_value_ = 0.0;
        d_value_ = 0.0;
        highs_.clear();
        lows_.clear();
        k_history_.clear();
    }

    bool ready() const override { return k_history_.size() == d_period_; }

    // Returns %K value
    double value() const override { return k_value_; }

    double k() const { return k_value_; }
    double d() const { return d_value_; }

private:
    size_t k_period_, d_period_;
    double k_value_ = 0.0;
    double d_value_ = 0.0;
    std::deque<double> highs_;
    std::deque<double> lows_;
    std::deque<double> k_history_;
};

}  // namespace tradecore::indicators
