#include <gtest/gtest.h>

#include "optimization/bias.hpp"
#include "optimization/optimizer.hpp"
#include "optimization/parameter.hpp"
#include "optimization/walk_forward.hpp"
#include "strategy/engine.hpp"
#include "strategy/strategy.hpp"

#include <cmath>
#include <memory>

using namespace tradecore::optimization;
using namespace tradecore::strategy;

// ============================================================================
// ParameterSpace Tests
// ============================================================================

TEST(ParameterSpace, RangeGeneration) {
    auto param = Parameter::range("x", 1.0, 5.0, 1.0);
    EXPECT_EQ(param.values.size(), 5u);
    EXPECT_DOUBLE_EQ(param.values[0], 1.0);
    EXPECT_DOUBLE_EQ(param.values[4], 5.0);
}

TEST(ParameterSpace, ChoicesGeneration) {
    auto param = Parameter::choices("mode", {1.0, 2.0, 3.0});
    EXPECT_EQ(param.values.size(), 3u);
}

TEST(ParameterSpace, GridCombinations) {
    ParameterSpace space;
    space.add(Parameter::range("a", 1.0, 3.0, 1.0));  // 3 values
    space.add(Parameter::range("b", 10.0, 20.0, 10.0)); // 2 values

    EXPECT_EQ(space.total_combinations(), 6u);

    auto grid = space.grid();
    EXPECT_EQ(grid.size(), 6u);

    // Check that all combinations exist
    bool found = false;
    for (const auto& p : grid) {
        if (p.at("a") == 2.0 && p.at("b") == 20.0) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(ParameterSpace, EmptySpace) {
    ParameterSpace space;
    EXPECT_EQ(space.total_combinations(), 0u);
    EXPECT_TRUE(space.grid().empty());
}

// ============================================================================
// Optimizer Tests
// ============================================================================

// Parametric strategy for optimization testing
class ParametricStrategy : public Strategy {
public:
    double threshold;
    explicit ParametricStrategy(double t) : threshold(t) {}
    std::string name() const override { return "Parametric"; }

    void on_bar(const Bar& bar) override {
        if (position() == 0.0 && bar.close > threshold) {
            submit_order(bar.symbol, OrderSide::Buy, 10.0);
        }
    }
};

static std::vector<Bar> make_test_bars(int count = 20) {
    std::vector<Bar> bars;
    for (int i = 0; i < count; ++i) {
        Bar b;
        b.timestamp = i;
        b.close = 90.0 + (i % 10) * 3.0;  // Oscillates 90-117
        b.symbol = "TEST";
        bars.push_back(b);
    }
    return bars;
}

TEST(Optimizer, GridSearch) {
    auto bars = make_test_bars();

    auto factory = [&bars](const ParameterSet& params) -> BacktestResult {
        double threshold = params.at("threshold");
        auto strat = std::make_shared<ParametricStrategy>(threshold);
        BacktestEngine engine(100000.0);
        engine.set_strategy(strat);
        engine.add_bars(bars);
        return engine.run();
    };

    ParameterSpace space;
    space.add(Parameter::range("threshold", 95.0, 115.0, 5.0));  // 5 values

    Optimizer opt(factory, objectives::total_return);
    auto result = opt.grid_search(space);

    EXPECT_EQ(result.all_results.size(), 5u);
    EXPECT_FALSE(result.best_params.empty());
}

TEST(Optimizer, RandomSearch) {
    auto bars = make_test_bars();

    auto factory = [&bars](const ParameterSet& params) -> BacktestResult {
        double threshold = params.at("threshold");
        auto strat = std::make_shared<ParametricStrategy>(threshold);
        BacktestEngine engine(100000.0);
        engine.set_strategy(strat);
        engine.add_bars(bars);
        return engine.run();
    };

    ParameterSpace space;
    space.add(Parameter::range("threshold", 90.0, 120.0, 1.0));

    Optimizer opt(factory, objectives::total_return);
    auto result = opt.random_search(space, 10, 42);

    EXPECT_EQ(result.all_results.size(), 10u);
}

TEST(Optimizer, SharpeProxy) {
    BacktestResult r;
    r.total_return = 0.2;
    r.max_drawdown = 0.1;
    EXPECT_DOUBLE_EQ(objectives::sharpe_proxy(r), 2.0);

    r.max_drawdown = 0.0;
    EXPECT_DOUBLE_EQ(objectives::sharpe_proxy(r), 0.2 * 100.0);
}

// ============================================================================
// Walk-Forward Tests
// ============================================================================

TEST(WalkForward, BasicRun) {
    auto bars = make_test_bars(100);

    auto factory = [&bars](const ParameterSet& params,
                           int64_t start_time, int64_t end_time) -> BacktestResult {
        double threshold = params.at("threshold");
        auto strat = std::make_shared<ParametricStrategy>(threshold);
        BacktestEngine engine(100000.0);
        engine.set_strategy(strat);

        std::vector<Bar> filtered;
        for (const auto& b : bars) {
            if (b.timestamp >= start_time && b.timestamp < end_time) {
                filtered.push_back(b);
            }
        }
        engine.add_bars(filtered);
        return engine.run();
    };

    ParameterSpace space;
    space.add(Parameter::range("threshold", 95.0, 110.0, 5.0));

    WalkForwardOptimizer wfo(factory, objectives::total_return);
    auto result = wfo.run(space, 0, 100, 3, 0.7);

    EXPECT_EQ(result.folds.size(), 3u);
    for (const auto& fold : result.folds) {
        EXPECT_GT(fold.window.train_end, fold.window.train_start);
        EXPECT_GE(fold.window.test_start, fold.window.train_end);
    }
}

// ============================================================================
// Bias Detection Tests
// ============================================================================

TEST(BiasDetector, OverfittingDetection) {
    WalkForwardResult wf;

    WalkForwardFoldResult fold1;
    fold1.in_sample_score = 0.5;
    fold1.out_of_sample_score = 0.1;
    fold1.best_params = {{"x", 10.0}};

    WalkForwardFoldResult fold2;
    fold2.in_sample_score = 0.6;
    fold2.out_of_sample_score = 0.15;
    fold2.best_params = {{"x", 10.0}};

    wf.folds = {fold1, fold2};
    wf.avg_is_score = 0.55;
    wf.avg_oos_score = 0.125;

    BiasDetector detector;
    auto report = detector.analyze(wf, 0.5);

    // (0.55 - 0.125) / 0.55 = 0.772 > 0.5, so overfitting
    EXPECT_TRUE(report.overfitting_detected);
    EXPECT_GT(report.is_oos_degradation, 0.5);
    EXPECT_FALSE(report.warnings.empty());
}

TEST(BiasDetector, NoOverfitting) {
    WalkForwardResult wf;

    WalkForwardFoldResult fold1;
    fold1.in_sample_score = 0.3;
    fold1.out_of_sample_score = 0.28;
    fold1.best_params = {{"x", 10.0}};

    WalkForwardFoldResult fold2;
    fold2.in_sample_score = 0.35;
    fold2.out_of_sample_score = 0.30;
    fold2.best_params = {{"x", 10.0}};

    wf.folds = {fold1, fold2};
    wf.avg_is_score = 0.325;
    wf.avg_oos_score = 0.29;

    BiasDetector detector;
    auto report = detector.analyze(wf, 0.5);

    EXPECT_FALSE(report.overfitting_detected);
}

TEST(BiasDetector, ParameterInstability) {
    WalkForwardResult wf;

    WalkForwardFoldResult fold1;
    fold1.in_sample_score = 0.3;
    fold1.out_of_sample_score = 0.25;
    fold1.best_params = {{"period", 5.0}};

    WalkForwardFoldResult fold2;
    fold2.in_sample_score = 0.35;
    fold2.out_of_sample_score = 0.30;
    fold2.best_params = {{"period", 50.0}};  // Very different from fold1

    wf.folds = {fold1, fold2};
    wf.avg_is_score = 0.325;
    wf.avg_oos_score = 0.275;

    BiasDetector detector;
    auto report = detector.analyze(wf, 0.5);

    EXPECT_TRUE(report.parameter_instability);
}

TEST(BiasDetector, SurvivorshipBias) {
    BiasDetector detector;
    BiasReport report;

    detector.check_survivorship(report, 100, 15);
    EXPECT_TRUE(report.survivorship_bias_warning);
    EXPECT_FALSE(report.warnings.empty());
}

TEST(BiasDetector, NoSurvivorshipBias) {
    BiasDetector detector;
    BiasReport report;

    detector.check_survivorship(report, 100, 5);
    EXPECT_FALSE(report.survivorship_bias_warning);
}

TEST(BiasDetector, LookAheadBias) {
    BiasDetector detector;
    BiasReport report;

    detector.check_look_ahead(report, {"compute_future_return", "next_bar_open"});
    EXPECT_TRUE(report.look_ahead_warning);
    EXPECT_EQ(report.warnings.size(), 2u);
}

TEST(BiasDetector, NoLookAheadBias) {
    BiasDetector detector;
    BiasReport report;

    detector.check_look_ahead(report, {"moving_average", "rsi_signal"});
    EXPECT_FALSE(report.look_ahead_warning);
    EXPECT_TRUE(report.warnings.empty());
}
