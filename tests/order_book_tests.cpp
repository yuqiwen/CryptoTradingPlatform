#include <gtest/gtest.h>

#include "order_book.hpp"

namespace {

using order_book::BookSide;
using order_book::BookSnapshot;
using order_book::BookUpdate;
using order_book::OrderBook;
using order_book::PriceLevel;

TEST(OrderBookTest, DefaultConstructionStartsEmpty) {
    const OrderBook book;

    EXPECT_TRUE(book.empty());
    EXPECT_TRUE(book.bids().empty());
    EXPECT_TRUE(book.asks().empty());
}

TEST(OrderBookTest, SnapshotLoadsBothSidesCorrectly) {
    OrderBook book;

    BookSnapshot snapshot{
        .bids = {
            PriceLevel{100, 4},
            PriceLevel{99, 2},
        },
        .asks = {
            PriceLevel{101, 3},
            PriceLevel{102, 5},
        },
    };

    book.apply_snapshot(snapshot);

    ASSERT_FALSE(book.empty());
    ASSERT_EQ(book.bids().size(), 2U);
    ASSERT_EQ(book.asks().size(), 2U);

    const auto best_bid = book.best_bid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_EQ(best_bid->price_ticks, 100);
    EXPECT_EQ(best_bid->quantity_lots, 4);

    const auto best_ask = book.best_ask();
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_EQ(best_ask->price_ticks, 101);
    EXPECT_EQ(best_ask->quantity_lots, 3);
}

TEST(OrderBookTest, SnapshotReplacesPreviousState) {
    OrderBook book;

    book.apply_snapshot(BookSnapshot{
        .bids = {
            PriceLevel{100, 4},
            PriceLevel{99, 2},
        },
        .asks = {
            PriceLevel{101, 3},
            PriceLevel{102, 5},
        },
    });

    book.apply_snapshot(BookSnapshot{
        .bids = {
            PriceLevel{98, 7},
        },
        .asks = {
            PriceLevel{103, 1},
        },
    });

    ASSERT_EQ(book.bids().size(), 1U);
    ASSERT_EQ(book.asks().size(), 1U);
    EXPECT_EQ(book.bids().begin()->first, 98);
    EXPECT_EQ(book.asks().begin()->first, 103);
}

TEST(OrderBookTest, UpdateInsertsAndOverwritesLevels) {
    OrderBook book;

    book.apply_update(BookUpdate{BookSide::Bid, 100, 4});
    book.apply_update(BookUpdate{BookSide::Bid, 100, 6});
    book.apply_update(BookUpdate{BookSide::Ask, 101, 3});

    ASSERT_EQ(book.bids().size(), 1U);
    ASSERT_EQ(book.asks().size(), 1U);
    EXPECT_EQ(book.bids().at(100), 6);
    EXPECT_EQ(book.asks().at(101), 3);
}

TEST(OrderBookTest, ZeroQuantityUpdateRemovesLevel) {
    OrderBook book;

    book.apply_update(BookUpdate{BookSide::Bid, 100, 4});
    ASSERT_EQ(book.bids().size(), 1U);

    book.apply_update(BookUpdate{BookSide::Bid, 100, 0});

    EXPECT_TRUE(book.bids().empty());
}

TEST(OrderBookTest, SpreadAndMidPriceAreComputedCorrectly) {
    OrderBook book;

    book.apply_snapshot(BookSnapshot{
        .bids = {
            PriceLevel{100, 4},
        },
        .asks = {
            PriceLevel{104, 3},
        },
    });

    const auto spread = book.spread_ticks();
    ASSERT_TRUE(spread.has_value());
    EXPECT_EQ(*spread, 4);

    const auto mid = book.mid_price_ticks();
    ASSERT_TRUE(mid.has_value());
    EXPECT_EQ(*mid, 102);
}

TEST(OrderBookTest, SpreadAndMidPriceReturnEmptyWhenBookIsIncomplete) {
    OrderBook bid_only_book;
    bid_only_book.apply_update(BookUpdate{BookSide::Bid, 100, 4});

    EXPECT_FALSE(bid_only_book.spread_ticks().has_value());
    EXPECT_FALSE(bid_only_book.mid_price_ticks().has_value());

    OrderBook ask_only_book;
    ask_only_book.apply_update(BookUpdate{BookSide::Ask, 101, 3});

    EXPECT_FALSE(ask_only_book.spread_ticks().has_value());
    EXPECT_FALSE(ask_only_book.mid_price_ticks().has_value());
}

TEST(OrderBookTest, ClearResetsAllVisibleState) {
    OrderBook book;

    book.apply_snapshot(BookSnapshot{
        .bids = {
            PriceLevel{100, 4},
        },
        .asks = {
            PriceLevel{101, 3},
        },
    });

    book.clear();

    EXPECT_TRUE(book.empty());
    EXPECT_TRUE(book.bids().empty());
    EXPECT_TRUE(book.asks().empty());
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_FALSE(book.spread_ticks().has_value());
    EXPECT_FALSE(book.mid_price_ticks().has_value());
}

}  // namespace
