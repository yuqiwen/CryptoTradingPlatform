# Project Context: Low-Latency Crypto Trading Infrastructure

## 1. Project Owner and Goal

This project belongs to Yuqi Wen.

The goal is to rebuild and refine previous crypto trading related repositories into a serious quant developer / quant infrastructure project.

This is not intended to be a simple crypto trading bot. The goal is to build a C++-focused event-driven trading infrastructure project that demonstrates knowledge relevant to:

- Quant Developer
- Quant Infrastructure Engineer
- Trading Systems Engineer
- Low-Latency C++ Engineer
- Backend / Infrastructure Engineer for financial systems

The project should emphasize system design, trading infrastructure, correctness, modularity, latency measurement, and clean C++ engineering.

The user wants to write the code personally. Codex should act as a teacher, reviewer, debugging assistant, and step-by-step implementation guide. Codex should not blindly generate large amounts of final code unless explicitly requested.

## 2. Existing GitHub Context

The user has several previous crypto/trading related repositories under github.com/yuqiwen, including:

- CCXTServices
- CryptocurrencyTradingEngine
- MarketDataProcessor
- CryptocurrencyTradingBackend
- DataPreprocessor


Some of these are incomplete, forked, experimental, or earlier-stage projects. They should be treated as source material and learning history, not final architecture.

The new work should be a careful refactor/rebuild based on lessons from these repositories.

Important: Do not assume the old code is correct or production-ready. Review, extract useful pieces, then rebuild into a cleaner architecture.

## 3. Core Project Positioning

The final project should be positioned as:

C++ Event-Driven Crypto Trading Infrastructure

or:

Low-Latency Crypto Trading Infrastructure

One-line project description:

Built a C++ event-driven crypto trading infrastructure with real-time market data ingestion, normalized event processing, in-memory order book reconstruction, strategy execution, risk checks, order lifecycle management, paper trading simulation, and latency benchmarking.

This project should focus on infrastructure, not prediction accuracy or profitability.

## 4. High-Level Architecture

Target architecture:

Exchange WebSocket / REST
        |
        v
Market Data Gateway
        |
        v
Message Normalizer
        |
        v
In-Memory Order Book
        |
        v
Strategy Engine
        |
        v
Risk Engine
        |
        v
Order Manager
        |
        v
Execution Gateway
        |
        v
Paper Trading Simulator / Live Exchange Adapter

Supporting systems:

- Logging
- Config
- Replay
- Backtesting
- Latency benchmarking
- Unit tests
- Documentation
- Optional metrics dashboard later

## 5. Main Design Principle

The project should not be written like a single trading script.

It should be modular:

- Market data should not directly place orders.
- Strategy should not directly talk to the exchange.
- Orders must pass through risk checks.
- Order lifecycle must be tracked by an order manager.
- Paper trading should be supported before live trading.
- Latency should be measured stage by stage.
- Components should be testable independently.

## 6. Suggested Repository Structure

Recommended structure:

crypto-trading-infra/
├── cpp/
│   ├── CMakeLists.txt
│   ├── common/
│   ├── market_data/
│   ├── order_book/
│   ├── strategy/
│   ├── risk/
│   ├── order_manager/
│   ├── execution/
│   ├── simulator/
│   ├── benchmark/
│   └── main.cpp
├── python/
│   └── ccxt_server_coinbase.py
├── config/
│   ├── default.yaml
│   └── symbols.yaml
├── tests/
├── scripts/
├── docs/
│   ├── architecture.md
│   ├── design_notes.md
│   ├── latency_benchmark.md
│   └── interview_notes.md
├── README.md
└── AGENTS.md

## 7. C++ First Policy

The core project should be written in C++.

C++ should own:

- Market data event types
- Order book
- Strategy engine
- Risk engine
- Order manager
- Paper trading simulator
- Latency benchmark
- Core event loop

Python can be kept for:

- Existing ccxt bridge
- REST fallback
- Research notebooks
- Plotting benchmark results
- Quick data collection
- Visualization

The Python ccxt server should not be the main trading engine.

## 8. Initial Technology Stack

Preferred C++ stack:

- C++17 or C++20
- CMake
- Boost.Asio or websocketpp for networking
- nlohmann/json initially
- simdjson later if JSON parsing becomes a bottleneck
- spdlog for logging
- GoogleTest for unit tests
- Google Benchmark or custom benchmark tools for latency testing

Avoid over-engineering in the first version.

Do not introduce Kafka, Flink, Kubernetes, or complex distributed systems until the single-process core trading infrastructure is clean and working.

## 9. Milestone Plan

### Milestone 1: Project Skeleton and Refactor Foundation

Goal:

Create a clean buildable C++ project.

Tasks:

- Create CMake project.
- Add common type definitions.
- Add logging utility.
- Add config loading.
- Add basic README.
- Add placeholder modules.
- Add one simple executable that prints system startup information.

Expected output:

The project builds successfully and has a clean directory structure.

### Milestone 2: Market Data Gateway

Goal:

Connect to one exchange and receive real-time crypto market data.

Start with one exchange and one symbol, such as BTC-USD or BTC-USDT.

Tasks:

- Implement WebSocket client.
- Receive raw market data.
- Parse JSON.
- Normalize raw messages into internal MarketDataEvent.
- Timestamp local receive time.
- Log raw and normalized events.

Example event type:

struct MarketDataEvent {
    std::string exchange;
    std::string symbol;
    double bid_price;
    double bid_size;
    double ask_price;
    double ask_size;
    int64_t exchange_ts_ns;
    int64_t local_recv_ts_ns;
};

Expected output:

Program can print real-time best bid / best ask updates.

### Milestone 3: Order Book Reconstruction

Goal:

Maintain an in-memory order book.

Tasks:

- Support snapshot.
- Support incremental update.
- Maintain bid side and ask side.
- Provide best_bid, best_ask, mid_price, spread.
- Provide top N levels.
- Add consistency checks.
- Add unit tests.

Initial data structure:

- bids: descending price map
- asks: ascending price map

Possible simple version:

std::map<double, double, std::greater<double>> bids;
std::map<double, double> asks;

Later optimization:

- flat_map
- vector price levels
- custom memory pool
- price-indexed array if tick size is fixed

Important concepts:

- Bid side sorted descending.
- Ask side sorted ascending.
- Size 0 means delete level.
- Snapshot + delta must remain consistent.

### Milestone 4: Strategy Engine

Goal:

Implement a simple market making strategy.

The strategy should be simple but professionally structured.

Basic logic:

mid = (best_bid + best_ask) / 2
quote_bid = mid - target_spread / 2
quote_ask = mid + target_spread / 2

Add inventory-aware skew:

If position is too long:
- reduce bid aggressiveness
- make ask more aggressive to sell inventory

If position is too short:
- make bid more aggressive to buy inventory
- reduce ask aggressiveness

The strategy should output desired orders, not submit them directly.

Example interface:

class Strategy {
public:
    virtual std::vector<OrderRequest> onMarketData(
        const OrderBook& book,
        const Portfolio& portfolio
    ) = 0;
};

Important policy:

Strategy should not directly call exchange API.

### Milestone 5: Risk Engine

Goal:

All strategy-generated orders must pass risk checks.

Risk checks:

- Max position
- Max order size
- Max notional exposure
- Max price deviation from mid
- Max orders per second
- Kill switch

Example config:

struct RiskConfig {
    double max_position;
    double max_order_size;
    double max_notional;
    double max_price_deviation_bps;
    int max_orders_per_second;
};

Important design rule:

No order should leave the system before passing RiskEngine.

### Milestone 6: Order Manager

Goal:

Track full order lifecycle.

Order states:

- Created
- PendingNew
- Open
- PartiallyFilled
- Filled
- PendingCancel
- Canceled
- Rejected

Order Manager responsibilities:

- Assign client order IDs.
- Track active orders.
- Track pending cancels.
- Prevent duplicate orders.
- Handle fills.
- Handle cancel acknowledgements.
- Keep local state consistent with exchange or simulator.
- Support cancel/replace logic.

This is a core quant infra component.

### Milestone 7: Paper Trading Simulator

Goal:

Support safe testing without real money.

Simulator behavior:

- If buy price >= best ask, simulate fill.
- If sell price <= best bid, simulate fill.
- Otherwise keep order open.
- Support partial fills later.
- Update position and PnL.
- Track fees and slippage later.

The simulator should produce execution reports similar to a real exchange.

Important:

The same strategy and order manager should work with both paper execution and live execution adapter.

### Milestone 8: Latency Benchmarking

Goal:

Measure system performance like a trading infra project.

Measure:

- Market data receive to parse latency
- Parse to order book update latency
- Order book update to strategy decision latency
- Strategy decision to risk check latency
- Risk check to order manager latency
- End-to-end market data to order request latency

Report:

- p50
- p90
- p99
- p999
- max
- throughput

Important:

Do not only report average latency. Tail latency matters in trading systems.

Create a LatencyRecorder class.

Example:

class LatencyRecorder {
public:
    void record(int64_t latency_ns);
    void report() const;
};

### Milestone 9: Replay and Backtesting

Goal:

Allow saved market data to be replayed.

Tasks:

- Save normalized market data events.
- Replay them into order book and strategy.
- Run paper trading on replayed data.
- Output position, PnL, number of orders, fills, cancels, and latency.

This turns the system into a testable research + infrastructure platform.

### Milestone 10: Final Polish

Goal:

Make the project resume-ready.

Tasks:

- Improve README.
- Add architecture diagram.
- Add benchmark report.
- Add unit test examples.
- Add design notes.
- Add interview talking points.
- Add clear build/run instructions.
- Add screenshots or terminal demo.
- Write resume bullets.

## 10. Coding Style Expectations

Use clean, modern C++.

Prefer:

- RAII
- const correctness
- move semantics where useful
- clear ownership
- small classes with focused responsibilities
- explicit interfaces
- no global mutable state unless justified
- strongly typed enums for order side, order status, event type
- clear error handling

Avoid:

- giant monolithic files
- hidden side effects
- strategy directly calling execution
- excessive inheritance
- premature template complexity
- premature lock-free optimization before correctness
- hardcoded API keys
- committing secrets

## 11. Teaching Mode for Codex

Codex should behave like a mentor.

When the user asks for implementation help:

1. First explain the design.
2. Then propose a small next step.
3. Then provide code only for that step.
4. Explain why the code is written that way.
5. Mention what should be tested.
6. Avoid generating the entire project at once.

The user wants to learn and personally write the code.

Codex should ask for existing file content when necessary instead of assuming the codebase structure.

Codex should prefer incremental diffs over massive rewrites.

## 12. Code Review Mode for Codex

When reviewing code, Codex should check:

- Correctness
- Ownership and lifetime
- C++ style
- Thread safety
- Latency impact
- Allocation behavior
- API design
- Testability
- Whether the component fits the trading infrastructure architecture

Codex should explain issues clearly and propose minimal changes.

## 13. Debugging Mode for Codex

When debugging, Codex should:

- Identify the exact failure mode.
- Explain likely root cause.
- Suggest minimal reproducible test.
- Avoid changing unrelated architecture.
- Explain how to verify the fix.

## 14. Performance Mode for Codex

Performance optimization should happen after correctness.

Priority order:

1. Correctness
2. Clean architecture
3. Test coverage
4. Measured performance
5. Optimization

Do not optimize based on guesses.

Use measurements:

- p50
- p99
- p999
- allocation count
- CPU usage
- throughput

Potential optimizations later:

- Avoid heap allocation in hot path.
- Use object pools.
- Use ring buffers.
- Avoid excessive string copies.
- Use string_view carefully.
- Use simdjson for parsing.
- Improve cache locality.
- Avoid false sharing.
- Use lock-free queue only when justified.

## 15. Important Interview Framing

This project should be framed as infrastructure, not as a trading profit project.

Good explanation:

A trading bot focuses mainly on signal generation. This project focuses on trading infrastructure: market data ingestion, order book reconstruction, event-driven architecture, order lifecycle management, risk checks, execution simulation, and latency profiling.

Important interview questions the project should prepare for:

1. What is the overall system architecture?
2. Why do you need an order manager?
3. Why should strategy not directly submit orders?
4. How do you reconstruct an order book?
5. How do you handle snapshot and incremental updates?
6. What risk checks do you apply before sending orders?
7. How do you measure latency?
8. Why do you care about p99 latency?
9. How would you improve performance?
10. How would you support multiple exchanges?
11. How would you support live trading and paper trading with the same strategy?
12. How do you prevent local order state from diverging from exchange state?

## 16. Scope Control

First version should support:

- One exchange
- One symbol
- One process
- One simple market making strategy
- Paper trading
- Order book
- Risk checks
- Order manager
- Latency benchmark

Do not start with:

- Multi-exchange arbitrage
- Real-money auto trading
- Complex ML/RL strategy
- Kafka/Flink pipeline
- Kubernetes
- Distributed system
- Kernel bypass networking
- Full FIX engine

Those can be future extensions.

## 17. Safety and Secrets

Never commit API keys, secrets, private credentials, or account information.

Live trading should be disabled by default.

Paper trading should be the default mode.

Use environment variables or local config files ignored by git for secrets.

## 18. Current Priority

The current priority is to rebuild the project from previous crypto repos into a clean C++ trading infrastructure.

Start with:

1. Repo structure
2. CMake
3. Common types
4. Logging
5. Market data event model
6. Order book
7. Unit tests

Only after the foundation is solid should we move to live WebSocket and execution.