#ifndef XJADEO_TIME_UTILS_H
#define XJADEO_TIME_UTILS_H

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

/**
 * Get monotonic time in microseconds
 * 
 * Returns a monotonic clock value that always increases and doesn't suffer
 * discontinuities when the system time changes.
 * 
 * On Linux, uses CLOCK_MONOTONIC via std::chrono.
 * 
 * @return Monotonic time in microseconds
 */
int64_t xj_get_monotonic_time(void);

#ifdef __cplusplus
}
#endif

#endif // XJADEO_TIME_UTILS_H

