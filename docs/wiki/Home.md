# tcamviewer 공식 위키 📹

`tcamviewer`는 로컬 비디오 파일(MP4, MKV), 실시간 스트림(RTSP, RTMP), USB 웹캠(V4L2), 그리고 **ROS 2 카메라 토픽**(`sensor_msgs/msg/Image`, `sensor_msgs/msg/CompressedImage`)을 터미널에서 초저지연·고성능으로 실시간 렌더링하는 터미널 비디오 플레이어 및 라이브러리입니다.

---

## 🚀 빠른 시작 (Quick Start)

### 1. 설치 및 빌드
```bash
# 저장소 클론 (서브모듈 포함)
git clone --recursive https://github.com/wkqco33/tcamviewer.git
cd tcamviewer

# 시스템 필수 패키지 설치 (Ubuntu 기준)
sudo apt-get update && sudo apt-get install -y \
    build-essential cmake pkg-config libgtest-dev \
    libavcodec-dev libavformat-dev libswscale-dev libavutil-dev

# 프로젝트 전체 빌드 (C++ 라이브러리, CLI, 테스트)
task build
```

### 2. 렌더링 테스트 실행
```bash
# TrueColor 30 FPS 애니메이션 테스트 패턴 실행
./build/tcamviewer test --duration 5 --fps 30
```

### 3. 비디오 파일 또는 실시간 웹캠 재생
```bash
# 동영상 파일 재생 (반복 재생)
./build/tcamviewer play /path/to/video.mp4 --loop

# USB 웹캠 모니터링
./build/tcamviewer play /dev/video0

# RTSP 네트워크 스트림 모니터링
./build/tcamviewer play rtsp://192.168.1.100:554/stream1
```

---

## 📚 위키 문서 목차

1. **[시스템 아키텍처 (Architecture)](Architecture)**:
   - 코어 엔진 설계, 제로카피 C-ABI 파이프라인, 다국어 바인딩 구조
2. **[CLI 사용 가이드 (CLI-Usage)](CLI-Usage)**:
   - CLI 명령어(`play`, `test`), 플래그, 재생 중 실시간 핫키(`a`, `r`, `q`) 제어
3. **[ROS 2 연동 가이드 (ROS2-Integration)](ROS2-Integration)**:
   - ROS 2 Jazzy/Humble 환경에서 C++ 네이티브 노드 및 Python 노드 설정과 실행
4. **[API 레퍼런스 (API-Reference)](API-Reference)**:
   - C-ABI (`tcamviewer.h`), Go 패키지(`pkg/tcamviewer`), Python 모듈(`tcamviewer`) 함수 명세
5. **[성능 최적화 기법 (Performance-Optimization)](Performance-Optimization)**:
   - Half-Block TrueColor 원리, Dirty-diff 캐시, Zero-Allocation 스트리밍 포맷터, 64-bit 레지스터 비교
