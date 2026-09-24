#include "paper_trading_engine.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace pipeline {

PaperTradingEngine::PaperTradingEngine(
    std::string exchange,
    std::string symbol,
    strategy::MarketMakingStrategyConfig strategy_config,
    risk::RiskConfig risk_config)
    : exchange_(std::move(exchange)),
      symbol_(std::move(symbol)),
      strategy_(strategy_config),
      risk_engine_(risk_config),
      portfolio_(exchange_, symbol_) {}

EventResult PaperTradingEngine::on_market_data(
    const market_data::MarketDataEvent& event) {
    EventResult result;
    if (event.exchange != exchange_ || event.symbol != symbol_ ||
        event.bid_price_ticks <= 0 || event.ask_price_ticks <= 0 ||
        event.bid_price_ticks >= event.ask_price_ticks ||
        event.bid_size_lots <= 0 || event.ask_size_lots <= 0) {
        return result;
    }

    result.accepted = true;
    order_book_.apply_snapshot(order_book::BookSnapshot{
        {{event.bid_price_ticks, event.bid_size_lots}},
        {{event.ask_price_ticks, event.ask_size_lots}}
    });

    for (const OrderId order_id : active_order_ids_) {
        process_order(order_id, result);
    }
    remove_terminal_orders();

    const strategy::PositionView position_view{
        exchange_, symbol_, portfolio_.net_position_lots()
    };
    const auto requests = strategy_.on_market_data(order_book_, position_view);
    const PriceTicks reference_price_ticks = *order_book_.mid_price_ticks();

    for (const auto& request : requests) {
        if (request.exchange != exchange_ || request.symbol != symbol_) {
            throw std::logic_error("strategy produced an order for another market");
        }

        // A single resting quote per side bounds unfilled exposure until cancel/replace exists.
        if (has_active_order(request.side)) {
            continue;
        }

        const auto risk_result = risk_engine_.check_order(
            request,
            portfolio_.net_position_lots(),
            reference_price_ticks,
            event.local_recv_ts_ns);
        result.risk_decisions.push_back(RiskDecision{request, risk_result});
        if (!risk_result.approved) {
            continue;
        }

        const auto order = order_manager_.create_order(request);
        if (!order_manager_.update_order_status(
                order.order_id, OrderStatus::PendingNew)) {
            throw std::logic_error("new order could not enter PendingNew");
        }

        active_order_ids_.push_back(order.order_id);
        result.new_order_ids.push_back(order.order_id);
        process_order(order.order_id, result);
    }
    remove_terminal_orders();
    return result;
}

const order_book::OrderBook& PaperTradingEngine::order_book() const {
    return order_book_;
}

const order_manager::OrderManager& PaperTradingEngine::order_manager() const {
    return order_manager_;
}

const simulator::PortfolioAccount& PaperTradingEngine::portfolio() const {
    return portfolio_;
}

void PaperTradingEngine::process_order(OrderId order_id, EventResult& result) {
    const auto order = order_manager_.get_order(order_id);
    if (!order) {
        throw std::logic_error("active order is missing from order manager");
    }

    const auto reports = simulator_.submit_order(*order, order_book_);
    for (const auto& report : reports) {
        if (report.status == OrderStatus::PartiallyFilled ||
            report.status == OrderStatus::Filled) {
            if (!portfolio_.apply_execution_report(*order, report)) {
                throw std::logic_error("portfolio rejected simulator fill");
            }
        }
        if (!order_manager_.apply_execution_report(report)) {
            throw std::logic_error("order manager rejected simulator report");
        }
        result.execution_reports.push_back(report);
    }
}

void PaperTradingEngine::remove_terminal_orders() {
    std::erase_if(active_order_ids_, [this](OrderId order_id) {
        const auto order = order_manager_.get_order(order_id);
        return order && (order->status == OrderStatus::Filled ||
                         order->status == OrderStatus::Canceled ||
                         order->status == OrderStatus::Rejected);
    });
}

bool PaperTradingEngine::has_active_order(Side side) const {
    return std::any_of(active_order_ids_.begin(), active_order_ids_.end(),
        [this, side](OrderId order_id) {
            const auto order = order_manager_.get_order(order_id);
            return order && order->side == side;
        });
}

}  // namespace pipeline
