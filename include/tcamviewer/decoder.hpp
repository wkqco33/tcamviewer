#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace tcamviewer {

struct StreamInfo {
    int width{0};
    int height{0};
    double fps{30.0};
    std::string codecName;
};

class VideoDecoder {
public:
    explicit VideoDecoder(const std::string& source, bool loop = false);
    ~VideoDecoder();

    bool open();
    void close();

    bool isOpened() const { return isOpened_; }
    const StreamInfo& getInfo() const { return info_; }

    // Read next frame and scale/convert to RGB24 buffer.
    // targetWidth, targetHeight: if > 0, downscale; if <= 0, decode to native resolution.
    // Returns pointer to internal RGB24 buffer, or nullptr on EOF/error.
    const uint8_t* readFrame(int targetWidth = 0, int targetHeight = 0,
                             int* outWidth = nullptr, int* outHeight = nullptr,
                             int* outStride = nullptr);

    // Seek back to start
    bool rewind();

private:
    std::string source_;
    bool loop_{false};
    bool isOpened_{false};
    StreamInfo info_;

    int videoStreamIdx_{-1};
    AVFormatContext* formatCtx_{nullptr};
    AVCodecContext* codecCtx_{nullptr};
    AVFrame* avFrame_{nullptr};
    AVFrame* rgbFrame_{nullptr};
    AVPacket* packet_{nullptr};
    SwsContext* swsCtx_{nullptr};

    int currentScaledW_{0};
    int currentScaledH_{0};
    std::vector<uint8_t> rgbBuffer_;
};

} // namespace tcamviewer
