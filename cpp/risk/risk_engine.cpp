#include "risk_engine.hpp"

namespace risk {

    RiskEngine::RiskEngine(RiskConfig config)
        : config_(config) {
    }

    RiskResult RiskEngine::check_order_size(
        const order_manager::OrderRequest& request) const {

        if (request.quantity_lots <= 0) {
            return { false, RiskRejectReason::InvalidQuantity };
        }

        if (request.quantity_lots > config_.max_order_size_lots) {
            return { false, RiskRejectReason::MaxOrderSizeExceeded };
        }

        return { true, RiskRejectReason::None };
    }


    RiskResult RiskEngine::check_position_limit(
        const order_manager::OrderRequest& request,
        QuantityLots current_position_lots) const {

        if (request.quantity_lots <= 0) {
            return { false, RiskRejectReason::InvalidQuantity };
        }

        QuantityLots projected_position_lots = current_position_lots;

        if (request.side == Side::Buy) {
            projected_position_lots += request.quantity_lots;
        }
        else {
            projected_position_lots -= request.quantity_lots;
        }

        if (projected_position_lots > config_.max_position_lots ||
            projected_position_lots < -config_.max_position_lots) {
            return { false, RiskRejectReason::MaxPositionExceeded };
        }

        return { true, RiskRejectReason::None };
    }

    RiskResult RiskEngine::check_notional(
        const order_manager::OrderRequest& request) const {

        if (request.quantity_lots <= 0) {
            return { false, RiskRejectReason::InvalidQuantity };
        }

        if (request.price_ticks <= 0) {
            return { false, RiskRejectReason::MaxNotionalExceeded };
        }

        if (config_.max_notional_units <= 0) {
            return { false, RiskRejectReason::MaxNotionalExceeded };
        }

        if (request.price_ticks >
            config_.max_notional_units / request.quantity_lots) {
            return { false, RiskRejectReason::MaxNotionalExceeded };
        }

        return { true, RiskRejectReason::None };
    }

    RiskResult RiskEngine::check_order(
        const order_manager::OrderRequest& request,
        QuantityLots current_position_lots,
        PriceTicks reference_price_ticks,
        TimestampNs now_ns) {

        const auto kill_switch_result = check_kill_switch();
        if (!kill_switch_result.approved) {
            return kill_switch_result;
        }

        const auto order_size_result = check_order_size(request);
        if (!order_size_result.approved) {
            return order_size_result;
        }

        const auto position_result =
            check_position_limit(request, current_position_lots);
        if (!position_result.approved) {
            return position_result;
        }

        const auto notional_result = check_notional(request);
        if (!notional_result.approved) {
            return notional_result;
        }

        const auto price_deviation_result =
            check_price_deviation(request, reference_price_ticks);
        if (!price_deviation_result.approved) {
            return price_deviation_result;
        }

        const auto rate_limit_result = check_rate_limit(now_ns);
        if (!rate_limit_result.approved) {
            return rate_limit_result;
        }

        return { true, RiskRejectReason::None };
    }

    RiskResult RiskEngine::check_price_deviation(
        const order_manager::OrderRequest& request,
        PriceTicks reference_price_ticks) const {

        if (request.price_ticks <= 0 || reference_price_ticks <= 0) {
            return { false, RiskRejectReason::MaxPriceDeviationExceeded };
        }

        if (config_.max_price_deviation_bps < 0) {
            return { false, RiskRejectReason::MaxPriceDeviationExceeded };
        }

        const PriceTicks diff =
            request.price_ticks > reference_price_ticks
            ? request.price_ticks - reference_price_ticks
            : reference_price_ticks - request.price_ticks;

        const PriceTicks max_allowed_diff =
            reference_price_ticks * config_.max_price_deviation_bps / 10'000;

        if (diff > max_allowed_diff) {
            return { false, RiskRejectReason::MaxPriceDeviationExceeded };
        }

        return { true, RiskRejectReason::None };
    }

    RiskResult RiskEngine::check_kill_switch() const {
        if (config_.kill_switch_enabled) {
            return { false, RiskRejectReason::KillSwitchEnabled };
        }

        return { true, RiskRejectReason::None };
    }

    RiskResult RiskEngine::check_rate_limit(TimestampNs now_ns) {
        constexpr TimestampNs window_size_ns = 1'000'000'000;

        if (config_.max_orders_per_second <= 0) {
            return { false, RiskRejectReason::RateLimitExceeded };
        }

        if (current_window_start_ns_ == 0 ||
            now_ns - current_window_start_ns_ >= window_size_ns) {
            current_window_start_ns_ = now_ns;
            orders_in_current_window_ = 0;
        }

        if (orders_in_current_window_ >= config_.max_orders_per_second) {
            return { false, RiskRejectReason::RateLimitExceeded };
        }

        ++orders_in_current_window_;

        return { true, RiskRejectReason::None };
    }

}  // namespace risk
