#pragma once

#include <string>

#include "types.hpp"

namespace market_data {

struct MarketDataEvent {
    std::string exchange;
    std::string symbol;

    PriceTicks bid_price_ticks;
    QuantityLots bid_size_lots;

    PriceTicks ask_price_ticks;
    QuantityLots ask_size_lots;

    TimestampNs exchange_ts_ns;
    TimestampNs local_recv_ts_ns;
};

}  // namespace market_data