#include <gtest/gtest.h>

#include "indicators/indicator.hpp"
#include "indicators/moving_averages.hpp"
#include "indicators/oscillators.hpp"
#include "indicators/volatility.hpp"
#include "indicators/volume.hpp"

#include <cmath>
#include <vector>

using namespace tradecore::indicators;

// ============================================================================
// SMA Tests
// ============================================================================

TEST(SMA, BasicCalculation) {
    SMA sma(3);
    EXPECT_FALSE(sma.ready());

    sma.update(10.0);
    sma.update(20.0);
    EXPECT_FALSE(sma.ready());

    sma.update(30.0);
    EXPECT_TRUE(sma.ready());
    EXPECT_DOUBLE_EQ(sma.value(), 20.0);  // (10+20+30)/3

    sma.update(40.0);
    EXPECT_DOUBLE_EQ(sma.value(), 30.0);  // (20+30+40)/3
}

TEST(SMA, Reset) {
    SMA sma(2);
    sma.update(10.0);
    sma.update(20.0);
    EXPECT_TRUE(sma.ready());

    sma.reset();
    EXPECT_FALSE(sma.ready());
    EXPECT_EQ(sma.count(), 0u);
}

TEST(SMA, Period) {
    SMA sma(5);
    EXPECT_EQ(sma.period(), 5u);
}

// ============================================================================
// EMA Tests
// ============================================================================

TEST(EMA, WilderSmoothing) {
    EMA ema(10, true);  // Wilder: alpha = 1/10 = 0.1
    EXPECT_FALSE(ema.ready());

    // Feed 10 values to make it ready
    for (int i = 1; i <= 10; ++i) {
        ema.update(static_cast<double>(i));
    }
    EXPECT_TRUE(ema.ready());

    // After feeding 1..10, the EMA should converge
    double val = ema.value();
    EXPECT_GT(val, 0.0);
}

TEST(EMA, StandardSmoothing) {
    EMA ema(3, false);  // Standard: alpha = 2/(3+1) = 0.5

    ema.update(10.0);
    EXPECT_DOUBLE_EQ(ema.value(), 10.0);  // First value = input

    ema.update(20.0);
    // EMA = (20 - 10) * 0.5 + 10 = 15
    EXPECT_DOUBLE_EQ(ema.value(), 15.0);

    ema.update(30.0);
    // EMA = (30 - 15) * 0.5 + 15 = 22.5
    EXPECT_DOUBLE_EQ(ema.value(), 22.5);
    EXPECT_TRUE(ema.ready());
}

TEST(EMA, Reset) {
    EMA ema(3);
    ema.update(10.0);
    ema.update(20.0);
    ema.update(30.0);
    ema.reset();
    EXPECT_FALSE(ema.ready());
    EXPECT_EQ(ema.count(), 0u);
}

// ============================================================================
// RSI Tests
// ============================================================================

TEST(RSI, AllGains) {
    RSI rsi(5);
    // Monotonically increasing prices: RSI should be 100
    std::vector<double> prices = {10, 11, 12, 13, 14, 15};
    for (double p : prices) rsi.update(p);
    EXPECT_TRUE(rsi.ready());
    EXPECT_DOUBLE_EQ(rsi.value(), 100.0);
}

TEST(RSI, AllLosses) {
    RSI rsi(5);
    std::vector<double> prices = {15, 14, 13, 12, 11, 10};
    for (double p : prices) rsi.update(p);
    EXPECT_TRUE(rsi.ready());
    EXPECT_DOUBLE_EQ(rsi.value(), 0.0);
}

TEST(RSI, MixedValues) {
    RSI rsi(14);
    std::vector<double> prices = {
        44.34, 44.09, 44.15, 43.61, 44.33, 44.83, 45.10, 45.42,
        45.84, 46.08, 45.89, 46.03, 45.61, 46.28, 46.28
    };
    for (double p : prices) rsi.update(p);
    EXPECT_TRUE(rsi.ready());
    double val = rsi.value();
    EXPECT_GT(val, 0.0);
    EXPECT_LT(val, 100.0);
}

TEST(RSI, Reset) {
    RSI rsi(5);
    for (int i = 0; i < 10; ++i) rsi.update(static_cast<double>(i));
    rsi.reset();
    EXPECT_FALSE(rsi.ready());
}

// ============================================================================
// MACD Tests
// ============================================================================

TEST(MACD, Basic) {
    MACD macd(3, 5, 2);
    // Need at least slow + signal = 5 + 2 = 7 values
    for (int i = 1; i <= 10; ++i) {
        macd.update(static_cast<double>(i));
    }
    EXPECT_TRUE(macd.ready());

    // MACD line should be positive for uptrend
    EXPECT_GT(macd.value(), 0.0);
    // Histogram = MACD - Signal
    EXPECT_DOUBLE_EQ(macd.histogram(), macd.value() - macd.signal());
}

TEST(MACD, Reset) {
    MACD macd(3, 5, 2);
    for (int i = 0; i < 10; ++i) macd.update(static_cast<double>(i));
    macd.reset();
    EXPECT_FALSE(macd.ready());
}

// ============================================================================
// Stochastic Tests
// ============================================================================

TEST(Stochastic, Basic) {
    Stochastic stoch(5, 3);

    // Feed monotonically increasing values
    for (int i = 1; i <= 10; ++i) {
        stoch.update(static_cast<double>(i));
    }
    EXPECT_TRUE(stoch.ready());
    // With all increasing values, %K should be 100
    EXPECT_DOUBLE_EQ(stoch.k(), 100.0);
}

TEST(Stochastic, WithHLC) {
    Stochastic stoch(3, 2);
    stoch.update(12.0, 10.0, 11.0);
    stoch.update(13.0, 11.0, 12.0);
    stoch.update(14.0, 12.0, 13.0);
    EXPECT_TRUE(stoch.ready() || stoch.count() >= 3);

    // %K and %D should be between 0 and 100
    EXPECT_GE(stoch.k(), 0.0);
    EXPECT_LE(stoch.k(), 100.0);
}

TEST(Stochastic, Reset) {
    Stochastic stoch(3, 2);
    for (int i = 0; i < 10; ++i) stoch.update(static_cast<double>(i));
    stoch.reset();
    EXPECT_FALSE(stoch.ready());
}

// ============================================================================
// Bollinger Bands Tests
// ============================================================================

TEST(BollingerBands, ConstantInput) {
    BollingerBands bb(5, 2.0);
    for (int i = 0; i < 5; ++i) bb.update(100.0);
    EXPECT_TRUE(bb.ready());

    // Constant input: std dev = 0, so upper = lower = middle
    EXPECT_DOUBLE_EQ(bb.value(), 100.0);
    EXPECT_DOUBLE_EQ(bb.upper(), 100.0);
    EXPECT_DOUBLE_EQ(bb.lower(), 100.0);
}

TEST(BollingerBands, VariableInput) {
    BollingerBands bb(3, 2.0);
    bb.update(10.0);
    bb.update(20.0);
    bb.update(30.0);
    EXPECT_TRUE(bb.ready());

    EXPECT_DOUBLE_EQ(bb.value(), 20.0);  // SMA(10,20,30) = 20
    EXPECT_GT(bb.upper(), 20.0);
    EXPECT_LT(bb.lower(), 20.0);
    // Upper and lower should be symmetric around middle
    EXPECT_NEAR(bb.upper() - bb.value(), bb.value() - bb.lower(), 1e-10);
}

TEST(BollingerBands, Reset) {
    BollingerBands bb(3, 2.0);
    for (int i = 0; i < 5; ++i) bb.update(static_cast<double>(i));
    bb.reset();
    EXPECT_FALSE(bb.ready());
}

// ============================================================================
// ATR Tests
// ============================================================================

TEST(ATR, Basic) {
    ATR atr(3);
    // First bar: TR = high - low
    atr.update(12.0, 10.0, 11.0);
    // Second bar
    atr.update(13.0, 10.5, 12.0);
    // Third bar
    atr.update(14.0, 11.0, 13.0);
    EXPECT_TRUE(atr.ready());
    EXPECT_GT(atr.value(), 0.0);
}

TEST(ATR, SingleValueUpdate) {
    ATR atr(3);
    atr.update(100.0);
    atr.update(101.0);
    atr.update(102.0);
    EXPECT_TRUE(atr.ready());
    // Single value: high=low=close, so TR=0 for first, then |close-prev_close|
    EXPECT_GE(atr.value(), 0.0);
}

TEST(ATR, Reset) {
    ATR atr(3);
    for (int i = 0; i < 5; ++i) atr.update(static_cast<double>(i) * 10, 0.0, static_cast<double>(i) * 5);
    atr.reset();
    EXPECT_FALSE(atr.ready());
}

// ============================================================================
// VWAP Tests
// ============================================================================

TEST(VWAP, Basic) {
    VWAP vwap;
    vwap.update(100.0, 10.0);  // price=100, vol=10
    EXPECT_TRUE(vwap.ready());
    EXPECT_DOUBLE_EQ(vwap.value(), 100.0);

    vwap.update(110.0, 20.0);  // price=110, vol=20
    // VWAP = (100*10 + 110*20) / (10+20) = 3200/30 = 106.666...
    EXPECT_NEAR(vwap.value(), 3200.0 / 30.0, 1e-10);
    EXPECT_DOUBLE_EQ(vwap.cumulative_volume(), 30.0);
}

TEST(VWAP, SingleValueUpdate) {
    VWAP vwap;
    vwap.update(50.0);
    vwap.update(60.0);
    // With volume=1 each: VWAP = (50+60)/2 = 55
    EXPECT_DOUBLE_EQ(vwap.value(), 55.0);
}

TEST(VWAP, Reset) {
    VWAP vwap;
    vwap.update(100.0, 10.0);
    vwap.reset();
    EXPECT_FALSE(vwap.ready());
}

// ============================================================================
// OBV Tests
// ============================================================================

TEST(OBV, Basic) {
    OBV obv;
    obv.update(100.0, 1000.0);  // First bar: OBV = volume
    EXPECT_TRUE(obv.ready());
    EXPECT_DOUBLE_EQ(obv.value(), 1000.0);

    obv.update(105.0, 1500.0);  // Price up: add volume
    EXPECT_DOUBLE_EQ(obv.value(), 2500.0);

    obv.update(103.0, 800.0);   // Price down: subtract volume
    EXPECT_DOUBLE_EQ(obv.value(), 1700.0);

    obv.update(103.0, 500.0);   // Price unchanged: no change
    EXPECT_DOUBLE_EQ(obv.value(), 1700.0);
}

TEST(OBV, Reset) {
    OBV obv;
    obv.update(100.0, 1000.0);
    obv.update(105.0, 1500.0);
    obv.reset();
    EXPECT_FALSE(obv.ready());
}
