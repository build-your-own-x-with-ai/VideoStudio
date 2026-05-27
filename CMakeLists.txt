cmake_minimum_required(VERSION 3.16)
project(VideoStudio VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "/Users/i/Qt/6.11.0/macos")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
    libavformat
    libavcodec
    libavutil
    libswscale
)

set(SOURCES
    src/main.cpp
    src/core/VideoDecoder.cpp
    src/ui/MainWindow.cpp
    src/ui/StreamInfoPanel.cpp
)

set(HEADERS
    src/core/FrameInfo.h
    src/core/StreamInfo.h
    src/core/VideoDecoder.h
    src/ui/MainWindow.h
    src/ui/StreamInfoPanel.h
)

add_executable(VideoStudio ${SOURCES} ${HEADERS})

target_include_directories(VideoStudio PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(VideoStudio
    Qt6::Core
    Qt6::Widgets
    ${FFMPEG_LIBRARIES}
)

target_link_directories(VideoStudio PRIVATE
    ${FFMPEG_LIBRARY_DIRS}
)

if(APPLE)
    set_target_properties(VideoStudio PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.videostudio.app
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()
