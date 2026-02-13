#pragma once

#include <fix_messages.pb.h>
#include <optional>
#include <string>

namespace tradecore::instrument {

enum class AssetClass { Equity, Future, Option, FX };

inline AssetClass asset_class_from_security_type(fix::SecurityType st) {
    switch (st) {
        case fix::SECURITY_TYPE_FUTURE: return AssetClass::Future;
        case fix::SECURITY_TYPE_OPTION: return AssetClass::Option;
        case fix::SECURITY_TYPE_FX_SPOT: return AssetClass::FX;
        default: return AssetClass::Equity;
    }
}

inline fix::SecurityType asset_class_to_security_type(AssetClass ac) {
    switch (ac) {
        case AssetClass::Equity: return fix::SECURITY_TYPE_COMMON_STOCK;
        case AssetClass::Future: return fix::SECURITY_TYPE_FUTURE;
        case AssetClass::Option: return fix::SECURITY_TYPE_OPTION;
        case AssetClass::FX:     return fix::SECURITY_TYPE_FX_SPOT;
    }
    return fix::SECURITY_TYPE_UNSPECIFIED;
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

    static Instrument from_proto(const fix::Instrument& proto) {
        Instrument inst;
        inst.symbol = proto.symbol();
        inst.asset_class = asset_class_from_security_type(proto.security_type());
        inst.exchange = proto.exchange();
        if (!proto.currency().empty()) inst.currency = proto.currency();

        if (!proto.maturity_date().empty()) inst.expiry = proto.maturity_date();
        if (proto.contract_multiplier() > 0) inst.contract_size = proto.contract_multiplier();
        if (!proto.underlying_symbol().empty()) inst.underlying = proto.underlying_symbol();
        if (proto.strike_price() > 0) inst.strike = proto.strike_price();
        if (!proto.put_or_call().empty()) inst.option_type = proto.put_or_call();
        if (proto.min_price_increment() > 0) inst.pip_size = proto.min_price_increment();

        return inst;
    }

    fix::Instrument to_proto() const {
        fix::Instrument proto;
        proto.set_symbol(symbol);
        proto.set_security_type(asset_class_to_security_type(asset_class));
        if (!exchange.empty()) proto.set_exchange(exchange);
        proto.set_currency(currency);

        if (expiry) proto.set_maturity_date(*expiry);
        if (contract_size != 1.0) proto.set_contract_multiplier(contract_size);
        if (underlying) proto.set_underlying_symbol(*underlying);
        if (strike) proto.set_strike_price(*strike);
        if (option_type) proto.set_put_or_call(*option_type);
        if (pip_size) proto.set_min_price_increment(*pip_size);

        return proto;
    }
};

}  // namespace tradecore::instrument
