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
- **libmpv High-Performance & HDR Rendering Pipeline**:
  - Direct3D 11 hardware decoding on Windows (`d3d11va`) and `VideoToolbox` on macOS.
  - Core Profile 3.3 zero-overhead rendering surface without depth/stencil buffers.
  - High-precision HDR Tone-Mapping & Pass-Through (`target-colorspace-hint`, `hdr-compute-peak`, `tone-mapping=auto`).
  - Audio pitch correction, low-latency seek hacks, and 128 MiB in-memory cache.
- **Playback Intelligence & Navigation**:
  - **Resume Playback Position**: Automatically remembers playback progress per file and resumes with an OSD notification.
  - **Smart Folder Navigation**: Natural alphanumeric folder traversal (`Episode 1` -> `Episode 2` -> `Episode 10`) via `]` / `PageDown` and `[` / `PageUp`.
  - **Auto-Advance on EOF**: Automatically loads and plays the next episode or file in the folder when current media ends.
  - **Chapter Markers & Jumping**: Embedded chapters render as subtle tick marks on the timeline groove; jump between chapters using `Shift + ]` and `Shift + [`.
  - **Hold-to-Speed (2.0x Boost)**: Hold `C` or hold the Fast-Forward button to temporarily boost playback to 2.0x with pitch correction, instantly resuming 1.0x on release.
  - **External Subtitles**: Automatic fuzzy discovery for subtitles in the same directory, plus instant drag-and-drop support for `.srt`, `.vtt`, `.ass`, `.sub`, `.idx`.
  - **Subtitle Timing Sync**: Adjust subtitle delay on the fly using `Z` (-100ms) and `X` (+100ms).
- **Picture-in-Picture (PiP) Snap**:
  - Shortcut `P` instantly docks and scales the window into an always-on-top mini player snapped to the bottom-right corner.
- **Stream & Codec Inspector**:
  - Shortcut `I` toggles a high-tech translucent HUD displaying video codec, resolution, display FPS, bitrate, hardware decoder, dropped frames, and audio layout.
- **Built-in Help & Shortcuts Guide**:
  - Access "Help & Shortcuts..." from the right-click menu anytime to open an in-app visual guide and reference modal.
- **Audio Enhancements**:
  - **Quiet Dialogue Boost / Dynaudnorm**: Toggle dynamic audio normalization (`N` key or context menu) to boost quiet speech and soften loud explosions without clipping.
- **Instant Clean Frame Capture**:
  - Shortcut `S` captures a pixel-perfect, clean video frame directly to Desktop and copies it to the system clipboard.
  - Shortcut `Shift+S` captures the frame with rendered subtitles included.
- **Interactive Timeline Hover Thumbnail Preview**:
  - Hovering or dragging over the timeline displays a floating rounded preview card with real-time timestamp and decoded video keyframe thumbnail.
- **Recent Files**:
  - Context menu submenu with recent files list (up to 8 items, deduplicated) and clear action.
- **Fluid Mouse & Trackpad Interactions**:
  - **Right Click / Trackpad Two-Finger Click**: Quick tap/click toggles Play / Pause anywhere on the video (when window is focused).
  - **Right Click / Trackpad Two-Finger Hold (> 0.5s)**: Holding for longer than 0.5s opens the minimalist context menu.
  - **Click on Unfocused Window**: Brings the window into focus without accidentally pausing/playing.
  - **Left Click and Drag**: Moves the frameless window smoothly across desktops.
  - **Double Left Click**: Toggles between fullscreen and windowed mode, remembering the windowed dimensions.
  - **Trackpad Two-Finger Scrubbing**: Drag right to scrub forward, drag left to scrub backward (option in context menu to reverse direction).
  - **Generous Resize Zones**: 32px corner zones and 16px border zones make resizing effortless.
  - **Mouse Wheel**: Seek forwards/backwards 10s.
  - **Shift + Mouse Wheel**: Seek forwards/backwards 30s.
  - **Ctrl + Mouse Wheel**: Volume adjustment in 5% increments.
  - **Context Menu**: Minimalist context menu with track selectors, reverse trackpad scrubbing, Always on Top, Fullscreen, and Settings.
  - **Hover & Inactivity**: Displays the OSD as soon as the mouse hovers over the window; hides OSD after 5s of mouse inactivity and restores it instantly on movement.
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
| `Space` | Play / Pause |
| `C` (Hold) | 2.0x Quick Speed Boost (release to restore 1.0x) |
| `1` | Original size (100% video resolution) |
| `2` | Half size (50% video resolution) |
| `4` | Quarter size (25% video resolution) |
| `Left Arrow` | Seek backward 10 seconds |
| `Right Arrow` | Seek forward 10 seconds |
| `Shift + Left` | Seek backward 30 seconds |
| `Shift + Right` | Seek forward 30 seconds |
| `Up Arrow` | Increase volume (+5%) |
| `Down Arrow` | Decrease volume (-5%) |
| `]` / `PageDown` | Play next media file in folder |
| `[` / `PageUp` | Play previous media file in folder |
| `Shift + ]` | Next chapter |
| `Shift + [` | Previous chapter |
| `Z` | Subtitle delay -100ms |
| `X` | Subtitle delay +100ms |
| `P` | Toggle Picture-in-Picture mode |
| `I` | Toggle Stream & Codec Inspector HUD |
| `S` | Instant clean screenshot (copied to clipboard & saved to Desktop) |
| `Shift + S` | Screenshot with subtitles (copied to clipboard & saved to Desktop) |
| `N` | Toggle Quiet Dialogue Boost (dynaudnorm) |
| `M` | Toggle mute / unmute |
| `F` | Toggle fullscreen |
| `Escape` | Exit fullscreen |
| `Q` | Quit application |

---

## Mouse & Trackpad Interactions

| Action | Result |
|---|---|
| `Right Click / Trackpad Two-Finger Click` | Toggle Play / Pause (when focused) |
| `Right Click / Two-Finger Hold (> 0.5s)` | Open Context Menu |
| `Double Left Click` | Toggle Fullscreen / Windowed (restores windowed size) |
| `Left Click + Drag` | Move frameless window |
| `Drag 4 Corners / Edges` | Resize window (32px corner zones, 16px edge zones) |
| `Two-Finger Trackpad Scrub` | Drag right/left to seek forward/backward (reversable) |
| `Scroll Up / Down` | Seek forward / backward 10s |
| `Shift + Scroll Up / Down` | Seek forward / backward 30s |
| `Ctrl + Scroll Up / Down` | Increase / decrease volume by 5% |
| `Hover Window` | Reveal overlay controls (auto-hides after 5s stillness) |
| `Drag and Drop File` | Open video or audio file and start playback immediately |

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
