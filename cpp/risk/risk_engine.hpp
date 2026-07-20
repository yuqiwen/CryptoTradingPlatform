#pragma once

#include "order_request.hpp"
#include "risk_config.hpp"
#include "risk_result.hpp"

namespace risk {

    class RiskEngine {
    public:
        explicit RiskEngine(RiskConfig config);

        [[nodiscard]] RiskResult check_order_size(
            const order_manager::OrderRequest& request) const;

        [[nodiscard]] RiskResult check_position_limit(
            const order_manager::OrderRequest& request,
            QuantityLots current_position_lots) const;

        [[nodiscard]] RiskResult check_notional(
            const order_manager::OrderRequest& request) const;

        [[nodiscard]] RiskResult check_order(
            const order_manager::OrderRequest& request,
            QuantityLots current_position_lots,
            PriceTicks reference_price_ticks,
            TimestampNs now_ns);

        [[nodiscard]] RiskResult check_price_deviation(
            const order_manager::OrderRequest& request,
            PriceTicks reference_price_ticks) const;

        [[nodiscard]] RiskResult check_kill_switch() const;

        [[nodiscard]] RiskResult check_rate_limit(TimestampNs now_ns);
    private:
        RiskConfig config_;
        TimestampNs current_window_start_ns_ = 0;
        std::int64_t orders_in_current_window_ = 0;
    };

}  // namespace risk
