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
    src/player/FolderIndex.h
    src/player/FolderIndex.cpp
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
    src/platform/SingleInstance.h
    src/platform/SingleInstance.cpp
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

# One version number: project(VERSION) above -> version.h, Info.plist, app.rc
configure_file(src/app/version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h" @ONLY)
target_include_directories(kvikplay PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/generated")

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
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )

    add_executable(generate_icon packaging/macos/generate_icon.cpp)
    target_link_libraries(generate_icon PRIVATE Qt6::Core Qt6::Gui Qt6::Svg)
elseif(WIN32)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/app.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
        @ONLY
    )
    target_sources(kvikplay PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
    target_link_libraries(kvikplay PRIVATE
        dwmapi
        shell32
    )
endif()

qt_finalize_executable(kvikplay)

# Standalone Deployment Automation
option(KVIKPLAY_STANDALONE "Automatically bundle all dependencies into a standalone app on build" ON)

if(KVIKPLAY_STANDALONE)
    if(APPLE)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
            HINTS
            "/opt/homebrew/opt/qtbase/bin"
            "/opt/homebrew/bin"
            "/usr/local/opt/qtbase/bin"
            "/usr/local/bin"
        )
        if(MACDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET kvikplay POST_BUILD
                COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/package_macos.sh" "$<TARGET_BUNDLE_DIR:kvikplay>" "${MACDEPLOYQT_EXECUTABLE}"
                COMMENT "Bundling dependencies to create a 100% standalone portable macOS app"
                VERBATIM
            )
        else()
            message(WARNING "macdeployqt not found. Skipping automatic standalone packaging.")
        endif()
    elseif(WIN32)
        get_target_property(_qt_moc_loc Qt6::moc IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qt_moc_loc}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt
            HINTS
            "${_qt_bin_dir}"
            "$ENV{QTDIR}/bin"
        )

        set(MPV_DLL_HINT "")
        if(TARGET PkgConfig::MPV)
            get_target_property(_mpv_lib_dirs PkgConfig::MPV INTERFACE_LINK_DIRECTORIES)
            if(_mpv_lib_dirs)
                list(GET _mpv_lib_dirs 0 MPV_DLL_HINT)
            endif()
        endif()

        add_custom_command(TARGET kvikplay POST_BUILD
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/package_windows.bat" "$<TARGET_FILE:kvikplay>" "${WINDEPLOYQT_EXECUTABLE}" "${MPV_DLL_HINT}"
            COMMENT "Bundling dependencies to create a 100% standalone portable Windows app"
            VERBATIM
        )
    endif()
endif()

# Standalone bundle target alias
add_custom_target(standalone_bundle
    DEPENDS kvikplay
    COMMENT "Standalone bundle generated automatically with kvikplay target"
)

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

add_executable(test_thumbnails
    tests/TestThumbnails.cpp
    src/player/ThumbnailWorker.h
    src/player/ThumbnailWorker.cpp
    src/formats/FormatRegistry.h
    src/formats/FormatRegistry.cpp
)
target_include_directories(test_thumbnails PRIVATE src)
target_link_libraries(test_thumbnails PRIVATE Qt6::Core Qt6::Gui PkgConfig::MPV)
add_test(NAME test_thumbnails COMMAND test_thumbnails)

add_executable(test_folder_index
    tests/TestFolderIndex.cpp
    src/player/FolderIndex.h
    src/player/FolderIndex.cpp
    src/formats/FormatRegistry.h
    src/formats/FormatRegistry.cpp
)
target_include_directories(test_folder_index PRIVATE src)
target_link_libraries(test_folder_index PRIVATE Qt6::Core)
add_test(NAME test_folder_index COMMAND test_folder_index)

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

add_test(NAME test_version_consistency
    COMMAND ${CMAKE_COMMAND} -DVERSION=${PROJECT_VERSION} -DSRC=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/CheckVersion.cmake)

# Run every test from the source tree (so test_media/ is found from any build
# folder) and point the settings at a throwaway folder instead of the user's.
set(KVIKPLAY_TESTS test_format_registry test_aspect_ratio test_settings test_playback test_thumbnails test_folder_index test_platform)
set_tests_properties(${KVIKPLAY_TESTS} PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    ENVIRONMENT "KVIKPLAY_CONFIG_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-config"
    TIMEOUT 60
)
