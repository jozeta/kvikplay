# Kvikplay

> An extremely lightweight, fast, minimalistic cross-platform video player for macOS and Windows.

Kvikplay provides the core playback power of a traditional media player (powered by **libmpv**), built around a philosophy of Nordic minimalism, pure speed, and an unobtrusive desktop presence.

During normal playback, there is practically no interface chrome—no traditional toolbars, no title bar, no permanent playback controls, no window borders, and no playlists. The video occupies 100% of the frameless window, perfectly locked to its native display aspect ratio.

---

## Key Features

- **Frameless Nordic Minimalist Design**:
  - Rounded corners (`14px` radius) with native OS drop shadow.
  - No title bar, no window decorations, no standard buttons.
  - Video fills the entire window surface without letterboxing, pillarboxing, or distortion.
- **Strict Aspect-Ratio Locking & Quick Sizing**:
  - Automatically constrains window geometry to preserve display aspect ratio (accounting for non-square pixel SAR and metadata).
  - Native 4-corner interactive resizing on both macOS (`NSWindow setAspectRatio:`, resizable borderless window) and Windows (`WM_SIZING` and `WM_NCHITTEST`).
  - Shortcuts `1`, `2`, `4` to jump to 100% original video size, 50% half size, and 25% quarter size.
- **libmpv Hardware Accelerated Playback**:
  - Renders directly into Qt 6 OpenGL surface (`QOpenGLWidget`) via the `mpv_render_context` OpenGL Render API.
  - Automatic hardware decoding (`VideoToolbox` on macOS, `D3D11VA` on Windows).
  - Audio-only mode with a clean, neutral slate graphic rather than a broken screen.
  - Multi-track support: Audio, Video, and Subtitle selection submenus in the context menu (automatically greyed out if no additional tracks exist).
- **Fluid Mouse Interactions**:
  - **Single Click**: Toggle Play / Pause anywhere on the video.
  - **Click and Drag**: Move the frameless window.
  - **Mouse Wheel**: Seek forwards/backwards 10s.
  - **Shift + Mouse Wheel**: Seek forwards/backwards 30s.
  - **Ctrl + Mouse Wheel**: Volume adjustment in 5% increments.
  - **Right-Click**: Minimalist context menu with track selectors, Always on Top, Fullscreen, and Settings.
  - **Hover**: Displays the OSD as soon as the mouse is hovering over the window and keeps it visible as long as the mouse is on top of the player.
- **Transparent Frosted OSD**:
  - Highly transparent, frosted glass styling with zero black background.
  - Filename displayed in the top-left corner in clean, small typography.
  - Interactive timeline seek bar with draggable handle marker.
  - Time display: `Current Time / Total Time` (e.g. `01:23 / 03:45`).
  - Clickable Time: Click the time label to toggle between elapsed time and remaining time (e.g. `-02:22 / 03:45`).
  - Semi-transparent circular controls (Seek -10s, Play/Pause, Seek +10s, Close).
  - Vertical volume slider with mute/unmute toggle.
  - Visual OSD feedback badge (`+10s`, `-10s`, `Volume 75%`, `Size: 100%`).
- **Fullscreen Mode**:
  - Full display occupancy with auto-hiding cursor after 2.5s inactivity.
  - Aspect ratio preserved on pure black background.
  - `Escape` or `F` exits fullscreen.
- **Centralized Format Registry & Associations**:
  - Single source of truth for all formats across GUI, file validation, and OS registration.
  - Categorized file associations dialog (Common Video, Other Video, Audio).
  - Native macOS UTI / Launch Services & Windows Registry integration.
- **JSON Configuration Persistence**:
  - Persists window geometry, volume, always-on-top, and format associations in standard OS app config directory.

---

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Space` / `Click` | Play / Pause |
| `1` | Original size (100% video resolution) |
| `2` | Half size (50% video resolution) |
| `4` | Quarter size (25% video resolution) |
| `Left Arrow` | Seek backward 10 seconds |
| `Right Arrow` | Seek forward 10 seconds |
| `Shift + Left` | Seek backward 30 seconds |
| `Shift + Right` | Seek forward 30 seconds |
| `Up Arrow` | Increase volume (+5%) |
| `Down Arrow` | Decrease volume (-5%) |
| `M` | Toggle mute / unmute |
| `F` | Toggle fullscreen |
| `Escape` | Exit fullscreen |
| `Q` | Quit application |

---

## Mouse Interactions

| Action | Result |
|---|---|
| `Scroll Up / Down` | Seek forward / backward 10s |
| `Shift + Scroll Up / Down` | Seek forward / backward 30s |
| `Ctrl + Scroll Up / Down` | Increase / decrease volume by 5% |
| `Double Click` | Toggle Play / Pause |
| `Right Click` | Open Context Menu |
| `Drag Window Area` | Move frameless window |
| `Drag 4 Corners / Edges` | Resize window constrained by aspect ratio |
| `Hover Window` | Reveal overlay controls (150ms fade) |
| `Drag and Drop File` | Open video or audio file immediately |

---

## Technology Stack

- **Language**: C++20
- **GUI Framework**: Qt 6 Widgets (`QtCore`, `QtGui`, `QtWidgets`, `QtOpenGLWidgets`, `QtSvg`)
- **Playback Engine**: `libmpv` (0.35+ / 0.42+)
- **Video Renderer**: `libmpv` Render API (`MPV_RENDER_API_TYPE_OPENGL`) rendering into Qt's `QOpenGLWidget` FBO
- **Build System**: CMake (3.25+)
- **Platform Integrations**:
  - macOS: Objective-C++ / Cocoa (`NSWindow`, AppKit, Launch Services, UTTypes)
  - Windows: Win32 API (`WM_SIZING`, `WM_NCHITTEST`, DWM, Registry)

---

## Building and Running

### Prerequisites

#### macOS
Install dependencies via Homebrew:
```bash
brew install cmake pkg-config mpv qtbase qtsvg
```

#### Windows
- Visual Studio 2022 (MSVC with C++20 support)
- Qt 6.5+ (MSVC 2022 64-bit)
- libmpv development build (DLL and headers)

### Compilation

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j8

# Run Unit Tests
ctest --test-dir build --output-on-failure
```

### Running

```bash
# macOS
./build/kvikplay.app/Contents/MacOS/kvikplay [optional_media_file]

# Or drag and drop any supported media file directly into the player window.
```

---

## Supported File Formats

Defined centrally in `src/formats/FormatRegistry.cpp`:

- **Common Video**: `.mp4`, `.m4v`, `.mkv`, `.avi`, `.mov`, `.webm`
- **Other Video**: `.ogv`, `.3gp`, `.3g2`, `.mts`, `.m2ts`, `.ts`, `.m2v`, `.vob`, `.mpg`, `.mpeg`, `.wmv`, `.asf`, `.flv`
- **Audio**: `.mp3`, `.wav`, `.flac`, `.aac`, `.m4a`, `.ogg`, `.opus`, `.wma`
