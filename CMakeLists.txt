cmake_minimum_required(VERSION 3.16)
project(VideoStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
set(CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.11.0/macos" ${CMAKE_PREFIX_PATH})
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Network)

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
    src/core/tsparser.cpp
    src/core/mp4parser.cpp
    src/core/mkvparser.cpp
    src/core/aviparser.cpp
    src/core/flvparser.cpp
    src/core/yuvreader.cpp
    src/core/timinganalyzer.cpp
    src/core/duplicateframedetector.cpp
    src/widgets/videooutput.cpp
    src/widgets/barchart.cpp
    src/widgets/thumbnailbar.cpp
    src/widgets/areachart.cpp
    src/widgets/gopviewer.cpp
    src/widgets/packetview.cpp
    src/widgets/logviewer.cpp
    src/panels/streampanel.cpp
    src/panels/overlaypanel.cpp
    src/panels/explorerpanel.cpp
    src/panels/propertypanel.cpp
    src/panels/hexviewerpanel.cpp
    src/panels/messagespanel.cpp
    src/panels/tr101290panel.cpp
    src/panels/timedynamicspanel.cpp
    src/panels/bitratepanel.cpp
    src/panels/bufferpanel.cpp
    src/panels/graphicspanel.cpp
    src/panels/commentspanel.cpp
    src/panels/epgpanel.cpp
    src/panels/blockstatspanel.cpp
    src/dialogs/savestreaminfodialog.cpp
    src/dialogs/qualitymetricsdialog.cpp
    src/dialogs/aboutdialog.cpp
    src/dialogs/csvexportdialog.cpp
    src/dialogs/referencecomparisondialog.cpp
    src/dialogs/yuvexportdialog.cpp
    src/dialogs/yuvviewerdialog.cpp
    src/dialogs/duplicateframedetectiondialog.cpp
)

set(HEADERS
    src/mainwindow.h
    src/core/videodecoder.h
    src/core/framedata.h
    src/core/tsdata.h
    src/core/tsparser.h
    src/core/mp4parser.h
    src/core/mkvparser.h
    src/core/aviparser.h
    src/core/flvparser.h
    src/core/yuvreader.h
    src/core/tr101290data.h
    src/core/timinganalyzer.h
    src/core/duplicateframedetector.h
    src/widgets/videooutput.h
    src/widgets/barchart.h
    src/widgets/thumbnailbar.h
    src/widgets/areachart.h
    src/widgets/gopviewer.h
    src/widgets/packetview.h
    src/widgets/logviewer.h
    src/panels/streampanel.h
    src/panels/overlaypanel.h
    src/panels/explorerpanel.h
    src/panels/propertypanel.h
    src/panels/hexviewerpanel.h
    src/panels/messagespanel.h
    src/panels/tr101290panel.h
    src/panels/timedynamicspanel.h
    src/panels/bitratepanel.h
    src/panels/bufferpanel.h
    src/panels/graphicspanel.h
    src/panels/commentspanel.h
    src/panels/epgpanel.h
    src/panels/blockstatspanel.h
    src/dialogs/savestreaminfodialog.h
    src/dialogs/qualitymetricsdialog.h
    src/dialogs/aboutdialog.h
    src/dialogs/csvexportdialog.h
    src/dialogs/referencecomparisondialog.h
    src/dialogs/yuvexportdialog.h
    src/dialogs/yuvviewerdialog.h
    src/dialogs/duplicateframedetectiondialog.h
)

# Create executable
add_executable(VideoStudio MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
)

# Include directories
target_include_directories(VideoStudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

# Link directories
target_link_directories(VideoStudio PRIVATE
    ${FFMPEG_LIBRARY_DIRS}
)

# Link libraries
target_link_libraries(VideoStudio PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    ${FFMPEG_LIBRARIES}
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
    MACOSX_BUNDLE_BUNDLE_NAME "VideoStudio"
    MACOSX_BUNDLE_ICON_FILE VideoStudio.icns
)

# Copy icon file to bundle
set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/VideoStudio.icns)
set_source_files_properties(${ICON_FILE} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)
target_sources(VideoStudio PRIVATE ${ICON_FILE})
