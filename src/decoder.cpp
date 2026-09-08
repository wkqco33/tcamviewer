#include "tcamviewer/decoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

#include <iostream>

namespace tcamviewer {

VideoDecoder::VideoDecoder(const std::string& source, bool loop)
    : source_(source), loop_(loop) {
}

VideoDecoder::~VideoDecoder() {
    close();
}

void VideoDecoder::close() {
    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }
    if (rgbFrame_) {
        av_frame_free(&rgbFrame_);
    }
    if (avFrame_) {
        av_frame_free(&avFrame_);
    }
    if (packet_) {
        av_packet_free(&packet_);
    }
    if (codecCtx_) {
        avcodec_free_context(&codecCtx_);
    }
    if (formatCtx_) {
        avformat_close_input(&formatCtx_);
    }

    isOpened_ = false;
    videoStreamIdx_ = -1;
    currentScaledW_ = 0;
    currentScaledH_ = 0;
    rgbBuffer_.clear();
}

bool VideoDecoder::open() {
    if (source_.empty()) {
        return false;
    }

    close();

    AVDictionary* opts = nullptr;
    const AVInputFormat* ifmt = nullptr;

    // Check for V4L2 device
    if (source_.rfind("/dev/video", 0) == 0) {
        ifmt = av_find_input_format("v4l2");
    } else if (source_.rfind("rtsp://", 0) == 0) {
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
        av_dict_set(&opts, "fflags", "nobuffer", 0);
        av_dict_set(&opts, "max_delay", "500000", 0);
    }

    int ret = avformat_open_input(&formatCtx_, source_.c_str(), ifmt, &opts);
    if (opts) {
        av_dict_free(&opts);
    }

    if (ret < 0) {
        return false;
    }

    if (avformat_find_stream_info(formatCtx_, nullptr) < 0) {
        close();
        return false;
    }

    const AVCodec* decoder = nullptr;
    videoStreamIdx_ = av_find_best_stream(formatCtx_, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (videoStreamIdx_ < 0 || !decoder) {
        close();
        return false;
    }

    AVStream* stream = formatCtx_->streams[videoStreamIdx_];
    codecCtx_ = avcodec_alloc_context3(decoder);
    if (!codecCtx_) {
        close();
        return false;
    }

    if (avcodec_parameters_to_context(codecCtx_, stream->codecpar) < 0) {
        close();
        return false;
    }

    if (avcodec_open2(codecCtx_, decoder, nullptr) < 0) {
        close();
        return false;
    }

    avFrame_ = av_frame_alloc();
    rgbFrame_ = av_frame_alloc();
    packet_ = av_packet_alloc();

    if (!avFrame_ || !rgbFrame_ || !packet_) {
        close();
        return false;
    }

    info_.width = codecCtx_->width;
    info_.height = codecCtx_->height;
    info_.codecName = decoder->name ? decoder->name : "unknown";

    if (stream->avg_frame_rate.den > 0 && stream->avg_frame_rate.num > 0) {
        info_.fps = av_q2d(stream->avg_frame_rate);
    } else if (stream->r_frame_rate.den > 0 && stream->r_frame_rate.num > 0) {
        info_.fps = av_q2d(stream->r_frame_rate);
    } else {
        info_.fps = 30.0;
    }

    isOpened_ = true;
    return true;
}

bool VideoDecoder::rewind() {
    if (!isOpened_ || !formatCtx_) return false;
    if (av_seek_frame(formatCtx_, videoStreamIdx_, 0, AVSEEK_FLAG_BACKWARD) >= 0) {
        if (codecCtx_) avcodec_flush_buffers(codecCtx_);
        return true;
    }
    return false;
}

const uint8_t* VideoDecoder::readFrame(int targetWidth, int targetHeight,
                                       int* outWidth, int* outHeight,
                                       int* outStride) {
    if (!isOpened_ || !codecCtx_ || !formatCtx_) return nullptr;

    int targetW = (targetWidth > 0) ? targetWidth : info_.width;
    int targetH = (targetHeight > 0) ? targetHeight : info_.height;

    // Check if SwsContext needs creation or update
    if (!swsCtx_ || currentScaledW_ != targetW || currentScaledH_ != targetH) {
        if (swsCtx_) {
            sws_freeContext(swsCtx_);
            swsCtx_ = nullptr;
        }

        swsCtx_ = sws_getContext(
            codecCtx_->width, codecCtx_->height, codecCtx_->pix_fmt,
            targetW, targetH, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (!swsCtx_) return nullptr;

        currentScaledW_ = targetW;
        currentScaledH_ = targetH;
        int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, targetW, targetH, 1);
        rgbBuffer_.resize(bufSize);

        av_image_fill_arrays(
            rgbFrame_->data, rgbFrame_->linesize,
            rgbBuffer_.data(), AV_PIX_FMT_RGB24,
            targetW, targetH, 1
        );
    }

    while (true) {
        int ret = avcodec_receive_frame(codecCtx_, avFrame_);
        if (ret == 0) {
            // Convert to RGB24
            sws_scale(
                swsCtx_,
                avFrame_->data, avFrame_->linesize, 0, codecCtx_->height,
                rgbFrame_->data, rgbFrame_->linesize
            );

            if (outWidth) *outWidth = currentScaledW_;
            if (outHeight) *outHeight = currentScaledH_;
            if (outStride) *outStride = rgbFrame_->linesize[0];

            return rgbBuffer_.data();
        }

        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            return nullptr;
        }

        ret = av_read_frame(formatCtx_, packet_);
        if (ret == AVERROR_EOF) {
            if (loop_) {
                if (rewind()) {
                    continue;
                }
            }
            return nullptr;
        } else if (ret < 0) {
            return nullptr;
        }

        if (packet_->stream_index == videoStreamIdx_) {
            avcodec_send_packet(codecCtx_, packet_);
        }
        av_packet_unref(packet_);
    }
}

} // namespace tcamviewer
