#include "walk_forward.hpp"

#include <limits>

namespace tradecore::optimization {

WalkForwardOptimizer::WalkForwardOptimizer(TimeFilteredFactory factory,
                                           ObjectiveFunction objective)
    : factory_(std::move(factory)), objective_(std::move(objective)) {}

WalkForwardResult WalkForwardOptimizer::run(const ParameterSpace& space,
                                            int64_t start_time, int64_t end_time,
                                            int num_folds, double train_pct) {
    WalkForwardResult result;
    if (num_folds <= 0) return result;

    int64_t total_range = end_time - start_time;
    int64_t fold_size = total_range / num_folds;

    double sum_is = 0.0;
    double sum_oos = 0.0;

    for (int i = 0; i < num_folds; ++i) {
        WalkForwardWindow window;
        int64_t fold_start = start_time + i * fold_size;
        int64_t fold_end = (i == num_folds - 1) ? end_time : fold_start + fold_size;
        int64_t train_length = static_cast<int64_t>((fold_end - fold_start) * train_pct);

        window.train_start = fold_start;
        window.train_end = fold_start + train_length;
        window.test_start = window.train_end;
        window.test_end = fold_end;

        // Optimize on training period
        auto train_factory = [this, &window](const ParameterSet& params) {
            return factory_(params, window.train_start, window.train_end);
        };

        Optimizer opt(train_factory, objective_);
        auto opt_result = opt.grid_search(space);

        // Evaluate best params on test period
        auto test_result = factory_(opt_result.best_params, window.test_start, window.test_end);
        double oos_score = objective_(test_result);

        WalkForwardFoldResult fold;
        fold.window = window;
        fold.best_params = opt_result.best_params;
        fold.in_sample_score = opt_result.best_score;
        fold.out_of_sample_score = oos_score;

        sum_is += fold.in_sample_score;
        sum_oos += fold.out_of_sample_score;

        result.folds.push_back(fold);
    }

    result.avg_is_score = sum_is / num_folds;
    result.avg_oos_score = sum_oos / num_folds;
    result.efficiency_ratio = (std::abs(result.avg_is_score) > 1e-10)
                                  ? result.avg_oos_score / result.avg_is_score
                                  : 0.0;

    return result;
}

}  // namespace tradecore::optimization
