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
    EXPECT_NEAR(result.fill_price, 150.075, 0.1);  // fills at best ask (150 + half-spread)
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
    engine.seed_book("AAPL", 150.0, 10.0, 5, 1000.0);

    // Buy limit at a price above the best ask should cross
    auto order = make_limit_order("AAPL", Side::Buy, 50.0, 155.0);
    auto result = engine.try_match(order);

    EXPECT_TRUE(result.matched);
    EXPECT_GT(result.fill_quantity, 0.0);
    EXPECT_EQ(result.remaining_quantity, 0.0);
}

TEST(MatchingEngine, UpdateMarketPrice) {
    MatchingEngine engine;

    engine.update_market_price("AAPL", 150.0);
    EXPECT_EQ(engine.get_market_price("AAPL"), 150.0);

    engine.update_market_price("AAPL", 155.0);
    EXPECT_EQ(engine.get_market_price("AAPL"), 155.0);

    EXPECT_EQ(engine.get_market_price("UNKNOWN"), 0.0);
}

// --- New tests for Phase 1.2 ---

TEST(MatchingEngine, PartialFillMarketOrder) {
    MatchingEngine engine;
    engine.seed_book("AAPL", 150.0, 10.0, 2, 100.0);

    // Buy 250 but only 200 available on ask (2 levels * 100)
    auto order = make_market_order("AAPL", Side::Buy, 250.0);
    auto result = engine.try_match(order);

    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.fill_quantity, 200.0);
    EXPECT_EQ(result.remaining_quantity, 50.0);
    EXPECT_GE(result.fills.size(), 1);
}

TEST(MatchingEngine, LimitOrderRests) {
    MatchingEngine engine;
    engine.seed_book("AAPL", 150.0, 10.0, 5, 1000.0);

    // Buy limit below best ask should rest
    auto order = make_limit_order("AAPL", Side::Buy, 50.0, 140.0);
    order.order_id = "RESTING-001";
    auto result = engine.try_match(order);

    EXPECT_FALSE(result.matched);
    EXPECT_EQ(result.remaining_quantity, 50.0);

    // Should be resting in the book
    auto* book = engine.get_book("AAPL");
    ASSERT_NE(book, nullptr);
    auto depth = book->get_depth(BookSide::Bid, 10);
    bool found = false;
    for (const auto& d : depth) {
        if (d.price == 140.0) found = true;
    }
    EXPECT_TRUE(found);
}

TEST(MatchingEngine, LimitOrderCrosses) {
    MatchingEngine engine;
    engine.seed_book("AAPL", 150.0, 10.0, 5, 1000.0);

    // Sell limit at a price below best bid should cross
    auto order = make_limit_order("AAPL", Side::Sell, 50.0, 140.0);
    auto result = engine.try_match(order);

    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.fill_quantity, 50.0);
    EXPECT_EQ(result.remaining_quantity, 0.0);
}

TEST(MatchingEngine, SeedBookCreatesDepth) {
    MatchingEngine engine;
    engine.seed_book("TSLA", 200.0, 20.0, 3, 500.0);

    auto* book = engine.get_book("TSLA");
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->bid_levels(), 3);
    EXPECT_EQ(book->ask_levels(), 3);

    auto bid = book->best_bid();
    auto ask = book->best_ask();
    ASSERT_TRUE(bid.has_value());
    ASSERT_TRUE(ask.has_value());
    EXPECT_LT(bid.value(), 200.0);
    EXPECT_GT(ask.value(), 200.0);
}

TEST(MatchingEngine, CancelOrder) {
    MatchingEngine engine;
    engine.seed_book("AAPL", 150.0, 10.0, 5, 1000.0);

    // Place a resting limit order
    auto order = make_limit_order("AAPL", Side::Buy, 50.0, 140.0);
    order.order_id = "CANCEL-ME";
    engine.try_match(order);

    // Cancel it
    EXPECT_TRUE(engine.cancel_order("AAPL", "CANCEL-ME"));
    EXPECT_FALSE(engine.cancel_order("AAPL", "CANCEL-ME"));  // already cancelled
    EXPECT_FALSE(engine.cancel_order("NONEXIST", "CANCEL-ME"));
}

TEST(MatchingEngine, WalkPriceLevels) {
    MatchingEngine engine;
    // Seed with small quantities per level
    engine.seed_book("GOOG", 100.0, 100.0, 3, 10.0);

    // Buy 25 units - should walk across multiple ask levels
    auto order = make_market_order("GOOG", Side::Buy, 25.0);
    auto result = engine.try_match(order);

    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.fill_quantity, 25.0);
    EXPECT_EQ(result.remaining_quantity, 0.0);
    // Should have fills from multiple levels
    EXPECT_GE(result.fills.size(), 2);
    // VWAP should be higher than best ask since we walked levels
    EXPECT_GT(result.fill_price, result.fills[0].fill_price);
}
