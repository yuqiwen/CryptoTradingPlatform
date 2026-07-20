#pragma once

#include <string>

#include "types.hpp"

namespace strategy {

    struct PositionView {
        std::string exchange;
        std::string symbol;

        QuantityLots net_position_lots;
    };

}  // namespace strategy