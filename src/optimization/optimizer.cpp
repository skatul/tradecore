#include "optimizer.hpp"

#include <limits>

namespace tradecore::optimization {

Optimizer::Optimizer(EngineFactory factory, ObjectiveFunction objective)
    : factory_(std::move(factory)), objective_(std::move(objective)) {}

OptimizationResult Optimizer::grid_search(const ParameterSpace& space) {
    auto combinations = space.grid();
    OptimizationResult result;
    result.best_score = -std::numeric_limits<double>::infinity();

    for (const auto& params : combinations) {
        auto bt_result = factory_(params);
        double score = objective_(bt_result);

        result.all_results.emplace_back(params, score);

        if (score > result.best_score) {
            result.best_score = score;
            result.best_params = params;
            result.best_result = bt_result;
        }
    }

    return result;
}

OptimizationResult Optimizer::random_search(const ParameterSpace& space, size_t num_samples,
                                            unsigned seed) {
    OptimizationResult result;
    result.best_score = -std::numeric_limits<double>::infinity();

    std::mt19937 rng(seed);

    const auto& params = space.parameters();

    for (size_t i = 0; i < num_samples; ++i) {
        ParameterSet pset;
        for (const auto& param : params) {
            std::uniform_int_distribution<size_t> dist(0, param.values.size() - 1);
            pset[param.name] = param.values[dist(rng)];
        }

        auto bt_result = factory_(pset);
        double score = objective_(bt_result);

        result.all_results.emplace_back(pset, score);

        if (score > result.best_score) {
            result.best_score = score;
            result.best_params = pset;
            result.best_result = bt_result;
        }
    }

    return result;
}

}  // namespace tradecore::optimization
