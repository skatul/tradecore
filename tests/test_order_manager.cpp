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

    fix::FixMessage make_new_order_msg(const std::string& symbol = "AAPL",
                                        fix::Side side = fix::SIDE_BUY,
                                        double qty = 100.0) {
        fix::FixMessage msg;
        msg.set_sender_comp_id("TEST_CLIENT");
        msg.set_msg_seq_num(generate_uuid());
        msg.set_sending_time(current_timestamp());

        auto* nos = msg.mutable_new_order_single();
        nos->set_cl_ord_id("test-001");
        nos->mutable_instrument()->set_symbol(symbol);
        nos->mutable_instrument()->set_security_type(fix::SECURITY_TYPE_COMMON_STOCK);
        nos->set_side(side);
        nos->set_order_qty(qty);
        nos->set_ord_type(fix::ORD_TYPE_MARKET);
        nos->set_time_in_force(fix::TIF_DAY);
        nos->set_text("test_strat");

        return msg;
    }
};

TEST_F(OrderManagerTest, AcceptAndFillMarketOrder) {
    auto msg = make_new_order_msg();
    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_TRUE(responses[0].has_execution_report());

    const auto& er = responses[0].execution_report();
    EXPECT_EQ(er.last_px(), 150.0);
    EXPECT_EQ(er.last_qty(), 100.0);
    EXPECT_EQ(er.ord_status(), fix::ORD_STATUS_FILLED);
    EXPECT_EQ(er.exec_type(), fix::EXEC_TYPE_FILL);
    EXPECT_EQ(er.cl_ord_id(), "test-001");
}

TEST_F(OrderManagerTest, RejectInvalidOrder) {
    auto msg = make_new_order_msg();
    msg.mutable_new_order_single()->set_order_qty(-10.0);

    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_TRUE(responses[0].has_reject());
    EXPECT_FALSE(responses[0].reject().text().empty());
}

TEST_F(OrderManagerTest, RejectMissingSymbol) {
    auto msg = make_new_order_msg();
    msg.mutable_new_order_single()->mutable_instrument()->set_symbol("");

    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_TRUE(responses[0].has_reject());
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
    msg2.mutable_new_order_single()->set_cl_ord_id("test-002");
    msg2.set_msg_seq_num(generate_uuid());

    mgr->handle_new_order(msg1);
    mgr->handle_new_order(msg2);

    EXPECT_EQ(mgr->order_count(), 2);
}

TEST_F(OrderManagerTest, NoMatchWhenNoPriceAvailable) {
    auto msg = make_new_order_msg("UNKNOWN");

    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_TRUE(responses[0].has_reject());
}

TEST_F(OrderManagerTest, RejectWhenNoNewOrderSingle) {
    fix::FixMessage msg;
    msg.set_sender_comp_id("TEST_CLIENT");
    msg.set_msg_seq_num(generate_uuid());
    msg.mutable_heartbeat();  // wrong message type

    auto responses = mgr->handle_new_order(msg);

    ASSERT_EQ(responses.size(), 1);
    EXPECT_TRUE(responses[0].has_reject());
}
