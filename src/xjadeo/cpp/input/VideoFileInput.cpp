#include "VideoFileInput.h"
#include "../../ffcompat.h"
#include <cstring>
#include <cassert>
#include <algorithm>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/mem.h>
#include <libavutil/imgutils.h>
}

namespace xjadeo {

VideoFileInput::VideoFileInput()
    : formatCtx_(nullptr)
    , codecCtx_(nullptr)
    , frame_(nullptr)
    , frameFMT_(nullptr)
    , swsCtx_(nullptr)
    , videoStream_(-1)
    , frameIndex_(nullptr)
    , frameCount_(0)
    , lastDecodedPTS_(-1)
    , lastDecodedFrameNo_(-1)
    , scanComplete_(false)
    , byteSeek_(false)
    , ignoreStartOffset_(false)
    , currentFrame_(-1)
    , ready_(false)
{
    frameRateQ_ = {1, 1};
    frameInfo_ = {};
}

VideoFileInput::~VideoFileInput() {
    close();
}

bool VideoFileInput::initializeFFmpeg() {
    // FFmpeg should already be initialized globally
    // This is just a placeholder for any per-instance initialization
    return true;
}

bool VideoFileInput::open(const std::string& source) {
    if (source.empty()) {
        return false;
    }

    // Close any existing file
    close();

    currentFile_ = source;
    ready_ = false;

    // Initialize FFmpeg
    if (!initializeFFmpeg()) {
        return false;
    }

    // Open video file
    if (avformat_open_input(&formatCtx_, source.c_str(), nullptr, nullptr) != 0) {
        formatCtx_ = nullptr;
        return false;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(formatCtx_, nullptr) < 0) {
        avformat_close_input(&formatCtx_);
        formatCtx_ = nullptr;
        return false;
    }

    // Find video stream
    if (!findVideoStream()) {
        avformat_close_input(&formatCtx_);
        formatCtx_ = nullptr;
        return false;
    }

    // Open codec
    if (!openCodec()) {
        avformat_close_input(&formatCtx_);
        formatCtx_ = nullptr;
        return false;
    }

    // Get video properties
    AVStream* avStream = formatCtx_->streams[videoStream_];
    
    // Frame rate
    double framerate = 0.0;
    if (avStream->r_frame_rate.den > 0 && avStream->r_frame_rate.num > 0) {
        framerate = av_q2d(avStream->r_frame_rate);
        frameRateQ_.den = avStream->r_frame_rate.num;
        frameRateQ_.num = avStream->r_frame_rate.den;
    }

    // Dimensions
    int width = codecCtx_->width;
    int height = codecCtx_->height;
    
    // Duration
    double duration = 0.0;
    if (formatCtx_->duration != AV_NOPTS_VALUE) {
        duration = (double)formatCtx_->duration / AV_TIME_BASE;
    }

    // Store frame info
    frameInfo_.width = width;
    frameInfo_.height = height;
    frameInfo_.aspect = (float)width / (float)height;
    frameInfo_.framerate = framerate;
    frameInfo_.duration = duration;
    frameInfo_.format = PixelFormat::YUV420P; // Default, will be set based on render format

    // Index frames
    if (!indexFrames()) {
        cleanup();
        return false;
    }

    // Calculate total frames
    if (framerate > 0 && duration > 0) {
        frameInfo_.totalFrames = (int64_t)(duration * framerate);
    } else {
        frameInfo_.totalFrames = frameCount_;
    }

    ready_ = true;
    currentFrame_ = -1;
    return true;
}

void VideoFileInput::close() {
    cleanup();
    currentFile_.clear();
    ready_ = false;
    currentFrame_ = -1;
    frameInfo_ = {};
}

bool VideoFileInput::isReady() const {
    return ready_ && formatCtx_ != nullptr;
}

bool VideoFileInput::findVideoStream() {
    videoStream_ = -1;
    for (unsigned int i = 0; i < formatCtx_->nb_streams; ++i) {
        if (formatCtx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream_ = i;
            break;
        }
    }
    return videoStream_ >= 0;
}

bool VideoFileInput::openCodec() {
    AVStream* avStream = formatCtx_->streams[videoStream_];
    codecCtx_ = avStream->codec;

    AVCodec* codec = avcodec_find_decoder(codecCtx_->codec_id);
    if (!codec) {
        return false;
    }

    if (avcodec_open2(codecCtx_, codec, nullptr) < 0) {
        return false;
    }

    // Allocate frames
    frame_ = av_frame_alloc();
    if (!frame_) {
        return false;
    }

    frameFMT_ = av_frame_alloc();
    if (!frameFMT_) {
        av_free(frame_);
        frame_ = nullptr;
        return false;
    }

    return true;
}

bool VideoFileInput::indexFrames() {
    // Simplified indexing - in full implementation, this would index all frames
    // For now, we'll do a basic implementation
    
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

    frameCount_ = 0;
    frameIndex_ = nullptr;

    // Allocate initial index array (will grow as needed)
    const size_t initialSize = 10000;
    frameIndex_ = static_cast<FrameIndex*>(calloc(initialSize, sizeof(FrameIndex)));
    if (!frameIndex_) {
        return false;
    }

    size_t indexSize = initialSize;
    AVRational timeBase = formatCtx_->streams[videoStream_]->time_base;

    while (av_read_frame(formatCtx_, &packet) >= 0) {
        if (packet.stream_index != videoStream_) {
            av_free_packet(&packet);
            continue;
        }

        int64_t ts = AV_NOPTS_VALUE;
        if (packet.pts != AV_NOPTS_VALUE) {
            ts = packet.pts;
        } else if (packet.dts != AV_NOPTS_VALUE) {
            ts = packet.dts;
        }

        if (ts == AV_NOPTS_VALUE) {
            av_free_packet(&packet);
            continue;
        }

        // Grow array if needed
        if (frameCount_ >= static_cast<int64_t>(indexSize)) {
            indexSize *= 2;
            frameIndex_ = static_cast<FrameIndex*>(realloc(frameIndex_, indexSize * sizeof(FrameIndex)));
            if (!frameIndex_) {
                av_free_packet(&packet);
                return false;
            }
        }

        FrameIndex& idx = frameIndex_[frameCount_];
        idx.pkt_pts = packet.pts;
        idx.pkt_pos = packet.pos;
        idx.frame_pts = ts;
        idx.frame_pos = packet.pos;
        idx.timestamp = frameCount_;
        idx.seekpts = ts;
        idx.seekpos = packet.pos;
        idx.key = (packet.flags & AV_PKT_FLAG_KEY) ? 1 : 0;

        frameCount_++;
        av_free_packet(&packet);
    }

    scanComplete_ = true;
    return true;
}

bool VideoFileInput::seek(int64_t frameNumber) {
    if (!isReady() || !scanComplete_) {
        return false;
    }

    int64_t targetFrame = frameNumber;
    if (ignoreStartOffset_) {
        targetFrame += frameInfo_.fileFrameOffset;
    }

    if (targetFrame < 0 || targetFrame >= frameCount_) {
        return false;
    }

    return seekToFrame(targetFrame);
}

bool VideoFileInput::seekToFrame(int64_t frameNumber) {
    if (frameNumber < 0 || frameNumber >= frameCount_ || !frameIndex_) {
        return false;
    }

    const FrameIndex& idx = frameIndex_[frameNumber];
    int64_t timestamp = idx.timestamp;

    if (timestamp < 0) {
        return false;
    }

    // Check if we need to seek
    bool needSeek = false;
    if (lastDecodedPTS_ < 0 || lastDecodedFrameNo_ < 0) {
        needSeek = true;
    } else if (lastDecodedPTS_ > timestamp) {
        needSeek = true;
    } else if ((frameNumber - lastDecodedFrameNo_) != 1) {
        if (idx.seekpts != frameIndex_[lastDecodedFrameNo_].seekpts) {
            needSeek = true;
        }
    }

    lastDecodedPTS_ = -1;
    lastDecodedFrameNo_ = -1;

    if (needSeek) {
        int seekResult;
        if (byteSeek_ && idx.seekpos > 0) {
            seekResult = av_seek_frame(formatCtx_, videoStream_, idx.seekpos, 
                                      AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_BYTE);
        } else {
            seekResult = av_seek_frame(formatCtx_, videoStream_, idx.seekpts, 
                                      AVSEEK_FLAG_BACKWARD);
        }

        if (codecCtx_->codec->flush) {
            avcodec_flush_buffers(codecCtx_);
        }

        if (seekResult < 0) {
            return false;
        }
    }

    currentFrame_ = frameNumber;
    return true;
}

bool VideoFileInput::readFrame(int64_t frameNumber, FrameBuffer& buffer) {
    if (!isReady()) {
        return false;
    }

    // Seek to frame if needed
    if (!seek(frameNumber)) {
        return false;
    }

    // Decode frame
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

    int bailout = 20;
    bool frameFinished = false;

    while (bailout > 0 && !frameFinished) {
        int err = av_read_frame(formatCtx_, &packet);
        if (err < 0) {
            if (err == AVERROR_EOF) {
                --bailout;
                av_free_packet(&packet);
                continue;
            } else {
                av_free_packet(&packet);
                return false;
            }
        }

        if (packet.stream_index != videoStream_) {
            av_free_packet(&packet);
            continue;
        }

    // Decode video frame
    int gotFrame = 0;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 21, 0)
    err = avcodec_decode_video(codecCtx_, frame_, &gotFrame, packet.data, packet.size);
#else
    err = avcodec_decode_video2(codecCtx_, frame_, &gotFrame, &packet);
#endif
        av_free_packet(&packet);

        if (err < 0) {
            --bailout;
            continue;
        }

        if (gotFrame) {
            frameFinished = true;
        } else {
            --bailout;
        }
    }

    if (!frameFinished) {
        return false;
    }

    // Allocate buffer if needed
    if (!buffer.isValid() || buffer.info().width != frameInfo_.width || 
        buffer.info().height != frameInfo_.height) {
        if (!buffer.allocate(frameInfo_)) {
            return false;
        }
    }

    // Convert frame format using sws_scale
    // Initialize sws context if needed
    if (!swsCtx_) {
        swsCtx_ = sws_getContext(
            codecCtx_->width, codecCtx_->height, codecCtx_->pix_fmt,
            frameInfo_.width, frameInfo_.height, AV_PIX_FMT_YUV420P,
            SWS_BICUBIC, nullptr, nullptr, nullptr
        );
        if (!swsCtx_) {
            return false;
        }
    }

    // Prepare destination frame
    uint8_t* dstData[4] = {buffer.data(), nullptr, nullptr, nullptr};
    int dstLinesize[4] = {frameInfo_.width, 0, 0, 0};
    
    // Scale and convert
    sws_scale(swsCtx_,
              (const uint8_t* const*)frame_->data, frame_->linesize,
              0, codecCtx_->height,
              dstData, dstLinesize);

    int64_t pts = parsePTSFromFrame(frame_);
    if (pts != AV_NOPTS_VALUE) {
        lastDecodedPTS_ = pts;
        lastDecodedFrameNo_ = frameNumber;
    }

    currentFrame_ = frameNumber;
    return true;
}

int64_t VideoFileInput::parsePTSFromFrame(AVFrame* frame) {
    int64_t pts = AV_NOPTS_VALUE;
    
    if (frame->pts != AV_NOPTS_VALUE) {
        pts = frame->pts;
    } else if (frame->pkt_pts != AV_NOPTS_VALUE) {
        pts = frame->pkt_pts;
    } else if (frame->pkt_dts != AV_NOPTS_VALUE) {
        pts = frame->pkt_dts;
    }

    return pts;
}

FrameInfo VideoFileInput::getFrameInfo() const {
    return frameInfo_;
}

int64_t VideoFileInput::getCurrentFrame() const {
    return currentFrame_;
}

void VideoFileInput::cleanup() {
    // Free frame index
    if (frameIndex_) {
        free(frameIndex_);
        frameIndex_ = nullptr;
    }
    frameCount_ = 0;

    // Free software scaler
    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }

    // Free frames
    if (frameFMT_) {
        av_free(frameFMT_);
        frameFMT_ = nullptr;
    }
    if (frame_) {
        av_free(frame_);
        frame_ = nullptr;
    }

    // Close codec
    if (codecCtx_) {
        avcodec_close(codecCtx_);
        codecCtx_ = nullptr;
    }

    // Close format context
    if (formatCtx_) {
        avformat_close_input(&formatCtx_);
        formatCtx_ = nullptr;
    }

    videoStream_ = -1;
    lastDecodedPTS_ = -1;
    lastDecodedFrameNo_ = -1;
    scanComplete_ = false;
    currentFrame_ = -1;
}

} // namespace xjadeo

