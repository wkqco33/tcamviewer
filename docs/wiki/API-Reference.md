# API 레퍼런스 (API Reference)

`tcamviewer`는 C, Go, Python에서 손쉽게 바인딩할 수 있는 C-ABI를 제공합니다.

---

## 1. C-ABI 명세 (`include/tcamviewer/tcamviewer.h`)

### 타입 및 구조체
```c
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
    int target_cols;        /* 터미널 가로 문자 수 (0이면 자동 감지) */
    int target_rows;        /* 터미널 세로 문자 수 (0이면 자동 감지) */
    bool use_diff;          /* dirty-diff 렌더링 활성화 (기본 true) */
    bool alt_screen;        /* alternate screen buffer 사용 여부 */
    bool hide_cursor;       /* 커서 숨김 여부 */
    int rotation;           /* 회전 각도: 0, 90, 180, 270 (시계 방향) */
    bool keep_aspect_ratio; /* 원본 종횡비 유지 (레터박스/필러박스) */
} tcam_render_config_t;
```

### 핵심 함수 목록
- `tcam_get_terminal_size(int* out_cols, int* out_rows)`: 시스템 터미널의 현재 가로/세로 셀 크기 조회
- `tcam_renderer_create(const tcam_render_config_t* config)`: 렌더러 인스턴스 생성
- `tcam_renderer_destroy(tcam_renderer_t* renderer)`: 렌더러 메모리 해제
- `tcam_renderer_resize(tcam_renderer_t* renderer, int cols, int rows)`: 렌더러 타깃 크기 변경
- `tcam_renderer_render_rgb24(...)`: RGB24 버퍼 터미널 직접 렌더링
- `tcam_renderer_render_bgr24(...)`: BGR24 버퍼 터미널 직접 렌더링
- `tcam_renderer_set_rotation(renderer, degrees)`: 회전 각도 동적 변경
- `tcam_renderer_set_keep_aspect_ratio(renderer, enable)`: 종횡비 유지 동적 변경

---

## 2. Go 언어 패키지 (`pkg/tcamviewer`)

CGO를 통해 C-ABI와 직접 연결됩니다.

```go
package main

import (
    "github.com/wkqco33/tcamviewer/pkg/tcamviewer"
)

func main() {
    cols, rows, _ := tcamviewer.GetTerminalSize()
    renderer, err := tcamviewer.NewRenderer(tcamviewer.Config{
        TargetCols:      cols,
        TargetRows:      rows,
        UseDiff:         true,
        AltScreen:       true,
        HideCursor:      true,
        Rotation:        0,
        KeepAspectRatio: true,
    })
    if err != nil {
        panic(err)
    }
    defer renderer.Close()

    // rgb: []byte (width * height * 3)
    renderer.RenderRGB(rgb, width, height, stride)
}
```

---

## 3. Python 언어 패키지 (`python/tcamviewer`)

`ctypes` 기반으로 컴파일 없이 즉시 임포트하여 사용하며, NumPy ndarray와 표준 바이트 버퍼를 모두 지원합니다.

```python
from tcamviewer import TerminalRenderer, get_terminal_size
import numpy as np

cols, rows = get_terminal_size()
renderer = TerminalRenderer(
    cols=cols,
    rows=rows,
    use_diff=True,
    rotation=0,
    keep_aspect_ratio=True
)

# NumPy 배열 렌더링 (H x W x 3)
frame = np.zeros((480, 640, 3), dtype=np.uint8)
renderer.render_rgb(frame, width=640, height=480)

# 런타임 제어
renderer.set_rotation(90)
renderer.set_keep_aspect_ratio(False)

renderer.close()
```
