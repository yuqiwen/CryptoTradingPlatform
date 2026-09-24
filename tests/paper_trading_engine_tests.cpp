#include "paper_trading_engine.hpp"

#include <gtest/gtest.h>

namespace {

pipeline::PaperTradingEngine make_engine(QuantityLots max_order_size = 2) {
    return pipeline::PaperTradingEngine{
        "coinbase",
        "BTC-USD",
        strategy::MarketMakingStrategyConfig{20, 2, 10, 0},
        risk::RiskConfig{false, 10, max_order_size, 1'000'000, 100, 10}
    };
}

market_data::MarketDataEvent make_event(
    PriceTicks bid = 49'990,
    PriceTicks ask = 50'010,
    std::uint64_t sequence = 1) {
    return market_data::MarketDataEvent{
        "coinbase", "BTC-USD", sequence, 0, 1'000'000'000,
        order_book::BookSnapshot{{{bid, 5}}, {{ask, 5}}}
    };
}

market_data::MarketDataEvent make_delta(std::uint64_t sequence) {
    return {"coinbase", "BTC-USD", sequence, 0, 1'000'000'000,
            std::vector<order_book::BookUpdate>{
                {order_book::BookSide::Bid, 49'990, 0},
                {order_book::BookSide::Ask, 50'010, 0},
                {order_book::BookSide::Bid, 49'980, 5},
                {order_book::BookSide::Ask, 49'985, 5}
            }};
}

}  // namespace

TEST(PaperTradingEngineTests, InitialEventCreatesTwoRiskApprovedRestingOrders) {
    auto engine = make_engine();

    const auto result = engine.on_market_data(make_event());

    ASSERT_TRUE(result.accepted);
    ASSERT_EQ(result.risk_decisions.size(), 2);
    EXPECT_TRUE(result.risk_decisions[0].result.approved);
    EXPECT_TRUE(result.risk_decisions[1].result.approved);
    ASSERT_EQ(result.new_order_ids.size(), 2);
    ASSERT_EQ(result.execution_reports.size(), 2);
    EXPECT_EQ(result.execution_reports[0].status, OrderStatus::Open);
    EXPECT_EQ(result.execution_reports[1].status, OrderStatus::Open);
    EXPECT_EQ(engine.order_manager().get_order(result.new_order_ids[0])->status,
              OrderStatus::Open);
    EXPECT_TRUE(engine.order_manager().get_order(result.new_order_ids[0])->post_only);
    EXPECT_EQ(engine.portfolio().net_position_lots(), 0);
}

TEST(PaperTradingEngineTests, RepeatedQuoteDoesNotCreateDuplicateOrders) {
    auto engine = make_engine();
    const auto first = engine.on_market_data(make_event());
    ASSERT_EQ(first.new_order_ids.size(), 2);

    const auto second = engine.on_market_data(make_event(49'990, 50'010, 2));

    EXPECT_TRUE(second.accepted);
    EXPECT_TRUE(second.new_order_ids.empty());
    EXPECT_TRUE(second.risk_decisions.empty());
    EXPECT_TRUE(second.execution_reports.empty());
}

TEST(PaperTradingEngineTests, RestingBuyFillsOnNextEventAndUpdatesPortfolio) {
    auto engine = make_engine();
    const auto first = engine.on_market_data(make_event());
    ASSERT_EQ(first.new_order_ids.size(), 2);

    const auto second = engine.on_market_data(make_delta(2));

    EXPECT_TRUE(second.accepted);
    EXPECT_EQ(engine.order_manager().get_order(first.new_order_ids[0])->status,
              OrderStatus::Filled);
    EXPECT_EQ(engine.order_manager().get_order(first.new_order_ids[1])->status,
              OrderStatus::Open);
    ASSERT_EQ(second.new_order_ids.size(), 1);
    EXPECT_EQ(engine.order_manager().get_order(second.new_order_ids[0])->side,
              Side::Buy);
    EXPECT_EQ(engine.portfolio().net_position_lots(), 2);
    EXPECT_EQ(engine.portfolio().cash_ticks_lots(), -99'970);
    ASSERT_EQ(second.execution_reports.size(), 2);
    EXPECT_EQ(second.execution_reports[0].status, OrderStatus::Filled);
    EXPECT_EQ(second.execution_reports[0].last_fill_price_ticks, 49'985);
}

TEST(PaperTradingEngineTests, RiskRejectionPreventsOrderCreation) {
    auto engine = make_engine(1);

    const auto result = engine.on_market_data(make_event());

    ASSERT_EQ(result.risk_decisions.size(), 2);
    for (const auto& decision : result.risk_decisions) {
        EXPECT_FALSE(decision.result.approved);
        EXPECT_EQ(decision.result.reject_reason,
                  risk::RiskRejectReason::MaxOrderSizeExceeded);
    }
    EXPECT_TRUE(result.new_order_ids.empty());
    EXPECT_TRUE(result.execution_reports.empty());
}

TEST(PaperTradingEngineTests, InvalidOrOtherMarketEventDoesNotMutateBook) {
    auto engine = make_engine();
    auto event = make_event();
    event.symbol = "ETH-USD";

    EXPECT_FALSE(engine.on_market_data(event).accepted);
    EXPECT_TRUE(engine.order_book().empty());

    event.symbol = "BTC-USD";
    event.payload = order_book::BookSnapshot{{{50'010, 5}}, {{50'010, 5}}};
    EXPECT_FALSE(engine.on_market_data(event).accepted);
    EXPECT_TRUE(engine.order_book().empty());
}

TEST(PaperTradingEngineTests, GapSkipsStrategyAndWaitsForSnapshot) {
    auto engine = make_engine();
    const auto first = engine.on_market_data(make_event());
    ASSERT_TRUE(first.accepted);
    ASSERT_EQ(first.new_order_ids.size(), 2);

    const auto gap = engine.on_market_data(make_delta(3));
    EXPECT_FALSE(gap.accepted);
    EXPECT_EQ(gap.book_status, market_data::ApplyStatus::Gap);
    EXPECT_TRUE(gap.new_order_ids.empty());
    EXPECT_TRUE(engine.order_book().empty());
    ASSERT_EQ(gap.execution_reports.size(), 2);
    EXPECT_EQ(gap.execution_reports[0].status, OrderStatus::Canceled);
    EXPECT_EQ(engine.order_manager().get_order(first.new_order_ids[0])->status,
              OrderStatus::Canceled);

    const auto recovered = engine.on_market_data(make_event(49'980, 49'985, 4));
    EXPECT_TRUE(recovered.accepted);
    EXPECT_EQ(engine.portfolio().net_position_lots(), 0);
    EXPECT_EQ(recovered.new_order_ids.size(), 2);
}
