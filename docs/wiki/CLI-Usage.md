# CLI 사용 가이드 (CLI Usage Guide)

`tcamviewer` CLI 도구는 초경량 C++17 CLI 프레임워크 [`wcppcli`](https://github.com/wkqco33/wcppcli)를 기반으로 작성되었습니다.

---

## 1. 명령어 목록

```bash
tcamviewer [command] [flags]
```

| 서브커맨드 | 설명 |
| :--- | :--- |
| **`play`** | 비디오 파일, 네트워크 스트림(RTSP/RTMP), USB 웹캠 실시간 재생 |
| **`test`** | 터미널 렌더링 성능 및 TrueColor 색상 애니메이션 테스트 패턴 실행 |
| **`help`** | 도움말 출력 |

---

## 2. `play` 명령어 상세

```bash
tcamviewer play <source> [flags]
```

### 주요 플래그 (Flags)
| 플래그 | 단축 플래그 | 타입 | 기본값 | 설명 |
| :--- | :--- | :--- | :--- | :--- |
| `--loop` | `-l` | bool | `false` | 비디오 파일 무한 반복 재생 |
| `--no-diff` | | bool | `false` | Dirty-diff 최적화 비활성화 (매 프레임 전체 갱신) |
| `--alt-screen` | | bool | `false` | 터미널 대체 버퍼(Alternate Screen) 사용 |
| `--width` | `-w` | int | `0` | 터미널 가로 컬럼 수 강제 지정 (0: 터미널 창 자동 감지) |
| `--height` | `-h` | int | `0` | 터미널 세로 로우 수 강제 지정 (0: 터미널 창 자동 감지) |
| `--rotate` | `-r` | int | `-1` | 시계 방향 회전 (0, 90, 180, 270 / -1: 메타데이터 자동 감지) |
| `--stretch` | | bool | `false` | 종횡비 유지(레터박스)를 끄고 터미널 전체로 늘려서 출력 |

### 사용 예시
```bash
# 1. 로컬 비디오 반복 재생
./build/tcamviewer play /path/to/sample.mp4 --loop

# 2. USB 웹캠 실시간 모니터링 (화면 채우기)
./build/tcamviewer play /dev/video0 --stretch

# 3. 90도 회전된 스마트폰 영상 보정 재생
./build/tcamviewer play /path/to/vertical_video.mp4 --rotate 90

# 4. RTSP 저지연 네트워크 카메라 스트림 재생
./build/tcamviewer play rtsp://admin:password@192.168.1.50:554/h264Preview_01_main
```

---

## 3. 재생 중 실시간 단축키 (Interactive Hotkeys)

비디오가 재생되는 동안 터미널에서 다음 키를 눌러 실시간으로 화면을 제어할 수 있습니다:

| 단축키 | 동작 |
| :---: | :--- |
| **`a`** 또는 **`A`** | **종횡비 유지(Fit) ↔ 터미널 채우기(Stretch)** 실시간 토글 |
| **`r`** 또는 **`R`** | **시계 방향 90도 회전** (+90° → +180° → +270° → 0°) 실시간 토글 |
| **`q`** 또는 **`Q`** | 비디오 재생 종료 및 터미널 상태 복원 |
| **`Ctrl + C`** | 안전한 강제 종료 |

---

## 4. `test` 명령어 상세

```bash
tcamviewer test [flags]
```
터미널의 TrueColor(24비트 색상) 지원 여부와 초당 프레임 수(FPS) 성능을 측정합니다.

| 플래그 | 단축 플래그 | 타입 | 기본값 | 설명 |
| :--- | :--- | :--- | :--- | :--- |
| `--duration` | `-d` | int | `5` | 테스트 패턴 실행 시간 (초 단위) |
| `--fps` | | int | `30` | 목표 프레임 레이트 |
