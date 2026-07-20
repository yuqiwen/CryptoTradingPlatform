#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#include "types.hpp"

namespace order_book {

struct PriceLevel {
    PriceTicks price_ticks;
    QuantityLots quantity_lots;
};

using BidLevels = std::map<PriceTicks, QuantityLots, std::greater<PriceTicks>>;
using AskLevels = std::map<PriceTicks, QuantityLots>;

enum class BookSide : std::int8_t {
    Bid = 0,
    Ask = 1
};

struct BookSnapshot {
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
};

struct BookUpdate {
    BookSide side;
    PriceTicks price_ticks;
    QuantityLots quantity_lots;
};

}  // namespace order_book