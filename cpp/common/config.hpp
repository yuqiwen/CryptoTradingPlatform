#pragma once

#include <cstdint>
#include <string>

namespace config {

struct AppConfig {
    std::string mode;
    std::string exchange;
    std::string symbol;
};

struct StrategyConfig {
    std::int64_t target_spread_ticks;
    std::int64_t quote_size_lots;
};

struct RiskConfig {
    std::int64_t max_position_lots;
    std::int64_t max_order_size_lots;
    std::int64_t max_notional_ticks;
    std::int64_t max_price_deviation_bps;
    std::int64_t max_orders_per_second;
};

struct Config {
    AppConfig app;
    StrategyConfig strategy;
    RiskConfig risk;
};

}  // namespace config