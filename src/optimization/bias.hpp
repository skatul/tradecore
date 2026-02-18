#pragma once

#include "walk_forward.hpp"

#include <cmath>
#include <string>
#include <vector>

namespace tradecore::optimization {

struct BiasReport {
    bool overfitting_detected = false;
    bool parameter_instability = false;
    bool survivorship_bias_warning = false;
    bool look_ahead_warning = false;
    double is_oos_degradation = 0.0;  // (IS - OOS) / IS
    std::vector<std::string> warnings;
};

class BiasDetector {
public:
    // Overfitting detection: compares in-sample vs out-of-sample performance.
    // If OOS degrades more than threshold (e.g., 0.5 = 50%) from IS, flag overfitting.
    BiasReport analyze(const WalkForwardResult& wf_result,
                       double degradation_threshold = 0.5) const {
        BiasReport report;

        // IS vs OOS degradation
        if (std::abs(wf_result.avg_is_score) > 1e-10) {
            report.is_oos_degradation =
                (wf_result.avg_is_score - wf_result.avg_oos_score) / std::abs(wf_result.avg_is_score);
        }

        if (report.is_oos_degradation > degradation_threshold) {
            report.overfitting_detected = true;
            report.warnings.push_back(
                "Overfitting detected: IS to OOS degradation of " +
                std::to_string(report.is_oos_degradation * 100.0) + "%");
        }

        // Parameter stability: check if best params vary significantly across folds
        if (wf_result.folds.size() >= 2) {
            report.parameter_instability = check_parameter_stability(wf_result);
            if (report.parameter_instability) {
                report.warnings.push_back(
                    "Parameter instability: optimal parameters vary significantly across folds");
            }
        }

        return report;
    }

    // Survivorship bias warning: check if the dataset might exclude delisted instruments.
    void check_survivorship(BiasReport& report, int num_instruments, int num_delisted) const {
        if (num_delisted > 0 && num_instruments > 0) {
            double delisted_pct = static_cast<double>(num_delisted) / num_instruments;
            if (delisted_pct > 0.1) {
                report.survivorship_bias_warning = true;
                report.warnings.push_back(
                    "Survivorship bias warning: " +
                    std::to_string(static_cast<int>(delisted_pct * 100)) +
                    "% of instruments were delisted");
            }
        }
    }

    // Look-ahead bias: flag if the code uses future data patterns.
    // This is a heuristic check - actual look-ahead detection requires code analysis.
    void check_look_ahead(BiasReport& report, const std::vector<std::string>& patterns) const {
        for (const auto& pattern : patterns) {
            if (pattern.find("future") != std::string::npos ||
                pattern.find("next_bar") != std::string::npos ||
                pattern.find("peek") != std::string::npos ||
                pattern.find("lookahead") != std::string::npos) {
                report.look_ahead_warning = true;
                report.warnings.push_back("Look-ahead bias warning: suspicious pattern '" +
                                          pattern + "'");
            }
        }
    }

private:
    bool check_parameter_stability(const WalkForwardResult& wf_result) const {
        if (wf_result.folds.empty()) return false;

        // Collect parameter names from first fold
        const auto& first_params = wf_result.folds[0].best_params;

        for (const auto& [name, _] : first_params) {
            // Check if this parameter has the same value in all folds
            double min_val = first_params.at(name);
            double max_val = min_val;
            for (const auto& fold : wf_result.folds) {
                auto it = fold.best_params.find(name);
                if (it != fold.best_params.end()) {
                    min_val = std::min(min_val, it->second);
                    max_val = std::max(max_val, it->second);
                }
            }
            // If range of best values is > 50% of max, flag instability
            if (max_val > 1e-10 && (max_val - min_val) / max_val > 0.5) {
                return true;
            }
        }
        return false;
    }
};

}  // namespace tradecore::optimization
