#include "paper_trading_dispatcher.hpp"

#include <functional>
#include <stdexcept>
#include <utility>

namespace pipeline {

PaperTradingDispatcher::Worker::~Worker() {
    {
        std::lock_guard lock(mutex);
        stopping = true;
    }
    ready.notify_all();
}

PaperTradingDispatcher::PaperTradingDispatcher(
    std::string exchange,
    std::vector<SymbolConfig> symbols,
    std::size_t worker_count,
    std::size_t queue_capacity_per_worker)
    : exchange_(std::move(exchange)),
      queue_capacity_per_worker_(queue_capacity_per_worker) {
    if (exchange_.empty() || symbols.empty() || worker_count == 0 ||
        queue_capacity_per_worker_ == 0) {
        throw std::invalid_argument("dispatcher requires markets, workers, and queue capacity");
    }

    workers_.reserve(worker_count);
    for (std::size_t i = 0; i < worker_count; ++i) {
        workers_.push_back(std::make_unique<Worker>());
    }

    std::vector<std::size_t> worker_load(worker_count, 0);
    for (const auto& config : symbols) {
        if (config.symbol.empty()) {
            throw std::invalid_argument("symbol must not be empty");
        }
        const std::size_t preferred =
            std::hash<std::string>{}(config.symbol) % worker_count;
        std::size_t worker_id = preferred;
        for (std::size_t offset = 1; offset < worker_count; ++offset) {
            const std::size_t candidate = (preferred + offset) % worker_count;
            if (worker_load[candidate] < worker_load[worker_id]) {
                worker_id = candidate;
            }
        }
        if (!symbol_to_worker_.emplace(config.symbol, worker_id).second) {
            throw std::invalid_argument("duplicate symbol");
        }
        ++worker_load[worker_id];
        workers_[worker_id]->engines.emplace(
            config.symbol,
            std::make_unique<PaperTradingEngine>(
                exchange_, config.symbol, config.strategy, config.risk));
    }

    for (std::size_t i = 0; i < workers_.size(); ++i) {
        Worker* worker = workers_[i].get();
        worker->thread = std::jthread([worker, i] { run_worker(*worker, i); });
    }
}

PaperTradingDispatcher::~PaperTradingDispatcher() {
    stop();
}

std::optional<std::future<ProcessedEvent>> PaperTradingDispatcher::submit(
    market_data::MarketDataEvent event) {
    if (event.exchange != exchange_) {
        return std::nullopt;
    }
    const auto route = symbol_to_worker_.find(event.symbol);
    if (route == symbol_to_worker_.end()) {
        return std::nullopt;
    }
    auto& worker = *workers_[route->second];

    std::promise<ProcessedEvent> completion;
    auto future = completion.get_future();
    {
        std::lock_guard lock(worker.mutex);
        if (worker.stopping || worker.queue.size() >= queue_capacity_per_worker_) {
            return std::nullopt;
        }
        worker.queue.push_back(Task{std::move(event), std::move(completion)});
    }
    worker.ready.notify_one();
    return future;
}

std::size_t PaperTradingDispatcher::worker_for(std::string_view symbol) const {
    return symbol_to_worker_.at(std::string(symbol));
}

void PaperTradingDispatcher::stop() {
    for (const auto& worker : workers_) {
        {
            std::lock_guard lock(worker->mutex);
            worker->stopping = true;
        }
        worker->ready.notify_all();
    }
    for (const auto& worker : workers_) {
        if (worker->thread.joinable()) {
            worker->thread.join();
        }
    }
}

void PaperTradingDispatcher::run_worker(Worker& worker, std::size_t worker_id) {
    for (;;) {
        Task task;
        {
            std::unique_lock lock(worker.mutex);
            worker.ready.wait(lock, [&] {
                return worker.stopping || !worker.queue.empty();
            });
            if (worker.queue.empty()) {
                return;
            }
            task = std::move(worker.queue.front());
            worker.queue.pop_front();
        }

        try {
            auto& engine = *worker.engines.at(task.event.symbol);
            auto result = engine.on_market_data(task.event);
            const auto bid = engine.order_book().best_bid();
            const auto ask = engine.order_book().best_ask();
            task.completion.set_value(ProcessedEvent{
                task.event.symbol,
                worker_id,
                std::move(result),
                bid ? std::optional<PriceTicks>{bid->price_ticks} : std::nullopt,
                ask ? std::optional<PriceTicks>{ask->price_ticks} : std::nullopt,
                engine.portfolio().snapshot()
            });
        } catch (...) {
            task.completion.set_exception(std::current_exception());
        }
    }
}

}  // namespace pipeline
