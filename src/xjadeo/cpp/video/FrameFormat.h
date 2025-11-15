#ifndef XJADEO_FRAMEFORMAT_H
#define XJADEO_FRAMEFORMAT_H

#include <cstdint>

namespace xjadeo {

enum class PixelFormat {
    YUV420P,
    RGB24,
    RGBA32,
    BGRA32,
    UYVY422
};

struct FrameInfo {
    int width = 0;
    int height = 0;
    float aspect = 0.0f;
    double framerate = 0.0;
    int64_t totalFrames = 0;
    double duration = 0.0;
    int64_t fileFrameOffset = 0;
    PixelFormat format = PixelFormat::YUV420P;
};

} // namespace xjadeo

#endif // XJADEO_FRAMEFORMAT_H

