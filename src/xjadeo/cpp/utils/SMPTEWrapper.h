#ifndef XJADEO_SMPTE_WRAPPER_H
#define XJADEO_SMPTE_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * C-compatible wrappers for SMPTEUtils C++ class
 * 
 * These functions provide the same interface as the original smpte.c functions
 * but use the C++ SMPTEUtils implementation internally.
 */

/**
 * Convert SMPTE timecode string to frame number
 * Format: [[[HH:]MM:]SS:]FF
 */
int64_t smptestring_to_frame(const char* str);

/**
 * Convert frame number to SMPTE timecode string
 * @param smptestring Output buffer (must be at least 14 bytes)
 * @param frame Frame number
 * @param add_sign Add sign prefix for negative frames
 * @return Overflow value (from original implementation)
 */
int frame_to_smptestring(char* smptestring, int64_t frame, uint8_t add_sign);

/**
 * Convert SMPTE timecode components to frame number
 * @param type SMPTE type: 0=24fps, 1=25fps, 2=29.97fps, 3=30fps
 * @param f Frame (0-29)
 * @param s Second (0-59)
 * @param m Minute (0-59)
 * @param h Hour (0-23)
 * @param overflow Overflow flag
 * @return Frame number
 */
int64_t smpte_to_frame(int type, int f, int s, int m, int h, int overflow);

#ifdef __cplusplus
}
#endif

#endif // XJADEO_SMPTE_WRAPPER_H

