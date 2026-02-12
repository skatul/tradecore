#include <gtest/gtest.h>
#include "orders/order_manager.hpp"

using namespace tradecore;
using namespace tradecore::orders;
using namespace tradecore::messaging;

class OrderManagerTest : public ::testing::Test {
protected:
    matching::MatchingEngine matcher;
    booking::BookKeeper book_keeper;
    std::unique_ptr<OrderManager> mgr;

    void SetUp() override {
        mgr = std::make_unique<OrderManager>(matcher, book_keeper);
        matcher.update_market_price("AAPL", 150.0);
    }

    Message make_new_order_msg(const std::string& symbol = "AAPL",
                               const std::string& side = "buy",
                               double qty = 100.0) {
        Message msg;
        msg.msg_type = "new_order";
        msg.msg_id = generate_uuid();
        msg.timestamp = current_timestamp();
        msg.payload = {
            {"cl_ord_id", "test-001"},
            {"instrument", {{"symbol", symbol}, {"asset_class", "equity"}}},
            {"side", side},
            {"quantity", qty},
            {"order_type", "market"},
            {"time_in_force", "day"},
            {"strategy_id", "test_strat"},
        };
        return msg;
    }
};

TEST_F(OrderManagerTest, AcceptAndFillMarketOrder) {
    auto msg = make_new_order_msg();
    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_EQ(responses[0].msg_type, "fill");
    EXPECT_EQ(responses[0].payload["fill_price"], 150.0);
    EXPECT_EQ(responses[0].payload["fill_quantity"], 100.0);
    EXPECT_EQ(responses[0].payload["status"], "filled");
    EXPECT_EQ(responses[0].ref_msg_id, msg.msg_id);
}

TEST_F(OrderManagerTest, RejectInvalidOrder) {
    auto msg = make_new_order_msg();
    msg.payload["quantity"] = -10.0;  // invalid

    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_EQ(responses[0].msg_type, "reject");
    EXPECT_EQ(responses[0].payload["reason"], "validation_error");
}

TEST_F(OrderManagerTest, RejectMissingSymbol) {
    auto msg = make_new_order_msg();
    msg.payload["instrument"]["symbol"] = "";

    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_EQ(responses[0].msg_type, "reject");
}

TEST_F(OrderManagerTest, FillBooksTrade) {
    auto msg = make_new_order_msg();
    mgr->handle_new_order(msg);

    EXPECT_EQ(book_keeper.trade_count(), 1);
    auto* pos = book_keeper.get_position("AAPL");
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->quantity, 100.0);
    EXPECT_EQ(pos->avg_price, 150.0);
}

TEST_F(OrderManagerTest, OrderIdSequence) {
    auto msg1 = make_new_order_msg();
    auto msg2 = make_new_order_msg();
    msg2.payload["cl_ord_id"] = "test-002";
    msg2.msg_id = generate_uuid();

    mgr->handle_new_order(msg1);
    mgr->handle_new_order(msg2);

    EXPECT_EQ(mgr->order_count(), 2);
}

TEST_F(OrderManagerTest, NoMatchWhenNoPriceAvailable) {
    auto msg = make_new_order_msg("UNKNOWN");

    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_EQ(responses[0].msg_type, "reject");
    EXPECT_EQ(responses[0].payload["reason"], "no_match");
}

TEST_F(OrderManagerTest, ParseErrorOnBadPayload) {
    Message msg;
    msg.msg_type = "new_order";
    msg.msg_id = generate_uuid();
    msg.timestamp = current_timestamp();
    msg.payload = {{"bad", "data"}};  // missing required fields

    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_EQ(responses[0].msg_type, "reject");
    EXPECT_EQ(responses[0].payload["reason"], "parse_error");
}
