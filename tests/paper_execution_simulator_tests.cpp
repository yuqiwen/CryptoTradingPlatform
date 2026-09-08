#include "order_manager.hpp"
#include "paper_execution_simulator.hpp"

#include <gtest/gtest.h>

namespace {

order_book::OrderBook make_order_book() {
    order_book::OrderBook book;
    book.apply_snapshot(order_book::BookSnapshot{
        {{49'990, 5}},
        {{50'010, 5}}
    });
    return book;
}

order_manager::OrderRequest make_limit_request(
    Side side,
    PriceTicks price_ticks,
    QuantityLots quantity_lots = 2) {
    return order_manager::OrderRequest{
        "coinbase",
        "BTC-USD",
        side,
        OrderType::Limit,
        price_ticks,
        quantity_lots,
        true
    };
}

order_manager::OrderRequest make_market_request(Side side) {
    return order_manager::OrderRequest{
        "coinbase",
        "BTC-USD",
        side,
        OrderType::Market,
        0,
        2,
        false
    };
}

order_manager::Order create_pending_order(
    order_manager::OrderManager& order_manager,
    const order_manager::OrderRequest& request) {
    const auto order = order_manager.create_order(request);

    EXPECT_TRUE(order_manager.update_order_status(
        order.order_id,
        OrderStatus::PendingNew));

    const auto stored_order = order_manager.get_order(order.order_id);
    EXPECT_TRUE(stored_order.has_value());

    return *stored_order;
}

}  // namespace

TEST(PaperExecutionSimulatorTests, RestingLimitOrderProducesOpenReport) {
    order_manager::OrderManager order_manager;
    const simulator::PaperExecutionSimulator simulator;
    const auto book = make_order_book();
    const auto order = create_pending_order(
        order_manager,
        make_limit_request(Side::Buy, 50'000));

    const auto reports = simulator.submit_order(order, book);

    ASSERT_EQ(reports.size(), 1);
    EXPECT_EQ(reports[0].status, OrderStatus::Open);
    EXPECT_EQ(reports[0].last_fill_quantity_lots, 0);
    EXPECT_EQ(reports[0].last_fill_price_ticks, 0);
    EXPECT_EQ(reports[0].cumulative_filled_quantity_lots, 0);

    EXPECT_TRUE(order_manager.apply_execution_report(reports[0]));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Open);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}

TEST(PaperExecutionSimulatorTests, MarketableBuyLimitProducesOpenThenFilled) {
    order_manager::OrderManager order_manager;
    const simulator::PaperExecutionSimulator simulator;
    const auto book = make_order_book();
    const auto order = create_pending_order(
        order_manager,
        make_limit_request(Side::Buy, 50'010));

    const auto reports = simulator.submit_order(order, book);

    ASSERT_EQ(reports.size(), 2);
    EXPECT_EQ(reports[0].status, OrderStatus::Open);
    EXPECT_EQ(reports[1].status, OrderStatus::Filled);
    EXPECT_EQ(reports[1].last_fill_quantity_lots, 2);
    EXPECT_EQ(reports[1].last_fill_price_ticks, 50'010);
    EXPECT_EQ(reports[1].cumulative_filled_quantity_lots, 2);

    for (const auto& report : reports) {
        ASSERT_TRUE(order_manager.apply_execution_report(report));
    }

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Filled);
    EXPECT_EQ(stored_order->filled_quantity_lots, 2);
}

TEST(PaperExecutionSimulatorTests, MarketableSellLimitFillsAtBestBid) {
    order_manager::OrderManager order_manager;
    const simulator::PaperExecutionSimulator simulator;
    const auto book = make_order_book();
    const auto order = create_pending_order(
        order_manager,
        make_limit_request(Side::Sell, 49'990));

    const auto reports = simulator.submit_order(order, book);

    ASSERT_EQ(reports.size(), 2);
    EXPECT_EQ(reports[1].status, OrderStatus::Filled);
    EXPECT_EQ(reports[1].last_fill_quantity_lots, 2);
    EXPECT_EQ(reports[1].last_fill_price_ticks, 49'990);
    EXPECT_EQ(reports[1].cumulative_filled_quantity_lots, 2);
}

TEST(PaperExecutionSimulatorTests, MarketOrderWithoutOppositeLiquidityRejects) {
    order_manager::OrderManager order_manager;
    const simulator::PaperExecutionSimulator simulator;
    const order_book::OrderBook empty_book;
    const auto order = create_pending_order(
        order_manager,
        make_market_request(Side::Buy));

    const auto reports = simulator.submit_order(order, empty_book);

    ASSERT_EQ(reports.size(), 1);
    EXPECT_EQ(reports[0].status, OrderStatus::Rejected);
    EXPECT_EQ(reports[0].last_fill_quantity_lots, 0);
    EXPECT_EQ(reports[0].last_fill_price_ticks, 0);
    EXPECT_EQ(reports[0].cumulative_filled_quantity_lots, 0);

    EXPECT_TRUE(order_manager.apply_execution_report(reports[0]));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Rejected);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}

TEST(PaperExecutionSimulatorTests, OpenOrderCanFillWhenItBecomesMarketable) {
    order_manager::OrderManager order_manager;
    const simulator::PaperExecutionSimulator simulator;
    const auto book = make_order_book();
    const auto pending_order = create_pending_order(
        order_manager,
        make_limit_request(Side::Buy, 50'010));

    ASSERT_TRUE(order_manager.update_order_status(
        pending_order.order_id,
        OrderStatus::Open));

    const auto open_order = order_manager.get_order(pending_order.order_id);
    ASSERT_TRUE(open_order.has_value());

    const auto reports = simulator.submit_order(*open_order, book);

    ASSERT_EQ(reports.size(), 1);
    EXPECT_EQ(reports[0].status, OrderStatus::Filled);
    EXPECT_EQ(reports[0].last_fill_price_ticks, 50'010);

    EXPECT_TRUE(order_manager.apply_execution_report(reports[0]));

    const auto stored_order = order_manager.get_order(pending_order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Filled);
    EXPECT_EQ(stored_order->filled_quantity_lots, 2);
}
