#pragma once

#include <string>

#include "types.hpp"

namespace order_manager {

    struct ExecutionReport {
        OrderId order_id;

        std::string exchange;
        std::string symbol;

        OrderStatus status;

        QuantityLots last_fill_quantity_lots;
        PriceTicks last_fill_price_ticks;

        QuantityLots cumulative_filled_quantity_lots;
    };

}  // namespace order_manager