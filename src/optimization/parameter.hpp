#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace tradecore::optimization {

// A single parameter with either a range (start, stop, step) or discrete choices.
struct Parameter {
    std::string name;
    std::vector<double> values;  // Pre-expanded set of values

    // Range-based parameter
    static Parameter range(const std::string& name, double start, double stop, double step) {
        if (step <= 0.0) throw std::invalid_argument("step must be > 0");
        Parameter p;
        p.name = name;
        for (double v = start; v <= stop + step * 0.001; v += step) {
            p.values.push_back(v);
        }
        return p;
    }

    // Discrete choices
    static Parameter choices(const std::string& name, const std::vector<double>& vals) {
        Parameter p;
        p.name = name;
        p.values = vals;
        return p;
    }
};

// A named set of parameter values (one specific configuration).
using ParameterSet = std::map<std::string, double>;

// Defines the full parameter space as a collection of parameters.
class ParameterSpace {
public:
    void add(const Parameter& param) {
        params_.push_back(param);
    }

    const std::vector<Parameter>& parameters() const { return params_; }

    // Generate all combinations (grid)
    std::vector<ParameterSet> grid() const {
        std::vector<ParameterSet> result;
        if (params_.empty()) return result;

        result.push_back({});
        for (const auto& param : params_) {
            std::vector<ParameterSet> expanded;
            for (const auto& existing : result) {
                for (double v : param.values) {
                    auto copy = existing;
                    copy[param.name] = v;
                    expanded.push_back(copy);
                }
            }
            result = std::move(expanded);
        }
        return result;
    }

    // Total number of combinations
    size_t total_combinations() const {
        if (params_.empty()) return 0;
        size_t total = 1;
        for (const auto& p : params_) {
            total *= p.values.size();
        }
        return total;
    }

private:
    std::vector<Parameter> params_;
};

}  // namespace tradecore::optimization
