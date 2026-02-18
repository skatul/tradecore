#include <gtest/gtest.h>

#include "instrument/instrument.hpp"

using namespace tradecore::instrument;

TEST(InstrumentExtended, CryptoAssetClass) {
    auto btc = make_crypto("BTC-USD", "COINBASE", "USD", 0.01);
    EXPECT_EQ(btc.symbol, "BTC-USD");
    EXPECT_EQ(btc.asset_class, AssetClass::Crypto);
    EXPECT_EQ(btc.exchange, "COINBASE");
    EXPECT_EQ(btc.currency, "USD");
    EXPECT_EQ(btc.trading_hours, "24/7");
    EXPECT_DOUBLE_EQ(btc.tick_size, 0.01);
}

TEST(InstrumentExtended, ETFAssetClass) {
    auto spy = make_etf("SPY", "NYSE", "USD", 0.01);
    EXPECT_EQ(spy.symbol, "SPY");
    EXPECT_EQ(spy.asset_class, AssetClass::ETF);
    EXPECT_EQ(spy.exchange, "NYSE");
    EXPECT_EQ(spy.trading_hours, "regular");
}

TEST(InstrumentExtended, CryptoToString) {
    EXPECT_EQ(asset_class_to_string(AssetClass::Crypto), "crypto");
    EXPECT_EQ(asset_class_to_string(AssetClass::ETF), "etf");
}

TEST(InstrumentExtended, CryptoProtoRoundTrip) {
    auto btc = make_crypto("BTC-USD", "COINBASE", "USD");
    btc.trading_hours = "24/7";

    auto proto = btc.to_proto();
    EXPECT_EQ(proto.security_type(), fix::SECURITY_TYPE_CRYPTO);
    EXPECT_EQ(proto.trading_hours(), "24/7");

    auto restored = Instrument::from_proto(proto);
    EXPECT_EQ(restored.asset_class, AssetClass::Crypto);
    EXPECT_EQ(restored.symbol, "BTC-USD");
    EXPECT_EQ(restored.trading_hours, "24/7");
}

TEST(InstrumentExtended, ETFProtoRoundTrip) {
    auto spy = make_etf("SPY", "NYSE", "USD");

    auto proto = spy.to_proto();
    EXPECT_EQ(proto.security_type(), fix::SECURITY_TYPE_ETF);

    auto restored = Instrument::from_proto(proto);
    EXPECT_EQ(restored.asset_class, AssetClass::ETF);
    EXPECT_EQ(restored.symbol, "SPY");
}

TEST(InstrumentExtended, SecurityTypeMapping) {
    EXPECT_EQ(asset_class_from_security_type(fix::SECURITY_TYPE_CRYPTO), AssetClass::Crypto);
    EXPECT_EQ(asset_class_from_security_type(fix::SECURITY_TYPE_ETF), AssetClass::ETF);
    EXPECT_EQ(asset_class_to_security_type(AssetClass::Crypto), fix::SECURITY_TYPE_CRYPTO);
    EXPECT_EQ(asset_class_to_security_type(AssetClass::ETF), fix::SECURITY_TYPE_ETF);
}

TEST(InstrumentExtended, DefaultTradingHours) {
    Instrument inst;
    EXPECT_EQ(inst.trading_hours, "regular");
}

TEST(InstrumentExtended, ExistingAssetClassesUnchanged) {
    EXPECT_EQ(asset_class_to_string(AssetClass::Equity), "equity");
    EXPECT_EQ(asset_class_to_string(AssetClass::Future), "future");
    EXPECT_EQ(asset_class_to_string(AssetClass::Option), "option");
    EXPECT_EQ(asset_class_to_string(AssetClass::FX), "fx");
}
