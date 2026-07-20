#pragma once

#include "types.hpp"

namespace order_manager {

    inline bool is_valid_transition(OrderStatus from, OrderStatus to) {
        switch (from) {
        case OrderStatus::Created:
            return to == OrderStatus::PendingNew;

        case OrderStatus::PendingNew:
            return to == OrderStatus::Open ||
                to == OrderStatus::Rejected;

        case OrderStatus::Open:
            return to == OrderStatus::PartiallyFilled ||
                to == OrderStatus::Filled ||
                to == OrderStatus::PendingCancel;

        case OrderStatus::PartiallyFilled:
            return to == OrderStatus::PartiallyFilled ||
                to == OrderStatus::Filled ||
                to == OrderStatus::PendingCancel;

        case OrderStatus::PendingCancel:
            return to == OrderStatus::Canceled;

        case OrderStatus::Filled:
        case OrderStatus::Canceled:
        case OrderStatus::Rejected:
            return false;
        }

        return false;
    }

}  // namespace order_manager
