#include <gtest/gtest.h>
#include "booking/book_keeper.hpp"

using namespace tradecore::booking;

namespace {

Trade make_trade(const std::string& symbol, const std::string& side,
                 double qty, double price, const std::string& trade_id = "T-001") {
    Trade trade;
    trade.trade_id = trade_id;
    trade.order_id = "TC-00001";
    trade.cl_ord_id = "test-001";
    trade.symbol = symbol;
    trade.side = side;
    trade.quantity = qty;
    trade.price = price;
    trade.commission = 0.0;
    trade.timestamp = "2024-01-01T00:00:00Z";
    trade.strategy_id = "test_strat";
    return trade;
}

}  // namespace

TEST(BookKeeper, BookTradeCreatesPosition) {
    BookKeeper keeper;
    keeper.book_trade(make_trade("AAPL", "buy", 100, 150.0));

    auto* pos = keeper.get_position("AAPL");
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->quantity, 100.0);
    EXPECT_EQ(pos->avg_price, 150.0);
}

TEST(BookKeeper, BookMultipleTradesSameSymbol) {
    BookKeeper keeper;
    keeper.book_trade(make_trade("AAPL", "buy", 100, 150.0, "T-001"));
    keeper.book_trade(make_trade("AAPL", "buy", 100, 160.0, "T-002"));

    auto* pos = keeper.get_position("AAPL");
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->quantity, 200.0);
    EXPECT_DOUBLE_EQ(pos->avg_price, 155.0);  // (15000+16000)/200
}

TEST(BookKeeper, BookBuySellCalculatesPnL) {
    BookKeeper keeper;
    keeper.book_trade(make_trade("AAPL", "buy", 100, 150.0, "T-001"));
    keeper.book_trade(make_trade("AAPL", "sell", 100, 160.0, "T-002"));

    auto* pos = keeper.get_position("AAPL");
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->quantity, 0.0);
    EXPECT_DOUBLE_EQ(pos->realized_pnl, 1000.0);
}

TEST(BookKeeper, TradeHistory) {
    BookKeeper keeper;
    keeper.book_trade(make_trade("AAPL", "buy", 100, 150.0, "T-001"));
    keeper.book_trade(make_trade("MSFT", "buy", 50, 300.0, "T-002"));

    EXPECT_EQ(keeper.trade_count(), 2);
    EXPECT_EQ(keeper.get_trades()[0].symbol, "AAPL");
    EXPECT_EQ(keeper.get_trades()[1].symbol, "MSFT");
}

TEST(BookKeeper, GetAllPositions) {
    BookKeeper keeper;
    keeper.book_trade(make_trade("AAPL", "buy", 100, 150.0, "T-001"));
    keeper.book_trade(make_trade("MSFT", "buy", 50, 300.0, "T-002"));

    auto positions = keeper.get_all_positions();
    EXPECT_EQ(positions.size(), 2);
}

TEST(BookKeeper, NoPositionReturnsNull) {
    BookKeeper keeper;
    EXPECT_EQ(keeper.get_position("AAPL"), nullptr);
}

TEST(BookKeeper, ShortPosition) {
    BookKeeper keeper;
    keeper.book_trade(make_trade("AAPL", "sell", 100, 150.0, "T-001"));

    auto* pos = keeper.get_position("AAPL");
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->quantity, -100.0);
    EXPECT_EQ(pos->avg_price, 150.0);
}
