#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace tradecore::core {

class Metrics {
public:
    static Metrics& instance() {
        static Metrics m;
        return m;
    }

    // Counters
    std::atomic<uint64_t> orders_received{0};
    std::atomic<uint64_t> orders_filled{0};
    std::atomic<uint64_t> orders_rejected{0};
    std::atomic<uint64_t> orders_cancelled{0};
    std::atomic<uint64_t> partial_fills{0};
    std::atomic<uint64_t> messages_in{0};
    std::atomic<uint64_t> messages_out{0};
    std::atomic<uint64_t> total_notional_x100{0};  // store as integer cents for atomicity

    void add_notional(double notional) {
        total_notional_x100.fetch_add(
            static_cast<uint64_t>(notional * 100.0), std::memory_order_relaxed);
    }

    double get_notional() const {
        return static_cast<double>(total_notional_x100.load(std::memory_order_relaxed)) / 100.0;
    }

    // Latency tracking (ring buffer)
    void record_latency_us(uint64_t micros) {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        latency_samples_[latency_idx_ % kMaxSamples] = micros;
        ++latency_idx_;
    }

    struct LatencyStats {
        uint64_t avg_us = 0;
        uint64_t p99_us = 0;
        size_t count = 0;
    };

    LatencyStats latency_stats() const {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        LatencyStats stats;
        size_t n = std::min(latency_idx_, kMaxSamples);
        if (n == 0) return stats;

        std::vector<uint64_t> sorted;
        sorted.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            sorted.push_back(latency_samples_[i]);
        }
        std::sort(sorted.begin(), sorted.end());

        uint64_t sum = 0;
        for (auto v : sorted) sum += v;
        stats.avg_us = sum / n;
        stats.p99_us = sorted[static_cast<size_t>(n * 0.99)];
        stats.count = n;

        return stats;
    }

    std::string to_string() const {
        auto lat = latency_stats();
        std::ostringstream ss;
        ss << "Metrics {"
           << " orders_received=" << orders_received.load()
           << " orders_filled=" << orders_filled.load()
           << " orders_rejected=" << orders_rejected.load()
           << " orders_cancelled=" << orders_cancelled.load()
           << " partial_fills=" << partial_fills.load()
           << " messages_in=" << messages_in.load()
           << " messages_out=" << messages_out.load()
           << " total_notional=$" << get_notional()
           << " latency_avg=" << lat.avg_us << "us"
           << " latency_p99=" << lat.p99_us << "us"
           << " latency_samples=" << lat.count
           << " }";
        return ss.str();
    }

    void reset() {
        orders_received = 0;
        orders_filled = 0;
        orders_rejected = 0;
        orders_cancelled = 0;
        partial_fills = 0;
        messages_in = 0;
        messages_out = 0;
        total_notional_x100 = 0;
        std::lock_guard<std::mutex> lock(latency_mutex_);
        latency_idx_ = 0;
    }

private:
    Metrics() = default;

    static constexpr size_t kMaxSamples = 10000;
    mutable std::mutex latency_mutex_;
    uint64_t latency_samples_[kMaxSamples] = {};
    size_t latency_idx_ = 0;
};

/// RAII timer that records elapsed time to Metrics on destruction.
class ScopedTimer {
public:
    ScopedTimer() : start_(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        Metrics::instance().record_latency_us(static_cast<uint64_t>(us));
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    std::chrono::steady_clock::time_point start_;
};

}  // namespace tradecore::core
