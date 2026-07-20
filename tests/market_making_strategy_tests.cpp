#include <gtest/gtest.h>

#include "market_making_strategy.hpp"
#include "order_book.hpp"

namespace {

    using order_book::BookSnapshot;
    using order_book::OrderBook;
    using order_book::PriceLevel;
    using strategy::MarketMakingStrategy;
    using strategy::MarketMakingStrategyConfig;
    using strategy::PositionView;

    TEST(MarketMakingStrategyTest, ReturnsNoOrdersWhenMidPriceIsUnavailable) {
        MarketMakingStrategy strategy(MarketMakingStrategyConfig{
            .target_spread_ticks = 4,
            .quote_size_lots = 1,
            .inventory_limit_lots = 5,
            .inventory_skew_ticks = 2,
            });

        OrderBook book;
        PositionView position_view{
            .exchange = "coinbase",
            .symbol = "BTC-USD",
            .net_position_lots = 0,
        };

        const auto requests = strategy.on_market_data(book, position_view);
        EXPECT_TRUE(requests.empty());
    }

    TEST(MarketMakingStrategyTest, ProducesSymmetricQuotesWithinInventoryLimits) {
        MarketMakingStrategy strategy(MarketMakingStrategyConfig{
            .target_spread_ticks = 4,
            .quote_size_lots = 1,
            .inventory_limit_lots = 5,
            .inventory_skew_ticks = 2,
            });

        OrderBook book;
        book.apply_snapshot(BookSnapshot{
            .bids = {PriceLevel{100, 1}},
            .asks = {PriceLevel{104, 1}},
            });

        PositionView position_view{
            .exchange = "coinbase",
            .symbol = "BTC-USD",
            .net_position_lots = 0,
        };

        const auto requests = strategy.on_market_data(book, position_view);

        ASSERT_EQ(requests.size(), 2U);

        EXPECT_EQ(requests[0].side, Side::Buy);
        EXPECT_EQ(requests[0].price_ticks, 100);
        EXPECT_EQ(requests[0].quantity_lots, 1);

        EXPECT_EQ(requests[1].side, Side::Sell);
        EXPECT_EQ(requests[1].price_ticks, 104);
        EXPECT_EQ(requests[1].quantity_lots, 1);
    }

    TEST(MarketMakingStrategyTest, SkewsQuotesDownWhenInventoryIsTooLong) {
        MarketMakingStrategy strategy(MarketMakingStrategyConfig{
            .target_spread_ticks = 4,
            .quote_size_lots = 1,
            .inventory_limit_lots = 5,
            .inventory_skew_ticks = 2,
            });

        OrderBook book;
        book.apply_snapshot(BookSnapshot{
            .bids = {PriceLevel{100, 1}},
            .asks = {PriceLevel{104, 1}},
            });

        PositionView position_view{
            .exchange = "coinbase",
            .symbol = "BTC-USD",
            .net_position_lots = 6,
        };

        const auto requests = strategy.on_market_data(book, position_view);

        ASSERT_EQ(requests.size(), 2U);
        EXPECT_EQ(requests[0].price_ticks, 98);
        EXPECT_EQ(requests[1].price_ticks, 102);
    }

    TEST(MarketMakingStrategyTest, SkewsQuotesUpWhenInventoryIsTooShort) {
        MarketMakingStrategy strategy(MarketMakingStrategyConfig{
            .target_spread_ticks = 4,
            .quote_size_lots = 1,
            .inventory_limit_lots = 5,
            .inventory_skew_ticks = 2,
            });

        OrderBook book;
        book.apply_snapshot(BookSnapshot{
            .bids = {PriceLevel{100, 1}},
            .asks = {PriceLevel{104, 1}},
            });

        PositionView position_view{
            .exchange = "coinbase",
            .symbol = "BTC-USD",
            .net_position_lots = -6,
        };

        const auto requests = strategy.on_market_data(book, position_view);

        ASSERT_EQ(requests.size(), 2U);
        EXPECT_EQ(requests[0].price_ticks, 102);
        EXPECT_EQ(requests[1].price_ticks, 106);
    }

}  // namespace