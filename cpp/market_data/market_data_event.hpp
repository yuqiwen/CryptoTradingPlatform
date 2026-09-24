#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "order_book_types.hpp"

namespace market_data {

struct MarketDataEvent {
    std::string exchange;
    std::string symbol;
    std::uint64_t sequence;
    TimestampNs exchange_ts_ns;
    TimestampNs local_recv_ts_ns;
    std::variant<order_book::BookSnapshot,
                 std::vector<order_book::BookUpdate>> payload;
};

}  // namespace market_data
