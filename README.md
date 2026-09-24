# Crypto Trading Platform

A C++ event-driven crypto trading infrastructure project focused on market data, order book reconstruction, risk checks, order lifecycle management, paper trading, and latency measurement.

## Current status

The C++ core has an order book, market-making strategy, pre-trade risk checks,
order lifecycle manager, paper execution simulator, and integer-based portfolio
accounting. `PaperTradingEngine` connects them for one exchange and one symbol:

```text
Normalized best-bid/best-ask event -> order book -> outstanding paper fills
    -> strategy -> risk -> order manager -> paper simulator -> portfolio
```

`trading_app` runs two deterministic paper events: it opens quotes and then
fills the resting buy when the market moves. It does not connect to an
exchange or send live orders.

Current simulator limits: it fills a marketable order at the best opposite
price without modeling displayed size, fees, or slippage. A post-only order
that would immediately trade is rejected. The engine keeps at most one active
quote per side; cancel/replace, live market data, replay, and latency
measurement are still future work.

## Build and test

With CMake and a C++20 compiler:

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

GoogleTest is downloaded by CMake when tests are enabled. To build only the
application, configure with `-DCTP_BUILD_TESTS=OFF`.
