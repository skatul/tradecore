#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace tradecore::instrument {

enum class AssetClass { Equity, Future, Option, FX };

inline AssetClass asset_class_from_string(const std::string& s) {
    if (s == "equity") return AssetClass::Equity;
    if (s == "future") return AssetClass::Future;
    if (s == "option") return AssetClass::Option;
    if (s == "fx") return AssetClass::FX;
    throw std::invalid_argument("Unknown asset class: " + s);
}

inline std::string asset_class_to_string(AssetClass ac) {
    switch (ac) {
        case AssetClass::Equity: return "equity";
        case AssetClass::Future: return "future";
        case AssetClass::Option: return "option";
        case AssetClass::FX:     return "fx";
    }
    return "unknown";
}

struct Instrument {
    std::string symbol;
    AssetClass asset_class = AssetClass::Equity;
    std::string exchange;
    std::string currency = "USD";

    // Futures
    std::optional<std::string> expiry;
    double contract_size = 1.0;
    double tick_size = 0.01;

    // Options
    std::optional<std::string> underlying;
    std::optional<double> strike;
    std::optional<std::string> option_type;
    std::optional<std::string> expiration;

    // FX
    std::optional<std::string> base_currency;
    std::optional<std::string> quote_currency;
    std::optional<double> pip_size;

    static Instrument from_json(const nlohmann::json& j) {
        Instrument inst;
        inst.symbol = j.at("symbol").get<std::string>();
        inst.asset_class = asset_class_from_string(j.at("asset_class").get<std::string>());

        if (j.contains("exchange")) inst.exchange = j["exchange"].get<std::string>();
        if (j.contains("currency")) inst.currency = j["currency"].get<std::string>();
        if (j.contains("expiry")) inst.expiry = j["expiry"].get<std::string>();
        if (j.contains("contract_size")) inst.contract_size = j["contract_size"].get<double>();
        if (j.contains("tick_size")) inst.tick_size = j["tick_size"].get<double>();
        if (j.contains("underlying")) inst.underlying = j["underlying"].get<std::string>();
        if (j.contains("strike")) inst.strike = j["strike"].get<double>();
        if (j.contains("option_type")) inst.option_type = j["option_type"].get<std::string>();
        if (j.contains("expiration")) inst.expiration = j["expiration"].get<std::string>();
        if (j.contains("base_currency")) inst.base_currency = j["base_currency"].get<std::string>();
        if (j.contains("quote_currency")) inst.quote_currency = j["quote_currency"].get<std::string>();
        if (j.contains("pip_size")) inst.pip_size = j["pip_size"].get<double>();

        return inst;
    }

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["symbol"] = symbol;
        j["asset_class"] = asset_class_to_string(asset_class);
        if (!exchange.empty()) j["exchange"] = exchange;
        j["currency"] = currency;

        if (expiry) j["expiry"] = *expiry;
        if (contract_size != 1.0) j["contract_size"] = contract_size;
        if (tick_size != 0.01) j["tick_size"] = tick_size;
        if (underlying) j["underlying"] = *underlying;
        if (strike) j["strike"] = *strike;
        if (option_type) j["option_type"] = *option_type;
        if (expiration) j["expiration"] = *expiration;
        if (base_currency) j["base_currency"] = *base_currency;
        if (quote_currency) j["quote_currency"] = *quote_currency;
        if (pip_size) j["pip_size"] = *pip_size;

        return j;
    }
};

}  // namespace tradecore::instrument
