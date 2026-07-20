#pragma once

#include <vector>

#include "market_making_strategy_config.hpp"
#include "strategy.hpp"

namespace strategy {

    class MarketMakingStrategy : public Strategy {
    public:
        explicit MarketMakingStrategy(MarketMakingStrategyConfig config)
            : config_(config) {
        }

        std::vector<order_manager::OrderRequest> on_market_data(
            const order_book::OrderBook& book,
            const PositionView& position_view) override;

    private:
        MarketMakingStrategyConfig config_;
    };

}  // namespace strategy