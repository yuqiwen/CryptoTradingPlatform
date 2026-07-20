#pragma once

#include <chrono>
#include "types.hpp"

inline TimestampNs now(){

    const auto now = std::chrono::high_resolution_clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()
    );

    return ns.count();
}