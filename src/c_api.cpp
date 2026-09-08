#include "tcamviewer/tcamviewer.h"
#include "tcamviewer/terminal.hpp"
#include "tcamviewer/renderer.hpp"
#include "tcamviewer/decoder.hpp"
#include <cstring>
#include <new>

struct tcam_renderer {
    tcamviewer::Renderer impl;
    explicit tcam_renderer(const tcamviewer::RenderConfig& cfg) : impl(cfg) {}
};

struct tcam_decoder {
    tcamviewer::VideoDecoder impl;
    tcam_decoder(const std::string& src, bool loop) : impl(src, loop) {}
};

extern "C" {

tcam_status_t tcam_get_terminal_size(int* out_cols, int* out_rows) {
    if (!out_cols || !out_rows) return TCAM_ERR_INVALID_ARG;
    auto sz = tcamviewer::Terminal::getSize();
    *out_cols = sz.cols;
    *out_rows = sz.rows;
    return TCAM_OK;
}

tcam_renderer_t* tcam_renderer_create(const tcam_render_config_t* config) {
    tcamviewer::RenderConfig cfg;
    if (config) {
        cfg.targetCols = config->target_cols;
        cfg.targetRows = config->target_rows;
        cfg.useDiff = config->use_diff;
        cfg.altScreen = config->alt_screen;
        cfg.hideCursor = config->hide_cursor;
        cfg.rotation = config->rotation;
        cfg.keepAspectRatio = config->keep_aspect_ratio;
    }
    return new (std::nothrow) tcam_renderer(cfg);
}

void tcam_renderer_destroy(tcam_renderer_t* renderer) {
    delete renderer;
}

tcam_status_t tcam_renderer_resize(tcam_renderer_t* renderer, int cols, int rows) {
    if (!renderer || cols <= 0 || rows <= 0) return TCAM_ERR_INVALID_ARG;
    renderer->impl.resize(cols, rows);
    return TCAM_OK;
}

tcam_status_t tcam_renderer_render_rgb24(tcam_renderer_t* renderer,
                                         const uint8_t* rgb_data,
                                         int width,
                                         int height,
                                         int stride) {
    if (!renderer || !rgb_data || width <= 0 || height <= 0) return TCAM_ERR_INVALID_ARG;
    renderer->impl.renderRgb24(rgb_data, width, height, stride);
    return TCAM_OK;
}

tcam_status_t tcam_renderer_render_bgr24(tcam_renderer_t* renderer,
                                         const uint8_t* bgr_data,
                                         int width,
                                         int height,
                                         int stride) {
    if (!renderer || !bgr_data || width <= 0 || height <= 0) return TCAM_ERR_INVALID_ARG;
    renderer->impl.renderBgr24(bgr_data, width, height, stride);
    return TCAM_OK;
}

tcam_status_t tcam_renderer_render_to_buffer(tcam_renderer_t* renderer,
                                             const uint8_t* data,
                                             int width,
                                             int height,
                                             int stride,
                                             bool is_bgr,
                                             char* out_buf,
                                             size_t buf_size,
                                             size_t* out_len) {
    if (!renderer || !data || width <= 0 || height <= 0 || !out_buf || buf_size == 0) {
        return TCAM_ERR_INVALID_ARG;
    }

    std::string ansi = renderer->impl.generateAnsiString(data, width, height, stride, is_bgr);
    if (ansi.size() + 1 > buf_size) {
        return TCAM_ERR_IO;
    }

    std::memcpy(out_buf, ansi.data(), ansi.size());
    out_buf[ansi.size()] = '\0';
    if (out_len) *out_len = ansi.size();
    return TCAM_OK;
}

void tcam_renderer_invalidate_cache(tcam_renderer_t* renderer) {
    if (renderer) renderer->impl.invalidateCache();
}

tcam_status_t tcam_renderer_set_rotation(tcam_renderer_t* renderer, int rotation_degrees) {
    if (!renderer) return TCAM_ERR_INVALID_ARG;
    renderer->impl.setRotation(rotation_degrees);
    return TCAM_OK;
}

int tcam_renderer_get_rotation(const tcam_renderer_t* renderer) {
    return renderer ? renderer->impl.getRotation() : 0;
}

tcam_status_t tcam_renderer_set_keep_aspect_ratio(tcam_renderer_t* renderer, bool enable) {
    if (!renderer) return TCAM_ERR_INVALID_ARG;
    renderer->impl.setKeepAspectRatio(enable);
    return TCAM_OK;
}

bool tcam_renderer_get_keep_aspect_ratio(const tcam_renderer_t* renderer) {
    return renderer ? renderer->impl.isKeepAspectRatio() : true;
}

int tcam_renderer_get_cols(const tcam_renderer_t* renderer) {
    return renderer ? renderer->impl.getCols() : 0;
}

int tcam_renderer_get_rows(const tcam_renderer_t* renderer) {
    return renderer ? renderer->impl.getRows() : 0;
}

tcam_decoder_t* tcam_decoder_create(const char* source, bool loop) {
    if (!source) return nullptr;
    auto* dec = new (std::nothrow) tcam_decoder(source, loop);
    if (dec && !dec->impl.open()) {
        delete dec;
        return nullptr;
    }
    return dec;
}

void tcam_decoder_destroy(tcam_decoder_t* decoder) {
    delete decoder;
}

int tcam_decoder_get_rotation(const tcam_decoder_t* decoder) {
    return (decoder && decoder->impl.isOpened()) ? decoder->impl.getInfo().rotation : 0;
}

tcam_status_t tcam_decoder_get_info(tcam_decoder_t* decoder,
                                     int* out_width,
                                     int* out_height,
                                     double* out_fps) {
    if (!decoder || !decoder->impl.isOpened()) return TCAM_ERR_INVALID_ARG;
    const auto& info = decoder->impl.getInfo();
    if (out_width) *out_width = info.width;
    if (out_height) *out_height = info.height;
    if (out_fps) *out_fps = info.fps;
    return TCAM_OK;
}

tcam_status_t tcam_decoder_read_frame(tcam_decoder_t* decoder,
                                      int target_width,
                                      int target_height,
                                      const uint8_t** out_rgb,
                                      int* out_width,
                                      int* out_height,
                                      int* out_stride) {
    if (!decoder || !out_rgb) return TCAM_ERR_INVALID_ARG;
    const uint8_t* ptr = decoder->impl.readFrame(target_width, target_height,
                                                 out_width, out_height, out_stride);
    if (!ptr) {
        *out_rgb = nullptr;
        return TCAM_ERR_EOF;
    }
    *out_rgb = ptr;
    return TCAM_OK;
}

tcam_status_t tcam_decoder_rewind(tcam_decoder_t* decoder) {
    if (!decoder) return TCAM_ERR_INVALID_ARG;
    return decoder->impl.rewind() ? TCAM_OK : TCAM_ERR_IO;
}

} // extern "C"
