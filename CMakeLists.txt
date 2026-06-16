cmake_minimum_required(VERSION 3.16)
project(VideoStudio VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
if(APPLE)
    set(CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.11.0/macos" ${CMAKE_PREFIX_PATH})
endif()
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Network Multimedia LinguistTools)

# Find FFmpeg
if(WIN32)
    # On Windows, use find_path/find_library
    set(FFMPEG_SEARCH_PATHS
        "$ENV{FFMPEG_DIR}/include"
        "C:/ProgramData/chocolatey/lib/ffmpeg/tools/ffmpeg/include"
        "C:/ffmpeg/include"
    )

    set(FFMPEG_LIB_PATHS
        "$ENV{FFMPEG_DIR}/lib"
        "C:/ProgramData/chocolatey/lib/ffmpeg/tools/ffmpeg/lib"
        "C:/ffmpeg/lib"
    )

    find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h
        PATHS ${FFMPEG_SEARCH_PATHS}
    )

    find_library(AVCODEC_LIBRARY avcodec
        PATHS ${FFMPEG_LIB_PATHS}
    )
    find_library(AVFORMAT_LIBRARY avformat
        PATHS ${FFMPEG_LIB_PATHS}
    )
    find_library(AVUTIL_LIBRARY avutil
        PATHS ${FFMPEG_LIB_PATHS}
    )
    find_library(SWSCALE_LIBRARY swscale
        PATHS ${FFMPEG_LIB_PATHS}
    )

    if(NOT FFMPEG_INCLUDE_DIR OR NOT AVCODEC_LIBRARY OR NOT AVFORMAT_LIBRARY
       OR NOT AVUTIL_LIBRARY OR NOT SWSCALE_LIBRARY)
        message(FATAL_ERROR "FFmpeg not found. Please set FFMPEG_DIR environment variable or install FFmpeg.")
    endif()

    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
    set(FFMPEG_LIBRARIES ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})

    message(STATUS "FFmpeg include: ${FFMPEG_INCLUDE_DIRS}")
    message(STATUS "FFmpeg libraries: ${FFMPEG_LIBRARIES}")
else()
    # On Unix-like systems, use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED
        libavcodec
        libavformat
        libavutil
        libswscale
        libswresample
    )
endif()

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
    src/core/nalunitparser.cpp
    src/core/compliancevalidator.cpp
    src/core/bufferanalyzer.cpp
    src/core/audioanalyzer.cpp
    src/core/audiomonitor.cpp
    src/plugins/pluginmanager.cpp
    src/widgets/videooutput.cpp
    src/widgets/barchart.cpp
    src/widgets/thumbnailbar.cpp
    src/widgets/areachart.cpp
    src/widgets/gopviewer.cpp
    src/widgets/waveformwidget.cpp
    src/widgets/audiolevelwidget.cpp
    src/widgets/spectrumwidget.cpp
    src/widgets/lufswidget.cpp
    src/widgets/packetview.cpp
    src/widgets/nalunitview.cpp
    src/widgets/logviewer.cpp
    src/panels/streampanel.cpp
    src/panels/overlaypanel.cpp
    src/panels/explorerpanel.cpp
    src/panels/propertypanel.cpp
    src/panels/hexviewerpanel.cpp
    src/panels/audioinfopanel.cpp
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
    src/dialogs/compliancedialog.cpp
    src/dialogs/plugindialog.cpp
    src/dialogs/aboutdialog.cpp
    src/dialogs/csvexportdialog.cpp
    src/dialogs/referencecomparisondialog.cpp
    src/dialogs/yuvexportdialog.cpp
    src/dialogs/yuvviewerdialog.cpp
    src/dialogs/duplicateframedetectiondialog.cpp
    src/dialogs/settingsdialog.cpp
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
    src/core/naldata.h
    src/core/nalunitparser.h
    src/widgets/videooutput.h
    src/widgets/barchart.h
    src/widgets/thumbnailbar.h
    src/widgets/areachart.h
    src/widgets/gopviewer.h
    src/widgets/packetview.h
    src/widgets/nalunitview.h
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
    src/dialogs/settingsdialog.h
)

# Translation files
set(TS_FILES translations/videostudio_zh_CN.ts)

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
    Qt6::Multimedia
    ${FFMPEG_LIBRARIES}
)

# Add translations
qt_add_translations(VideoStudio TS_FILES ${TS_FILES})

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

# ========================================
# CLI Executable (videostudio-cli)
# ========================================

add_executable(videostudio-cli
    src/cli/main_cli.cpp
    # Reuse core components
    src/core/videodecoder.cpp
    src/core/framedata.cpp
    src/core/compliancevalidator.cpp
    src/core/bufferanalyzer.cpp
    src/plugins/pluginmanager.cpp
)

target_include_directories(videostudio-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(videostudio-cli PRIVATE
    Qt6::Core
)

# Link FFmpeg libraries
if(WIN32)
    target_link_libraries(videostudio-cli PRIVATE ${FFMPEG_LIBRARIES})
else()
    target_link_libraries(videostudio-cli PRIVATE ${FFMPEG_LDFLAGS})
endif()

target_compile_options(videostudio-cli PRIVATE
    ${FFMPEG_CFLAGS_OTHER}
)

# Set AUTOMOC include directories
set_property(TARGET videostudio-cli PROPERTY AUTOMOC_MOC_OPTIONS "-I${CMAKE_CURRENT_SOURCE_DIR}/src")

# Install CLI to bin directory
install(TARGETS videostudio-cli
    RUNTIME DESTINATION bin
)

# ============================================================
# Sample Plugin (demonstrating plugin system)
# ============================================================

add_library(sampleanalyzer MODULE
    src/plugins/sampleanalyzerplugin.cpp
)

target_link_libraries(sampleanalyzer PRIVATE
    Qt6::Core
)

# Link FFmpeg libraries
if(WIN32)
    target_link_libraries(sampleanalyzer PRIVATE ${FFMPEG_LIBRARIES})
else()
    target_link_libraries(sampleanalyzer PRIVATE ${FFMPEG_LDFLAGS})
endif()

target_compile_options(sampleanalyzer PRIVATE
    ${FFMPEG_CFLAGS_OTHER}
)

target_include_directories(sampleanalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

set_target_properties(sampleanalyzer PROPERTIES
    AUTOMOC ON
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    PREFIX ""
    SUFFIX ".dylib"
)

# Install plugin to plugins directory
install(TARGETS sampleanalyzer
    LIBRARY DESTINATION plugins
)

# Post-build: Copy plugin to app bundle (macOS only)
if(APPLE)
    add_custom_command(TARGET sampleanalyzer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:VideoStudio>/../MacOS/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:sampleanalyzer>"
            "$<TARGET_FILE_DIR:VideoStudio>/../MacOS/plugins/"
        COMMENT "Copying plugin to app bundle"
    )
endif()

