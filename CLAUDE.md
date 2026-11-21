# CLAUDE.md

Guidance for Claude Code in this repository. The general rulebook is
`~/Desktop/code/tools/standards/STANDARDS.md`; decisions and waivers are in `.claude-standards`.

## What this is

Kvikplay: a minimal, frameless video and audio player for **macOS and Windows**, written in C++20
with Qt 6 Widgets and libmpv. The main goals, in order: **fast and responsive** (instant open,
instant seek, low CPU while playing), **lightweight**, and **equal features on both OSes**,
each feature adapted to the OS's own conventions where they differ.

Design: its own minimal look (`design=own`). It is **not** part of the Låda Glass family; don't
bring Låda Glass in.

## Commands

```sh
# Configure + build (Release, -O3/LTO). Packaging into a standalone app runs after every build.
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8

# Fast dev loop: no standalone packaging, asserts enabled
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DKVIKPLAY_STANDALONE=OFF
cmake --build build-debug -j8
ctest --test-dir build-debug --output-on-failure

# Run
open build/kvikplay.app                       # macOS
./build/kvikplay.app/Contents/MacOS/kvikplay file.mkv
```

Tests use plain `assert()`. **Run them in a Debug build**: Release defines `NDEBUG`, which removes
every assert, so a Release `ctest` passes whatever happens. `test_playback` needs the media in
`test_media/` (gitignored for now, so a fresh clone can't run it).

Tests use `CHECK()` from `tests/TestCheck.h` (never `assert()`, which Release removes), run from
the source tree and get `KVIKPLAY_CONFIG_DIR` pointed at a temp folder (see `CMakeLists.txt`).
CI (`.github/workflows/ci.yml`) builds and tests on macOS and on Windows via MSYS2.

A `build/` configured before the repo moved to `~/Desktop/code/kvikplay` points at old paths;
delete it and configure again if `ctest` reports "Not Run".

## Screenshots

README screenshots go in `docs/screenshots/`. Whenever a screenshot needs a video, use
`~/Desktop/video_temp/big_buck_bunny_1080p_h264.mov` (local only, not in the repo), and
keep **every** screenshot on that same file. When retaking one, retake any older shot that shows a
different video too.

Current shots (2026-11-21, macOS): `playback.jpg`, `menu.jpg`, `inspector.jpg`, saved as 1280 px
JPEG. Still owed: a timeline hover-preview shot, once the preview card's missing background and
the blank thumbnails are fixed. Retake `inspector.jpg` too once its background is fixed.

How they were taken: run the app with the file above, then drive it with synthetic `CGEvent`
mouse and key events. The window must be clicked (focused) before hovering shows the overlay.
Capture the window with `screencapture -l <windowID>`, or use a screen region (`-R`) for the
popup menu.

## Layout

| Path | What |
|---|---|
| `src/main.cpp`, `src/app/` | Entry point, `QApplication` subclass (command-line file, macOS `FileOpen`) |
| `src/player/MpvController.*` | All libmpv calls: options, property observers, commands |
| `src/player/MediaPlayer.*` | Thin facade over `MpvController`; persists volume/mute |
| `src/player/ThumbnailWorker.*` | Second, headless mpv instance on its own thread for timeline thumbnails |
| `src/rendering/VideoRenderer.*` | `QOpenGLWidget` that hosts the mpv OpenGL render context |
| `src/window/MainWindow.*` | Frameless window, input (mouse, wheel, keys), fullscreen/PiP, Windows `nativeEvent` |
| `src/window/AspectRatioManager.*` | Pure geometry: aspect-locked sizes and resize constraints (unit tested) |
| `src/ui/` | Overlay, timeline, volume, OSD, preview card, inspector, menus and dialogs |
| `src/config/SettingsManager.*` | JSON settings in `QStandardPaths::AppConfigLocation/config.json` |
| `src/formats/FormatRegistry.*` | The single list of supported extensions (filter, associations, validation) |
| `src/platform/` | `PlatformIntegration` interface; `macos/` (Obj-C++) and `windows/` (Win32) |
| `packaging/`, `scripts/` | Info.plist, Windows `.rc`, icon generator, `macdeployqt`/`windeployqt` scripts |
| `tests/` | Assert-based test executables registered with CTest |

## Rules for this repo

- **Speed first.** Nothing on the GUI thread may block on mpv (use `mpv_command_async` /
  `mpv_set_property_async`; synchronous `mpv_get_property` only for cheap reads), on disk or on
  LaunchServices/registry in a hot path. Don't repaint per video frame from the UI side.
- **Cross-platform parity.** Every user-facing feature exists on both macOS and Windows. If a
  feature uses an OS API (Now Playing, media keys, sleep inhibit, file associations, window
  rounding), implement both sides in `src/platform/` behind `PlatformIntegration`, or write down
  the gap in the README's platform table.
- **Untrusted input.** File names, track and chapter titles and any media metadata are untrusted:
  show them as plain text (`Qt::PlainText`), never as Qt rich text.
- **One version number.** `project(Kvikplay VERSION x.y.z)` in `CMakeLists.txt` is the source;
  README, CHANGELOG, Info.plist and the Windows `.rc` must match it.
- libmpv requires `LC_NUMERIC=C`; keep the `setlocale` calls.
- The render context must be freed before the mpv core is destroyed (see `~MainWindow`).
- Never load a file before `VideoRenderer` has created its render context
  (`isRenderSetupDone()`): mpv would pick its `null` video output and the window stays black.
  `MainWindow::openFile` queues the file until `renderSetupFinished`.
- Every libmpv instance starts with `applyBaseMpvOptions()` (`src/player/MpvOptions.h`).
- Opening from Finder/Explorer goes through `MainWindow::openExternalFile` (obeys the
  "new window" setting); everything else uses `openFile` (always replaces). Windows single
  instance: `src/platform/SingleInstance.*`.
- Driving the app with synthetic CGEvent mouse moves does not reliably reach Qt widgets; keys
  do. Check hover behaviour by hand.
