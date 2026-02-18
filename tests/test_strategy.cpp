#include <gtest/gtest.h>

#include "strategy/bar.hpp"
#include "strategy/engine.hpp"
#include "strategy/multi_instrument_strategy.hpp"
#include "strategy/strategy.hpp"
#include "strategy/examples/pair_trading.hpp"
#include "strategy/examples/spread_arbitrage.hpp"

#include <cmath>
#include <memory>
#include <vector>

using namespace tradecore::strategy;
using namespace tradecore::strategy::examples;

// ============================================================================
// Simple test strategy: buys on first bar, holds
// ============================================================================
class BuyAndHold : public Strategy {
public:
    std::string name() const override { return "BuyAndHold"; }

    void on_bar(const Bar& bar) override {
        if (position() == 0.0) {
            submit_order(bar.symbol, OrderSide::Buy, 10.0);
        }
    }
};

// ============================================================================
// BacktestEngine Tests
// ============================================================================

TEST(BacktestEngine, BuyAndHoldProfit) {
    auto strategy = std::make_shared<BuyAndHold>();
    BacktestEngine engine(100000.0);
    engine.set_strategy(strategy);

    std::vector<Bar> bars;
    for (int i = 0; i < 10; ++i) {
        Bar b;
        b.timestamp = i;
        b.open = 100.0 + i;
        b.high = 101.0 + i;
        b.low = 99.0 + i;
        b.close = 100.0 + i;
        b.volume = 1000.0;
        b.symbol = "AAPL";
        bars.push_back(b);
    }

    engine.add_bars(bars);
    auto result = engine.run();

    EXPECT_DOUBLE_EQ(result.initial_capital, 100000.0);
    EXPECT_GT(result.final_equity, result.initial_capital);
    EXPECT_GT(result.total_return, 0.0);
    EXPECT_EQ(result.total_trades, 1);
    EXPECT_EQ(result.equity_curve.size(), 10u);
}

TEST(BacktestEngine, EmptyBars) {
    auto strategy = std::make_shared<BuyAndHold>();
    BacktestEngine engine(100000.0);
    engine.set_strategy(strategy);

    auto result = engine.run();
    EXPECT_DOUBLE_EQ(result.final_equity, 100000.0);
    EXPECT_EQ(result.total_trades, 0);
}

TEST(BacktestEngine, NoStrategy) {
    BacktestEngine engine(100000.0);
    auto result = engine.run();
    EXPECT_DOUBLE_EQ(result.final_equity, 0.0);
}

TEST(BacktestEngine, MaxDrawdown) {
    // Strategy that buys then price drops
    class BuyThenDrop : public Strategy {
    public:
        std::string name() const override { return "BuyThenDrop"; }
        void on_bar(const Bar& bar) override {
            if (position() == 0.0) {
                submit_order(bar.symbol, OrderSide::Buy, 100.0);
            }
        }
    };

    auto strategy = std::make_shared<BuyThenDrop>();
    BacktestEngine engine(100000.0);
    engine.set_strategy(strategy);

    std::vector<Bar> bars;
    // Price goes up then down
    double prices[] = {100, 110, 120, 130, 120, 110, 100, 90, 95, 100};
    for (int i = 0; i < 10; ++i) {
        Bar b;
        b.timestamp = i;
        b.close = prices[i];
        b.symbol = "TEST";
        bars.push_back(b);
    }

    engine.add_bars(bars);
    auto result = engine.run();
    EXPECT_GT(result.max_drawdown, 0.0);
}

// ============================================================================
// Multi-Instrument Tests
// ============================================================================

TEST(MultiInstrumentStrategy, SynchronizedBars) {
    class SimpleMulti : public MultiInstrumentStrategy {
    public:
        std::string name() const override { return "SimpleMulti"; }
        int bar_count = 0;

        void on_bars(const std::map<std::string, Bar>& bars) override {
            bar_count += static_cast<int>(bars.size());
        }
    };

    auto strategy = std::make_shared<SimpleMulti>();
    BacktestEngine engine(100000.0);
    engine.set_strategy(strategy);

    std::vector<Bar> bars_a, bars_b;
    for (int i = 0; i < 5; ++i) {
        Bar a;
        a.timestamp = i;
        a.close = 100.0 + i;
        a.symbol = "A";
        bars_a.push_back(a);

        Bar b;
        b.timestamp = i;
        b.close = 50.0 + i;
        b.symbol = "B";
        bars_b.push_back(b);
    }

    engine.add_multi_bars("A", bars_a);
    engine.add_multi_bars("B", bars_b);
    engine.run();

    EXPECT_EQ(strategy->bar_count, 10);  // 5 timestamps * 2 instruments each
}

// ============================================================================
// Spread Arbitrage Tests
// ============================================================================

TEST(SpreadArbitrage, DetectsSpread) {
    auto strategy = std::make_shared<SpreadArbitrage>("VENUE_A", "VENUE_B", 0.02, 10.0);
    BacktestEngine engine(100000.0);
    engine.set_strategy(strategy);

    std::vector<Bar> bars_a, bars_b;

    // Create a spread that exceeds threshold, then converges
    double prices_a[] = {100.0, 103.0, 105.0, 102.0, 100.5};
    double prices_b[] = {100.0, 100.0, 100.0, 101.0, 100.0};

    for (int i = 0; i < 5; ++i) {
        Bar a;
        a.timestamp = i;
        a.close = prices_a[i];
        a.symbol = "VENUE_A";
        bars_a.push_back(a);

        Bar b;
        b.timestamp = i;
        b.close = prices_b[i];
        b.symbol = "VENUE_B";
        bars_b.push_back(b);
    }

    engine.add_multi_bars("VENUE_A", bars_a);
    engine.add_multi_bars("VENUE_B", bars_b);
    auto result = engine.run();

    EXPECT_GT(result.total_trades, 0);
}

TEST(SpreadArbitrage, NoTradeOnSmallSpread) {
    auto strategy = std::make_shared<SpreadArbitrage>("A", "B", 0.10, 10.0);
    BacktestEngine engine(100000.0);
    engine.set_strategy(strategy);

    std::vector<Bar> bars_a, bars_b;
    for (int i = 0; i < 5; ++i) {
        Bar a, b;
        a.timestamp = i;
        a.close = 100.0;
        a.symbol = "A";
        b.timestamp = i;
        b.close = 100.5;  // Only 0.5% spread, below 10% threshold
        b.symbol = "B";
        bars_a.push_back(a);
        bars_b.push_back(b);
    }

    engine.add_multi_bars("A", bars_a);
    engine.add_multi_bars("B", bars_b);
    auto result = engine.run();

    EXPECT_EQ(result.total_trades, 0);
}

// ============================================================================
// Pair Trading Tests
// ============================================================================

TEST(PairTrading, ConvergesOnMeanReversion) {
    auto strategy = std::make_shared<PairTrading>("X", "Y", 5, 1.5, 0.5, 10.0);
    BacktestEngine engine(100000.0);
    engine.set_strategy(strategy);

    std::vector<Bar> bars_x, bars_y;

    // Create a spread that diverges then converges
    double px[] = {100, 100, 100, 100, 100, 110, 115, 108, 102, 100};
    double py[] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};

    for (int i = 0; i < 10; ++i) {
        Bar x, y;
        x.timestamp = i;
        x.close = px[i];
        x.symbol = "X";
        y.timestamp = i;
        y.close = py[i];
        y.symbol = "Y";
        bars_x.push_back(x);
        bars_y.push_back(y);
    }

    engine.add_multi_bars("X", bars_x);
    engine.add_multi_bars("Y", bars_y);
    auto result = engine.run();

    // Should have made some trades as spread diverged
    EXPECT_GT(result.total_trades, 0);
}

TEST(PairTrading, Getters) {
    PairTrading pt("A", "B", 20, 2.0, 0.5, 1.0);
    EXPECT_DOUBLE_EQ(pt.entry_z(), 2.0);
    EXPECT_DOUBLE_EQ(pt.exit_z(), 0.5);

    pt.set_entry_z(3.0);
    pt.set_exit_z(1.0);
    EXPECT_DOUBLE_EQ(pt.entry_z(), 3.0);
    EXPECT_DOUBLE_EQ(pt.exit_z(), 1.0);
}

// ============================================================================
// Bar Tests
// ============================================================================

TEST(Bar, DefaultValues) {
    Bar bar;
    EXPECT_EQ(bar.timestamp, 0);
    EXPECT_DOUBLE_EQ(bar.open, 0.0);
    EXPECT_DOUBLE_EQ(bar.high, 0.0);
    EXPECT_DOUBLE_EQ(bar.low, 0.0);
    EXPECT_DOUBLE_EQ(bar.close, 0.0);
    EXPECT_DOUBLE_EQ(bar.volume, 0.0);
    EXPECT_TRUE(bar.symbol.empty());
}

// ============================================================================
// Strategy Order Tests
// ============================================================================

TEST(Strategy, SubmitAndTakeOrders) {
    class OrderStrategy : public Strategy {
    public:
        std::string name() const override { return "OrderStrategy"; }
        void on_bar(const Bar& bar) override {
            submit_order(bar.symbol, OrderSide::Buy, 100.0, 50.0);
        }
    };

    OrderStrategy strat;
    Bar bar;
    bar.symbol = "TEST";
    strat.on_bar(bar);

    auto orders = strat.take_orders();
    ASSERT_EQ(orders.size(), 1u);
    EXPECT_EQ(orders[0].symbol, "TEST");
    EXPECT_EQ(orders[0].side, OrderSide::Buy);
    EXPECT_DOUBLE_EQ(orders[0].quantity, 100.0);
    EXPECT_DOUBLE_EQ(orders[0].price, 50.0);

    // After take_orders, should be empty
    auto orders2 = strat.take_orders();
    EXPECT_TRUE(orders2.empty());
}
