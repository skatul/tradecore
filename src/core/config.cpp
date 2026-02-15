#include "core/config.hpp"

#include <toml++/toml.hpp>
#include <spdlog/spdlog.h>

#include <cstring>
#include <filesystem>

namespace tradecore::core {

Config Config::defaults() {
    return Config{};
}

Config Config::load(const std::string& path) {
    Config cfg;

    if (!std::filesystem::exists(path)) {
        spdlog::warn("Config file not found: {}, using defaults", path);
        return cfg;
    }

    try {
        auto tbl = toml::parse_file(path);

        // [server]
        if (auto server = tbl["server"].as_table()) {
            if (auto v = (*server)["bind_address"].value<std::string>())
                cfg.server.bind_address = *v;
            if (auto v = (*server)["poll_timeout_ms"].value<int>())
                cfg.server.poll_timeout_ms = *v;
        }

        // [matching]
        if (auto matching = tbl["matching"].as_table()) {
            if (auto v = (*matching)["spread_bps"].value<double>())
                cfg.matching.spread_bps = *v;
            if (auto v = (*matching)["depth_levels"].value<int>())
                cfg.matching.depth_levels = *v;
            if (auto v = (*matching)["qty_per_level"].value<double>())
                cfg.matching.qty_per_level = *v;
            if (auto v = (*matching)["auto_seed_book"].value<bool>())
                cfg.matching.auto_seed_book = *v;
        }

        // [commission]
        if (auto commission = tbl["commission"].as_table()) {
            if (auto v = (*commission)["rate"].value<double>())
                cfg.commission.rate = *v;
            if (auto v = (*commission)["min"].value<double>())
                cfg.commission.min = *v;
        }

        // [logging]
        if (auto logging = tbl["logging"].as_table()) {
            if (auto v = (*logging)["level"].value<std::string>())
                cfg.logging.level = *v;
            if (auto v = (*logging)["file"].value<std::string>())
                cfg.logging.file = *v;
        }

        // [metrics]
        if (auto metrics = tbl["metrics"].as_table()) {
            if (auto v = (*metrics)["report_interval_s"].value<int>())
                cfg.metrics.report_interval_s = *v;
            if (auto v = (*metrics)["enabled"].value<bool>())
                cfg.metrics.enabled = *v;
        }
    } catch (const toml::parse_error& e) {
        spdlog::error("Failed to parse config: {}", e.what());
    }

    return cfg;
}

Config Config::load_with_overrides(const std::string& path, int argc, char* argv[]) {
    auto cfg = load(path);

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.rfind("--bind=", 0) == 0) {
            cfg.server.bind_address = arg.substr(7);
        } else if (arg.rfind("--log-level=", 0) == 0) {
            cfg.logging.level = arg.substr(12);
        } else if (arg.rfind("--commission-rate=", 0) == 0) {
            cfg.commission.rate = std::stod(arg.substr(18));
        } else if (arg.rfind("--spread-bps=", 0) == 0) {
            cfg.matching.spread_bps = std::stod(arg.substr(13));
        } else if (arg.rfind("--config=", 0) == 0) {
            // already handled via path
        }
    }

    return cfg;
}

}  // namespace tradecore::core
