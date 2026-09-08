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
    int target_cols;    /* 터미널 가로 문자 수 (0이면 터미널 자동 감지) */
    int target_rows;    /* 터미널 세로 문자 수 (0이면 터미널 자동 감지) */
    bool use_diff;      /* dirty-diff 렌더링 활성화 (기본 true) */
    bool alt_screen;    /* alternate screen buffer 사용 여부 */
    bool hide_cursor;   /* 커서 숨김 여부 */
    int rotation;       /* 회전 각도: 0, 90, 180, 270 (시계 방향) */
    bool keep_aspect_ratio; /* 원본 영상 종횡비 유지 (레터박스/필러박스) */
} tcam_render_config_t;

/* Helper: 시스템 터미널 크기 조회 */
TCAM_API tcam_status_t tcam_get_terminal_size(int* out_cols, int* out_rows);

/* Renderer Lifecycle */
TCAM_API tcam_renderer_t* tcam_renderer_create(const tcam_render_config_t* config);
TCAM_API void tcam_renderer_destroy(tcam_renderer_t* renderer);
TCAM_API tcam_status_t tcam_renderer_resize(tcam_renderer_t* renderer, int cols, int rows);

/* Frame Rendering */
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

/* ANSI 문자열 버퍼로 렌더링 (화면 출력 없이 버퍼로 수신) */
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

/* Decoder Lifecycle */
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
