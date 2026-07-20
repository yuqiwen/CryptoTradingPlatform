#pragma once

#include <cstdint>

using PriceTicks = std::int64_t;
using QuantityLots = std::int64_t;
using TimestampNs = std::int64_t;
using OrderId = std::uint64_t;

enum class Side : std::int8_t
{
    Buy = 0,
    Sell = 1
};

enum class OrderType : std::int8_t
{
    Market = 0,
    Limit = 1
};

enum class OrderStatus : std::int8_t
{
    Created = 0,
    PendingNew = 1,
    Open = 2,
    PartiallyFilled = 3,
    Filled = 4,
    PendingCancel = 5,
    Canceled = 6,
    Rejected = 7
};

