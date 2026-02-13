#include <csignal>
#include <iostream>
#include <string>

#include "booking/book_keeper.hpp"
#include "matching/matching_engine.hpp"
#include "messaging/zmq_server.hpp"
#include "orders/order_manager.hpp"

static tradecore::messaging::ZmqServer* g_server = nullptr;

void signal_handler(int) {
    if (g_server) g_server->stop();
}

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::string bind_addr = "tcp://*:5555";
    if (argc > 1) bind_addr = argv[1];

    tradecore::matching::MatchingEngine matcher;
    tradecore::booking::BookKeeper book_keeper;
    tradecore::orders::OrderManager order_mgr(matcher, book_keeper);

    tradecore::messaging::ZmqServer server(bind_addr);
    g_server = &server;

    server.set_handler(
        [&](const std::string& client_id,
            const fix::FixMessage& msg)
            -> std::vector<fix::FixMessage> {

        if (msg.has_new_order_single()) {
            const auto& nos = msg.new_order_single();
            std::cout << "[RECV] NewOrderSingle from=" << client_id
                      << " cl_ord_id=" << nos.cl_ord_id()
                      << " symbol=" << nos.instrument().symbol() << std::endl;

            // Extract market price hint
            if (nos.market_price() > 0.0) {
                matcher.update_market_price(
                    nos.instrument().symbol(), nos.market_price());
            }

            return order_mgr.handle_new_order(msg);
        }

        if (msg.has_heartbeat()) {
            std::cout << "[RECV] Heartbeat from=" << client_id << std::endl;
            return {tradecore::messaging::make_heartbeat_response(msg)};
        }

        if (msg.has_position_request()) {
            std::cout << "[RECV] PositionRequest from=" << client_id << std::endl;
            auto response = tradecore::messaging::make_position_report(
                msg, tradecore::messaging::generate_uuid());

            auto* pr = response.mutable_position_report();
            for (const auto& pos : book_keeper.get_all_positions()) {
                auto* entry = pr->add_positions();
                entry->mutable_instrument()->set_symbol(pos.symbol);
                entry->mutable_instrument()->set_security_type(fix::SECURITY_TYPE_COMMON_STOCK);
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

        std::cout << "[RECV] Unknown message from=" << client_id << std::endl;
        return {tradecore::messaging::make_reject(msg, "Unknown message type")};
    });

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "tradecore listening on " << bind_addr << " (FIX/protobuf)" << std::endl;
    server.run();

    std::cout << "\nShutdown. Trades booked: " << book_keeper.trade_count() << std::endl;
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
