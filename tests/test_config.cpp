#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "core/config.hpp"

using namespace tradecore::core;

class ConfigTest : public ::testing::Test {
protected:
    std::string temp_dir_;

    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() / "tradecore_test_config";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(temp_dir_);
    }

    std::string write_toml(const std::string& content) {
        auto path = temp_dir_ + "/test.toml";
        std::ofstream f(path);
        f << content;
        return path;
    }
};

TEST_F(ConfigTest, DefaultValues) {
    auto cfg = Config::defaults();
    EXPECT_EQ(cfg.server.bind_address, "tcp://*:5555");
    EXPECT_EQ(cfg.server.poll_timeout_ms, 100);
    EXPECT_EQ(cfg.matching.spread_bps, 10.0);
    EXPECT_EQ(cfg.commission.rate, 0.001);
    EXPECT_EQ(cfg.logging.level, "info");
    EXPECT_TRUE(cfg.metrics.enabled);
}

TEST_F(ConfigTest, LoadFromFile) {
    auto path = write_toml(R"(
[server]
bind_address = "tcp://*:6666"
poll_timeout_ms = 200

[commission]
rate = 0.002

[logging]
level = "debug"
)");

    auto cfg = Config::load(path);
    EXPECT_EQ(cfg.server.bind_address, "tcp://*:6666");
    EXPECT_EQ(cfg.server.poll_timeout_ms, 200);
    EXPECT_EQ(cfg.commission.rate, 0.002);
    EXPECT_EQ(cfg.logging.level, "debug");
    // Unset values use defaults
    EXPECT_EQ(cfg.matching.spread_bps, 10.0);
    EXPECT_TRUE(cfg.metrics.enabled);
}

TEST_F(ConfigTest, CLIOverrides) {
    auto path = write_toml(R"(
[server]
bind_address = "tcp://*:5555"

[commission]
rate = 0.001
)");

    const char* argv[] = {"tradecore", "--bind=tcp://*:7777", "--commission-rate=0.005", "--log-level=warn"};
    auto cfg = Config::load_with_overrides(path, 4, const_cast<char**>(argv));

    EXPECT_EQ(cfg.server.bind_address, "tcp://*:7777");
    EXPECT_EQ(cfg.commission.rate, 0.005);
    EXPECT_EQ(cfg.logging.level, "warn");
}

TEST_F(ConfigTest, MissingFileFallback) {
    auto cfg = Config::load("/nonexistent/path.toml");
    // Should use defaults
    EXPECT_EQ(cfg.server.bind_address, "tcp://*:5555");
    EXPECT_EQ(cfg.commission.rate, 0.001);
}

TEST_F(ConfigTest, PartialToml) {
    auto path = write_toml(R"(
[matching]
spread_bps = 20.0
depth_levels = 10
)");

    auto cfg = Config::load(path);
    EXPECT_EQ(cfg.matching.spread_bps, 20.0);
    EXPECT_EQ(cfg.matching.depth_levels, 10);
    // Other sections use defaults
    EXPECT_EQ(cfg.server.bind_address, "tcp://*:5555");
    EXPECT_EQ(cfg.commission.rate, 0.001);
}
