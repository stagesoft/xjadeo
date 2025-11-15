#ifndef XJADEO_VIDEOFILEINPUT_H
#define XJADEO_VIDEOFILEINPUT_H

#include "InputSource.h"
#include <string>
#include <memory>
#include <cstdint>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace xjadeo {

/**
 * VideoFileInput - FFmpeg-based video file input source
 * 
 * Implements InputSource interface for reading video files using FFmpeg/libav.
 * This is the only input source implementation for now, but the architecture
 * is ready for future implementations (live video, streaming, etc.).
 */
class VideoFileInput : public InputSource {
public:
    VideoFileInput();
    virtual ~VideoFileInput();

    // InputSource interface
    bool open(const std::string& source) override;
    void close() override;
    bool isReady() const override;
    bool readFrame(int64_t frameNumber, FrameBuffer& buffer) override;
    bool seek(int64_t frameNumber) override;
    FrameInfo getFrameInfo() const override;
    int64_t getCurrentFrame() const override;

    // Additional methods specific to video files
    void setIgnoreStartOffset(bool ignore) { ignoreStartOffset_ = ignore; }
    bool getIgnoreStartOffset() const { return ignoreStartOffset_; }

private:
    struct FrameIndex {
        int64_t pkt_pts;
        int64_t pkt_pos;
        int64_t frame_pts;
        int64_t frame_pos;
        int64_t timestamp;
        int64_t seekpts;
        int64_t seekpos;
        uint8_t key;
    };

    bool initializeFFmpeg();
    bool findVideoStream();
    bool openCodec();
    bool indexFrames();
    bool seekToFrame(int64_t frameNumber);
    int64_t parsePTSFromFrame(AVFrame* frame);
    void cleanup();

    // FFmpeg objects
    AVFormatContext* formatCtx_;
    AVCodecContext* codecCtx_;
    AVFrame* frame_;
    AVFrame* frameFMT_;
    SwsContext* swsCtx_;
    int videoStream_;

    // Frame indexing
    FrameIndex* frameIndex_;
    int64_t frameCount_;
    int64_t lastDecodedPTS_;
    int64_t lastDecodedFrameNo_;
    bool scanComplete_;
    bool byteSeek_;

    // Video properties
    FrameInfo frameInfo_;
    std::string currentFile_;
    bool ignoreStartOffset_;
    int64_t currentFrame_;

    // Internal state
    bool ready_;
    AVRational frameRateQ_;
};

} // namespace xjadeo

#endif // XJADEO_VIDEOFILEINPUT_H

