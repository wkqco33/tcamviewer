cmake_minimum_required(VERSION 3.16)
project(tcamviewer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)

# Include wcppcli CLI framework
add_subdirectory(third_party/wcppcli)

# Core Shared Library (C-ABI and C++ API)
add_library(tcamviewer SHARED
    src/terminal.cpp
    src/renderer.cpp
    src/decoder.cpp
    src/c_api.cpp
)

# Core Static Library (정적 링크용: 단일 바이너리 배포 등)
# OUTPUT_NAME을 tcamviewer로 맞춰 build/libtcamviewer.a 를 생성한다.
add_library(tcamviewer_static STATIC
    src/terminal.cpp
    src/renderer.cpp
    src/decoder.cpp
    src/c_api.cpp
)
set_target_properties(tcamviewer_static PROPERTIES OUTPUT_NAME tcamviewer)
target_include_directories(tcamviewer_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(tcamviewer_static PUBLIC
    PkgConfig::AVCODEC
    PkgConfig::AVFORMAT
    PkgConfig::SWSCALE
    PkgConfig::AVUTIL
    pthread
)

target_include_directories(tcamviewer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(tcamviewer PUBLIC
    PkgConfig::AVCODEC
    PkgConfig::AVFORMAT
    PkgConfig::SWSCALE
    PkgConfig::AVUTIL
    pthread
)

target_compile_definitions(tcamviewer PRIVATE TCAMVIEWER_EXPORT)

# Standalone CLI Executable
add_executable(tcamviewer_cli src/cli/main.cpp)
set_target_properties(tcamviewer_cli PROPERTIES OUTPUT_NAME "tcamviewer")
target_link_libraries(tcamviewer_cli PRIVATE
    tcamviewer
    wcppcli
)

# Testing
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS tcamviewer tcamviewer_static tcamviewer_cli
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/tcamviewer
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
