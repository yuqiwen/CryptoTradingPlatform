#pragma once

#include <string>
#include <vector>

#include "market_data_event.hpp"
#include "book_stream_processor.hpp"
#include "market_making_strategy.hpp"
#include "order_manager.hpp"
#include "paper_execution_simulator.hpp"
#include "portfolio_account.hpp"
#include "risk_engine.hpp"

namespace pipeline {

struct RiskDecision {
    order_manager::OrderRequest request;
    risk::RiskResult result;
};

struct EventResult {
    bool accepted = false;
    market_data::ApplyStatus book_status = market_data::ApplyStatus::Invalid;
    std::vector<RiskDecision> risk_decisions;
    std::vector<OrderId> new_order_ids;
    std::vector<order_manager::ExecutionReport> execution_reports;
};

class PaperTradingEngine {
public:
    PaperTradingEngine(
        std::string exchange,
        std::string symbol,
        strategy::MarketMakingStrategyConfig strategy_config,
        risk::RiskConfig risk_config);

    [[nodiscard]] EventResult on_market_data(
        const market_data::MarketDataEvent& event);

    [[nodiscard]] const order_book::OrderBook& order_book() const;
    [[nodiscard]] const order_manager::OrderManager& order_manager() const;
    [[nodiscard]] const simulator::PortfolioAccount& portfolio() const;

private:
    void process_order(OrderId order_id, EventResult& result);
    void cancel_active_orders(EventResult& result);
    void remove_terminal_orders();
    [[nodiscard]] bool has_active_order(Side side) const;

    std::string exchange_;
    std::string symbol_;
    market_data::BookStreamProcessor book_stream_;
    strategy::MarketMakingStrategy strategy_;
    risk::RiskEngine risk_engine_;
    order_manager::OrderManager order_manager_;
    simulator::PaperExecutionSimulator simulator_;
    simulator::PortfolioAccount portfolio_;
    std::vector<OrderId> active_order_ids_;
};

}  // namespace pipeline
