#pragma once

#include <nlohmann/json.hpp>
#include <chrono>
#include <random>
#include <sstream>
#include <string>

namespace tradecore::messaging {

using json = nlohmann::json;

inline std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;

    std::ostringstream ss;
    ss << std::hex;
    ss << dis(gen) << "-" << (dis(gen) & 0xFFFF) << "-"
       << (dis(gen) & 0xFFFF) << "-" << (dis(gen) & 0xFFFF) << "-"
       << dis(gen) << dis(gen);
    return ss.str();
}

inline std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    return ss.str();
}

struct Message {
    std::string msg_type;
    std::string msg_id;
    std::string timestamp;
    std::string ref_msg_id;
    json payload;

    static Message from_json(const json& j) {
        Message m;
        m.msg_type = j.at("msg_type").get<std::string>();
        m.msg_id = j.at("msg_id").get<std::string>();
        m.timestamp = j.at("timestamp").get<std::string>();
        if (j.contains("ref_msg_id") && !j["ref_msg_id"].is_null()) {
            m.ref_msg_id = j["ref_msg_id"].get<std::string>();
        }
        m.payload = j.at("payload");
        return m;
    }

    json to_json() const {
        json j;
        j["msg_type"] = msg_type;
        j["msg_id"] = msg_id;
        j["timestamp"] = timestamp;
        j["ref_msg_id"] = ref_msg_id.empty() ? json(nullptr) : json(ref_msg_id);
        j["payload"] = payload;
        return j;
    }

    static Message make_response(const Message& request,
                                 const std::string& type,
                                 json payload) {
        Message m;
        m.msg_type = type;
        m.msg_id = generate_uuid();
        m.timestamp = current_timestamp();
        m.ref_msg_id = request.msg_id;
        m.payload = std::move(payload);
        return m;
    }
};

Message make_order_ack(const Message& request, const std::string& order_id);
Message make_fill(const Message& request, const std::string& order_id,
                  const std::string& fill_id, double fill_price,
                  double fill_qty, double remaining_qty, double commission);
Message make_reject(const Message& request, const std::string& reason,
                    const std::string& detail);

}  // namespace tradecore::messaging
