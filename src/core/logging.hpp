#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <memory>
#include <string>
#include <vector>

namespace tradecore::core {

inline void init_logging(const std::string& log_level = "info",
                          const std::string& log_file = "logs/tradecore.log") {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file, 10 * 1024 * 1024, 3);  // 10MB, 3 rotations

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("tradecore", sinks.begin(), sinks.end());

    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    auto level = spdlog::level::from_str(log_level);
    logger->set_level(level);
    console_sink->set_level(level);
    file_sink->set_level(level);

    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(3));
}

}  // namespace tradecore::core
