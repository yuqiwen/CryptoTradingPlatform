#include "order_manager.hpp"

#include <gtest/gtest.h>

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
