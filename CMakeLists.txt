cmake_minimum_required(VERSION 3.25)
project(Kvikplay VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to high-performance Release build with Link-Time Optimization
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    add_compile_options(-O3 -flto)
    add_link_options(-flto)
elseif(MSVC)
    add_compile_options(/O2 /GL)
    add_link_options(/LTCG)
endif()

set(CMAKE_AUTORCC ON)

# Homebrew paths for macOS discovery
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH 
        "/opt/homebrew" 
        "/opt/homebrew/opt/qtbase" 
        "/opt/homebrew/opt/qtsvg" 
        "/opt/homebrew/opt/libarchive"
    )
    set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/lib/pkgconfig:/opt/homebrew/share/pkgconfig:/opt/homebrew/opt/libarchive/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets Svg)
qt_standard_project_setup()

find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

set(KVIKPLAY_SOURCES
    src/main.cpp
    src/app/Application.h
    src/app/Application.cpp
    src/formats/FormatRegistry.h
    src/formats/FormatRegistry.cpp
    src/config/SettingsManager.h
    src/config/SettingsManager.cpp
    src/player/PlaybackState.h
    src/player/MediaInfo.h
    src/player/MpvController.h
    src/player/MpvController.cpp
    src/player/MediaPlayer.h
    src/player/MediaPlayer.cpp
    src/player/ThumbnailWorker.h
    src/player/ThumbnailWorker.cpp
    src/rendering/VideoRenderer.h
    src/rendering/VideoRenderer.cpp
    src/window/AspectRatioManager.h
    src/window/AspectRatioManager.cpp
    src/window/MainWindow.h
    src/window/MainWindow.cpp
    src/ui/OsdNotification.h
    src/ui/OsdNotification.cpp
    src/ui/VolumeControl.h
    src/ui/VolumeControl.cpp
    src/ui/PlaybackOverlay.h
    src/ui/PlaybackOverlay.cpp
    src/ui/TimelinePreviewCard.h
    src/ui/TimelinePreviewCard.cpp
    src/ui/ContextMenu.h
    src/ui/ContextMenu.cpp
    src/ui/MediaInspectorOverlay.h
    src/ui/MediaInspectorOverlay.cpp
    src/ui/HelpDialog.h
    src/ui/HelpDialog.cpp
    src/ui/FileAssociationDialog.h
    src/ui/FileAssociationDialog.cpp
    src/platform/PlatformIntegration.h
    src/platform/PlatformIntegration.cpp
    resources/resources.qrc
)

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)

    set(MACOSX_BUNDLE_ICON_FILE "kvikplay.icns")
    set(MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/kvikplay.icns")
    set_source_files_properties(${MACOS_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    set(PLATFORM_SOURCES
        src/platform/macos/MacOSIntegration.h
        src/platform/macos/MacOSIntegration.mm
        ${MACOS_ICON}
    )
elseif(WIN32)
    set(PLATFORM_SOURCES
        src/platform/windows/WindowsIntegration.h
        src/platform/windows/WindowsIntegration.cpp
    )
endif()

list(APPEND KVIKPLAY_SOURCES ${PLATFORM_SOURCES})

qt_add_executable(kvikplay
    MANUAL_FINALIZATION
    ${KVIKPLAY_SOURCES}
)

target_include_directories(kvikplay PRIVATE src)

target_link_libraries(kvikplay PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Svg
    PkgConfig::MPV
)

if(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(CORESERVICES_LIBRARY CoreServices REQUIRED)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(MEDIAPLAYER_LIBRARY MediaPlayer REQUIRED)
    target_link_libraries(kvikplay PRIVATE
        ${COCOA_LIBRARY}
        ${CORESERVICES_LIBRARY}
        ${OPENGL_LIBRARY}
        ${IOKIT_LIBRARY}
        ${MEDIAPLAYER_LIBRARY}
    )

    set_target_properties(kvikplay PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.kvikplay.player"
        MACOSX_BUNDLE_BUNDLE_NAME "Kvikplay"
        MACOSX_BUNDLE_ICON_FILE "kvikplay.icns"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
    )

    add_executable(generate_icon packaging/macos/generate_icon.cpp)
    target_link_libraries(generate_icon PRIVATE Qt6::Core Qt6::Gui Qt6::Svg)
elseif(WIN32)
    target_link_libraries(kvikplay PRIVATE
        dwmapi
        shell32
    )
endif()

qt_finalize_executable(kvikplay)

# Unit Testing
enable_testing()

add_executable(test_format_registry
    tests/TestFormatRegistry.cpp
    src/formats/FormatRegistry.h
    src/formats/FormatRegistry.cpp
)
target_include_directories(test_format_registry PRIVATE src)
target_link_libraries(test_format_registry PRIVATE Qt6::Core)
add_test(NAME test_format_registry COMMAND test_format_registry)

add_executable(test_aspect_ratio
    tests/TestAspectRatio.cpp
    src/window/AspectRatioManager.h
    src/window/AspectRatioManager.cpp
)
target_include_directories(test_aspect_ratio PRIVATE src)
target_link_libraries(test_aspect_ratio PRIVATE Qt6::Core)
add_test(NAME test_aspect_ratio COMMAND test_aspect_ratio)

add_executable(test_settings
    tests/TestSettings.cpp
    src/config/SettingsManager.h
    src/config/SettingsManager.cpp
)
target_include_directories(test_settings PRIVATE src)
target_link_libraries(test_settings PRIVATE Qt6::Core)
add_test(NAME test_settings COMMAND test_settings)

add_executable(test_playback
    tests/TestPlayback.cpp
    src/player/PlaybackState.h
    src/player/MediaInfo.h
    src/player/MpvController.h
    src/player/MpvController.cpp
    src/player/MediaPlayer.h
    src/player/MediaPlayer.cpp
    src/config/SettingsManager.h
    src/config/SettingsManager.cpp
    src/formats/FormatRegistry.h
    src/formats/FormatRegistry.cpp
)
target_include_directories(test_playback PRIVATE src)
target_link_libraries(test_playback PRIVATE Qt6::Core PkgConfig::MPV)
add_test(NAME test_playback COMMAND test_playback)

add_executable(test_platform
    tests/TestPlatform.cpp
    src/platform/PlatformIntegration.h
    src/platform/PlatformIntegration.cpp
    ${PLATFORM_SOURCES}
)
target_include_directories(test_platform PRIVATE src)
target_link_libraries(test_platform PRIVATE Qt6::Widgets)
if(APPLE)
    target_link_libraries(test_platform PRIVATE
        ${COCOA_LIBRARY}
        ${CORESERVICES_LIBRARY}
        ${IOKIT_LIBRARY}
        ${MEDIAPLAYER_LIBRARY}
    )
endif()
add_test(NAME test_platform COMMAND test_platform)
