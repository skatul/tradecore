#include <gtest/gtest.h>
#include <zmq.hpp>
#include <thread>
#include <chrono>

#include "booking/book_keeper.hpp"
#include "matching/matching_engine.hpp"
#include "messaging/protocol.hpp"
#include "messaging/zmq_server.hpp"
#include "orders/order_manager.hpp"

using namespace tradecore;

class IntegrationTest : public ::testing::Test {
protected:
    static constexpr const char* BIND_ADDR = "tcp://127.0.0.1:5558";

    matching::MatchingEngine matcher;
    booking::BookKeeper book_keeper;
    std::unique_ptr<orders::OrderManager> order_mgr;
    std::unique_ptr<messaging::ZmqServer> server;
    std::thread server_thread;

    void SetUp() override {
        order_mgr = std::make_unique<orders::OrderManager>(matcher, book_keeper);
        server = std::make_unique<messaging::ZmqServer>(BIND_ADDR);

        server->set_handler(
            [&](const std::string& client_id,
                const fix::FixMessage& msg)
                -> std::vector<fix::FixMessage> {

            if (msg.has_new_order_single()) {
                const auto& nos = msg.new_order_single();
                if (nos.market_price() > 0.0) {
                    matcher.update_market_price(
                        nos.instrument().symbol(), nos.market_price());
                }
                return order_mgr->handle_new_order(msg);
            }
            if (msg.has_heartbeat()) {
                return {messaging::make_heartbeat_response(msg)};
            }
            if (msg.has_position_request()) {
                auto response = messaging::make_position_report(
                    msg, messaging::generate_uuid());
                auto* pr = response.mutable_position_report();
                for (const auto& pos : book_keeper.get_all_positions()) {
                    auto* entry = pr->add_positions();
                    entry->mutable_instrument()->set_symbol(pos.symbol);
                    entry->mutable_instrument()->set_security_type(
                        fix::SECURITY_TYPE_COMMON_STOCK);
                    if (pos.quantity >= 0) {
                        entry->set_long_qty(pos.quantity);
                    } else {
                        entry->set_short_qty(-pos.quantity);
                    }
                    entry->set_avg_price(pos.avg_price);
                    entry->set_realized_pnl(pos.realized_pnl);
                }
                return {response};
            }
            return {messaging::make_reject(msg, "Unknown message type")};
        });

        server_thread = std::thread([this] { server->run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    void TearDown() override {
        server->stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }

    // Send a protobuf message and receive a response using a raw DEALER socket
    fix::FixMessage send_and_recv(const fix::FixMessage& msg, int timeout_ms = 2000) {
        zmq::context_t ctx(1);
        zmq::socket_t sock(ctx, zmq::socket_type::dealer);
        sock.set(zmq::sockopt::routing_id, "test-cpp-client");
        sock.connect(BIND_ADDR);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        std::string data = messaging::serialize(msg);
        sock.send(zmq::message_t{}, zmq::send_flags::sndmore);
        sock.send(zmq::buffer(data), zmq::send_flags::none);

        zmq::pollitem_t items[] = {{sock, 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, std::chrono::milliseconds(timeout_ms));

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t empty, response;
            (void)sock.recv(empty, zmq::recv_flags::none);
            (void)sock.recv(response, zmq::recv_flags::none);

            sock.close();
            ctx.close();
            return messaging::deserialize(response.data(), response.size());
        }

        sock.close();
        ctx.close();
        return {};
    }

    fix::FixMessage make_order_msg(const std::string& cl_ord_id,
                                    const std::string& symbol = "AAPL",
                                    fix::Side side = fix::SIDE_BUY,
                                    double qty = 100.0,
                                    double market_price = 150.0) {
        fix::FixMessage msg;
        msg.set_sender_comp_id("TEST_CLIENT");
        msg.set_msg_seq_num(cl_ord_id + "-seq");
        msg.set_sending_time(messaging::current_timestamp());

        auto* nos = msg.mutable_new_order_single();
        nos->set_cl_ord_id(cl_ord_id);
        nos->mutable_instrument()->set_symbol(symbol);
        nos->mutable_instrument()->set_security_type(fix::SECURITY_TYPE_COMMON_STOCK);
        nos->set_side(side);
        nos->set_order_qty(qty);
        nos->set_ord_type(fix::ORD_TYPE_MARKET);
        nos->set_time_in_force(fix::TIF_DAY);
        nos->set_text("test");
        nos->set_market_price(market_price);

        return msg;
    }
};

TEST_F(IntegrationTest, MarketOrderFillOverZmq) {
    auto response = send_and_recv(make_order_msg("zmq-001"));

    EXPECT_TRUE(response.has_execution_report());
    const auto& er = response.execution_report();
    EXPECT_EQ(er.cl_ord_id(), "zmq-001");
    EXPECT_EQ(er.last_px(), 150.0);
    EXPECT_EQ(er.last_qty(), 100.0);
    EXPECT_EQ(er.ord_status(), fix::ORD_STATUS_FILLED);
}

TEST_F(IntegrationTest, SellOrderFillOverZmq) {
    send_and_recv(make_order_msg("zmq-buy", "MSFT", fix::SIDE_BUY, 50.0, 300.0));

    auto response = send_and_recv(
        make_order_msg("zmq-sell", "MSFT", fix::SIDE_SELL, 50.0, 310.0));

    EXPECT_TRUE(response.has_execution_report());
    EXPECT_EQ(response.execution_report().last_px(), 310.0);
}

TEST_F(IntegrationTest, RejectBadOrderOverZmq) {
    auto bad_order = make_order_msg("zmq-bad", "AAPL", fix::SIDE_BUY, -10.0, 150.0);
    auto response = send_and_recv(bad_order);

    EXPECT_TRUE(response.has_reject());
    EXPECT_FALSE(response.reject().text().empty());
}

TEST_F(IntegrationTest, HeartbeatOverZmq) {
    fix::FixMessage msg;
    msg.set_sender_comp_id("TEST_CLIENT");
    msg.set_msg_seq_num("hb-seq-001");
    msg.set_sending_time(messaging::current_timestamp());
    msg.mutable_heartbeat()->set_test_req_id("test-req-001");

    auto response = send_and_recv(msg);

    EXPECT_TRUE(response.has_heartbeat());
    EXPECT_EQ(response.heartbeat().test_req_id(), "test-req-001");
}

TEST_F(IntegrationTest, PositionQueryOverZmq) {
    send_and_recv(make_order_msg("zmq-pos-001", "TSLA", fix::SIDE_BUY, 25.0, 250.0));

    fix::FixMessage query;
    query.set_sender_comp_id("TEST_CLIENT");
    query.set_msg_seq_num("pq-seq-001");
    query.set_sending_time(messaging::current_timestamp());
    query.mutable_position_request()->set_pos_req_id("pos-req-001");

    auto response = send_and_recv(query);

    EXPECT_TRUE(response.has_position_report());
    EXPECT_EQ(response.position_report().pos_req_id(), "pos-req-001");

    bool found = false;
    for (const auto& entry : response.position_report().positions()) {
        if (entry.instrument().symbol() == "TSLA") {
            EXPECT_EQ(entry.long_qty(), 25.0);
            EXPECT_EQ(entry.avg_price(), 250.0);
            found = true;
        }
    }
    EXPECT_TRUE(found) << "TSLA position not found";
}

TEST_F(IntegrationTest, MultipleOrdersBookedCorrectly) {
    send_and_recv(make_order_msg("zmq-m1", "NVDA", fix::SIDE_BUY, 100.0, 500.0));
    send_and_recv(make_order_msg("zmq-m2", "NVDA", fix::SIDE_BUY, 50.0, 510.0));

    EXPECT_EQ(book_keeper.trade_count(), 2);

    auto* pos = book_keeper.get_position("NVDA");
    ASSERT_NE(pos, nullptr);
    EXPECT_EQ(pos->quantity, 150.0);
    EXPECT_NEAR(pos->avg_price, 503.333, 0.01);
}
