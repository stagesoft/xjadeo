#ifndef XJADEO_INPUTSOURCE_H
#define XJADEO_INPUTSOURCE_H

#include "../video/FrameBuffer.h"
#include "../video/FrameFormat.h"
#include <string>
#include <cstdint>

namespace xjadeo {

/**
 * Abstract base class for all input sources.
 * 
 * This interface allows different input types (video files, live video, streaming, etc.)
 * to be used interchangeably. Each layer can have its own input source.
 */
class InputSource {
public:
    virtual ~InputSource() = default;

    /**
     * Open the input source
     * @param source Path or identifier for the source
     * @return true on success, false on failure
     */
    virtual bool open(const std::string& source) = 0;

    /**
     * Close the input source and release resources
     */
    virtual void close() = 0;

    /**
     * Check if input source is ready/opened
     * @return true if ready, false otherwise
     */
    virtual bool isReady() const = 0;

    /**
     * Read a frame at the given frame number
     * @param frameNumber Frame number to read
     * @param buffer FrameBuffer to store the decoded frame
     * @return true on success, false on failure
     */
    virtual bool readFrame(int64_t frameNumber, FrameBuffer& buffer) = 0;

    /**
     * Seek to a specific frame number
     * @param frameNumber Target frame number
     * @return true on success, false on failure
     */
    virtual bool seek(int64_t frameNumber) = 0;

    /**
     * Get information about the video source
     * @return FrameInfo structure with video properties
     */
    virtual FrameInfo getFrameInfo() const = 0;

    /**
     * Get the current frame number (last frame read)
     * @return Current frame number, or -1 if not available
     */
    virtual int64_t getCurrentFrame() const = 0;
};

} // namespace xjadeo

#endif // XJADEO_INPUTSOURCE_H

