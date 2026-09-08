#pragma once

#include "execution_report.hpp"
#include "order.hpp"
#include "order_book.hpp"

#include <vector>

namespace simulator {

class PaperExecutionSimulator {
public:
    [[nodiscard]] std::vector<order_manager::ExecutionReport> submit_order(
        const order_manager::Order& order,
        const order_book::OrderBook& order_book) const;

private:
    [[nodiscard]] static order_manager::ExecutionReport make_report(
        const order_manager::Order& order,
        OrderStatus status,
        QuantityLots last_fill_quantity_lots,
        PriceTicks last_fill_price_ticks,
        QuantityLots cumulative_filled_quantity_lots);
};

}  // namespace simulator
