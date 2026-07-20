#pragma once

#include "order.hpp"
#include "order_request.hpp"

#include <unordered_map>

namespace order_manager {

    class OrderManager {
    public:
        OrderManager() = default;

        [[nodiscard]] Order create_order(const OrderRequest& request);

    private:
        OrderId next_order_id_ = 1;
        std::unordered_map<OrderId, Order> orders_;
    };

}  // namespace order_manager
