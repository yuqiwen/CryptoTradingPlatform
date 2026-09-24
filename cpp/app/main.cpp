#include <iostream>

#include "logging.hpp"
#include "paper_trading_engine.hpp"
#include "time.hpp"

int main() {
    logging::initialize();

    std::cout << "Starting crypto_trading_platform\n";
    std::cout << "Version: 0.1.0\n";
    std::cout << "Mode: paper\n";
    pipeline::PaperTradingEngine engine{
        "coinbase",
        "BTC-USD",
        strategy::MarketMakingStrategyConfig{20, 2, 10, 0},
        risk::RiskConfig{false, 10, 2, 1'000'000, 100, 10}
    };

    const auto first = engine.on_market_data(market_data::MarketDataEvent{
        "coinbase", "BTC-USD", 49'990, 5, 50'010, 5, 0, now()
    });
    const auto second = engine.on_market_data(market_data::MarketDataEvent{
        "coinbase", "BTC-USD", 49'980, 5, 49'985, 5, 0, now()
    });
    std::cout << "Paper events accepted: "
              << (first.accepted && second.accepted) << '\n';
    std::cout << "Orders created: "
              << first.new_order_ids.size() + second.new_order_ids.size()
              << '\n';
    std::cout << "Execution reports: "
              << first.execution_reports.size() + second.execution_reports.size()
              << '\n';
    std::cout << "Net position (lots): "
              << engine.portfolio().net_position_lots() << '\n';
    std::cout << "Cash (tick-lots): "
              << engine.portfolio().cash_ticks_lots() << '\n';

    return 0;
}
