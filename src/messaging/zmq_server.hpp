#pragma once

#include <zmq.hpp>
#include <functional>
#include <string>
#include <vector>

#include "messaging/protocol.hpp"

namespace tradecore::messaging {

class ZmqServer {
public:
    explicit ZmqServer(const std::string& bind_address = "tcp://*:5555");
    ~ZmqServer();

    ZmqServer(const ZmqServer&) = delete;
    ZmqServer& operator=(const ZmqServer&) = delete;

    using MessageHandler = std::function<std::vector<Message>(
        const std::string& client_id, const Message& msg)>;

    void set_handler(MessageHandler handler);

    /// Poll for one message, dispatch to handler, send responses.
    /// Returns true if a message was processed.
    bool poll_once(int timeout_ms = 100);

    /// Run the event loop (blocking).
    void run();
    void stop();

private:
    zmq::context_t ctx_;
    zmq::socket_t socket_;
    MessageHandler handler_;
    bool running_ = false;
};

}  // namespace tradecore::messaging
