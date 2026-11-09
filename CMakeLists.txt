cmake_minimum_required(VERSION 3.16)
project(tcamviewer VERSION 0.2.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(TCAMVIEWER_FFMPEG_STATIC
    "정적 FFmpeg(TCAMVIEWER_FFMPEG_DIR)로 링크해 런타임 FFmpeg 의존성을 제거합니다" OFF)

if(TCAMVIEWER_FFMPEG_STATIC)
    if(NOT TCAMVIEWER_FFMPEG_DIR)
        message(FATAL_ERROR
            "TCAMVIEWER_FFMPEG_STATIC=ON 이면 TCAMVIEWER_FFMPEG_DIR에 정적 FFmpeg prefix 경로를 지정해야 합니다")
    endif()
    foreach(lib avformat avcodec swscale swresample avutil)
        if(NOT EXISTS "${TCAMVIEWER_FFMPEG_DIR}/lib/lib${lib}.a")
            message(FATAL_ERROR "정적 라이브러리 없음: ${TCAMVIEWER_FFMPEG_DIR}/lib/lib${lib}.a")
        endif()
    endforeach()

    add_library(tcamviewer_ffmpeg INTERFACE)
    target_include_directories(tcamviewer_ffmpeg INTERFACE
        "${TCAMVIEWER_FFMPEG_DIR}/include")
    # 링크 순서가 중요하다: 상위 라이브러리가 하위 라이브러리를 참조한다.
    target_link_libraries(tcamviewer_ffmpeg INTERFACE
        "${TCAMVIEWER_FFMPEG_DIR}/lib/libavformat.a"
        "${TCAMVIEWER_FFMPEG_DIR}/lib/libavcodec.a"
        "${TCAMVIEWER_FFMPEG_DIR}/lib/libswscale.a"
        "${TCAMVIEWER_FFMPEG_DIR}/lib/libswresample.a"
        "${TCAMVIEWER_FFMPEG_DIR}/lib/libavutil.a"
        m pthread
    )
    message(STATUS "FFmpeg: static (${TCAMVIEWER_FFMPEG_DIR})")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
    pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
    pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)

    add_library(tcamviewer_ffmpeg INTERFACE)
    target_link_libraries(tcamviewer_ffmpeg INTERFACE
        PkgConfig::AVCODEC
        PkgConfig::AVFORMAT
        PkgConfig::SWSCALE
        PkgConfig::AVUTIL
        pthread
    )
endif()

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
    tcamviewer_ffmpeg
)

target_include_directories(tcamviewer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(tcamviewer PUBLIC
    tcamviewer_ffmpeg
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
