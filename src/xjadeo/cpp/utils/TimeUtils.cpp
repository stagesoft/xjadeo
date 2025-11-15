#include "TimeUtils.h"
#include <chrono>

extern "C" {

int64_t xj_get_monotonic_time(void) {
    // Use std::chrono::steady_clock for monotonic time on Linux
    // This is equivalent to CLOCK_MONOTONIC
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    return microseconds.count();
}

} // extern "C"

