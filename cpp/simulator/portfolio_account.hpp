#pragma once

#include "execution_report.hpp"
#include "order.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace simulator {

struct PortfolioSnapshot {
    std::string exchange;
    std::string symbol;
    QuantityLots net_position_lots;
    std::int64_t cash_ticks_lots;
};

class PortfolioAccount {
public:
    explicit PortfolioAccount(std::string exchange, std::string symbol);

    [[nodiscard]] bool apply_execution_report(
        const order_manager::Order& order,
        const order_manager::ExecutionReport& report);

    [[nodiscard]] PortfolioSnapshot snapshot() const;

    [[nodiscard]] QuantityLots net_position_lots() const;
    [[nodiscard]] std::int64_t cash_ticks_lots() const;
    [[nodiscard]] std::optional<std::int64_t> mark_to_market_pnl_ticks_lots(
        PriceTicks mark_price_ticks) const;

private:
    std::string exchange_;
    std::string symbol_;
    QuantityLots net_position_lots_ = 0;
    std::int64_t cash_ticks_lots_ = 0;
    std::unordered_map<OrderId, QuantityLots> accounted_cumulative_fills_;
};

}  // namespace simulator
