# Design Notes

## Current Dependency Direction

The current codebase follows a low-level to high-level dependency structure.

- `common/` contains low-level shared building blocks such as primitive trading types, timestamps, configuration structs, and small utilities.
- Domain-specific objects should remain in the module that owns their meaning.
- Higher-level modules may depend on lower-level types and utilities.
- Lower-level modules should not depend on higher-level domain modules.

Examples of the current intended direction:

- `time.hpp` may depend on `types.hpp`
- `market_data_event.hpp` may depend on `types.hpp`
- `common/` should not depend on `market_data/`, `strategy/`, or `order_manager/`

## Common vs Domain Ownership

Shared usage alone is not enough reason to move a type into `common/`.

- `common/` is for lower-level cross-domain building blocks
- `market_data/` owns normalized market data domain objects
- future execution-related types should remain in execution-facing modules unless they are truly low-level shared abstractions

For example, `MarketDataEvent` may be consumed by multiple modules, but it still belongs to `market_data/` because it represents a market-data domain event rather than a general-purpose infrastructure primitive.

## Naming Notes

Current naming choices aim to make internal units explicit.

- prices use tick-based internal representation: `PriceTicks`
- quantities use lot-based internal representation: `QuantityLots`
- timestamps use nanosecond integer representation: `TimestampNs`
- order identifiers use `OrderId`

Current conventions should stay consistent unless a strong reason appears to change them.

- use explicit unit-bearing names when internal values are discrete
- keep market-data fields explicit, such as `bid_price_ticks` and `ask_size_lots`
- avoid introducing alternate names for the same concept without need

## Include Guidance

Headers should include only the dependencies they directly need.

- low-level headers should remain lightweight
- implementation files should include their own header first when practical
- include direction should follow module layering rather than convenience

The current include layout is intentionally simple. As the project grows, include paths may be made more explicit to reflect module ownership more clearly.

## Order Book Container Choice

The first order book implementation uses `std::map` to prioritize correctness, ordering clarity, and simple update behavior.

- bids are stored in descending price order
- asks are stored in ascending price order
- snapshot and delta handling should be easy to reason about
- testing and debugging should remain straightforward

This is an intentional first-version tradeoff rather than a claim that `std::map` is the final high-performance structure. More cache-friendly or allocation-aware alternatives can be evaluated later once the first implementation is correct and benchmarked.

## Order Book Invariants

The order book implementation should preserve the following invariants:

- bids are ordered from highest price to lowest price
- asks are ordered from lowest price to highest price
- each side contains at most one level per price
- stored price levels must have positive price values
- stored price levels must have positive quantity values
- an update with zero quantity removes the corresponding level
- if both sides are non-empty, the best bid must be strictly less than the best ask

Some of these properties are supported naturally by the current container choice, while others must be enforced explicitly by order book update logic. These invariants define correctness for the first implementation and should guide both implementation and unit testing.

## Order Book Test Plan

The first order book implementation should be validated against the following scenarios:

- empty book after default construction
- snapshot loads both sides correctly
- snapshot replaces previous state rather than merging into it
- update inserts a new bid or ask level at the correct position
- update modifies an existing level without creating duplicates
- zero-quantity update removes the corresponding level
- best bid and best ask are returned correctly
- spread and mid price are computed correctly
- clear resets the book to an empty state
- crossed-book scenarios are covered explicitly as invalid or special-case behavior

These scenarios define the minimum test surface for the first implementation. Later tests can expand into malformed inputs, replay-driven cases, and higher-volume update sequences once the core behavior is stable.

## Snapshot vs Delta Handling

The order book should treat snapshots and incremental updates as different operations with different semantics.

### Snapshot Semantics

A snapshot represents a full replacement view of the current book state.

- applying a snapshot should clear the existing book state first
- the new bid side should be rebuilt entirely from the snapshot bids
- the new ask side should be rebuilt entirely from the snapshot asks
- levels not present in the new snapshot should not remain in the book

This means snapshot handling should behave as replace, not merge.

### Delta Semantics

A delta update represents a modification to a single side and price level.

- if the updated quantity is positive, the corresponding level should be inserted or overwritten
- if the updated quantity is zero, the corresponding level should be removed
- a delta update should affect only the specified side and price

Delta handling should preserve the existing book state everywhere else.

### Correctness Notes

Snapshot and delta processing should both preserve the order book invariants already defined above.

- bid ordering must remain descending
- ask ordering must remain ascending
- no stored level should have zero or negative quantity
- crossed-book states should be treated as explicitly invalid or handled by a clearly documented policy

The first implementation should keep these semantics simple and explicit before any attempt to optimize update throughput or parsing performance.

## First Order Book Implementation Summary

The current `OrderBook` implementation is intentionally correctness-first.

Implemented behavior:

- empty-state handling through `clear()` and `empty()`
- full replacement snapshot handling through `apply_snapshot(...)`
- single-level delta handling through `apply_update(...)`
- best bid and best ask queries
- spread and mid-price queries

Current implementation choices:

- bids and asks are stored in `std::map`
- invalid snapshot or update levels with non-positive price are ignored
- invalid snapshot levels with non-positive quantity are ignored
- delta updates with zero quantity remove the corresponding level
- delta updates with negative quantity are ignored
- best-level queries return `std::optional<PriceLevel>`
- spread and mid-price queries return `std::optional<PriceTicks>`

Intentional limitations of the first version:

- crossed-book handling is not yet enforced by explicit validation logic
- `mid_price_ticks()` currently uses integer division and therefore truncates half-tick cases
- there is no explicit sequence-number or exchange-ordering handling yet
- there is no top-N snapshot/export interface yet
- invalid input policy is currently ignore-based rather than reject/assert-based

This version is meant to establish clear semantics, test coverage, and a stable public interface before performance optimization or richer market-data handling is introduced.

## Order Book Interview Framing

The first version of the order book is designed to demonstrate infrastructure thinking rather than low-level optimization.

Key points:

- the initial container choice favors correctness and simple reasoning over cache efficiency
- snapshot and delta are modeled as distinct state-transition operations
- invalid levels are filtered to preserve explicit invariants
- best-price and derived-price queries are separated from raw update logic
- absence of market state is represented explicitly with `std::optional`

If asked why the implementation is not yet highly optimized, the correct answer is that correctness, invariants, and testability were prioritized first, and container or numeric refinements should follow measurement rather than speculation.

## Order Status Transitions

The first order lifecycle model uses the following allowed transitions:

- `Created -> PendingNew`
- `PendingNew -> Open`
- `PendingNew -> Rejected`
- `Open -> PartiallyFilled`
- `Open -> Filled`
- `Open -> PendingCancel`
- `PartiallyFilled -> PartiallyFilled`
- `PartiallyFilled -> Filled`
- `PartiallyFilled -> PendingCancel`
- `PendingCancel -> Cancelled`

The following states are treated as terminal in the first implementation:

- `Filled`
- `Cancelled`
- `Rejected`

This is an intentionally simplified first-version lifecycle. More complex exchange behaviors can be added later once the basic order manager flow is implemented and tested.

## Why Strategy Does Not Submit Orders Directly

The strategy layer is responsible for producing desired order requests from market state and position state. It is not responsible for direct exchange communication or execution-side state transitions.

This separation is intentional for several reasons:

- all strategy output must pass through centralized risk checks before leaving the system
- order lifecycle tracking belongs to the order manager rather than the strategy
- the same strategy should work with both paper execution and live execution backends
- strategy behavior is easier to unit test when it returns `OrderRequest` objects instead of calling execution interfaces directly
- decision logic and execution mechanics are different concerns and should remain decoupled

In this architecture, the strategy proposes actions, but later layers decide whether those actions are allowed, how they are tracked, and where they are executed.
