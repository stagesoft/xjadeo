#ifndef XJADEO_FRAMEBUFFER_H
#define XJADEO_FRAMEBUFFER_H

#include "FrameFormat.h"
#include <cstdint>
#include <cstddef>
#include <memory>

namespace xjadeo {

class FrameBuffer {
public:
    FrameBuffer();
    ~FrameBuffer();

    // Allocate buffer for given format and dimensions
    bool allocate(const FrameInfo& info);
    
    // Release buffer
    void release();

    // Get buffer pointer
    uint8_t* data() { return buffer_; }
    const uint8_t* data() const { return buffer_; }

    // Get buffer size
    size_t size() const { return size_; }

    // Get frame info
    const FrameInfo& info() const { return info_; }

    // Check if buffer is valid
    bool isValid() const { return buffer_ != nullptr && size_ > 0; }

private:
    uint8_t* buffer_;
    size_t size_;
    FrameInfo info_;
};

} // namespace xjadeo

#endif // XJADEO_FRAMEBUFFER_H

