#pragma once

#include <vector>

#include "order_book.hpp"
#include "order_request.hpp"
#include "position_view.hpp"

namespace strategy {

    class Strategy {
    public:
        virtual ~Strategy() = default;

        virtual std::vector<order_manager::OrderRequest> on_market_data(
            const order_book::OrderBook& book,
            const PositionView& position_view) = 0;
    };

}  // namespace strategy