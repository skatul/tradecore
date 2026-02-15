#include "messaging/zmq_server.hpp"

#include <spdlog/spdlog.h>

namespace tradecore::messaging {

ZmqServer::ZmqServer(const std::string& bind_address)
    : ctx_(1), socket_(ctx_, zmq::socket_type::router) {
    socket_.bind(bind_address);
}

ZmqServer::~ZmqServer() {
    socket_.close();
    ctx_.close();
}

void ZmqServer::set_handler(MessageHandler handler) {
    handler_ = std::move(handler);
}

bool ZmqServer::poll_once(int timeout_ms) {
    zmq::pollitem_t items[] = {{socket_, 0, ZMQ_POLLIN, 0}};
    zmq::poll(items, 1, std::chrono::milliseconds(timeout_ms));

    if (!(items[0].revents & ZMQ_POLLIN)) {
        return false;
    }

    // Receive identity frame
    zmq::message_t identity;
    (void)socket_.recv(identity, zmq::recv_flags::none);
    std::string client_id(static_cast<char*>(identity.data()), identity.size());

    // Receive empty delimiter
    zmq::message_t empty;
    (void)socket_.recv(empty, zmq::recv_flags::none);

    // Receive protobuf data frame
    zmq::message_t data;
    (void)socket_.recv(data, zmq::recv_flags::none);

    try {
        auto msg = deserialize(data.data(), data.size());

        if (handler_) {
            auto responses = handler_(client_id, msg);
            for (const auto& response : responses) {
                std::string resp_bytes = serialize(response);

                socket_.send(zmq::buffer(client_id), zmq::send_flags::sndmore);
                socket_.send(zmq::message_t{}, zmq::send_flags::sndmore);
                socket_.send(zmq::buffer(resp_bytes), zmq::send_flags::none);
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("Error processing message: {}", e.what());
    }

    return true;
}

void ZmqServer::run() {
    running_ = true;
    spdlog::info("tradecore server running...");
    while (running_) {
        poll_once(100);
    }
}

void ZmqServer::stop() {
    running_ = false;
}

}  // namespace tradecore::messaging
