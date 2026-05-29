cmake_minimum_required(VERSION 3.16)
project(VideoStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
set(CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.11.0/macos" ${CMAKE_PREFIX_PATH})
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Find FFmpeg using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
    libavcodec
    libavformat
    libavutil
    libswscale
)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/core/videodecoder.cpp
    src/core/framedata.cpp
    src/widgets/videooutput.cpp
    src/widgets/barchart.cpp
)

set(HEADERS
    src/mainwindow.h
    src/core/videodecoder.h
    src/core/framedata.h
    src/widgets/videooutput.h
    src/widgets/barchart.h
)

# Create executable
add_executable(VideoStudio MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
)

# Link libraries
target_link_libraries(VideoStudio PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${FFMPEG_LIBRARIES}
)

# Include directories
target_include_directories(VideoStudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

# Compiler flags
target_compile_options(VideoStudio PRIVATE
    ${FFMPEG_CFLAGS_OTHER}
)

# macOS bundle settings
set_target_properties(VideoStudio PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.videostudio.app"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in
)
