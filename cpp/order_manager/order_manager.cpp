#include "order_manager.hpp"

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

}  // namespace order_manager
