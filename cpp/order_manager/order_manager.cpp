#include "order_manager.hpp"
#include "order_status_transition.hpp"

namespace order_manager {

    Order OrderManager::create_order(const OrderRequest& request) {
        const OrderId order_id = next_order_id_++;

        Order order{
            order_id,
            request.exchange,
            request.symbol,
            request.side,
            request.order_type,
            OrderStatus::Created,
            request.price_ticks,
            request.quantity_lots,
            0
        };

        orders_.emplace(order_id, order);

        return order;
    }

    std::optional<Order> OrderManager::get_order(OrderId order_id) const {
        const auto iter = orders_.find(order_id);
        if (iter == orders_.end()) {
            return std::nullopt;
        }

        return iter->second;
    }

    bool OrderManager::update_order_status(
        OrderId order_id,
        OrderStatus new_status) {
        auto iter = orders_.find(order_id);
        if (iter == orders_.end()) {
            return false;
        }

        if (!is_valid_transition(iter->second.status, new_status)) {
            return false;
        }

        iter->second.status = new_status;
        return true;
    }

    bool OrderManager::apply_execution_report(const ExecutionReport& report) {
        auto iter = orders_.find(report.order_id);
        if (iter == orders_.end()) {
            return false;
        }

        Order& order = iter->second;

        if (report.exchange != order.exchange || report.symbol != order.symbol) {
            return false;
        }

        if (!is_valid_transition(order.status, report.status)) {
            return false;
        }

        if (report.last_fill_quantity_lots < 0 ||
            report.cumulative_filled_quantity_lots < 0) {
            return false;
        }

        if (report.cumulative_filled_quantity_lots <
            order.filled_quantity_lots) {
            return false;
        }

        if (report.cumulative_filled_quantity_lots > order.quantity_lots) {
            return false;
        }

        if (report.status == OrderStatus::PartiallyFilled &&
            (report.cumulative_filled_quantity_lots <= 0 ||
             report.cumulative_filled_quantity_lots >= order.quantity_lots)) {
            return false;
        }

        if (report.status == OrderStatus::Filled &&
            report.cumulative_filled_quantity_lots != order.quantity_lots) {
            return false;
        }

        order.status = report.status;
        order.filled_quantity_lots = report.cumulative_filled_quantity_lots;

        return true;
    }
}  // namespace order_manager
