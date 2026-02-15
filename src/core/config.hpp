#pragma once

#include <string>

namespace tradecore::core {

struct ServerConfig {
    std::string bind_address = "tcp://*:5555";
    int poll_timeout_ms = 100;
};

struct MatchingConfig {
    double spread_bps = 10.0;
    int depth_levels = 5;
    double qty_per_level = 1000.0;
    bool auto_seed_book = true;
};

struct CommissionConfig {
    double rate = 0.001;
    double min = 0.0;
};

struct LoggingConfig {
    std::string level = "info";
    std::string file = "logs/tradecore.log";
};

struct MetricsConfig {
    int report_interval_s = 60;
    bool enabled = true;
};

struct Config {
    ServerConfig server;
    MatchingConfig matching;
    CommissionConfig commission;
    LoggingConfig logging;
    MetricsConfig metrics;

    static Config load(const std::string& path);
    static Config load_with_overrides(const std::string& path, int argc, char* argv[]);
    static Config defaults();
};

}  // namespace tradecore::core
