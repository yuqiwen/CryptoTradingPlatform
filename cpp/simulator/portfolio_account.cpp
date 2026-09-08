#include "portfolio_account.hpp"

#include <limits>
#include <utility>

namespace simulator {
namespace {

bool checked_multiply(
    std::int64_t lhs,
    std::int64_t rhs,
    std::int64_t& result) {
    if (lhs < 0 || rhs < 0) {
        return false;
    }

    if (lhs != 0 &&
        rhs > std::numeric_limits<std::int64_t>::max() / lhs) {
        return false;
    }

    result = lhs * rhs;
    return true;
}

bool checked_add(
    std::int64_t lhs,
    std::int64_t rhs,
    std::int64_t& result) {
    if (rhs > 0 &&
        lhs > std::numeric_limits<std::int64_t>::max() - rhs) {
        return false;
    }

    if (rhs < 0 &&
        lhs < std::numeric_limits<std::int64_t>::min() - rhs) {
        return false;
    }

    result = lhs + rhs;
    return true;
}

}  // namespace

PortfolioAccount::PortfolioAccount(std::string exchange, std::string symbol)
    : exchange_(std::move(exchange)), symbol_(std::move(symbol)) {}

bool PortfolioAccount::apply_execution_report(
    const order_manager::Order& order,
    const order_manager::ExecutionReport& report) {
    if (order.exchange != exchange_ || order.symbol != symbol_) {
        return false;
    }

    if (report.exchange != exchange_ || report.symbol != symbol_) {
        return false;
    }

    if (report.order_id != order.order_id) {
        return false;
    }

    if (report.status != OrderStatus::PartiallyFilled &&
        report.status != OrderStatus::Filled) {
        return false;
    }

    if (report.last_fill_quantity_lots <= 0 ||
        report.last_fill_price_ticks <= 0 ||
        report.cumulative_filled_quantity_lots <= 0) {
        return false;
    }

    if (report.cumulative_filled_quantity_lots > order.quantity_lots) {
        return false;
    }

    const auto accounted_iter =
        accounted_cumulative_fills_.find(order.order_id);
    const QuantityLots accounted_cumulative_fill =
        accounted_iter == accounted_cumulative_fills_.end()
            ? 0
            : accounted_iter->second;

    if (report.cumulative_filled_quantity_lots <=
        accounted_cumulative_fill) {
        return false;
    }

    const QuantityLots fill_delta =
        report.cumulative_filled_quantity_lots - accounted_cumulative_fill;

    if (report.last_fill_quantity_lots != fill_delta) {
        return false;
    }

    std::int64_t fill_notional = 0;
    if (!checked_multiply(
            report.last_fill_price_ticks,
            fill_delta,
            fill_notional)) {
        return false;
    }

    const QuantityLots signed_position_delta =
        order.side == Side::Buy ? fill_delta : -fill_delta;
    const std::int64_t signed_cash_delta =
        order.side == Side::Buy ? -fill_notional : fill_notional;

    QuantityLots new_position = 0;
    if (!checked_add(
            net_position_lots_,
            signed_position_delta,
            new_position)) {
        return false;
    }

    std::int64_t new_cash = 0;
    if (!checked_add(cash_ticks_lots_, signed_cash_delta, new_cash)) {
        return false;
    }

    net_position_lots_ = new_position;
    cash_ticks_lots_ = new_cash;
    accounted_cumulative_fills_[order.order_id] =
        report.cumulative_filled_quantity_lots;

    return true;
}

PortfolioSnapshot PortfolioAccount::snapshot() const {
    return PortfolioSnapshot{
        exchange_,
        symbol_,
        net_position_lots_,
        cash_ticks_lots_
    };
}

QuantityLots PortfolioAccount::net_position_lots() const {
    return net_position_lots_;
}

std::int64_t PortfolioAccount::cash_ticks_lots() const {
    return cash_ticks_lots_;
}

std::optional<std::int64_t> PortfolioAccount::mark_to_market_pnl_ticks_lots(
    PriceTicks mark_price_ticks) const {
    if (mark_price_ticks <= 0) {
        return std::nullopt;
    }

    std::int64_t inventory_value = 0;
    if (net_position_lots_ >= 0) {
        if (!checked_multiply(
                mark_price_ticks,
                net_position_lots_,
                inventory_value)) {
            return std::nullopt;
        }
    } else {
        if (!checked_multiply(
                mark_price_ticks,
                -net_position_lots_,
                inventory_value)) {
            return std::nullopt;
        }
        inventory_value = -inventory_value;
    }

    std::int64_t marked_pnl = 0;
    if (!checked_add(cash_ticks_lots_, inventory_value, marked_pnl)) {
        return std::nullopt;
    }

    return marked_pnl;
}

}  // namespace simulator
