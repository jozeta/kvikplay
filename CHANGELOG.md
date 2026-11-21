# Changelog

All notable changes to Kvikplay. Newest first. Versions follow [semantic versioning](https://semver.org/).

## Unreleased

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
