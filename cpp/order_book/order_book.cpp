#include "order_book.hpp"

namespace order_book {

void OrderBook::clear() {
    bids_.clear();
    asks_.clear();
}

bool OrderBook::empty() const {
    return bids_.empty() && asks_.empty();
}

const BidLevels& OrderBook::bids() const {
    return bids_;
}

const AskLevels& OrderBook::asks() const {
    return asks_;
}

std::optional<PriceLevel> OrderBook::best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }

    const auto& [price_ticks, quantity_lots] = *bids_.begin();
    return PriceLevel{price_ticks, quantity_lots};
}

std::optional<PriceLevel> OrderBook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }

    const auto& [price_ticks, quantity_lots] = *asks_.begin();
    return PriceLevel{price_ticks, quantity_lots};
}

std::optional<PriceTicks> OrderBook::spread_ticks() const {
    const auto best_bid_level = best_bid();
    const auto best_ask_level = best_ask();

    if (!best_bid_level || !best_ask_level) {
        return std::nullopt;
    }

    return best_ask_level->price_ticks - best_bid_level->price_ticks;
}

std::optional<PriceTicks> OrderBook::mid_price_ticks() const {
    const auto best_bid_level = best_bid();
    const auto best_ask_level = best_ask();

    if (!best_bid_level || !best_ask_level) {
        return std::nullopt;
    }

    return (best_bid_level->price_ticks + best_ask_level->price_ticks) / 2;
}

void OrderBook::apply_snapshot(const BookSnapshot& snapshot) {
    clear();

    for (const auto& level : snapshot.bids) {
        if (level.price_ticks <= 0 || level.quantity_lots <= 0) {
            continue;
        }

        bids_[level.price_ticks] = level.quantity_lots;
    }

    for (const auto& level : snapshot.asks) {
        if (level.price_ticks <= 0 || level.quantity_lots <= 0) {
            continue;
        }

        asks_[level.price_ticks] = level.quantity_lots;
    }
}

void OrderBook::apply_update(const BookUpdate& update) {
    if (update.price_ticks <= 0 || update.quantity_lots < 0) {
        return;
    }

    if (update.side == BookSide::Bid) {
        if (update.quantity_lots == 0) {
            bids_.erase(update.price_ticks);
        } else {
            bids_[update.price_ticks] = update.quantity_lots;
        }
        return;
    }

    if (update.quantity_lots == 0) {
        asks_.erase(update.price_ticks);
    } else {
        asks_[update.price_ticks] = update.quantity_lots;
    }
}

}  // namespace order_book