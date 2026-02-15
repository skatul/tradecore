#include <csignal>
#include <string>

#include <spdlog/spdlog.h>

#include "booking/book_keeper.hpp"
#include "core/config.hpp"
#include "core/logging.hpp"
#include "core/metrics.hpp"
#include "matching/matching_engine.hpp"
#include "messaging/zmq_server.hpp"
#include "orders/order_manager.hpp"

static tradecore::messaging::ZmqServer* g_server = nullptr;

void signal_handler(int) {
    if (g_server) g_server->stop();
}

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Find config file from --config= arg, or use default path
    std::string config_path = "config/default.toml";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--config=", 0) == 0) {
            config_path = arg.substr(9);
        }
    }

    auto cfg = tradecore::core::Config::load_with_overrides(config_path, argc, argv);

    tradecore::core::init_logging(cfg.logging.level, cfg.logging.file);

    tradecore::matching::MatchingEngine matcher;
    tradecore::booking::BookKeeper book_keeper;
    tradecore::orders::OrderManager order_mgr(matcher, book_keeper, cfg.commission.rate);

    auto& metrics = tradecore::core::Metrics::instance();

    tradecore::messaging::ZmqServer server(cfg.server.bind_address);
    g_server = &server;

    server.set_handler(
        [&](const std::string& client_id,
            const fix::FixMessage& msg)
            -> std::vector<fix::FixMessage> {

        metrics.messages_in++;

        if (msg.has_new_order_single()) {
            const auto& nos = msg.new_order_single();
            spdlog::info("[RECV] NewOrderSingle from={} cl_ord_id={} symbol={}",
                         client_id, nos.cl_ord_id(), nos.instrument().symbol());

            // Extract market price hint
            if (nos.market_price() > 0.0) {
                matcher.update_market_price(
                    nos.instrument().symbol(), nos.market_price());
            }

            metrics.orders_received++;
            tradecore::core::ScopedTimer timer;
            auto responses = order_mgr.handle_new_order(msg);

            for (const auto& r : responses) {
                metrics.messages_out++;
                if (r.has_execution_report()) {
                    const auto& er = r.execution_report();
                    if (er.exec_type() == fix::EXEC_TYPE_FILL) {
                        metrics.orders_filled++;
                        metrics.add_notional(er.last_px() * er.last_qty());
                    } else if (er.exec_type() == fix::EXEC_TYPE_PARTIAL_FILL) {
                        metrics.partial_fills++;
                        metrics.add_notional(er.last_px() * er.last_qty());
                    }
                } else if (r.has_reject()) {
                    metrics.orders_rejected++;
                }
            }

            return responses;
        }

        if (msg.has_order_cancel_request()) {
            const auto& cancel = msg.order_cancel_request();
            spdlog::info("[RECV] OrderCancelRequest from={} orig_cl_ord_id={}",
                         client_id, cancel.orig_cl_ord_id());

            auto responses = order_mgr.handle_cancel_request(msg);
            for (const auto& r : responses) {
                metrics.messages_out++;
                if (r.has_execution_report() &&
                    r.execution_report().exec_type() == fix::EXEC_TYPE_CANCELLED) {
                    metrics.orders_cancelled++;
                }
            }
            return responses;
        }

        if (msg.has_heartbeat()) {
            spdlog::debug("[RECV] Heartbeat from={}", client_id);
            metrics.messages_out++;
            return {tradecore::messaging::make_heartbeat_response(msg)};
        }

        if (msg.has_position_request()) {
            spdlog::info("[RECV] PositionRequest from={}", client_id);
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

            metrics.messages_out++;
            return {response};
        }

        spdlog::warn("[RECV] Unknown message from={}", client_id);
        metrics.messages_out++;
        return {tradecore::messaging::make_reject(msg, "Unknown message type")};
    });

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    spdlog::info("tradecore listening on {} (FIX/protobuf)", cfg.server.bind_address);
    server.run();

    spdlog::info("Shutdown. Trades booked: {}", book_keeper.trade_count());
    spdlog::info("{}", metrics.to_string());
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
