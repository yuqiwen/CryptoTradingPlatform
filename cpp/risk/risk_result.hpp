#pragma once

#include <cstdint>

namespace risk {

    enum class RiskRejectReason : std::int8_t {
        None = 0,
        InvalidQuantity = 1,
        MaxOrderSizeExceeded = 2,
        MaxPositionExceeded = 3,
        MaxNotionalExceeded = 4,
        MaxPriceDeviationExceeded = 5,
        RateLimitExceeded = 6,
        KillSwitchEnabled = 7
    };

    struct RiskResult {
        bool approved;
        RiskRejectReason reject_reason;
    };

}  // namespace risk