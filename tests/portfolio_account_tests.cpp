#include "portfolio_account.hpp"

#include <gtest/gtest.h>

#include <limits>

namespace {

order_manager::Order make_order(
    OrderId order_id,
    Side side,
    QuantityLots quantity_lots = 2) {
    return order_manager::Order{
        order_id,
        "coinbase",
        "BTC-USD",
        side,
        OrderType::Limit,
        OrderStatus::Open,
        50'000,
        quantity_lots,
        0
    };
}

order_manager::ExecutionReport make_fill_report(
    OrderId order_id,
    OrderStatus status,
    QuantityLots last_fill_quantity_lots,
    PriceTicks last_fill_price_ticks,
    QuantityLots cumulative_filled_quantity_lots) {
    return order_manager::ExecutionReport{
        order_id,
        "coinbase",
        "BTC-USD",
        status,
        last_fill_quantity_lots,
        last_fill_price_ticks,
        cumulative_filled_quantity_lots
    };
}

}  // namespace

TEST(PortfolioAccountTests, StartsFlatWithZeroCash) {
    const simulator::PortfolioAccount account{"coinbase", "BTC-USD"};

    const auto snapshot = account.snapshot();

    EXPECT_EQ(snapshot.exchange, "coinbase");
    EXPECT_EQ(snapshot.symbol, "BTC-USD");
    EXPECT_EQ(snapshot.net_position_lots, 0);
    EXPECT_EQ(snapshot.cash_ticks_lots, 0);
    EXPECT_EQ(account.net_position_lots(), 0);
    EXPECT_EQ(account.cash_ticks_lots(), 0);
}

TEST(PortfolioAccountTests, BuyFillIncreasesPositionAndReducesCash) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Buy, 2);
    const auto report = make_fill_report(
        order.order_id,
        OrderStatus::Filled,
        2,
        50'000,
        2);

    EXPECT_TRUE(account.apply_execution_report(order, report));

    EXPECT_EQ(account.net_position_lots(), 2);
    EXPECT_EQ(account.cash_ticks_lots(), -100'000);
}

TEST(PortfolioAccountTests, SellFillDecreasesPositionAndIncreasesCash) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Sell, 2);
    const auto report = make_fill_report(
        order.order_id,
        OrderStatus::Filled,
        2,
        50'000,
        2);

    EXPECT_TRUE(account.apply_execution_report(order, report));

    EXPECT_EQ(account.net_position_lots(), -2);
    EXPECT_EQ(account.cash_ticks_lots(), 100'000);
}

TEST(PortfolioAccountTests, MarkToMarketOpenLongPosition) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Buy, 2);
    const auto report = make_fill_report(
        order.order_id,
        OrderStatus::Filled,
        2,
        50'000,
        2);

    ASSERT_TRUE(account.apply_execution_report(order, report));

    const auto marked_pnl = account.mark_to_market_pnl_ticks_lots(50'100);

    ASSERT_TRUE(marked_pnl.has_value());
    EXPECT_EQ(*marked_pnl, 200);
}

TEST(PortfolioAccountTests, RoundTripTradeLeavesRealizedCashPnlWhenFlat) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto buy_order = make_order(1, Side::Buy, 2);
    const auto sell_order = make_order(2, Side::Sell, 2);

    ASSERT_TRUE(account.apply_execution_report(
        buy_order,
        make_fill_report(
            buy_order.order_id,
            OrderStatus::Filled,
            2,
            50'000,
            2)));
    ASSERT_TRUE(account.apply_execution_report(
        sell_order,
        make_fill_report(
            sell_order.order_id,
            OrderStatus::Filled,
            2,
            50'100,
            2)));

    const auto marked_pnl = account.mark_to_market_pnl_ticks_lots(50'100);

    ASSERT_TRUE(marked_pnl.has_value());
    EXPECT_EQ(account.net_position_lots(), 0);
    EXPECT_EQ(account.cash_ticks_lots(), 200);
    EXPECT_EQ(*marked_pnl, 200);
}

TEST(PortfolioAccountTests, PartialFillsOnlyAccountNewCumulativeDelta) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Buy, 3);

    ASSERT_TRUE(account.apply_execution_report(
        order,
        make_fill_report(
            order.order_id,
            OrderStatus::PartiallyFilled,
            1,
            50'000,
            1)));
    ASSERT_TRUE(account.apply_execution_report(
        order,
        make_fill_report(
            order.order_id,
            OrderStatus::Filled,
            2,
            50'010,
            3)));

    EXPECT_EQ(account.net_position_lots(), 3);
    EXPECT_EQ(account.cash_ticks_lots(), -150'020);
}

TEST(PortfolioAccountTests, RejectsDuplicateExecutionReport) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Buy, 2);
    const auto report = make_fill_report(
        order.order_id,
        OrderStatus::PartiallyFilled,
        1,
        50'000,
        1);

    ASSERT_TRUE(account.apply_execution_report(order, report));
    EXPECT_FALSE(account.apply_execution_report(order, report));

    EXPECT_EQ(account.net_position_lots(), 1);
    EXPECT_EQ(account.cash_ticks_lots(), -50'000);
}

TEST(PortfolioAccountTests, RejectsMismatchedOrderId) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Buy, 2);
    const auto report = make_fill_report(
        999,
        OrderStatus::Filled,
        2,
        50'000,
        2);

    EXPECT_FALSE(account.apply_execution_report(order, report));

    EXPECT_EQ(account.net_position_lots(), 0);
    EXPECT_EQ(account.cash_ticks_lots(), 0);
}

TEST(PortfolioAccountTests, RejectsMismatchedSymbol) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Buy, 2);
    auto report = make_fill_report(
        order.order_id,
        OrderStatus::Filled,
        2,
        50'000,
        2);
    report.symbol = "ETH-USD";

    EXPECT_FALSE(account.apply_execution_report(order, report));

    EXPECT_EQ(account.net_position_lots(), 0);
    EXPECT_EQ(account.cash_ticks_lots(), 0);
}

TEST(PortfolioAccountTests, RejectsNonFillReport) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Buy, 2);
    const auto report = make_fill_report(
        order.order_id,
        OrderStatus::Open,
        0,
        0,
        0);

    EXPECT_FALSE(account.apply_execution_report(order, report));

    EXPECT_EQ(account.net_position_lots(), 0);
    EXPECT_EQ(account.cash_ticks_lots(), 0);
}

TEST(PortfolioAccountTests, RejectsInconsistentLastFillAndCumulativeDelta) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Buy, 3);

    ASSERT_TRUE(account.apply_execution_report(
        order,
        make_fill_report(
            order.order_id,
            OrderStatus::PartiallyFilled,
            1,
            50'000,
            1)));

    EXPECT_FALSE(account.apply_execution_report(
        order,
        make_fill_report(
            order.order_id,
            OrderStatus::Filled,
            1,
            50'010,
            3)));

    EXPECT_EQ(account.net_position_lots(), 1);
    EXPECT_EQ(account.cash_ticks_lots(), -50'000);
}

TEST(PortfolioAccountTests, RejectsNotionalOverflow) {
    simulator::PortfolioAccount account{"coinbase", "BTC-USD"};
    const auto order = make_order(1, Side::Buy, 2);
    const auto report = make_fill_report(
        order.order_id,
        OrderStatus::Filled,
        2,
        std::numeric_limits<PriceTicks>::max(),
        2);

    EXPECT_FALSE(account.apply_execution_report(order, report));

    EXPECT_EQ(account.net_position_lots(), 0);
    EXPECT_EQ(account.cash_ticks_lots(), 0);
}

TEST(PortfolioAccountTests, MarkToMarketRejectsInvalidMarkPrice) {
    const simulator::PortfolioAccount account{"coinbase", "BTC-USD"};

    EXPECT_FALSE(account.mark_to_market_pnl_ticks_lots(0).has_value());
    EXPECT_FALSE(account.mark_to_market_pnl_ticks_lots(-1).has_value());
}
