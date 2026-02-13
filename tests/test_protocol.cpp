#include <gtest/gtest.h>
#include "messaging/protocol.hpp"

using namespace tradecore::messaging;

TEST(Protocol, SerializeDeserializeRoundtrip) {
    fix::FixMessage msg;
    msg.set_sender_comp_id("CLIENT");
    msg.set_target_comp_id("TRADECORE");
    msg.set_msg_seq_num("seq-001");
    msg.set_sending_time(current_timestamp());

    auto* nos = msg.mutable_new_order_single();
    nos->set_cl_ord_id("ord-001");
    nos->mutable_instrument()->set_symbol("AAPL");
    nos->set_side(fix::SIDE_BUY);
    nos->set_order_qty(100.0);
    nos->set_ord_type(fix::ORD_TYPE_MARKET);

    std::string bytes = serialize(msg);
    auto restored = deserialize(bytes);

    EXPECT_EQ(restored.sender_comp_id(), "CLIENT");
    EXPECT_EQ(restored.target_comp_id(), "TRADECORE");
    EXPECT_TRUE(restored.has_new_order_single());
    EXPECT_EQ(restored.new_order_single().cl_ord_id(), "ord-001");
    EXPECT_EQ(restored.new_order_single().instrument().symbol(), "AAPL");
    EXPECT_EQ(restored.new_order_single().side(), fix::SIDE_BUY);
    EXPECT_EQ(restored.new_order_single().order_qty(), 100.0);
}

TEST(Protocol, MakeExecutionReportNew) {
    fix::FixMessage request;
    request.set_sender_comp_id("CLIENT");
    request.set_msg_seq_num("seq-001");
    auto* nos = request.mutable_new_order_single();
    nos->set_cl_ord_id("ord-001");
    nos->mutable_instrument()->set_symbol("AAPL");
    nos->set_side(fix::SIDE_BUY);
    nos->set_order_qty(100.0);

    auto response = make_execution_report_new(request, "TC-00001");

    EXPECT_EQ(response.sender_comp_id(), "TRADECORE");
    EXPECT_EQ(response.target_comp_id(), "CLIENT");
    EXPECT_TRUE(response.has_execution_report());

    const auto& er = response.execution_report();
    EXPECT_EQ(er.order_id(), "TC-00001");
    EXPECT_EQ(er.cl_ord_id(), "ord-001");
    EXPECT_EQ(er.exec_type(), fix::EXEC_TYPE_NEW);
    EXPECT_EQ(er.ord_status(), fix::ORD_STATUS_NEW);
    EXPECT_EQ(er.instrument().symbol(), "AAPL");
    EXPECT_EQ(er.side(), fix::SIDE_BUY);
    EXPECT_EQ(er.order_qty(), 100.0);
    EXPECT_EQ(er.leaves_qty(), 100.0);
    EXPECT_EQ(er.cum_qty(), 0.0);
}

TEST(Protocol, MakeExecutionReportFill) {
    fix::FixMessage request;
    request.set_sender_comp_id("CLIENT");
    auto* nos = request.mutable_new_order_single();
    nos->set_cl_ord_id("ord-001");
    nos->mutable_instrument()->set_symbol("AAPL");
    nos->set_side(fix::SIDE_BUY);
    nos->set_order_qty(100.0);

    auto response = make_execution_report_fill(
        request, "TC-00001", "F-00001", 150.0, 100.0, 0.0, 100.0, 1.5);

    EXPECT_TRUE(response.has_execution_report());
    const auto& er = response.execution_report();
    EXPECT_EQ(er.order_id(), "TC-00001");
    EXPECT_EQ(er.exec_id(), "F-00001");
    EXPECT_EQ(er.exec_type(), fix::EXEC_TYPE_FILL);
    EXPECT_EQ(er.ord_status(), fix::ORD_STATUS_FILLED);
    EXPECT_EQ(er.last_px(), 150.0);
    EXPECT_EQ(er.last_qty(), 100.0);
    EXPECT_EQ(er.leaves_qty(), 0.0);
    EXPECT_EQ(er.cum_qty(), 100.0);
    EXPECT_EQ(er.commission(), 1.5);
}

TEST(Protocol, MakeExecutionReportPartialFill) {
    fix::FixMessage request;
    auto* nos = request.mutable_new_order_single();
    nos->set_cl_ord_id("ord-001");
    nos->mutable_instrument()->set_symbol("AAPL");
    nos->set_side(fix::SIDE_BUY);
    nos->set_order_qty(100.0);

    auto response = make_execution_report_fill(
        request, "TC-00001", "F-00001", 150.0, 60.0, 40.0, 60.0, 0.9);

    const auto& er = response.execution_report();
    EXPECT_EQ(er.exec_type(), fix::EXEC_TYPE_PARTIAL_FILL);
    EXPECT_EQ(er.ord_status(), fix::ORD_STATUS_PARTIALLY_FILLED);
    EXPECT_EQ(er.leaves_qty(), 40.0);
    EXPECT_EQ(er.cum_qty(), 60.0);
}

TEST(Protocol, MakeReject) {
    fix::FixMessage request;
    request.set_msg_seq_num("seq-001");
    request.set_sender_comp_id("CLIENT");
    request.mutable_new_order_single()->set_cl_ord_id("ord-001");

    auto response = make_reject(request, "Invalid order quantity");

    EXPECT_TRUE(response.has_reject());
    EXPECT_EQ(response.reject().ref_msg_seq_num(), "seq-001");
    EXPECT_EQ(response.reject().text(), "Invalid order quantity");
    EXPECT_EQ(response.sender_comp_id(), "TRADECORE");
    EXPECT_EQ(response.target_comp_id(), "CLIENT");
}

TEST(Protocol, MakeHeartbeatResponse) {
    fix::FixMessage request;
    request.set_sender_comp_id("CLIENT");
    auto* hb = request.mutable_heartbeat();
    hb->set_test_req_id("test-req-001");

    auto response = make_heartbeat_response(request);

    EXPECT_TRUE(response.has_heartbeat());
    EXPECT_EQ(response.heartbeat().test_req_id(), "test-req-001");
    EXPECT_EQ(response.sender_comp_id(), "TRADECORE");
}

TEST(Protocol, MakePositionReport) {
    fix::FixMessage request;
    request.set_sender_comp_id("CLIENT");
    auto* pr = request.mutable_position_request();
    pr->set_pos_req_id("pos-req-001");

    auto response = make_position_report(request, "rpt-001");

    EXPECT_TRUE(response.has_position_report());
    EXPECT_EQ(response.position_report().pos_req_id(), "pos-req-001");
    EXPECT_EQ(response.position_report().pos_rpt_id(), "rpt-001");
}
