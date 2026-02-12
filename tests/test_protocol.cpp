#include <gtest/gtest.h>
#include "messaging/protocol.hpp"

using namespace tradecore::messaging;

TEST(Protocol, MessageRoundtrip) {
    Message msg;
    msg.msg_type = "new_order";
    msg.msg_id = "test-123";
    msg.timestamp = "2024-01-01T00:00:00.000Z";
    msg.payload = {{"cl_ord_id", "ord-001"}, {"side", "buy"}};

    auto j = msg.to_json();
    auto restored = Message::from_json(j);

    EXPECT_EQ(restored.msg_type, "new_order");
    EXPECT_EQ(restored.msg_id, "test-123");
    EXPECT_EQ(restored.payload["cl_ord_id"], "ord-001");
    EXPECT_TRUE(restored.ref_msg_id.empty());
}

TEST(Protocol, MakeResponse) {
    Message request;
    request.msg_type = "new_order";
    request.msg_id = "req-001";
    request.timestamp = current_timestamp();
    request.payload = {{"cl_ord_id", "ord-001"}};

    auto response = Message::make_response(request, "order_ack", {{"status", "accepted"}});

    EXPECT_EQ(response.msg_type, "order_ack");
    EXPECT_EQ(response.ref_msg_id, "req-001");
    EXPECT_EQ(response.payload["status"], "accepted");
    EXPECT_FALSE(response.msg_id.empty());
}

TEST(Protocol, MakeOrderAck) {
    Message request;
    request.msg_type = "new_order";
    request.msg_id = "req-001";
    request.timestamp = current_timestamp();
    request.payload = {{"cl_ord_id", "ord-001"}};

    auto ack = make_order_ack(request, "TC-00001");

    EXPECT_EQ(ack.msg_type, "order_ack");
    EXPECT_EQ(ack.payload["order_id"], "TC-00001");
    EXPECT_EQ(ack.payload["cl_ord_id"], "ord-001");
    EXPECT_EQ(ack.payload["status"], "accepted");
}

TEST(Protocol, MakeFill) {
    Message request;
    request.msg_type = "new_order";
    request.msg_id = "req-001";
    request.timestamp = current_timestamp();
    request.payload = {{"cl_ord_id", "ord-001"}};

    auto fill = make_fill(request, "TC-00001", "F-00001", 150.0, 100.0, 0.0, 1.5);

    EXPECT_EQ(fill.msg_type, "fill");
    EXPECT_EQ(fill.payload["order_id"], "TC-00001");
    EXPECT_EQ(fill.payload["fill_price"], 150.0);
    EXPECT_EQ(fill.payload["fill_quantity"], 100.0);
    EXPECT_EQ(fill.payload["status"], "filled");
    EXPECT_EQ(fill.payload["commission"], 1.5);
}

TEST(Protocol, MakeReject) {
    Message request;
    request.msg_type = "new_order";
    request.msg_id = "req-001";
    request.timestamp = current_timestamp();
    request.payload = {{"cl_ord_id", "ord-001"}};

    auto reject = make_reject(request, "no_match", "No price available");

    EXPECT_EQ(reject.msg_type, "reject");
    EXPECT_EQ(reject.payload["reason"], "no_match");
    EXPECT_EQ(reject.payload["detail"], "No price available");
}

TEST(Protocol, NullRefMsgId) {
    Message msg;
    msg.msg_type = "heartbeat";
    msg.msg_id = "hb-001";
    msg.timestamp = current_timestamp();
    msg.payload = {};

    auto j = msg.to_json();
    EXPECT_TRUE(j["ref_msg_id"].is_null());
}
