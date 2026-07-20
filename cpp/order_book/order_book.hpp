#pragma once

#include <optional>
#include "order_book_types.hpp"

namespace order_book {

class OrderBook {
public:
    OrderBook() = default;

    void clear();

    bool empty() const;

    void apply_snapshot(const BookSnapshot& snapshot);
    void apply_update(const BookUpdate& update);

    const BidLevels& bids() const;
    const AskLevels& asks() const;

    std::optional<PriceLevel> best_bid() const;
    std::optional<PriceLevel> best_ask() const;

    std::optional<PriceTicks> spread_ticks() const;
    std::optional<PriceTicks> mid_price_ticks() const;

private:
    BidLevels bids_;
    AskLevels asks_;
};

}  // namespace order_book