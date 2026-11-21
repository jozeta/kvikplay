# Changelog

All notable changes to Kvikplay. Newest first. Versions follow [semantic versioning](https://semver.org/).

## Unreleased

### Fixed
- **Windows: black picture with sound.** Kvikplay asked mpv for the video output list
  `libmpv,null`; mpv probes list entries, and outside macOS its `libmpv` output refuses to be
  probed, so it silently used `null`. It now asks for exactly `libmpv` (regression test in
  `test_playback`). This affected every Windows build so far.

### Added
- Diagnostics log `kvikplay.log` next to `config.json`: mpv's warnings and video-path messages,
  Qt warnings and Kvikplay's own steps, replaced at every start, capped at 1 MB, never sent
  anywhere. The media inspector shows its path.
- Media inspector: "Video Output" (what mpv draws with) and "Renderer" (OpenGL version and GPU,
  and whether mpv attached to it).
- Tests: the bundled libmpv must support the OpenGL render API; the diagnostics log.

## 1.2.2 — 2026-11-21

### Fixed
- Timeline thumbnails looked washed out: mpv's `bgr0` pixels have 0 in the alpha byte, which
  Qt's `Format_RGB32` requires to be 0xFF. Thumbnails are now made fully opaque (tested).
- Windows now builds: SDK's DWM corner types, `shlobj.h`, an absolute icon path in the resource
  file, and the right libraries for `test_platform`. All tests pass on Windows in CI.

### Added
- Windows release: a standalone `Kvikplay-<version>-Windows-x64.zip` (Qt, libmpv/FFmpeg and the
  licence texts of every bundled MSYS2 package), built and tested by CI. Not code-signed yet,
  so on Windows the update check opens the download page.

## 1.2.1 — 2026-11-21

### Fixed
- **macOS crash (1.1.0 and 1.2.0):** the app aborted when the pointer left its window, for
  example when the "Update available" dialog opened. The hover tracker that shows the controls
  while Kvikplay is in the background had no handler for "mouse exited". Regression test:
  `test_hover_tracking`. Kvikplay 1.1.0 can't install this update itself (it crashes at the
  dialog): download 1.2.1 from the release page.
- The media inspector opened over the title; it now sits below it, on the overlay's margin.
- A file without a metadata title showed its name twice ("movie" over "movie.mkv"); the second
  line now only appears when it adds something.

## 1.2.0 — 2026-11-21

### Changed
- New "cinematic" on-screen controls: no boxes, soft top and bottom shading, title top left,
  full-width timeline with chapter ticks and times at both ends (remaining time by default),
  white glyph buttons (play, ±10 s, volume, subtitles, fullscreen). In small windows only the
  essentials stay.
- Centre messages: volume as a ring, others as a dark capsule.
- Hover preview: rounded frame with a white border and a "time · chapter" caption.
- The subtitles button opens the subtitle choices (tracks, add file, timing) above it.
- The title shown is the file's metadata title when there is one.

### Fixed
- The "10" on the skip buttons is drawn as lines, so no font lookup at start.

### Tests
- `test_time_format` (clock and remaining-time text) and `test_overlay_render`, which renders
  the overlay offscreen and checks the scrims, times, glyphs, volume ring and preview.

## 1.1.0 — 2026-11-21

### Added
- Update check at start: when GitHub has a newer release, Kvikplay offers to download and install
  it (SHA-256 checked; on macOS also signature, Team ID and Gatekeeper checked) and restarts. Can
  be turned off in About, which also has "Check for Updates…". On Windows it opens the download
  page instead of installing (no signed Windows build to verify yet).
- MIT LICENSE and THIRD_PARTY_NOTICES.md; the app ships the licence texts of every bundled library.
- Right-click menu: **Opening Files from Finder/Explorer** → Replace Current Video (default) or
  Open in New Window. Drag-and-drop and Open File always replace the current video.
- Right-click menu: **Start in Fullscreen**.
- Windows: one player per session; opening a file from Explorer hands it to the running window
  (or opens a new window, per the setting above).
- Windows: media flyout, lock screen and media keys (System Media Transport Controls), matching
  macOS Now Playing.
- Version in the About box and `kvikplay --version`; one version number from CMake.
- `KVIKPLAY_CONFIG_DIR` to use another settings folder.
- macOS release script: Developer ID signing with Hardened Runtime, notarization, stapling
  (`scripts/sign_notarize_macos.sh`). The standalone bundle no longer keeps library search
  paths that point outside the app (`/opt/homebrew/lib`).
- CI on macOS and Windows (build, tests, ShellCheck, clang-tidy) and security scanning.
- Screen-reader names and tooltips on the overlay controls.

### Changed
- Right-click menu redesigned and grouped by task (playback, file, what's playing, window,
  tools, settings, help). Labels show the current choice ("Speed: 1.25×", "Subtitles: Off"),
  items that don't apply are hidden, shortcuts are shown, and with no video open the menu is
  short. New in the menu: Speed, Chapters, Next/Previous in Folder, Window Size, Add Subtitle
  File… and subtitle timing. Preferences moved into a Settings submenu.

### Fixed
- Black window at launch: a file opened before the video surface existed made mpv use its
  "null" video output. Files now open once the surface is ready.
- Replaying after the end left the player stuck in "ended" (Space restarted instead of pausing).
- Screenshots: reported only once saved, failures shown, clipboard copy works; no overwrite
  within the same second.
- Timeline thumbnails: real frames (no blank or stale images), correct shape for vertical video
  after a landscape one, none for audio files.
- Timeline preview card and media inspector now have their dark background.
- Minimum window size no longer grows from earlier videos; the window reopens on the monitor it
  was on; Always on Top no longer loses the rounded corners.
- Holding `C` while switching apps no longer leaves 2× speed on; tapping ⏩ no longer flashes the
  speed boost.
- Missing recent files are removed from the list; resume positions drop the oldest entry, not
  the alphabetically first; settings are written atomically.
- "Help & Shortcuts" label; Qt painter warnings from nested fade effects; speed set with
  `-`/`=`/`Backspace` is remembered.
- Windows: build error (`windowsx.h`), resize borders at display scaling above 100 %, minimum size
  while resizing, white frame strip, hardware decoding that fell back to software, console window.
- Tests could not fail in Release builds; one real failure (`setSpeed`) was hidden.

### Security
- File names, track and chapter titles are shown as plain text (no Qt rich text from media).
- Only supported formats are opened on every path; libmpv runs without user config or scripts,
  without youtube-dl and without following references inside files (a playlist renamed to `.mp4`
  can no longer pull in other files).
- macOS: the window activation observer is removed on shutdown.
- Unticking a format in File Associations removes Kvikplay's registration (Windows).

### Performance
- libmpv starts without its built-in Lua scripts: 7 threads instead of 13 and about 3 MB less
  per instance (two instances run).
- Thumbnails decode with hardware copy-back on a low-priority thread, and only after the first
  hover.
- Next/previous file uses a cached, background-prefetched folder index.
- Fewer per-frame and per-event costs (property ids, cached icons, one cursor update per move);
  Windows window is opaque (no layered-window composition).

### Documentation
- README rewritten to the project standard: installation, every setting with its default,
  how-tos, version, "What's new", a macOS/Windows feature table, and screenshots (playback, menu,
  inspector) taken with Big Buck Bunny.
- Added CLAUDE.md, SECURITY.md (threat model), CHANGELOG.md and `.claude-standards`
  (own design, not part of the Låda Glass family).
- `.gitignore` brought up to the current baseline.

## 1.0.0 — 2026-11-19

First complete version.

### Added
- Frameless, rounded window that locks to the video's display aspect ratio, with native resizing
  on macOS and Windows and window sizes clamped to the visible screen area.
- libmpv playback with VideoToolbox (macOS) / D3D11VA (Windows) hardware decoding and HDR tone
  mapping.
- Transparent overlay: timeline with chapter ticks and hover thumbnails, elapsed/remaining time,
  play/pause and ±10 s buttons, volume slider, on-screen messages.
- Resume position per file, natural-order folder navigation and auto-advance at the end of a file.
- Hold-to-speed (2×), speed steps, frame stepping, subtitle delay, dialogue boost (dynaudnorm).
- Track selection, external subtitle drag and drop, screenshots to Desktop and clipboard.
- Picture-in-picture, media inspector, help dialog, recent files, file associations dialog.
- macOS Now Playing and media keys; display-sleep inhibit while playing (macOS and Windows).
- Standalone packaging on build (`macdeployqt` / `windeployqt`) and a custom app icon.

### Fixed
- Memory and signal leaks, animation CPU usage, thumbnail distortion.
