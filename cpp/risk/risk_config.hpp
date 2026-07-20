#pragma once

#include "types.hpp"
#include <cstdint>

namespace risk {

    struct RiskConfig {
        bool kill_switch_enabled;
        QuantityLots max_position_lots;
        QuantityLots max_order_size_lots;
        std::int64_t max_notional_units;
        std::int64_t max_price_deviation_bps;
        std::int64_t max_orders_per_second;

    };

}  // namespace risk