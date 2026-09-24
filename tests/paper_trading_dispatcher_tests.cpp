#include "paper_trading_dispatcher.hpp"

#include <gtest/gtest.h>

#include <utility>

namespace {

pipeline::SymbolConfig config(std::string symbol) {
    return {std::move(symbol),
            strategy::MarketMakingStrategyConfig{20, 2, 10, 0},
            risk::RiskConfig{false, 10, 2, 1'000'000, 100, 10}};
}

market_data::MarketDataEvent snapshot(
    std::string symbol,
    std::uint64_t sequence,
    PriceTicks bid,
    PriceTicks ask) {
    return {"coinbase", std::move(symbol), sequence, 0, 1'000'000'000,
            order_book::BookSnapshot{{{bid, 5}}, {{ask, 5}}}};
}

market_data::MarketDataEvent delta(
    std::string symbol,
    std::uint64_t sequence,
    PriceTicks bid,
    PriceTicks ask,
    PriceTicks old_bid,
    PriceTicks old_ask) {
    return {"coinbase", std::move(symbol), sequence, 0, 1'000'000'000,
            std::vector<order_book::BookUpdate>{
                {order_book::BookSide::Bid, old_bid, 0},
                {order_book::BookSide::Ask, old_ask, 0},
                {order_book::BookSide::Bid, bid, 5},
                {order_book::BookSide::Ask, ask, 5}
            }};
}

}  // namespace

TEST(PaperTradingDispatcherTests, RoutesEachSymbolToStableWorkerAndKeepsStateSeparate) {
    pipeline::PaperTradingDispatcher dispatcher{
        "coinbase", {config("BTC-USD"), config("ETH-USD")}, 2, 8};

    auto btc_first = dispatcher.submit(snapshot("BTC-USD", 1, 49'990, 50'010));
    auto eth_first = dispatcher.submit(snapshot("ETH-USD", 1, 2'990, 3'010));
    auto btc_second = dispatcher.submit(delta(
        "BTC-USD", 2, 49'980, 49'985, 49'990, 50'010));
    ASSERT_TRUE(btc_first);
    ASSERT_TRUE(eth_first);
    ASSERT_TRUE(btc_second);

    const auto btc_open = btc_first->get();
    const auto eth_open = eth_first->get();
    const auto btc_fill = btc_second->get();

    EXPECT_EQ(btc_open.worker_id, dispatcher.worker_for("BTC-USD"));
    EXPECT_EQ(btc_fill.worker_id, btc_open.worker_id);
    EXPECT_EQ(eth_open.worker_id, dispatcher.worker_for("ETH-USD"));
    EXPECT_NE(eth_open.worker_id, btc_open.worker_id);
    EXPECT_TRUE(btc_open.result.accepted);
    EXPECT_TRUE(eth_open.result.accepted);
    EXPECT_TRUE(btc_fill.result.accepted);
    EXPECT_EQ(btc_fill.best_bid_ticks, 49'980);
    EXPECT_EQ(btc_fill.best_ask_ticks, 49'985);
    EXPECT_EQ(btc_fill.portfolio.net_position_lots, 2);
    EXPECT_EQ(btc_fill.portfolio.cash_ticks_lots, -99'970);
    EXPECT_EQ(eth_open.best_bid_ticks, 2'990);
    EXPECT_EQ(eth_open.portfolio.net_position_lots, 0);
}

TEST(PaperTradingDispatcherTests, SequenceGapInOneSymbolDoesNotStopAnother) {
    pipeline::PaperTradingDispatcher dispatcher{
        "coinbase", {config("BTC-USD"), config("ETH-USD")}, 2, 8};
    auto btc_first = dispatcher.submit(snapshot("BTC-USD", 1, 49'990, 50'010));
    auto eth_first = dispatcher.submit(snapshot("ETH-USD", 1, 2'990, 3'010));
    ASSERT_TRUE(btc_first);
    ASSERT_TRUE(eth_first);
    btc_first->get();
    eth_first->get();

    auto btc_gap = dispatcher.submit(delta(
        "BTC-USD", 3, 49'980, 49'985, 49'990, 50'010));
    auto eth_next = dispatcher.submit(snapshot("ETH-USD", 2, 2'980, 3'000));
    ASSERT_TRUE(btc_gap);
    ASSERT_TRUE(eth_next);

    const auto gap = btc_gap->get();
    const auto healthy = eth_next->get();
    EXPECT_EQ(gap.result.book_status, market_data::ApplyStatus::Gap);
    EXPECT_FALSE(gap.result.accepted);
    EXPECT_FALSE(gap.best_bid_ticks);
    EXPECT_TRUE(healthy.result.accepted);
    EXPECT_EQ(healthy.best_bid_ticks, 2'980);
}

TEST(PaperTradingDispatcherTests, StopDrainsSubmittedEventsAndRejectsNewOnes) {
    pipeline::PaperTradingDispatcher dispatcher{
        "coinbase", {config("BTC-USD")}, 1, 8};
    auto submitted = dispatcher.submit(snapshot("BTC-USD", 1, 49'990, 50'010));
    ASSERT_TRUE(submitted);

    dispatcher.stop();

    EXPECT_TRUE(submitted->get().result.accepted);
    EXPECT_FALSE(dispatcher.submit(snapshot("BTC-USD", 2, 49'980, 50'000)));
}

TEST(PaperTradingDispatcherTests, RejectsUnknownMarket) {
    pipeline::PaperTradingDispatcher dispatcher{
        "coinbase", {config("BTC-USD")}, 1, 8};
    EXPECT_FALSE(dispatcher.submit(snapshot("ETH-USD", 1, 2'990, 3'010)));
    auto wrong_exchange = snapshot("BTC-USD", 1, 49'990, 50'010);
    wrong_exchange.exchange = "kraken";
    EXPECT_FALSE(dispatcher.submit(std::move(wrong_exchange)));
}
