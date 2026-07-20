#pragma once

#include "types.hpp"

namespace strategy {

    struct MarketMakingStrategyConfig {
        PriceTicks target_spread_ticks;
        QuantityLots quote_size_lots;
        QuantityLots inventory_limit_lots;
        PriceTicks inventory_skew_ticks;
    };

}  // namespace strategy