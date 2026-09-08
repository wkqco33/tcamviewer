# ROS 2 연동 가이드 (ROS 2 Integration)

`tcamviewer`는 로봇 운영체제(ROS 2) 카메라 토픽을 터미널에서 모니터링할 수 있도록 **C++ 네이티브 노드**와 **Python 노드**를 모두 제공합니다.

지원 배포판: **ROS 2 Jazzy Jalisco**, **ROS 2 Humble Hawksbill**

---

## 1. C++ 네이티브 노드 (`ros2/`)

제로카피 포인터 교환으로 초저지연과 극도의 CPU 효율이 필요할 때 사용합니다.

### 빌드
```bash
# ROS 2 환경 로드
source /opt/ros/jazzy/setup.bash

# colcon 빌드 (Taskfile 사용)
task build:ros2

# 또는 직접 colcon 실행
colcon build --paths ros2
```

### 실행
```bash
source install/setup.bash

# 기본 토픽 (/camera/image_raw) 구독 실행
ros2 run tcamviewer_ros2 tcamviewer_node

# 토픽 이름 변경 및 90도 회전 파라미터 적용
ros2 run tcamviewer_ros2 tcamviewer_node --ros-args \
    -p topic:=/robot/head_camera/image_raw \
    -p rotation:=90

# 압축 이미지 (CompressedImage) 구독
ros2 run tcamviewer_ros2 tcamviewer_node --ros-args \
    -p topic:=/camera/image_raw/compressed \
    -p compressed:=true
```

---

## 2. Python 노드 (`python/ros2_node.py`)

빠른 프로토타이핑이나 AI 비전 파이프라인 디버깅 시 사용합니다.

### 실행
```bash
source /opt/ros/jazzy/setup.bash

# Raw 이미지 토픽 구독
python3 python/ros2_node.py --ros-args -p topic:=/camera/image_raw

# 압축(Compressed) 토픽 구독
python3 python/ros2_node.py --ros-args -p topic:=/camera/image_raw/compressed -p compressed:=true
```

---

## 3. ROS 2 파라미터 목록

| 파라미터 이름 | 타입 | 기본값 | 설명 |
| :--- | :--- | :--- | :--- |
| `topic` | `string` | `"/camera/image_raw"` | 구독할 카메라 토픽 경로 |
| `compressed` | `bool` | `false` | `sensor_msgs/msg/CompressedImage` 여부 |
| `width` | `int` | `0` | 터미널 가로 문자 수 (0: 자동 감지) |
| `height` | `int` | `0` | 터미널 세로 문자 수 (0: 자동 감지) |
| `use_diff` | `bool` | `true` | 이전 프레임 Dirty-diff 캐시 최적화 활성화 |
| `alt_screen` | `bool` | `true` | 종료 시 터미널 화면 복원을 위한 Alternate Buffer 사용 |
| `rotation` | `int` | `0` | 시계 방향 회전 각도 (0, 90, 180, 270) |
| `keep_aspect_ratio` | `bool` | `true` | 원본 영상 비율 유지 및 중앙 정렬 (레터박스) |
