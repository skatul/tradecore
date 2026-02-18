#pragma once

#include "parameter.hpp"
#include "../strategy/engine.hpp"

#include <algorithm>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace tradecore::optimization {

// Factory function: given parameters, creates and runs a backtest, returns the result.
using EngineFactory = std::function<strategy::BacktestResult(const ParameterSet&)>;

// Objective function: extracts a scalar score from a backtest result (higher = better).
using ObjectiveFunction = std::function<double(const strategy::BacktestResult&)>;

struct OptimizationResult {
    ParameterSet best_params;
    double best_score = 0.0;
    strategy::BacktestResult best_result;
    std::vector<std::pair<ParameterSet, double>> all_results;
};

enum class SearchMethod { Grid, Random };

class Optimizer {
public:
    Optimizer(EngineFactory factory, ObjectiveFunction objective);

    // Run grid search over entire parameter space
    OptimizationResult grid_search(const ParameterSpace& space);

    // Run random search with a fixed number of samples
    OptimizationResult random_search(const ParameterSpace& space, size_t num_samples,
                                     unsigned seed = 42);

private:
    EngineFactory factory_;
    ObjectiveFunction objective_;
};

// Common objective functions
namespace objectives {

inline double total_return(const strategy::BacktestResult& r) {
    return r.total_return;
}

inline double sharpe_proxy(const strategy::BacktestResult& r) {
    // Simple proxy: return / max_drawdown (avoid div by zero)
    if (r.max_drawdown < 1e-10) return r.total_return * 100.0;
    return r.total_return / r.max_drawdown;
}

}  // namespace objectives

}  // namespace tradecore::optimization
