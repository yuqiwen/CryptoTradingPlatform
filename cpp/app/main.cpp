#include <iostream>

#include "logging.hpp"
#include "paper_trading_dispatcher.hpp"
#include "time.hpp"

int main() {
    logging::initialize();

    std::cout << "Starting crypto_trading_platform\n";
    std::cout << "Version: 0.1.0\n";
    std::cout << "Mode: paper\n";
    pipeline::PaperTradingDispatcher dispatcher{
        "coinbase",
        {
            {"BTC-USD",
             strategy::MarketMakingStrategyConfig{20, 2, 10, 0},
             risk::RiskConfig{false, 10, 2, 1'000'000, 100, 10}},
            {"ETH-USD",
             strategy::MarketMakingStrategyConfig{20, 2, 10, 0},
             risk::RiskConfig{false, 10, 2, 1'000'000, 100, 10}}
        },
        2,
        1024
    };

    auto first = dispatcher.submit(market_data::MarketDataEvent{
        "coinbase", "BTC-USD", 1, 0, now(),
        order_book::BookSnapshot{{{49'990, 5}}, {{50'010, 5}}}
    });
    auto other = dispatcher.submit(market_data::MarketDataEvent{
        "coinbase", "ETH-USD", 1, 0, now(),
        order_book::BookSnapshot{{{2'990, 5}}, {{3'010, 5}}}
    });
    auto second = dispatcher.submit(market_data::MarketDataEvent{
        "coinbase", "BTC-USD", 2, 0, now(),
        std::vector<order_book::BookUpdate>{
            {order_book::BookSide::Bid, 49'990, 0},
            {order_book::BookSide::Ask, 50'010, 0},
            {order_book::BookSide::Bid, 49'980, 5},
            {order_book::BookSide::Ask, 49'985, 5}
        }
    });
    if (!first || !other || !second) {
        std::cerr << "Paper event queue is unavailable\n";
        return 1;
    }

    const auto first_result = first->get();
    const auto other_result = other->get();
    const auto second_result = second->get();
    std::cout << "Paper events accepted: "
              << (first_result.result.accepted &&
                  other_result.result.accepted &&
                  second_result.result.accepted) << '\n';
    std::cout << "Orders created: "
              << first_result.result.new_order_ids.size() +
                     other_result.result.new_order_ids.size() +
                     second_result.result.new_order_ids.size()
              << '\n';
    std::cout << "Execution reports: "
              << first_result.result.execution_reports.size() +
                     other_result.result.execution_reports.size() +
                     second_result.result.execution_reports.size()
              << '\n';
    std::cout << "BTC worker: " << second_result.worker_id << '\n';
    std::cout << "BTC net position (lots): "
              << second_result.portfolio.net_position_lots << '\n';
    std::cout << "BTC cash (tick-lots): "
              << second_result.portfolio.cash_ticks_lots << '\n';
    std::cout << "ETH worker: " << other_result.worker_id << '\n';
    std::cout << "ETH net position (lots): "
              << other_result.portfolio.net_position_lots << '\n';

    return 0;
}
