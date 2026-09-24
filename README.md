# Crypto Trading Platform

A C++ event-driven crypto trading infrastructure project focused on market data, order book reconstruction, risk checks, order lifecycle management, paper trading, and latency measurement.

## Current status

The C++ core has an order book, market-making strategy, pre-trade risk checks,
order lifecycle manager, paper execution simulator, and integer-based portfolio
accounting. `PaperTradingEngine` connects them for one exchange and one symbol.
`PaperTradingDispatcher` routes configured symbols to worker threads:

```text
Normalized snapshot/delta -> symbol route -> bounded worker queue
    -> sequenced order book -> outstanding paper fills
    -> strategy -> risk -> order manager -> paper simulator -> portfolio
```

Each symbol is pinned to one worker and has its own book, strategy, risk,
orders, and portfolio. Deltas require contiguous sequence numbers. A gap or
invalid delta clears that symbol's book, cancels its paper quotes, and waits
for a new snapshot. The worker queue rejects submissions when full.

`trading_app` runs deterministic BTC-USD and ETH-USD paper events across two
workers. A BTC delta moves the market and fills a resting buy. The app does
not connect to an exchange or send live orders.

Current simulator limits: it fills a marketable order at the best opposite
price without modeling displayed size, fees, or slippage. A post-only order
that would immediately trade is rejected. The engine keeps at most one active
quote per side; routine cancel/replace, live market data, replay, and latency
measurement are still future work. Incremental updates currently copy the
book before applying a batch so invalid updates cannot partially mutate it.

## Build and test

With CMake and a C++20 compiler:

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

GoogleTest is downloaded by CMake when tests are enabled. To build only the
application, configure with `-DCTP_BUILD_TESTS=OFF`.
