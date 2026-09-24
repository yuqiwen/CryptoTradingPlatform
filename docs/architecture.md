# Architecture

## Project Goal

This project is a C++ event-driven crypto trading infrastructure built to demonstrate core trading systems engineering skills rather than trading profitability.

The main focus is on clean system design, modular boundaries, correctness, testability, order lifecycle management, paper trading support, and latency measurement. The project is designed to look and behave like a small but serious trading infrastructure platform, not like a single-script trading bot.

The current scope is one exchange, multiple configured symbols, one process, one simple strategy per symbol, and paper trading by default.

## High-Level Pipeline

```text
Exchange WebSocket / REST
    -> Market Data Gateway
    -> Message Normalizer
    -> In-Memory Order Book
    -> Strategy Engine
    -> Risk Engine
    -> Order Manager
    -> Execution Gateway
    -> Paper Trading Simulator / Live Exchange Adapter
```

## Module Ownership

Shared usage alone is not sufficient reason to place a type in `common/`. The `common/` module is reserved for lower-level cross-domain building blocks such as primitive trading types, enums, timestamps, and small utilities.

Domain objects should remain in the module that owns their meaning. For example, `MarketDataEvent` may be consumed by multiple downstream modules, but it still belongs to `market_data/` because it represents normalized market data rather than a low-level common abstraction.

## Paper Event Flow

`pipeline::PaperTradingDispatcher` hashes each configured symbol to a worker,
then probes for a less loaded worker when possible. Each worker owns a bounded
FIFO queue and one thread. It alone accesses the `PaperTradingEngine` instances
for its symbols, so a symbol's events are processed in submission order
without locking its book or order manager. Different symbols can run in
parallel when assigned to different workers. A stopped dispatcher drains
accepted work before joining its threads.

`market_data::MarketDataEvent` carries a full snapshot or an incremental
batch and a normalized contiguous sequence number. `BookStreamProcessor`
rejects stale events, detects gaps, and validates an entire update batch
before exposing the new book. A gap or invalid delta clears the book and
requires a newer snapshot. The paper engine cancels its outstanding quotes
when the stream loses synchronization. The exchange adapter will need to
translate exchange-specific sequence rules into this contract.

`pipeline::PaperTradingEngine` owns each symbol's book stream, strategy, risk
engine, order manager, simulator, and portfolio. After a valid update it checks
existing resting orders for fills, then requests fresh strategy quotes. Each new quote
passes risk before it receives an order ID. Execution reports advance the
order lifecycle and fill reports update the portfolio. Only one active order
per side is retained until cancel/replace is implemented.

The simulator rejects a new post-only order if it crosses the opposite best
price; an already open order may fill when a later event moves the market.
The paper path currently models a best-price fill only. It has no queue
position, depth consumption, fees, slippage, exchange acknowledgements,
or live gateway.

Copying the book for each delta batch favors correctness and atomicity. This
cost must be measured before replacing it with an in-place or journaled
update path. Worker queues currently have no throughput or latency telemetry.
