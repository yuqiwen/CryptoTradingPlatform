#include <gtest/gtest.h>

#include "order_status_transition.hpp"

namespace {

    using order_manager::is_valid_transition;

    TEST(OrderStatusTransitionTest, AllowsValidNewOrderPath) {
        EXPECT_TRUE(is_valid_transition(OrderStatus::Created, OrderStatus::PendingNew));
        EXPECT_TRUE(is_valid_transition(OrderStatus::PendingNew, OrderStatus::Open));
    }

    TEST(OrderStatusTransitionTest, AllowsValidFillPath) {
        EXPECT_TRUE(is_valid_transition(OrderStatus::Open, OrderStatus::PartiallyFilled));
        EXPECT_TRUE(is_valid_transition(OrderStatus::PartiallyFilled, OrderStatus::Filled));
    }

    TEST(OrderStatusTransitionTest, AllowsRepeatedPartialFillState) {
        EXPECT_TRUE(is_valid_transition(OrderStatus::PartiallyFilled, OrderStatus::PartiallyFilled));
    }

    TEST(OrderStatusTransitionTest, RejectsInvalidTransitions) {
        EXPECT_FALSE(is_valid_transition(OrderStatus::Created, OrderStatus::Filled));
        EXPECT_FALSE(is_valid_transition(OrderStatus::Open, OrderStatus::Rejected));
        EXPECT_FALSE(is_valid_transition(OrderStatus::PendingCancel, OrderStatus::Open));
    }

TEST(OrderStatusTransitionTest, RejectsTransitionsFromTerminalStates) {
    EXPECT_FALSE(is_valid_transition(OrderStatus::Filled, OrderStatus::Open));
    EXPECT_FALSE(is_valid_transition(OrderStatus::Canceled, OrderStatus::PendingCancel));
    EXPECT_FALSE(is_valid_transition(OrderStatus::Rejected, OrderStatus::Created));
}

}  // namespace
