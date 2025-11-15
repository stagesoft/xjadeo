#include "FrameBuffer.h"
#include <cstdlib>
#include <cstring>

extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

namespace xjadeo {

FrameBuffer::FrameBuffer() : buffer_(nullptr), size_(0) {
}

FrameBuffer::~FrameBuffer() {
    release();
}

bool FrameBuffer::allocate(const FrameInfo& info) {
    release();
    
    info_ = info;
    
    // Convert PixelFormat to AVPixelFormat
    AVPixelFormat avFormat;
    switch (info.format) {
        case PixelFormat::YUV420P:
            avFormat = AV_PIX_FMT_YUV420P;
            break;
        case PixelFormat::RGB24:
            avFormat = AV_PIX_FMT_RGB24;
            break;
        case PixelFormat::RGBA32:
            avFormat = AV_PIX_FMT_RGB32;
            break;
        case PixelFormat::BGRA32:
            avFormat = AV_PIX_FMT_BGR32;
            break;
        case PixelFormat::UYVY422:
            avFormat = AV_PIX_FMT_UYVY422;
            break;
        default:
            avFormat = AV_PIX_FMT_YUV420P;
            break;
    }
    
    // Calculate buffer size using modern FFmpeg API
    int linesize[4];
    int ret = av_image_get_buffer_size(avFormat, info.width, info.height, 1);
    if (ret < 0) {
        return false;
    }
    size_ = ret;
    
    // Allocate buffer
    buffer_ = static_cast<uint8_t*>(calloc(1, size_));
    if (!buffer_) {
        size_ = 0;
        return false;
    }
    
    return true;
}

void FrameBuffer::release() {
    if (buffer_) {
        free(buffer_);
        buffer_ = nullptr;
    }
    size_ = 0;
}

} // namespace xjadeo

