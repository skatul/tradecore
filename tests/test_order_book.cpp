#include <gtest/gtest.h>
#include "matching/order_book.hpp"

using namespace tradecore::matching;

namespace {

OrderEntry make_entry(const std::string& id, double price, double qty) {
    OrderEntry e;
    e.order_id = id;
    e.cl_ord_id = "cl-" + id;
    e.price = price;
    e.remaining_quantity = qty;
    e.original_quantity = qty;
    return e;
}

}  // namespace

TEST(OrderBook, AddAndBestBidAsk) {
    OrderBook book;
    book.add_order(BookSide::Bid, make_entry("B1", 100.0, 50));
    book.add_order(BookSide::Bid, make_entry("B2", 101.0, 30));
    book.add_order(BookSide::Ask, make_entry("A1", 102.0, 40));
    book.add_order(BookSide::Ask, make_entry("A2", 103.0, 20));

    EXPECT_EQ(book.best_bid().value(), 101.0);
    EXPECT_EQ(book.best_ask().value(), 102.0);
    EXPECT_EQ(book.bid_levels(), 2);
    EXPECT_EQ(book.ask_levels(), 2);
}

TEST(OrderBook, FIFOPriority) {
    OrderBook book;
    book.add_order(BookSide::Ask, make_entry("A1", 100.0, 50));
    book.add_order(BookSide::Ask, make_entry("A2", 100.0, 30));

    auto fills = book.consume_asks(60);
    // Should fill A1 fully (50), then A2 partially (10)
    ASSERT_EQ(fills.size(), 2);
    EXPECT_EQ(fills[0].order_id, "A1");
    EXPECT_EQ(fills[0].remaining_quantity, 50.0);
    EXPECT_EQ(fills[1].order_id, "A2");
    EXPECT_EQ(fills[1].remaining_quantity, 10.0);

    // A2 should still have 20 remaining in the book
    auto depth = book.get_depth(BookSide::Ask, 5);
    ASSERT_EQ(depth.size(), 1);
    EXPECT_EQ(depth[0].quantity, 20.0);
}

TEST(OrderBook, CancelOrder) {
    OrderBook book;
    book.add_order(BookSide::Bid, make_entry("B1", 100.0, 50));
    book.add_order(BookSide::Bid, make_entry("B2", 100.0, 30));

    EXPECT_TRUE(book.cancel_order("B1"));
    EXPECT_FALSE(book.cancel_order("nonexistent"));

    auto depth = book.get_depth(BookSide::Bid, 5);
    ASSERT_EQ(depth.size(), 1);
    EXPECT_EQ(depth[0].quantity, 30.0);
}

TEST(OrderBook, DepthMultipleLevels) {
    OrderBook book;
    book.add_order(BookSide::Ask, make_entry("A1", 100.0, 50));
    book.add_order(BookSide::Ask, make_entry("A2", 100.0, 30));
    book.add_order(BookSide::Ask, make_entry("A3", 101.0, 20));
    book.add_order(BookSide::Ask, make_entry("A4", 102.0, 10));

    auto depth = book.get_depth(BookSide::Ask, 3);
    ASSERT_EQ(depth.size(), 3);
    EXPECT_EQ(depth[0].price, 100.0);
    EXPECT_EQ(depth[0].quantity, 80.0);
    EXPECT_EQ(depth[0].order_count, 2);
    EXPECT_EQ(depth[1].price, 101.0);
    EXPECT_EQ(depth[1].quantity, 20.0);
    EXPECT_EQ(depth[2].price, 102.0);
    EXPECT_EQ(depth[2].quantity, 10.0);
}

TEST(OrderBook, Spread) {
    OrderBook book;
    book.add_order(BookSide::Bid, make_entry("B1", 99.0, 50));
    book.add_order(BookSide::Ask, make_entry("A1", 101.0, 50));

    auto bid = book.best_bid();
    auto ask = book.best_ask();
    ASSERT_TRUE(bid.has_value());
    ASSERT_TRUE(ask.has_value());
    EXPECT_EQ(ask.value() - bid.value(), 2.0);
}

TEST(OrderBook, EmptyBookReturnsNullopt) {
    OrderBook book;
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_TRUE(book.get_depth(BookSide::Bid, 5).empty());
}

TEST(OrderBook, ConsumeEntireBook) {
    OrderBook book;
    book.add_order(BookSide::Ask, make_entry("A1", 100.0, 30));
    book.add_order(BookSide::Ask, make_entry("A2", 101.0, 20));

    auto fills = book.consume_asks(100);
    // Should consume all 50 available, leaving 50 unfilled
    ASSERT_EQ(fills.size(), 2);
    double total_filled = 0;
    for (const auto& f : fills) total_filled += f.remaining_quantity;
    EXPECT_EQ(total_filled, 50.0);

    EXPECT_FALSE(book.best_ask().has_value());
}
