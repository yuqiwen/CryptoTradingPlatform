#include "market_making_strategy.hpp"

namespace strategy {

    std::vector<order_manager::OrderRequest> MarketMakingStrategy::on_market_data(
        const order_book::OrderBook& book,
        const PositionView& position_view) {
        const auto mid_price_ticks = book.mid_price_ticks();
        if (!mid_price_ticks) {
            return {};
        }

        const PriceTicks half_spread_ticks = config_.target_spread_ticks / 2;

        PriceTicks skew_ticks = 0;
        if (position_view.net_position_lots > config_.inventory_limit_lots) {
            skew_ticks = -config_.inventory_skew_ticks;
        }
        else if (position_view.net_position_lots < -config_.inventory_limit_lots) {
            skew_ticks = config_.inventory_skew_ticks;
        }

        const PriceTicks bid_price_ticks =
            *mid_price_ticks - half_spread_ticks + skew_ticks;
        const PriceTicks ask_price_ticks =
            *mid_price_ticks + half_spread_ticks + skew_ticks;

        return {
            order_manager::OrderRequest{
                .exchange = position_view.exchange,
                .symbol = position_view.symbol,
                .side = Side::Buy,
                .order_type = OrderType::Limit,
                .price_ticks = bid_price_ticks,
                .quantity_lots = config_.quote_size_lots,
                .post_only = true,
            },
            order_manager::OrderRequest{
                .exchange = position_view.exchange,
                .symbol = position_view.symbol,
                .side = Side::Sell,
                .order_type = OrderType::Limit,
                .price_ticks = ask_price_ticks,
                .quantity_lots = config_.quote_size_lots,
                .post_only = true,
            },
        };
    }

}  // namespace strategy