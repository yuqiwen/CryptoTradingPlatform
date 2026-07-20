#pragma once

#include <string>

#include "types.hpp"

namespace order_manager {

    struct Order {
        OrderId order_id;

        std::string exchange;
        std::string symbol;

        Side side;
        OrderType order_type;
        OrderStatus status;

        PriceTicks price_ticks;
        QuantityLots quantity_lots;
        QuantityLots filled_quantity_lots;
    };

}  // namespace order_manager