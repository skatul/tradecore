#include <gtest/gtest.h>
#include <thread>
#include "core/metrics.hpp"

using namespace tradecore::core;

class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        Metrics::instance().reset();
    }
};

TEST_F(MetricsTest, Counters) {
    auto& m = Metrics::instance();
    m.orders_received++;
    m.orders_received++;
    m.orders_filled++;
    m.orders_rejected++;

    EXPECT_EQ(m.orders_received.load(), 2);
    EXPECT_EQ(m.orders_filled.load(), 1);
    EXPECT_EQ(m.orders_rejected.load(), 1);
    EXPECT_EQ(m.orders_cancelled.load(), 0);
}

TEST_F(MetricsTest, Notional) {
    auto& m = Metrics::instance();
    m.add_notional(1500.50);
    m.add_notional(2000.25);

    EXPECT_NEAR(m.get_notional(), 3500.75, 0.01);
}

TEST_F(MetricsTest, LatencyTracking) {
    auto& m = Metrics::instance();
    m.record_latency_us(100);
    m.record_latency_us(200);
    m.record_latency_us(300);

    auto stats = m.latency_stats();
    EXPECT_EQ(stats.count, 3);
    EXPECT_EQ(stats.avg_us, 200);
    EXPECT_GE(stats.p99_us, 200);
}

TEST_F(MetricsTest, ScopedTimer) {
    auto& m = Metrics::instance();

    {
        ScopedTimer timer;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto stats = m.latency_stats();
    EXPECT_EQ(stats.count, 1);
    EXPECT_GE(stats.avg_us, 500);  // at least 0.5ms
}

TEST_F(MetricsTest, ToString) {
    auto& m = Metrics::instance();
    m.orders_received = 5;
    m.orders_filled = 3;

    auto s = m.to_string();
    EXPECT_NE(s.find("orders_received=5"), std::string::npos);
    EXPECT_NE(s.find("orders_filled=3"), std::string::npos);
}

TEST_F(MetricsTest, Reset) {
    auto& m = Metrics::instance();
    m.orders_received = 10;
    m.record_latency_us(500);

    m.reset();

    EXPECT_EQ(m.orders_received.load(), 0);
    auto stats = m.latency_stats();
    EXPECT_EQ(stats.count, 0);
}
