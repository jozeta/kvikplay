#pragma once

#include <QString>

namespace kvikplay {

enum class PlaybackState {
    Idle,
    Loading,
    Playing,
    Paused,
    Stopped,
    Ended,
    Error
};

inline const char* playbackStateToString(PlaybackState state) {
    switch (state) {
    case PlaybackState::Idle:    return "Idle";
    case PlaybackState::Loading: return "Loading";
    case PlaybackState::Playing: return "Playing";
    case PlaybackState::Paused:  return "Paused";
    case PlaybackState::Stopped: return "Stopped";
    case PlaybackState::Ended:   return "Ended";
    case PlaybackState::Error:   return "Error";
    }
    return "Unknown";
}

} // namespace kvikplay
