#pragma once

#include "execution_report.hpp"
#include "order.hpp"
#include "order_request.hpp"

#include <optional>
#include <unordered_map>

namespace order_manager {

    class OrderManager {
    public:
        OrderManager() = default;

        [[nodiscard]] Order create_order(const OrderRequest& request);
        [[nodiscard]] std::optional<Order> get_order(OrderId order_id) const;
        [[nodiscard]] bool update_order_status(OrderId order_id, OrderStatus new_status);
        [[nodiscard]] bool apply_execution_report(const ExecutionReport& report);

    private:
        OrderId next_order_id_ = 1;
        std::unordered_map<OrderId, Order> orders_;
    };

}  // namespace order_manager
