# Kvikplay

> An extremely lightweight, fast, minimalistic cross-platform video player for macOS and Windows.

Kvikplay provides the core playback power of a traditional media player (powered by **libmpv**), built around a philosophy of Nordic minimalism, pure speed, and an unobtrusive desktop presence.

During normal playback, there is practically no interface chrome—no traditional toolbars, no title bar, no permanent playback controls, no window borders, and no playlists. The video occupies 100% of the frameless window, perfectly locked to its native display aspect ratio.

---

## Key Features

- **Frameless Nordic Minimalist Design**:
  - No title bar, no window decorations, no standard buttons.
  - Video fills the entire window surface without letterboxing, pillarboxing, or distortion.
- **Strict Aspect-Ratio Locking**:
  - Automatically constrains window geometry to preserve display aspect ratio (accounting for non-square pixel SAR and metadata).
  - Native 4-corner interactive resizing on both macOS (`NSWindow setAspectRatio:`, resizable borderless window) and Windows (`WM_SIZING` and `WM_NCHITTEST`).
- **libmpv Hardware Accelerated Playback**:
  - Renders directly into Qt 6 OpenGL surface (`QOpenGLWidget`) via the `mpv_render_context` OpenGL Render API.
  - Automatic hardware decoding (`VideoToolbox` on macOS, `D3D11VA` on Windows).
  - Audio-only mode with a clean, neutral slate graphic rather than a broken screen.
- **Fluid Mouse Interactions**:
  - **Mouse Wheel**: Seek forwards/backwards 10s.
  - **Shift + Mouse Wheel**: Seek forwards/backwards 30s.
  - **Ctrl + Mouse Wheel**: Volume adjustment in 5% increments.
  - **Double-Click**: Toggle Play/Pause.
  - **Right-Click**: Minimalist context menu (Play/Pause, Always on Top, Fullscreen, File Associations, About, Close).
  - **Window Move**: Click and drag video area outside controls to smoothly move window.
- **Lightweight Playback Overlay**:
  - Semi-transparent white circular controls (Seek -10s, Play/Pause, Seek +10s, Close button).
  - Vertical volume slider with mute/unmute toggle.
  - 150ms fade animation on hover; auto-hides when mouse leaves or stops moving.
  - Visual OSD feedback badge (`+10s`, `-10s`, `Volume 75%`, `Muted`).
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
| `Space` | Play / Pause |
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
