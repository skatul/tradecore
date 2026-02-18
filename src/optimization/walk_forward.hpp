#pragma once

#include "optimizer.hpp"
#include "parameter.hpp"
#include "../strategy/bar.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace tradecore::optimization {

struct WalkForwardWindow {
    int64_t train_start = 0;
    int64_t train_end = 0;
    int64_t test_start = 0;
    int64_t test_end = 0;
};

struct WalkForwardFoldResult {
    WalkForwardWindow window;
    ParameterSet best_params;
    double in_sample_score = 0.0;
    double out_of_sample_score = 0.0;
};

struct WalkForwardResult {
    std::vector<WalkForwardFoldResult> folds;
    double avg_is_score = 0.0;
    double avg_oos_score = 0.0;
    double efficiency_ratio = 0.0;  // avg_oos / avg_is
};

// Factory that takes parameters AND a time range filter to create the engine + run.
using TimeFilteredFactory = std::function<strategy::BacktestResult(
    const ParameterSet&, int64_t start_time, int64_t end_time)>;

class WalkForwardOptimizer {
public:
    WalkForwardOptimizer(TimeFilteredFactory factory, ObjectiveFunction objective);

    // Split the time range into train/test windows and optimize on each.
    // train_pct: fraction of each window used for training (e.g., 0.7)
    WalkForwardResult run(const ParameterSpace& space,
                          int64_t start_time, int64_t end_time,
                          int num_folds, double train_pct = 0.7);

private:
    TimeFilteredFactory factory_;
    ObjectiveFunction objective_;
};

}  // namespace tradecore::optimization
