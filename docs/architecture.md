# Architecture

## Project Goal

This project is a C++ event-driven crypto trading infrastructure built to demonstrate core trading systems engineering skills rather than trading profitability.

The main focus is on clean system design, modular boundaries, correctness, testability, order lifecycle management, paper trading support, and latency measurement. The project is designed to look and behave like a small but serious trading infrastructure platform, not like a single-script trading bot.

The initial scope is intentionally limited to one exchange, one symbol, one process, one simple strategy, and paper trading by default. The goal is to build a clean core first, then extend it in a controlled way.

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
