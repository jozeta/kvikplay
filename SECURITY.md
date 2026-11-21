# Security

Kvikplay is a private repository. Report a security problem to the maintainer directly rather than
in an issue.

## Threat model

### Assets

- The user's machine: Kvikplay runs with the user's full rights and decodes complex binary formats
  (FFmpeg through libmpv), so a decoder exploit runs as the user.
- The user's privacy: `config.json` holds the full paths of recently played files and per-file
  resume positions.
- The user's file associations (Windows registry under `HKCU\Software\Classes`, macOS Launch
  Services).

### Untrusted input

- Media files and everything inside them: streams, codec data, track titles, chapter titles and
  other metadata.
- File names and folder contents (folder navigation, drag and drop, `sub-auto=fuzzy` loading of
  subtitle files next to the video).
- External subtitle files (`.srt`, `.ass`, `.vtt`, …), including ASS styling and embedded fonts.
- Paths from the command line, macOS `FileOpen` events, drag and drop and the recent-files list.
- `config.json`, which any process running as the user can edit.

### Trust boundaries

- File → libmpv/FFmpeg demuxers and decoders: the main boundary. Kvikplay does not parse media
  itself; it relies on libmpv and keeps libmpv's user config, user scripts and network fetching
  (`ytdl`) off.
- Media metadata → Qt UI: titles and file names cross into Qt widgets. They must be shown as plain
  text, never as Qt rich text (HTML).
- `config.json` → app: values are range-checked on load; a corrupt file falls back to defaults.

### Top risks

1. **Decoder vulnerability in a crafted media file.** Mitigation: keep libmpv/FFmpeg current in
   the bundled app; hardware decoding narrows the software decoder surface for common codecs.
2. **Rich-text injection through a file name or metadata.** A name such as `<img src=…>.mkv` can be
   interpreted as HTML by a `QLabel` in `Qt::AutoText` mode. Mitigation (planned, not yet in
   place as of 2026-11-21): set labels that show untrusted text to `Qt::PlainText`.
3. **A media or playlist file that makes libmpv reach out to the network or read other files**
   (playlists, EDL, reference files). Today `ytdl` is off for the main player and libmpv refuses
   unsafe playlist entries by default (`load-unsafe-playlists=no`). Planned (not yet in place):
   open only `FormatRegistry` formats on every path (the file dialog's "All Files" and recent
   files currently bypass it) and set `access-references=no`.
4. **File association changes.** Registration writes only per-user keys (`HKCU`) and only for the
   extensions the user ticks.
