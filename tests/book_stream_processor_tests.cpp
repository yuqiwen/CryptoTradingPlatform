#include "book_stream_processor.hpp"

#include <gtest/gtest.h>

#include <utility>

namespace {

market_data::MarketDataEvent snapshot(std::uint64_t sequence = 1) {
    return {"coinbase", "BTC-USD", sequence, 0, 1'000'000'000,
            order_book::BookSnapshot{
                {{49'990, 5}, {49'980, 3}},
                {{50'010, 5}, {50'020, 3}}
            }};
}

market_data::MarketDataEvent delta(
    std::uint64_t sequence,
    std::vector<order_book::BookUpdate> updates) {
    return {"coinbase", "BTC-USD", sequence, 0, 1'000'000'000,
            std::move(updates)};
}

}  // namespace

TEST(BookStreamProcessorTests, RequiresSnapshotBeforeDelta) {
    market_data::BookStreamProcessor stream;

    EXPECT_EQ(stream.apply(delta(1, {{order_book::BookSide::Bid, 49'990, 5}})),
              market_data::ApplyStatus::NeedsSnapshot);
    EXPECT_FALSE(stream.synchronized());
    EXPECT_TRUE(stream.book().empty());
}

TEST(BookStreamProcessorTests, SnapshotAndContiguousDeltaUpdateDepth) {
    market_data::BookStreamProcessor stream;
    ASSERT_EQ(stream.apply(snapshot()), market_data::ApplyStatus::Applied);

    const auto status = stream.apply(delta(2, {
        {order_book::BookSide::Bid, 49'990, 0},
        {order_book::BookSide::Bid, 49'985, 7},
        {order_book::BookSide::Ask, 50'010, 0}
    }));

    EXPECT_EQ(status, market_data::ApplyStatus::Applied);
    EXPECT_EQ(stream.last_sequence(), 2);
    ASSERT_TRUE(stream.book().best_bid());
    ASSERT_TRUE(stream.book().best_ask());
    EXPECT_EQ(stream.book().best_bid()->price_ticks, 49'985);
    EXPECT_EQ(stream.book().best_bid()->quantity_lots, 7);
    EXPECT_EQ(stream.book().best_ask()->price_ticks, 50'020);
    EXPECT_EQ(stream.book().bids().size(), 2);
}

TEST(BookStreamProcessorTests, StaleEventLeavesBookUnchanged) {
    market_data::BookStreamProcessor stream;
    ASSERT_EQ(stream.apply(snapshot()), market_data::ApplyStatus::Applied);

    EXPECT_EQ(stream.apply(delta(1, {{order_book::BookSide::Bid, 49'995, 1}})),
              market_data::ApplyStatus::Stale);
    EXPECT_EQ(stream.book().best_bid()->price_ticks, 49'990);
    EXPECT_EQ(stream.last_sequence(), 1);
    EXPECT_TRUE(stream.synchronized());
}

TEST(BookStreamProcessorTests, GapClearsBookUntilNewSnapshot) {
    market_data::BookStreamProcessor stream;
    ASSERT_EQ(stream.apply(snapshot()), market_data::ApplyStatus::Applied);

    EXPECT_EQ(stream.apply(delta(3, {{order_book::BookSide::Bid, 49'995, 1}})),
              market_data::ApplyStatus::Gap);
    EXPECT_TRUE(stream.book().empty());
    EXPECT_FALSE(stream.synchronized());
    EXPECT_EQ(stream.apply(delta(2, {{order_book::BookSide::Bid, 49'995, 1}})),
              market_data::ApplyStatus::NeedsSnapshot);
    EXPECT_EQ(stream.apply(snapshot(4)), market_data::ApplyStatus::Applied);
    EXPECT_TRUE(stream.synchronized());
    EXPECT_EQ(stream.last_sequence(), 4);
}

TEST(BookStreamProcessorTests, InvalidDeltaCannotPartiallyMutateBook) {
    market_data::BookStreamProcessor stream;
    ASSERT_EQ(stream.apply(snapshot()), market_data::ApplyStatus::Applied);

    EXPECT_EQ(stream.apply(delta(2, {
        {order_book::BookSide::Bid, 49'990, 0},
        {order_book::BookSide::Bid, 50'030, 2}
    })), market_data::ApplyStatus::Invalid);
    EXPECT_TRUE(stream.book().empty());
    EXPECT_FALSE(stream.synchronized());
}

TEST(BookStreamProcessorTests, InvalidSnapshotDoesNotReplaceValidBook) {
    market_data::BookStreamProcessor stream;
    ASSERT_EQ(stream.apply(snapshot()), market_data::ApplyStatus::Applied);
    auto invalid = snapshot(2);
    invalid.payload = order_book::BookSnapshot{
        {{50'020, 1}}, {{50'010, 1}}
    };

    EXPECT_EQ(stream.apply(invalid), market_data::ApplyStatus::Invalid);
    EXPECT_EQ(stream.book().best_bid()->price_ticks, 49'990);
    EXPECT_EQ(stream.last_sequence(), 1);
}
