# 90-Day Build Plan

## Goal

Build a clean, interview-ready **C++ event-driven crypto trading infrastructure** in roughly 3 months.

This plan follows the principles in `AGENTS.md`:

- C++ first
- infrastructure over trading profit
- modular boundaries
- correctness before optimization
- paper trading before live trading
- latency measurement as a first-class concern
- small daily tasks instead of large bursts

The plan is designed for steady progress with **one focused task per day**. Some days are intentionally lighter and used for cleanup, testing, reading, or design notes.

## Weekly Rhythm

Use this default rhythm unless a week needs adjustment:

- `Day 1-4`: implement one small piece each day
- `Day 5`: tests + refactor
- `Day 6`: docs + notes + interview framing
- `Day 7`: buffer / catch-up / rest

That keeps the pace realistic and prevents the repo from turning into rushed prototype code.

## Month 1: Foundation and Core Market Data Structures

### Week 1: Repo Skeleton and Build System

Goal: get a clean C++ project that builds and runs.

- `Day 1`: create repo folders and decide module boundaries
- `Day 2`: write root `CMakeLists.txt` and `cpp/CMakeLists.txt`
- `Day 3`: add `main.cpp` with startup logging and version/build info
- `Day 4`: add `common/types.hpp` with basic aliases and enums
- `Day 5`: add `common/time.hpp` and a minimal timestamp helper
- `Day 6`: write `docs/architecture.md` with a one-page component overview
- `Day 7`: review structure and simplify anything that already feels awkward

Deliverable:

- clean directory structure
- buildable executable
- initial architecture note

### Week 2: Logging, Config, and Event Model

Goal: define shared infrastructure used by later modules.

- `Day 8`: add logging bootstrap interface
- `Day 9`: add logging implementation and startup log format
- `Day 10`: add config file layout under `config/`
- `Day 11`: define a small app config struct
- `Day 12`: define `MarketDataEvent`
- `Day 13`: document why `MarketDataEvent` belongs in `market_data/`, not `common/`
- `Day 14`: review naming, include paths, and dependency direction

Deliverable:

- logging utility
- config layout
- first normalized market data event type

### Week 3: Order Book API Design

Goal: design before implementation.

- `Day 15`: define `PriceLevel` and book-side representations
- `Day 16`: decide on initial container choice: `std::map` for correctness-first
- `Day 17`: write `OrderBook` public API only
- `Day 18`: define snapshot and update input types
- `Day 19`: write invariants the order book must preserve
- `Day 20`: create test cases on paper before coding
- `Day 21`: write `docs/design_notes.md` section on snapshot vs delta handling

Deliverable:

- stable `OrderBook` interface
- documented invariants
- planned tests before implementation

### Week 4: Order Book Implementation and Tests

Goal: implement the first important infra component well.

- `Day 22`: implement empty book state and clear/reset behavior
- `Day 23`: implement snapshot load
- `Day 24`: implement bid/ask level update logic
- `Day 25`: implement delete-on-zero-size logic
- `Day 26`: implement `best_bid`, `best_ask`, `mid_price`, `spread`
- `Day 27`: add unit tests for snapshot/update/delete/best-price behavior
- `Day 28`: refactor for clarity and write short interview notes on order book design

Deliverable:

- buildable in-memory order book
- first meaningful test coverage

## Month 2: Strategy, Risk, and Order Lifecycle

### Week 5: Core Trading Types

Goal: define types that will connect strategy, risk, and execution.

- `Day 29`: define `OrderRequest`
- `Day 30`: define `Order`
- `Day 31`: define `ExecutionReport`
- `Day 32`: define `Portfolio` or `PositionView`
- `Day 33`: define `OrderStatus` transitions
- `Day 34`: write tests or assertions for state assumptions
- `Day 35`: document order lifecycle in `docs/architecture.md`

Deliverable:

- shared order/execution domain model

### Week 6: Strategy Interface and First Quoting Logic

Goal: strategy produces desired orders, not exchange calls.

- `Day 36`: define `Strategy` interface
- `Day 37`: define a simple market making config struct
- `Day 38`: implement mid-price based quote generation
- `Day 39`: implement target spread logic
- `Day 40`: implement inventory-aware skew inputs
- `Day 41`: test strategy behavior against a mocked order book
- `Day 42`: write short notes explaining why strategy must not talk directly to execution

Deliverable:

- first strategy component
- clean separation between signal generation and order submission

### Week 7: Risk Engine

Goal: no order leaves the system without checks.

- `Day 43`: define `RiskConfig`
- `Day 44`: define `RiskResult` and rejection reason types
- `Day 45`: implement max order size check
- `Day 46`: implement max position check
- `Day 47`: implement max notional and max deviation from mid
- `Day 48`: add tests for approve/reject paths
- `Day 49`: document where risk sits in the event flow

Deliverable:

- first real risk gate
- tests for core rejection logic

### Week 8: Order Manager Foundations

Goal: build the infra component that makes the project feel like a trading system.

- `Day 50`: define `OrderManager` responsibilities and internal maps
- `Day 51`: implement client order ID generation
- `Day 52`: implement create -> pending new -> open path
- `Day 53`: implement reject handling
- `Day 54`: implement fill and partial fill handling
- `Day 55`: implement cancel request state transitions
- `Day 56`: test lifecycle transitions and duplicate-prevention cases

Deliverable:

- first order lifecycle tracker
- explicit local state transitions

## Month 3: Simulation, Latency, Replay, and Resume Polish

### Week 9: Paper Trading Simulator

Goal: support safe end-to-end strategy testing.

- `Day 57`: define simulator interface and execution contract
- `Day 58`: implement basic crossing logic for buys
- `Day 59`: implement basic crossing logic for sells
- `Day 60`: emit simulated execution reports
- `Day 61`: update portfolio and position
- `Day 62`: test simple fill scenarios
- `Day 63`: document paper vs live adapter boundary

Deliverable:

- paper execution path compatible with order manager

### Week 10: End-to-End Event Flow

Goal: wire the core together in one process.

- `Day 64`: define basic event flow in `main.cpp`
- `Day 65`: connect market data event -> order book update
- `Day 66`: connect order book -> strategy
- `Day 67`: connect strategy -> risk engine
- `Day 68`: connect risk-approved orders -> order manager
- `Day 69`: connect order manager -> simulator
- `Day 70`: run a toy scenario and log the full path

Deliverable:

- first single-process skeleton demonstrating the full pipeline

### Week 11: Latency Measurement

Goal: make this look like trading infrastructure, not just a bot.

- `Day 71`: define `LatencyRecorder`
- `Day 72`: implement raw latency recording
- `Day 73`: compute p50, p90, p99, p999, max
- `Day 74`: add stage-by-stage timestamps
- `Day 75`: measure end-to-end event-to-order latency
- `Day 76`: write a simple benchmark driver or replay harness
- `Day 77`: write `docs/latency_benchmark.md`

Deliverable:

- latency instrumentation
- tail latency reporting

### Week 12: Replay and Testability

Goal: make the project usable for repeatable experiments.

- `Day 78`: choose a normalized event serialization format
- `Day 79`: add market data event writer
- `Day 80`: add replay reader
- `Day 81`: feed replayed events into order book and strategy
- `Day 82`: collect replay outputs: orders, fills, position, PnL
- `Day 83`: add one deterministic replay test
- `Day 84`: document replay flow and future backtest extension

Deliverable:

- replayable data path
- deterministic development workflow

### Week 13: Resume-Ready Polish

Goal: package the project like a serious infra portfolio piece.

- `Day 85`: improve `README.md`
- `Day 86`: finalize architecture diagram and component descriptions
- `Day 87`: write interview Q&A notes
- `Day 88`: add build/run/test examples
- `Day 89`: summarize benchmark outputs and known limitations
- `Day 90`: write 3-5 resume bullets and a short project narrative

Deliverable:

- interview-ready documentation
- polished portfolio framing

## Scope Rules

To stay aligned with `AGENTS.md`, do not expand scope during these 90 days into:

- multi-exchange support
- live trading by default
- distributed services
- ML/RL strategy work
- kernel bypass optimization
- premature lock-free redesign

If you finish early, spend the extra time on:

- tests
- docs
- replay robustness
- order manager correctness
- latency measurement quality

## Recommended Milestone Checkpoints

Use these checkpoints to know whether progress is healthy:

- `End of Week 2`: project builds, logs startup, has shared types and event model
- `End of Week 4`: order book works and has unit tests
- `End of Week 8`: strategy, risk, and order manager all exist with clean interfaces
- `End of Week 10`: single-process end-to-end flow runs in paper mode
- `End of Week 12`: replay and latency reporting exist
- `End of Week 13`: docs and interview framing are polished

## How We Should Work Together

For this repo, a good cadence is:

1. decide the next tiny component
2. design the interface first
3. implement the smallest useful version
4. add tests
5. review for architecture and infra fit
6. only then move on

This keeps the project educational, clean, and useful for quant infrastructure interviews.
