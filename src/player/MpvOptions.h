#pragma once

#include <mpv/client.h>

namespace kvikplay {

// Options shared by every libmpv instance Kvikplay creates (player and thumbnails).
// Call before mpv_initialize(). Unknown options (older libmpv) are ignored on purpose.
inline void applyBaseMpvOptions(mpv_handle* mpv) {
    // Hardening: no user mpv config or scripts, no youtube-dl network lookups, and no
    // media files that pull in other files by reference (MOV refs, EDL-like tricks).
    mpv_set_option_string(mpv, "config", "no");
    mpv_set_option_string(mpv, "load-scripts", "no");
    mpv_set_option_string(mpv, "ytdl", "no");
    mpv_set_option_string(mpv, "access-references", "no");
    mpv_set_option_string(mpv, "load-unsafe-playlists", "no");

    // Speed: skip mpv's built-in Lua scripts (each one costs a Lua VM at startup).
    // Kvikplay draws its own UI, so none of them is used.
    mpv_set_option_string(mpv, "osc", "no");
    for (const char* opt : {"load-stats-overlay", "load-console", "load-select", "load-commands",
                            "load-context-menu", "load-positioning", "load-auto-profiles"}) {
        mpv_set_option_string(mpv, opt, "no");
    }

    mpv_set_option_string(mpv, "terminal", "no");
    mpv_set_option_string(mpv, "input-default-bindings", "no");
    mpv_set_option_string(mpv, "input-vo-keyboard", "no");
}

} // namespace kvikplay
