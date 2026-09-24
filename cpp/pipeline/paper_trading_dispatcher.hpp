#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "paper_trading_engine.hpp"

namespace pipeline {

struct SymbolConfig {
    std::string symbol;
    strategy::MarketMakingStrategyConfig strategy;
    risk::RiskConfig risk;
};

struct ProcessedEvent {
    std::string symbol;
    std::size_t worker_id;
    EventResult result;
    std::optional<PriceTicks> best_bid_ticks;
    std::optional<PriceTicks> best_ask_ticks;
    simulator::PortfolioSnapshot portfolio;
};

class PaperTradingDispatcher {
public:
    PaperTradingDispatcher(
        std::string exchange,
        std::vector<SymbolConfig> symbols,
        std::size_t worker_count,
        std::size_t queue_capacity_per_worker);
    ~PaperTradingDispatcher();

    PaperTradingDispatcher(const PaperTradingDispatcher&) = delete;
    PaperTradingDispatcher& operator=(const PaperTradingDispatcher&) = delete;

    [[nodiscard]] std::optional<std::future<ProcessedEvent>> submit(
        market_data::MarketDataEvent event);
    [[nodiscard]] std::size_t worker_for(std::string_view symbol) const;
    void stop();

private:
    struct Task {
        market_data::MarketDataEvent event;
        std::promise<ProcessedEvent> completion;
    };

    struct Worker {
        std::mutex mutex;
        std::condition_variable ready;
        std::deque<Task> queue;
        std::unordered_map<std::string, std::unique_ptr<PaperTradingEngine>> engines;
        bool stopping = false;
        std::jthread thread;

        ~Worker();
    };

    static void run_worker(Worker& worker, std::size_t worker_id);

    std::string exchange_;
    std::size_t queue_capacity_per_worker_;
    std::vector<std::unique_ptr<Worker>> workers_;
    std::unordered_map<std::string, std::size_t> symbol_to_worker_;
};

}  // namespace pipeline
