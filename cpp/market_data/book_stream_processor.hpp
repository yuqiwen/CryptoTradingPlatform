#pragma once

#include <cstdint>
#include <optional>

#include "market_data_event.hpp"
#include "order_book.hpp"

namespace market_data {

enum class ApplyStatus {
    Applied,
    Invalid,
    Stale,
    Gap,
    NeedsSnapshot
};

class BookStreamProcessor {
public:
    [[nodiscard]] ApplyStatus apply(const MarketDataEvent& event);

    [[nodiscard]] const order_book::OrderBook& book() const;
    [[nodiscard]] bool synchronized() const;
    [[nodiscard]] std::optional<std::uint64_t> last_sequence() const;

private:
    order_book::OrderBook book_;
    std::optional<std::uint64_t> last_sequence_;
    bool synchronized_ = false;
};

}  // namespace market_data
