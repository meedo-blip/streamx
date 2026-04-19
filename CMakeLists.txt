cmake_minimum_required(VERSION 3.16)
project(streamx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt support
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Options
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_NVENC "Enable NVIDIA NVENC encoding" OFF)
option(BUILD_GUI "Build GUI application" ON)

# Find required packages
find_package(Threads REQUIRED)

# Try to find FFmpeg - with fallback to pkg-config
find_package(FFmpeg QUIET COMPONENTS avutil avcodec avformat avdevice swscale)
if(NOT FFmpeg_FOUND)
    message(STATUS "FFmpeg not found via find_package, trying pkg-config...")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED
        libavutil libavcodec libavformat libavdevice libswscale
    )
    pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
    pkg_check_modules(LIBPORTAL REQUIRED libportal glib-2.0 gio-2.0 gobject-2.0)
    set(FFmpeg_FOUND TRUE)
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS})
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES})
    message(STATUS "FFmpeg found via pkg-config")
else()
    message(STATUS "FFmpeg found via find_package")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
    pkg_check_modules(LIBPORTAL REQUIRED libportal glib-2.0 gio-2.0 gobject-2.0)
endif()

# Linux distro detection
set(STREAMX_TARGET_DISTRO "auto" CACHE STRING "Target distro: auto, nobara, fedora, ubuntu, debian, arch, opensuse")
set(DISTRO_DETECTED "")

# Try to detect distro from os-release
if(EXISTS "/etc/os-release")
    file(READ "/etc/os-release" OS_RELEASE_CONTENT)
    if(OS_RELEASE_CONTENT MATCHES "ID=\"?nobara\"?")
        set(DISTRO_DETECTED "nobara")
    elseif(OS_RELEASE_CONTENT MATCHES "ID=\"?fedora\"?")
        set(DISTRO_DETECTED "fedora")
    elseif(OS_RELEASE_CONTENT MATCHES "ID=\"?ubuntu\"?")
        set(DISTRO_DETECTED "ubuntu")
    elseif(OS_RELEASE_CONTENT MATCHES "ID=\"?debian\"?")
        set(DISTRO_DETECTED "debian")
    elseif(OS_RELEASE_CONTENT MATCHES "ID=\"?arch\"?")
        set(DISTRO_DETECTED "arch")
    elseif(OS_RELEASE_CONTENT MATCHES "ID=\"?opensuse\"?")
        set(DISTRO_DETECTED "opensuse")
    endif()
endif()

# Use auto-detected or manually specified distro
if(STREAMX_TARGET_DISTRO STREQUAL "auto")
    set(TARGET_DISTRO ${DISTRO_DETECTED})
else()
    set(TARGET_DISTRO ${STREAMX_TARGET_DISTRO})
endif()

message(STATUS "Target distro: ${TARGET_DISTRO} (detected: ${DISTRO_DETECTED})")

# Plan configuration per distro
# Plan A: D-Bus screencast (OBS-style)
# Plan B: xdg-portal/PipeWire (most modern distros)
# Plan C: FFmpeg libavdevice (fallback)
# Plan D: GPU Screen Recorder (best for Wayland on Fedora/Nobara)

set(PLAN_A_DEFAULT ON)
set(PLAN_B_DEFAULT ON)
set(PLAN_C_DEFAULT ON)
set(PLAN_D_DEFAULT OFF)

# Distro-specific defaults
if(TARGET_DISTRO STREQUAL "nobara" OR TARGET_DISTRO STREQUAL "fedora")
    set(PLAN_D_DEFAULT ON)  # GPU Screen Recorder available
    message(STATUS "Enabling Plan D (GPU Screen Recorder) for ${TARGET_DISTRO}")
elseif(TARGET_DISTRO STREQUAL "ubuntu" OR TARGET_DISTRO STREQUAL "debian")
    set(PLAN_B_DEFAULT ON)  # Portal/PipeWire works well
    set(PLAN_D_DEFAULT OFF)  # GPU Screen Recorder needs manual install
elseif(TARGET_DISTRO STREQUAL "arch")
    set(PLAN_D_DEFAULT ON)  # Available on AUR
    set(PLAN_B_DEFAULT ON)
endif()

# Options with distro-specific defaults
option(ENABLE_PLAN_A "Enable Plan A (D-Bus OBS-style screencast)" ${PLAN_A_DEFAULT})
option(ENABLE_PLAN_B "Enable Plan B (xdg-portal/PipeWire)" ${PLAN_B_DEFAULT})
option(ENABLE_PLAN_C "Enable Plan C (FFmpeg libavdevice fallback)" ${PLAN_C_DEFAULT})
option(ENABLE_PLAN_D "Enable Plan D (GPU Screen Recorder)" ${PLAN_D_DEFAULT})
# Qt packages
if(BUILD_GUI)
    find_package(Qt6 COMPONENTS Core Gui Widgets Concurrent REQUIRED)
endif()

if(ENABLE_PLAN_A)
  add_definitions(-DENABLE_PLAN_A)
endif()
if(ENABLE_PLAN_B)
  add_definitions(-DENABLE_PLAN_B)
endif()
if(ENABLE_PLAN_C)
  add_definitions(-DENABLE_PLAN_C)
endif()
if(ENABLE_PLAN_D)
  add_definitions(-DENABLE_PLAN_D)
  list(APPEND CORE_SOURCES src/plan_d/plan_d.cpp)
endif()

# Optional packages
find_package(CUDA QUIET)
if(CUDA_FOUND AND ENABLE_NVENC)
    message(STATUS "NVIDIA CUDA found - NVENC support enabled")
    set(HAVE_NVENC TRUE)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${FFMPEG_INCLUDE_DIRS})
include_directories(${PIPEWIRE_INCLUDE_DIRS})
include_directories(${LIBPORTAL_INCLUDE_DIRS})

# Source files
set(CORE_SOURCES
    src/core/encoder.cpp
    src/core/capture.cpp
    src/core/buffer.cpp
    src/core/frame.cpp
    src/streaming_controller.cpp
)

# Plan A: D-Bus screencast (requires portal)
if(ENABLE_PLAN_A)
    list(APPEND CORE_SOURCES src/portal/dbus_screencast.cpp)
endif()

# Plan C: FFmpeg libavdevice
if(ENABLE_PLAN_C)
    list(APPEND CORE_SOURCES src/plan_c/plan_c.cpp)
endif()

# Plan D: GPU Screen Recorder
if(ENABLE_PLAN_D)
    list(APPEND CORE_SOURCES src/plan_d/plan_d.cpp)
endif()

set(PLATFORM_SOURCES
    src/platform/streaming_platform.cpp
    src/platform/rtmp_handler.cpp
)

set(PLATFORMS_SOURCES
    src/platforms/platform_implementations.cpp
)

set(UTILS_SOURCES
    src/utils/logger.cpp
    src/utils/config.cpp
    src/utils/thread_pool.cpp
)

set(MAIN_SOURCES
    src/main.cpp
)

# Core library
add_library(streamx_core STATIC
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${PLATFORMS_SOURCES}
    ${UTILS_SOURCES}
)

target_link_libraries(streamx_core
    PUBLIC Threads::Threads
    PUBLIC ${FFMPEG_LIBRARIES}
    PUBLIC ${PIPEWIRE_LIBRARIES}
    PUBLIC ${LIBPORTAL_LIBRARIES}
)

if(BUILD_GUI)
    target_link_libraries(streamx_core PUBLIC Qt6::Gui)
    target_compile_definitions(streamx_core PUBLIC STREAMX_HAVE_QT_GUI)
endif()

if(HAVE_NVENC)
    target_link_libraries(streamx_core PUBLIC ${CUDA_LIBRARIES})
    target_compile_definitions(streamx_core PUBLIC HAVE_NVENC)
endif()

# Main executable
add_executable(streamx ${MAIN_SOURCES})
target_link_libraries(streamx PRIVATE streamx_core)

# GUI executable
if(BUILD_GUI)
    set(GUI_SOURCES
        src/gui/main_gui.cpp
        src/gui/main_window.cpp
        src/gui/stream_control_widget.cpp
        src/gui/monitoring_widget.cpp
        src/gui/platforms_widget.cpp
        src/gui/settings_widget.cpp
        src/gui/resources.qrc
        include/streamx/gui/main_window.h
        include/streamx/gui/stream_control_widget.h
        include/streamx/gui/monitoring_widget.h
        include/streamx/gui/platforms_widget.h
        include/streamx/gui/settings_widget.h
    )

    add_executable(streamx_gui ${GUI_SOURCES})
    
    set_target_properties(streamx_gui PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
    )
    
    target_link_libraries(streamx_gui PRIVATE 
        streamx_core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Concurrent
    )

    target_include_directories(streamx_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Print configuration
message(STATUS "=== StreamX Configuration ===")
message(STATUS "Target Distro: ${TARGET_DISTRO} (detected: ${DISTRO_DETECTED})")
message(STATUS "Plan A (D-Bus): ${ENABLE_PLAN_A}")
message(STATUS "Plan B (Portal/PipeWire): ${ENABLE_PLAN_B}")
message(STATUS "Plan C (FFmpeg): ${ENABLE_PLAN_C}")
message(STATUS "Plan D (GPU Screen Recorder): ${ENABLE_PLAN_D}")
message(STATUS "NVENC Support: ${HAVE_NVENC}")
message(STATUS "Build Tests: ${BUILD_TESTS}")
message(STATUS "Build GUI: ${BUILD_GUI}")
message(STATUS "=============================")
