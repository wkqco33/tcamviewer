cmake_minimum_required(VERSION 3.16)
project(tcamviewer_ros2)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_executable(tcamviewer_node
  src/tcamviewer_node.cpp
)

target_link_libraries(tcamviewer_node
  ${CMAKE_CURRENT_SOURCE_DIR}/../build/libtcamviewer.so
  ${OpenCV_LIBRARIES}
)

ament_target_dependencies(tcamviewer_node
  rclcpp
  sensor_msgs
  cv_bridge
)

install(TARGETS tcamviewer_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
