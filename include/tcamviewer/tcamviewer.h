#ifndef TCAMVIEWER_H
#define TCAMVIEWER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
  #if defined(TCAMVIEWER_EXPORT)
    #define TCAM_API __declspec(dllexport)
  #else
    #define TCAM_API __declspec(dllimport)
  #endif
#else
  #define TCAM_API __attribute__((visibility("default")))
#endif

typedef struct tcam_renderer tcam_renderer_t;
typedef struct tcam_decoder tcam_decoder_t;

typedef enum {
    TCAM_OK = 0,
    TCAM_ERR_INVALID_ARG = -1,
    TCAM_ERR_INIT_FAILED = -2,
    TCAM_ERR_DECODE_FAILED = -3,
    TCAM_ERR_EOF = -4,
    TCAM_ERR_IO = -5
} tcam_status_t;

typedef struct {
    int target_cols;        /* Target terminal columns (0 for auto-detection) */
    int target_rows;        /* Target terminal rows (0 for auto-detection) */
    bool use_diff;          /* Enable dirty-diff frame optimization */
    bool alt_screen;        /* Use alternate screen buffer */
    bool hide_cursor;       /* Hide cursor during rendering */
    int rotation;           /* Rotation in degrees: 0, 90, 180, 270 (clockwise) */
    bool keep_aspect_ratio; /* Maintain aspect ratio with letterbox/pillarbox */
} tcam_render_config_t;

/* System terminal dimensions */
TCAM_API tcam_status_t tcam_get_terminal_size(int* out_cols, int* out_rows);

/* Renderer lifecycle */
TCAM_API tcam_renderer_t* tcam_renderer_create(const tcam_render_config_t* config);
TCAM_API void tcam_renderer_destroy(tcam_renderer_t* renderer);
TCAM_API tcam_status_t tcam_renderer_resize(tcam_renderer_t* renderer, int cols, int rows);

/* Frame rendering directly to terminal */
TCAM_API tcam_status_t tcam_renderer_render_rgb24(tcam_renderer_t* renderer,
                                                 const uint8_t* rgb_data,
                                                 int width,
                                                 int height,
                                                 int stride);

TCAM_API tcam_status_t tcam_renderer_render_bgr24(tcam_renderer_t* renderer,
                                                 const uint8_t* bgr_data,
                                                 int width,
                                                 int height,
                                                 int stride);

/* Render to ANSI string buffer without direct terminal output */
TCAM_API tcam_status_t tcam_renderer_render_to_buffer(tcam_renderer_t* renderer,
                                                     const uint8_t* data,
                                                     int width,
                                                     int height,
                                                     int stride,
                                                     bool is_bgr,
                                                     char* out_buf,
                                                     size_t buf_size,
                                                     size_t* out_len);

TCAM_API void tcam_renderer_invalidate_cache(tcam_renderer_t* renderer);
TCAM_API tcam_status_t tcam_renderer_set_rotation(tcam_renderer_t* renderer, int rotation_degrees);
TCAM_API int tcam_renderer_get_rotation(const tcam_renderer_t* renderer);
TCAM_API tcam_status_t tcam_renderer_set_keep_aspect_ratio(tcam_renderer_t* renderer, bool enable);
TCAM_API bool tcam_renderer_get_keep_aspect_ratio(const tcam_renderer_t* renderer);
TCAM_API int tcam_renderer_get_cols(const tcam_renderer_t* renderer);
TCAM_API int tcam_renderer_get_rows(const tcam_renderer_t* renderer);

/* Decoder lifecycle and frame retrieval */
TCAM_API tcam_decoder_t* tcam_decoder_create(const char* source, bool loop);
TCAM_API void tcam_decoder_destroy(tcam_decoder_t* decoder);
TCAM_API int tcam_decoder_get_rotation(const tcam_decoder_t* decoder);
TCAM_API tcam_status_t tcam_decoder_get_info(tcam_decoder_t* decoder,
                                             int* out_width,
                                             int* out_height,
                                             double* out_fps);

TCAM_API tcam_status_t tcam_decoder_read_frame(tcam_decoder_t* decoder,
                                              int target_width,
                                              int target_height,
                                              const uint8_t** out_rgb,
                                              int* out_width,
                                              int* out_height,
                                              int* out_stride);

TCAM_API tcam_status_t tcam_decoder_rewind(tcam_decoder_t* decoder);

#ifdef __cplusplus
}
#endif

#endif /* TCAMVIEWER_H */
