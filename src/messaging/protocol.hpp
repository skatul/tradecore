#pragma once

#include <fix_messages.pb.h>
#include <chrono>
#include <random>
#include <sstream>
#include <string>
#include <iomanip>

namespace tradecore::messaging {

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
    ss << std::put_time(std::gmtime(&time_t), "%Y%m%d-%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Convenience builders for ExecutionReport
fix::FixMessage make_execution_report_new(
    const fix::FixMessage& request,
    const std::string& order_id);

fix::FixMessage make_execution_report_fill(
    const fix::FixMessage& request,
    const std::string& order_id,
    const std::string& exec_id,
    double last_px,
    double last_qty,
    double leaves_qty,
    double cum_qty,
    double commission);

fix::FixMessage make_reject(
    const fix::FixMessage& request,
    const std::string& reason);

fix::FixMessage make_heartbeat_response(const fix::FixMessage& request);

fix::FixMessage make_position_report(
    const fix::FixMessage& request,
    const std::string& rpt_id);

// Serialize/deserialize helpers
std::string serialize(const fix::FixMessage& msg);
fix::FixMessage deserialize(const std::string& data);
fix::FixMessage deserialize(const void* data, size_t size);

}  // namespace tradecore::messaging
