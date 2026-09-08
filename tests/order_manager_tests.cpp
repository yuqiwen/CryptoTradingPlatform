#include "order_manager.hpp"

#include <gtest/gtest.h>

namespace {

    order_manager::OrderRequest make_request(QuantityLots quantity_lots = 2) {
        return order_manager::OrderRequest{
            "coinbase",
            "BTC-USD",
            Side::Buy,
            OrderType::Limit,
            50'000,
            quantity_lots,
            true
        };
    }

    order_manager::Order create_open_order(
        order_manager::OrderManager& order_manager,
        QuantityLots quantity_lots = 2) {
        const auto order = order_manager.create_order(
            make_request(quantity_lots));

        EXPECT_TRUE(order_manager.update_order_status(
            order.order_id,
            OrderStatus::PendingNew));
        EXPECT_TRUE(order_manager.update_order_status(
            order.order_id,
            OrderStatus::Open));

        return order;
    }

}  // namespace

TEST(OrderManagerTests, CreateOrderAssignsIdAndInitialState) {
    order_manager::OrderManager order_manager;

    const order_manager::OrderRequest request{
        "coinbase",
        "BTC-USD",
        Side::Buy,
        OrderType::Limit,
        50'000,
        2,
        true
    };

    const auto order = order_manager.create_order(request);

    EXPECT_EQ(order.order_id, 1);
    EXPECT_EQ(order.exchange, "coinbase");
    EXPECT_EQ(order.symbol, "BTC-USD");
    EXPECT_EQ(order.side, Side::Buy);
    EXPECT_EQ(order.order_type, OrderType::Limit);
    EXPECT_EQ(order.status, OrderStatus::Created);
    EXPECT_EQ(order.price_ticks, 50'000);
    EXPECT_EQ(order.quantity_lots, 2);
    EXPECT_EQ(order.filled_quantity_lots, 0);
}

TEST(OrderManagerTests, CreateOrderAssignsIncreasingIds) {
    order_manager::OrderManager order_manager;

    const order_manager::OrderRequest request{
        "coinbase",
        "BTC-USD",
        Side::Buy,
        OrderType::Limit,
        50'000,
        2,
        true
    };

    const auto first_order = order_manager.create_order(request);
    const auto second_order = order_manager.create_order(request);

    EXPECT_EQ(first_order.order_id, 1);
    EXPECT_EQ(second_order.order_id, 2);
}

TEST(OrderManagerTests, CreateOrderCanBeRetrievedById) {
    order_manager::OrderManager order_manager;

    const order_manager::OrderRequest request{
        "coinbase",
        "BTC-USD",
        Side::Buy,
        OrderType::Limit,
        50'000,
        2,
        true
    };

    const auto created_order = order_manager.create_order(request);
    const auto stored_order = order_manager.get_order(created_order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->order_id, created_order.order_id);
    EXPECT_EQ(stored_order->exchange, "coinbase");
    EXPECT_EQ(stored_order->symbol, "BTC-USD");
    EXPECT_EQ(stored_order->status, OrderStatus::Created);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}

TEST(OrderManagerTests, ApplyExecutionReportRejectsMismatchedExchange) {
    order_manager::OrderManager order_manager;

    const auto order = create_open_order(order_manager);

    const order_manager::ExecutionReport report{
        order.order_id,
        "binance",
        "BTC-USD",
        OrderStatus::PartiallyFilled,
        1,
        50'000,
        1
    };

    EXPECT_FALSE(order_manager.apply_execution_report(report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Open);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}

TEST(OrderManagerTests, ApplyExecutionReportRejectsMismatchedSymbol) {
    order_manager::OrderManager order_manager;

    const auto order = create_open_order(order_manager);

    const order_manager::ExecutionReport report{
        order.order_id,
        "coinbase",
        "ETH-USD",
        OrderStatus::PartiallyFilled,
        1,
        50'000,
        1
    };

    EXPECT_FALSE(order_manager.apply_execution_report(report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Open);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}

TEST(OrderManagerTests, ApplyExecutionReportRejectsNegativeFillQuantity) {
    order_manager::OrderManager order_manager;

    const auto order = create_open_order(order_manager);

    const order_manager::ExecutionReport report{
        order.order_id,
        "coinbase",
        "BTC-USD",
        OrderStatus::PartiallyFilled,
        -1,
        50'000,
        1
    };

    EXPECT_FALSE(order_manager.apply_execution_report(report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Open);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}

TEST(OrderManagerTests, ApplyExecutionReportRejectsDecreasingCumulativeFill) {
    order_manager::OrderManager order_manager;

    const auto order = create_open_order(order_manager);

    const order_manager::ExecutionReport first_report{
        order.order_id,
        "coinbase",
        "BTC-USD",
        OrderStatus::PartiallyFilled,
        1,
        50'000,
        1
    };

    ASSERT_TRUE(order_manager.apply_execution_report(first_report));

    const order_manager::ExecutionReport stale_report{
        order.order_id,
        "coinbase",
        "BTC-USD",
        OrderStatus::PartiallyFilled,
        0,
        50'000,
        0
    };

    EXPECT_FALSE(order_manager.apply_execution_report(stale_report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::PartiallyFilled);
    EXPECT_EQ(stored_order->filled_quantity_lots, 1);
}

TEST(OrderManagerTests, ApplyExecutionReportRejectsFillAboveOrderQuantity) {
    order_manager::OrderManager order_manager;

    const auto order = create_open_order(order_manager, 2);

    const order_manager::ExecutionReport report{
        order.order_id,
        "coinbase",
        "BTC-USD",
        OrderStatus::Filled,
        3,
        50'000,
        3
    };

    EXPECT_FALSE(order_manager.apply_execution_report(report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Open);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}

TEST(OrderManagerTests, ApplyExecutionReportRejectsFilledStatusBeforeFullFill) {
    order_manager::OrderManager order_manager;

    const auto order = create_open_order(order_manager, 2);

    const order_manager::ExecutionReport report{
        order.order_id,
        "coinbase",
        "BTC-USD",
        OrderStatus::Filled,
        1,
        50'000,
        1
    };

    EXPECT_FALSE(order_manager.apply_execution_report(report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Open);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}

TEST(OrderManagerTests, ApplyExecutionReportRejectsPartialFillAtFullQuantity) {
    order_manager::OrderManager order_manager;

    const auto order = create_open_order(order_manager, 2);

    const order_manager::ExecutionReport report{
        order.order_id,
        "coinbase",
        "BTC-USD",
        OrderStatus::PartiallyFilled,
        2,
        50'000,
        2
    };

    EXPECT_FALSE(order_manager.apply_execution_report(report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Open);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}

TEST(OrderManagerTests, ApplyExecutionReportAcceptsFilledStatusAtFullQuantity) {
    order_manager::OrderManager order_manager;

    const auto order = create_open_order(order_manager, 2);

    const order_manager::ExecutionReport report{
        order.order_id,
        "coinbase",
        "BTC-USD",
        OrderStatus::Filled,
        2,
        50'000,
        2
    };

    EXPECT_TRUE(order_manager.apply_execution_report(report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Filled);
    EXPECT_EQ(stored_order->filled_quantity_lots, 2);
}

TEST(OrderManagerTests, GetOrderReturnsEmptyForUnknownId) {
    const order_manager::OrderManager order_manager;

    const auto stored_order = order_manager.get_order(999);

    EXPECT_FALSE(stored_order.has_value());
}

TEST(OrderManagerTests, UpdateOrderStatusAllowsValidTransition) {
    order_manager::OrderManager order_manager;

    const order_manager::OrderRequest request{
        "coinbase",
        "BTC-USD",
        Side::Buy,
        OrderType::Limit,
        50'000,
        2,
        true
    };

    const auto order = order_manager.create_order(request);

    EXPECT_TRUE(order_manager.update_order_status(
        order.order_id,
        OrderStatus::PendingNew));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::PendingNew);
}

TEST(OrderManagerTests, UpdateOrderStatusRejectsInvalidTransition) {
    order_manager::OrderManager order_manager;

    const order_manager::OrderRequest request{
        "coinbase",
        "BTC-USD",
        Side::Buy,
        OrderType::Limit,
        50'000,
        2,
        true
    };

    const auto order = order_manager.create_order(request);

    EXPECT_FALSE(order_manager.update_order_status(
        order.order_id,
        OrderStatus::Filled));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Created);
}

TEST(OrderManagerTests, UpdateOrderStatusReturnsFalseForUnknownOrder) {
    order_manager::OrderManager order_manager;

    EXPECT_FALSE(order_manager.update_order_status(
        999,
        OrderStatus::PendingNew));
}

TEST(OrderManagerTests, ApplyExecutionReportUpdatesStatusAndFilledQuantity) {
    order_manager::OrderManager order_manager;

    const order_manager::OrderRequest request{
        "coinbase",
        "BTC-USD",
        Side::Buy,
        OrderType::Limit,
        50'000,
        2,
        true
    };

    const auto order = order_manager.create_order(request);

    ASSERT_TRUE(order_manager.update_order_status(
        order.order_id,
        OrderStatus::PendingNew));
    ASSERT_TRUE(order_manager.update_order_status(
        order.order_id,
        OrderStatus::Open));

    const order_manager::ExecutionReport report{
        order.order_id,
        "coinbase",
        "BTC-USD",
        OrderStatus::PartiallyFilled,
        1,
        50'000,
        1
    };

    EXPECT_TRUE(order_manager.apply_execution_report(report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::PartiallyFilled);
    EXPECT_EQ(stored_order->filled_quantity_lots, 1);
}

TEST(OrderManagerTests, ApplyExecutionReportReturnsFalseForUnknownOrder) {
    order_manager::OrderManager order_manager;

    const order_manager::ExecutionReport report{
        999,
        "coinbase",
        "BTC-USD",
        OrderStatus::PartiallyFilled,
        1,
        50'000,
        1
    };

    EXPECT_FALSE(order_manager.apply_execution_report(report));
}

TEST(OrderManagerTests, ApplyExecutionReportRejectsInvalidTransition) {
    order_manager::OrderManager order_manager;

    const order_manager::OrderRequest request{
        "coinbase",
        "BTC-USD",
        Side::Buy,
        OrderType::Limit,
        50'000,
        2,
        true
    };

    const auto order = order_manager.create_order(request);

    const order_manager::ExecutionReport report{
        order.order_id,
        "coinbase",
        "BTC-USD",
        OrderStatus::Filled,
        2,
        50'000,
        2
    };

    EXPECT_FALSE(order_manager.apply_execution_report(report));

    const auto stored_order = order_manager.get_order(order.order_id);

    ASSERT_TRUE(stored_order.has_value());
    EXPECT_EQ(stored_order->status, OrderStatus::Created);
    EXPECT_EQ(stored_order->filled_quantity_lots, 0);
}
