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
    std::string bind_addr = "tcp://*:5555";
    if (argc > 1) bind_addr = argv[1];

    tradecore::matching::MatchingEngine matcher;
    tradecore::booking::BookKeeper book_keeper;
    tradecore::orders::OrderManager order_mgr(matcher, book_keeper);

    tradecore::messaging::ZmqServer server(bind_addr);
    g_server = &server;

    server.set_handler(
        [&](const std::string& client_id,
            const tradecore::messaging::Message& msg)
            -> std::vector<tradecore::messaging::Message> {

        std::cout << "[RECV] msg_type=" << msg.msg_type
                  << " from=" << client_id << std::endl;

        if (msg.msg_type == "new_order") {
            // If the order payload contains a market_price hint, use it
            if (msg.payload.contains("instrument")) {
                auto symbol = msg.payload["instrument"].value("symbol", "");
                // Use limit_price as market price hint for market orders
                if (msg.payload.value("order_type", "") == "market" &&
                    msg.payload.contains("limit_price") &&
                    !msg.payload["limit_price"].is_null()) {
                    matcher.update_market_price(symbol,
                        msg.payload["limit_price"].get<double>());
                }
            }
            return order_mgr.handle_new_order(msg);
        }

        if (msg.msg_type == "heartbeat") {
            return {tradecore::messaging::Message::make_response(
                msg, "heartbeat", {})};
        }

        if (msg.msg_type == "position_query") {
            auto positions = book_keeper.get_all_positions();
            nlohmann::json pos_array = nlohmann::json::array();
            for (const auto& pos : positions) {
                nlohmann::json p;
                p["symbol"] = pos.symbol;
                p["quantity"] = pos.quantity;
                p["avg_price"] = pos.avg_price;
                p["realized_pnl"] = pos.realized_pnl;
                pos_array.push_back(p);
            }
            nlohmann::json payload;
            payload["positions"] = pos_array;
            return {tradecore::messaging::Message::make_response(
                msg, "position_report", payload)};
        }

        return {tradecore::messaging::make_reject(
            msg, "unknown_msg_type",
            "Unrecognized message type: " + msg.msg_type)};
    });

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "tradecore listening on " << bind_addr << std::endl;
    server.run();

    std::cout << "\nShutdown. Trades booked: " << book_keeper.trade_count() << std::endl;
    return 0;
}
