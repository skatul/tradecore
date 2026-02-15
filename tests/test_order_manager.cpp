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

    fix::FixMessage make_cancel_msg(const std::string& orig_cl_ord_id,
                                     const std::string& symbol = "AAPL") {
        fix::FixMessage msg;
        msg.set_sender_comp_id("TEST_CLIENT");
        msg.set_msg_seq_num(generate_uuid());
        msg.set_sending_time(current_timestamp());

        auto* cancel = msg.mutable_order_cancel_request();
        cancel->set_cl_ord_id(generate_uuid());
        cancel->set_orig_cl_ord_id(orig_cl_ord_id);
        cancel->mutable_instrument()->set_symbol(symbol);
        cancel->set_side(fix::SIDE_BUY);

        return msg;
    }
};

TEST_F(OrderManagerTest, AcceptAndFillMarketOrder) {
    auto msg = make_new_order_msg();
    auto responses = mgr->handle_new_order(msg);

    // Should have at least one fill response
    ASSERT_GE(responses.size(), 1);
    bool has_fill = false;
    for (const auto& r : responses) {
        if (r.has_execution_report()) {
            const auto& er = r.execution_report();
            if (er.exec_type() == fix::EXEC_TYPE_FILL || er.exec_type() == fix::EXEC_TYPE_PARTIAL_FILL) {
                has_fill = true;
                EXPECT_GT(er.last_px(), 0.0);
                EXPECT_EQ(er.last_qty(), 100.0);
                EXPECT_EQ(er.cl_ord_id(), "test-001");
            }
        }
    }
    EXPECT_TRUE(has_fill);
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

    EXPECT_GE(book_keeper.trade_count(), 1);
    auto* pos = book_keeper.get_position("AAPL");
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->quantity, 100.0);
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

// --- New tests for Phase 1.3 ---

TEST_F(OrderManagerTest, CancelRequest) {
    // First, seed a book and place a resting limit order
    matcher.seed_book("AAPL", 150.0, 10.0, 5, 1000.0);

    fix::FixMessage limit_msg;
    limit_msg.set_sender_comp_id("TEST_CLIENT");
    limit_msg.set_msg_seq_num(generate_uuid());
    limit_msg.set_sending_time(current_timestamp());
    auto* nos = limit_msg.mutable_new_order_single();
    nos->set_cl_ord_id("limit-to-cancel");
    nos->mutable_instrument()->set_symbol("AAPL");
    nos->mutable_instrument()->set_security_type(fix::SECURITY_TYPE_COMMON_STOCK);
    nos->set_side(fix::SIDE_BUY);
    nos->set_order_qty(50.0);
    nos->set_ord_type(fix::ORD_TYPE_LIMIT);
    nos->set_price(140.0);  // below best ask, will rest
    nos->set_time_in_force(fix::TIF_GTC);

    auto order_responses = mgr->handle_new_order(limit_msg);
    ASSERT_GE(order_responses.size(), 1);
    // Should get a NEW ack since the order rests
    EXPECT_TRUE(order_responses[0].has_execution_report());
    EXPECT_EQ(order_responses[0].execution_report().exec_type(), fix::EXEC_TYPE_NEW);

    // Now cancel it
    auto cancel_msg = make_cancel_msg("limit-to-cancel");
    auto cancel_responses = mgr->handle_cancel_request(cancel_msg);

    ASSERT_EQ(cancel_responses.size(), 1);
    EXPECT_TRUE(cancel_responses[0].has_execution_report());
    EXPECT_EQ(cancel_responses[0].execution_report().exec_type(), fix::EXEC_TYPE_CANCELLED);
    EXPECT_EQ(cancel_responses[0].execution_report().ord_status(), fix::ORD_STATUS_CANCELLED);

    // Verify the order is cancelled
    auto* order = mgr->find_order_by_cl_ord_id("limit-to-cancel");
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->status, OrderStatus::Cancelled);
}

TEST_F(OrderManagerTest, PartialFillCommission) {
    double custom_rate = 0.002;
    auto custom_mgr = std::make_unique<OrderManager>(matcher, book_keeper, custom_rate);
    matcher.seed_book("AAPL", 150.0, 10.0, 2, 100.0);

    auto msg = make_new_order_msg("AAPL", fix::SIDE_BUY, 250.0);
    auto responses = custom_mgr->handle_new_order(msg);

    // Should get multiple fill reports (from 2 levels)
    int fill_count = 0;
    double total_commission = 0.0;
    for (const auto& r : responses) {
        if (r.has_execution_report()) {
            const auto& er = r.execution_report();
            if (er.exec_type() == fix::EXEC_TYPE_FILL || er.exec_type() == fix::EXEC_TYPE_PARTIAL_FILL) {
                fill_count++;
                total_commission += er.commission();
                // Verify commission = price * qty * rate
                EXPECT_NEAR(er.commission(), er.last_px() * er.last_qty() * custom_rate, 0.01);
            }
        }
    }
    EXPECT_GE(fill_count, 1);
    EXPECT_GT(total_commission, 0.0);
}
