#pragma once

#include <string>

#include "types.hpp"

namespace order_manager {

    struct OrderRequest {
        std::string exchange;
        std::string symbol;

        Side side;
        OrderType order_type;

        PriceTicks price_ticks;
        QuantityLots quantity_lots;

        bool post_only;
    };

}  // namespace order_manager