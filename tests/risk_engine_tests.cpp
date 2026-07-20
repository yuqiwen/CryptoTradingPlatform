#include "risk_engine.hpp"
#include <limits>
#include <gtest/gtest.h>

namespace {

risk::RiskConfig make_default_config() {
    return {
        false,    // kill_switch_enabled
        100,      // max_position_lots
        10,       // max_order_size_lots
        100'000,  // max_notional_units
        100,      // max_price_deviation_bps
        5         // max_orders_per_second
    };
}

risk::RiskConfig make_kill_switch_config() {
    auto config = make_default_config();
    config.kill_switch_enabled = true;
    return config;
}

order_manager::OrderRequest make_order_request(
    Side side,
    QuantityLots quantity_lots,
    PriceTicks price_ticks = 50'000) {
    return {
        "coinbase",
        "BTC-USD",
        side,
        OrderType::Limit,
        price_ticks,
        quantity_lots,
        true
    };
}

}  // namespace

TEST(RiskEngineTests, ApprovesOrderWithinMaxOrderSize) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 10);

    const auto result = risk_engine.check_order_size(request);

    EXPECT_TRUE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::None);
}

TEST(RiskEngineTests, RejectsInvalidQuantity) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 0);

    const auto result = risk_engine.check_order_size(request);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::InvalidQuantity);
}

TEST(RiskEngineTests, RejectsOrderAboveMaxOrderSize) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 11);

    const auto result = risk_engine.check_order_size(request);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxOrderSizeExceeded);
}

TEST(RiskEngineTests, ApprovesProjectedPositionAtLimit) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 10);

    const auto result = risk_engine.check_position_limit(request, 90);

    EXPECT_TRUE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::None);
}

TEST(RiskEngineTests, RejectsProjectedLongPositionAboveLimit) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 11);

    const auto result = risk_engine.check_position_limit(request, 90);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxPositionExceeded);
}

TEST(RiskEngineTests, RejectsProjectedShortPositionBelowLimit) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Sell, 11);

    const auto result = risk_engine.check_position_limit(request, -90);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxPositionExceeded);
}

TEST(RiskEngineTests, ApprovesNotionalAtLimit) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 2, 50'000);

    const auto result = risk_engine.check_notional(request);

    EXPECT_TRUE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::None);
}

TEST(RiskEngineTests, RejectsNotionalAboveLimit) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 2, 50'001);

    const auto result = risk_engine.check_notional(request);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxNotionalExceeded);
}

TEST(RiskEngineTests, RejectsInvalidNotionalQuantity) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 0, 50'000);

    const auto result = risk_engine.check_notional(request);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::InvalidQuantity);
}

TEST(RiskEngineTests, RejectsNotionalThatWouldOverflow) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(
        Side::Buy,
        2,
        std::numeric_limits<PriceTicks>::max());

    const auto result = risk_engine.check_notional(request);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxNotionalExceeded);
}

TEST(RiskEngineTests, CheckOrderApprovesValidOrder) {
    risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 2, 50'000);

    const auto result = risk_engine.check_order(request, 0, 50'000, 1'000'000'000);

    EXPECT_TRUE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::None);
}

TEST(RiskEngineTests, CheckOrderRejectsOrderSizeBeforeOtherChecks) {
    risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 11, 50'001);

    const auto result = risk_engine.check_order(request, 95, 50'000, 1'000'000'000);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxOrderSizeExceeded);
}

TEST(RiskEngineTests, CheckOrderRejectsPositionLimit) {
    risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 10, 1'000);

    const auto result = risk_engine.check_order(request, 95, 1'000, 1'000'000'000);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxPositionExceeded);
}

TEST(RiskEngineTests, CheckOrderRejectsNotionalLimit) {
    risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 10, 10'001);

    const auto result = risk_engine.check_order(request, 0, 10'000, 1'000'000'000);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxNotionalExceeded);
}

TEST(RiskEngineTests, CheckOrderRejectsPriceDeviationLimit) {
    risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 1, 50'501);

    const auto result = risk_engine.check_order(request, 0, 50'000, 1'000'000'000);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxPriceDeviationExceeded);
}

TEST(RiskEngineTests, CheckOrderRejectsRateLimitAfterValidOrders) {
    risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 1, 50'000);

    for (int i = 0; i < 5; ++i) {
        const auto result =
            risk_engine.check_order(request, 0, 50'000, 1'000'000'000);
        EXPECT_TRUE(result.approved);
    }

    const auto result =
        risk_engine.check_order(request, 0, 50'000, 1'000'000'000);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::RateLimitExceeded);
}

TEST(RiskEngineTests, ApprovesWhenKillSwitchDisabled) {
    const risk::RiskEngine risk_engine(make_default_config());

    const auto result = risk_engine.check_kill_switch();

    EXPECT_TRUE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::None);
}

TEST(RiskEngineTests, CheckOrderRejectsWhenKillSwitchEnabled) {
    risk::RiskEngine risk_engine(make_kill_switch_config());
    const auto request = make_order_request(Side::Buy, 1, 50'000);

    const auto result = risk_engine.check_order(request, 0, 50'000, 1'000'000'000);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::KillSwitchEnabled);
}

TEST(RiskEngineTests, ApprovesPriceDeviationAtLimit) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 1, 50'500);

    const auto result = risk_engine.check_price_deviation(request, 50'000);

    EXPECT_TRUE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::None);
}

TEST(RiskEngineTests, RejectsPriceDeviationAboveLimit) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 1, 50'501);

    const auto result = risk_engine.check_price_deviation(request, 50'000);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxPriceDeviationExceeded);
}

TEST(RiskEngineTests, RejectsInvalidReferencePrice) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 1, 50'000);

    const auto result = risk_engine.check_price_deviation(request, 0);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxPriceDeviationExceeded);
}

TEST(RiskEngineTests, RejectsInvalidOrderPriceForDeviation) {
    const risk::RiskEngine risk_engine(make_default_config());
    const auto request = make_order_request(Side::Buy, 1, 0);

    const auto result = risk_engine.check_price_deviation(request, 50'000);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::MaxPriceDeviationExceeded);
}

TEST(RiskEngineTests, ApprovesOrdersWithinRateLimit) {
    risk::RiskEngine risk_engine(make_default_config());

    for (int i = 0; i < 5; ++i) {
        const auto result = risk_engine.check_rate_limit(1'000'000'000);

        EXPECT_TRUE(result.approved);
        EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::None);
    }
}

TEST(RiskEngineTests, RejectsOrderAboveRateLimitInSameWindow) {
    risk::RiskEngine risk_engine(make_default_config());

    for (int i = 0; i < 5; ++i) {
        const auto result = risk_engine.check_rate_limit(1'000'000'000);
        EXPECT_TRUE(result.approved);
    }

    const auto result = risk_engine.check_rate_limit(1'000'000'000);

    EXPECT_FALSE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::RateLimitExceeded);
}

TEST(RiskEngineTests, ResetsRateLimitInNewWindow) {
    risk::RiskEngine risk_engine(make_default_config());

    for (int i = 0; i < 5; ++i) {
        const auto result = risk_engine.check_rate_limit(1'000'000'000);
        EXPECT_TRUE(result.approved);
    }

    const auto result = risk_engine.check_rate_limit(2'000'000'000);

    EXPECT_TRUE(result.approved);
    EXPECT_EQ(result.reject_reason, risk::RiskRejectReason::None);
}
