#include "book_stream_processor.hpp"

#include <set>
#include <utility>
#include <variant>

namespace market_data {
namespace {

bool valid_snapshot(const order_book::BookSnapshot& snapshot) {
    std::set<PriceTicks> bid_prices;
    std::set<PriceTicks> ask_prices;
    for (const auto& level : snapshot.bids) {
        if (level.price_ticks <= 0 || level.quantity_lots <= 0 ||
            !bid_prices.insert(level.price_ticks).second) {
            return false;
        }
    }
    for (const auto& level : snapshot.asks) {
        if (level.price_ticks <= 0 || level.quantity_lots <= 0 ||
            !ask_prices.insert(level.price_ticks).second) {
            return false;
        }
    }
    return true;
}

bool valid_update(const order_book::BookUpdate& update) {
    return (update.side == order_book::BookSide::Bid ||
            update.side == order_book::BookSide::Ask) &&
           update.price_ticks > 0 && update.quantity_lots >= 0;
}

bool crossed(const order_book::OrderBook& book) {
    const auto bid = book.best_bid();
    const auto ask = book.best_ask();
    return bid && ask && bid->price_ticks >= ask->price_ticks;
}

}  // namespace

ApplyStatus BookStreamProcessor::apply(const MarketDataEvent& event) {
    if (event.sequence == 0) {
        return ApplyStatus::Invalid;
    }

    if (const auto* snapshot =
            std::get_if<order_book::BookSnapshot>(&event.payload)) {
        if (last_sequence_ && event.sequence <= *last_sequence_) {
            return ApplyStatus::Stale;
        }
        if (!valid_snapshot(*snapshot)) {
            return ApplyStatus::Invalid;
        }

        order_book::OrderBook next_book;
        next_book.apply_snapshot(*snapshot);
        if (crossed(next_book)) {
            return ApplyStatus::Invalid;
        }

        book_ = std::move(next_book);
        last_sequence_ = event.sequence;
        synchronized_ = true;
        return ApplyStatus::Applied;
    }

    if (!synchronized_) {
        return ApplyStatus::NeedsSnapshot;
    }
    if (event.sequence <= *last_sequence_) {
        return ApplyStatus::Stale;
    }
    if (event.sequence - *last_sequence_ != 1) {
        synchronized_ = false;
        book_.clear();
        return ApplyStatus::Gap;
    }

    const auto& updates =
        std::get<std::vector<order_book::BookUpdate>>(event.payload);
    if (updates.empty()) {
        synchronized_ = false;
        book_.clear();
        return ApplyStatus::Invalid;
    }

    order_book::OrderBook next_book = book_;
    for (const auto& update : updates) {
        if (!valid_update(update)) {
            synchronized_ = false;
            book_.clear();
            return ApplyStatus::Invalid;
        }
        next_book.apply_update(update);
    }
    if (crossed(next_book)) {
        synchronized_ = false;
        book_.clear();
        return ApplyStatus::Invalid;
    }

    book_ = std::move(next_book);
    last_sequence_ = event.sequence;
    return ApplyStatus::Applied;
}

const order_book::OrderBook& BookStreamProcessor::book() const {
    return book_;
}

bool BookStreamProcessor::synchronized() const {
    return synchronized_;
}

std::optional<std::uint64_t> BookStreamProcessor::last_sequence() const {
    return last_sequence_;
}

}  // namespace market_data
