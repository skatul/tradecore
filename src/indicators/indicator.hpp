#pragma once

#include <cstddef>

namespace tradecore::indicators {

// Abstract base class for all streaming indicators.
// Feed values one at a time via update(), check ready() before reading value().
class Indicator {
public:
    virtual ~Indicator() = default;

    // Feed a new data point
    virtual void update(double value) = 0;

    // Reset internal state
    virtual void reset() = 0;

    // Whether enough data has been consumed to produce a valid output
    virtual bool ready() const = 0;

    // Current indicator value (only valid when ready() == true)
    virtual double value() const = 0;

    // Number of data points consumed
    size_t count() const { return count_; }

protected:
    size_t count_ = 0;
};

}  // namespace tradecore::indicators
