#include <gtest/gtest.h>
#include "matching/matching_engine.hpp"

using namespace tradecore::matching;
using namespace tradecore::orders;
using namespace tradecore::instrument;

namespace {

Order make_market_order(const std::string& symbol, Side side, double qty) {
    Order order;
    order.cl_ord_id = "test-001";
    order.order_id = "TC-00001";
    order.instrument.symbol = symbol;
    order.instrument.asset_class = AssetClass::Equity;
    order.side = side;
    order.quantity = qty;
    order.order_type = OrderType::Market;
    return order;
}

Order make_limit_order(const std::string& symbol, Side side, double qty, double price) {
    Order order = make_market_order(symbol, side, qty);
    order.order_type = OrderType::Limit;
    order.limit_price = price;
    return order;
}

}  // namespace

TEST(MatchingEngine, MarketOrderFillsAtMarketPrice) {
    MatchingEngine engine;
    engine.update_market_price("AAPL", 150.0);

    auto order = make_market_order("AAPL", Side::Buy, 100.0);
    auto result = engine.try_match(order);

    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.fill_price, 150.0);
    EXPECT_EQ(result.fill_quantity, 100.0);
    EXPECT_EQ(result.remaining_quantity, 0.0);
}

TEST(MatchingEngine, MarketOrderNoPrice) {
    MatchingEngine engine;

    auto order = make_market_order("AAPL", Side::Buy, 100.0);
    auto result = engine.try_match(order);

    EXPECT_FALSE(result.matched);
}

TEST(MatchingEngine, MarketOrderFallsBackToLimitPrice) {
    MatchingEngine engine;

    auto order = make_market_order("AAPL", Side::Buy, 100.0);
    order.limit_price = 155.0;
    auto result = engine.try_match(order);

    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.fill_price, 155.0);
}

TEST(MatchingEngine, LimitOrderFills) {
    MatchingEngine engine;

    auto order = make_limit_order("AAPL", Side::Buy, 50.0, 145.0);
    auto result = engine.try_match(order);

    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.fill_price, 145.0);
    EXPECT_EQ(result.fill_quantity, 50.0);
}

TEST(MatchingEngine, UpdateMarketPrice) {
    MatchingEngine engine;

    engine.update_market_price("AAPL", 150.0);
    EXPECT_EQ(engine.get_market_price("AAPL"), 150.0);

    engine.update_market_price("AAPL", 155.0);
    EXPECT_EQ(engine.get_market_price("AAPL"), 155.0);

    EXPECT_EQ(engine.get_market_price("UNKNOWN"), 0.0);
}
