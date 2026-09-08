#include "paper_execution_simulator.hpp"

#include <optional>

namespace simulator {

std::vector<order_manager::ExecutionReport>
PaperExecutionSimulator::submit_order(
    const order_manager::Order& order,
    const order_book::OrderBook& order_book) const {
    std::vector<order_manager::ExecutionReport> reports;

    const QuantityLots remaining_quantity =
        order.quantity_lots - order.filled_quantity_lots;

    if (remaining_quantity <= 0) {
        return reports;
    }

    const auto fill_price = [&]() -> std::optional<PriceTicks> {
        if (order.side == Side::Buy) {
            const auto best_ask = order_book.best_ask();
            if (!best_ask) {
                return std::nullopt;
            }

            if (order.order_type == OrderType::Market ||
                order.price_ticks >= best_ask->price_ticks) {
                return best_ask->price_ticks;
            }

            return std::nullopt;
        }

        const auto best_bid = order_book.best_bid();
        if (!best_bid) {
            return std::nullopt;
        }

        if (order.order_type == OrderType::Market ||
            order.price_ticks <= best_bid->price_ticks) {
            return best_bid->price_ticks;
        }

        return std::nullopt;
    }();

    if (!fill_price && order.order_type == OrderType::Market) {
        reports.push_back(make_report(
            order,
            OrderStatus::Rejected,
            0,
            0,
            order.filled_quantity_lots));
        return reports;
    }

    if (order.status == OrderStatus::PendingNew) {
        reports.push_back(make_report(
            order,
            OrderStatus::Open,
            0,
            0,
            order.filled_quantity_lots));
    }

    if (!fill_price) {
        return reports;
    }

    reports.push_back(make_report(
        order,
        OrderStatus::Filled,
        remaining_quantity,
        *fill_price,
        order.quantity_lots));

    return reports;
}

order_manager::ExecutionReport PaperExecutionSimulator::make_report(
    const order_manager::Order& order,
    OrderStatus status,
    QuantityLots last_fill_quantity_lots,
    PriceTicks last_fill_price_ticks,
    QuantityLots cumulative_filled_quantity_lots) {
    return order_manager::ExecutionReport{
        order.order_id,
        order.exchange,
        order.symbol,
        status,
        last_fill_quantity_lots,
        last_fill_price_ticks,
        cumulative_filled_quantity_lots
    };
}

}  // namespace simulator
