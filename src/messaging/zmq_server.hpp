#pragma once

#include <zmq.hpp>
#include <functional>
#include <string>
#include <vector>

#include <fix_messages.pb.h>
#include "messaging/protocol.hpp"

namespace tradecore::messaging {

class ZmqServer {
public:
    explicit ZmqServer(const std::string& bind_address = "tcp://*:5555");
    ~ZmqServer();

    ZmqServer(const ZmqServer&) = delete;
    ZmqServer& operator=(const ZmqServer&) = delete;

    using MessageHandler = std::function<std::vector<fix::FixMessage>(
        const std::string& client_id, const fix::FixMessage& msg)>;

    void set_handler(MessageHandler handler);

    bool poll_once(int timeout_ms = 100);

    void run();
    void stop();

private:
    zmq::context_t ctx_;
    zmq::socket_t socket_;
    MessageHandler handler_;
    bool running_ = false;
};

}  // namespace tradecore::messaging
