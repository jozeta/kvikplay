#include "MpvController.h"
#include <QDebug>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <clocale>
#include <cstring>

namespace kvikplay {

MpvController::MpvController(QObject* parent)
    : QObject(parent) {
}

MpvController::~MpvController() {
    if (m_mpv) {
        mpv_set_wakeup_callback(m_mpv, nullptr, nullptr);
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
}

bool MpvController::initialize() {
    if (m_mpv) {
        return true;
    }

    // libmpv strictly requires LC_NUMERIC to be "C"
    std::setlocale(LC_NUMERIC, "C");

    m_mpv = mpv_create();
    if (!m_mpv) {
        qWarning() << "Failed to create mpv context";
        return false;
    }

    // Configure options before mpv_initialize
    mpv_set_option_string(m_mpv, "vo", "libmpv,null");
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    mpv_set_option_string(m_mpv, "idle", "yes");
    mpv_set_option_string(m_mpv, "terminal", "no");
    mpv_set_option_string(m_mpv, "msg-level", "all=warn");
    mpv_set_option_string(m_mpv, "audio-client-name", "Kvikplay");
    mpv_set_option_string(m_mpv, "ytdl", "no");

    // Hardware decoding: try auto-safe (VideoToolbox on macOS, D3D11VA on Windows)
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");

    // Position subtitles cleanly above the bottom controls area
    mpv_set_option_string(m_mpv, "sub-margin-y", "70");

    if (mpv_initialize(m_mpv) < 0) {
        qWarning() << "Failed to initialize mpv context";
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        return false;
    }

    // Property observation
    mpv_observe_property(m_mpv, 1, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 2, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 3, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 4, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 5, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 6, "speed", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 7, "video-params", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, 8, "eof-reached", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 9, "track-list", MPV_FORMAT_NODE);

    mpv_set_wakeup_callback(m_mpv, onMpvWakeup, this);
    return true;
}

void MpvController::onMpvWakeup(void* ctx) {
    auto* self = static_cast<MpvController*>(ctx);
    QMetaObject::invokeMethod(self, &MpvController::processMpvEvents, Qt::QueuedConnection);
}

void MpvController::processMpvEvents() {
    if (!m_mpv) return;

    while (true) {
        mpv_event* event = mpv_wait_event(m_mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) {
            break;
        }
        handleMpvEvent(event);
    }
}

void MpvController::handleMpvEvent(mpv_event* event) {
    switch (event->event_id) {
    case MPV_EVENT_START_FILE:
        m_state = PlaybackState::Loading;
        emit playbackStateChanged(m_state);
        break;

    case MPV_EVENT_FILE_LOADED:
        m_state = m_isPaused ? PlaybackState::Paused : PlaybackState::Playing;
        updateVideoParams();
        updateTracks();
        emit playbackStateChanged(m_state);
        emit mediaLoaded(m_mediaInfo);
        break;

    case MPV_EVENT_END_FILE: {
        auto* end_data = static_cast<mpv_event_end_file*>(event->data);
        if (end_data) {
            if (end_data->reason == MPV_END_FILE_REASON_ERROR) {
                m_state = PlaybackState::Error;
                QString err = QString::fromUtf8(mpv_error_string(end_data->error));
                emit playbackStateChanged(m_state);
                emit errorOccurred(QStringLiteral("Playback error: %1").arg(err));
            } else if (end_data->reason == MPV_END_FILE_REASON_EOF) {
                m_state = PlaybackState::Ended;
                emit playbackStateChanged(m_state);
                emit playbackEnded();
            } else {
                m_state = PlaybackState::Stopped;
                emit playbackStateChanged(m_state);
            }
        }
        break;
    }

    case MPV_EVENT_PROPERTY_CHANGE: {
        auto* prop = static_cast<mpv_event_property*>(event->data);
        if (prop) {
            handlePropertyChange(prop);
        }
        break;
    }

    default:
        break;
    }
}

void MpvController::handlePropertyChange(mpv_event_property* prop) {
    if (!prop->name) return;
    std::string name(prop->name);

    if (name == "time-pos" && prop->format == MPV_FORMAT_DOUBLE) {
        m_position = *static_cast<double*>(prop->data);
        emit positionChanged(m_position);
    } else if (name == "duration" && prop->format == MPV_FORMAT_DOUBLE) {
        m_duration = *static_cast<double*>(prop->data);
        m_mediaInfo.duration = m_duration;
        emit durationChanged(m_duration);
    } else if (name == "pause" && prop->format == MPV_FORMAT_FLAG) {
        int paused = *static_cast<int*>(prop->data);
        m_isPaused = (paused != 0);
        if (m_state == PlaybackState::Playing || m_state == PlaybackState::Paused) {
            m_state = m_isPaused ? PlaybackState::Paused : PlaybackState::Playing;
            emit playbackStateChanged(m_state);
        }
    } else if (name == "volume" && prop->format == MPV_FORMAT_DOUBLE) {
        m_volume = static_cast<int>(std::round(*static_cast<double*>(prop->data)));
        emit volumeChanged(m_volume);
    } else if (name == "mute" && prop->format == MPV_FORMAT_FLAG) {
        m_isMuted = (*static_cast<int*>(prop->data) != 0);
        emit muteChanged(m_isMuted);
    } else if (name == "speed" && prop->format == MPV_FORMAT_DOUBLE) {
        m_speed = *static_cast<double*>(prop->data);
        emit speedChanged(m_speed);
    } else if (name == "video-params") {
        updateVideoParams();
    } else if (name == "track-list") {
        updateTracks();
    } else if (name == "eof-reached" && prop->format == MPV_FORMAT_FLAG) {
        int eof = *static_cast<int*>(prop->data);
        if (eof) {
            m_state = PlaybackState::Ended;
            emit playbackStateChanged(m_state);
            emit playbackEnded();
        }
    }
}

void MpvController::updateVideoParams() {
    if (!m_mpv) return;

    mpv_node node;
    if (mpv_get_property(m_mpv, "video-params", MPV_FORMAT_NODE, &node) >= 0) {
        if (node.format == MPV_FORMAT_NODE_MAP) {
            int w = 0, h = 0, dw = 0, dh = 0;
            double aspect = 0.0;

            for (int i = 0; i < node.u.list->num; ++i) {
                const char* key = node.u.list->keys[i];
                const mpv_node& val = node.u.list->values[i];
                if (std::strcmp(key, "w") == 0 && val.format == MPV_FORMAT_INT64) {
                    w = static_cast<int>(val.u.int64);
                } else if (std::strcmp(key, "h") == 0 && val.format == MPV_FORMAT_INT64) {
                    h = static_cast<int>(val.u.int64);
                } else if (std::strcmp(key, "dw") == 0 && val.format == MPV_FORMAT_INT64) {
                    dw = static_cast<int>(val.u.int64);
                } else if (std::strcmp(key, "dh") == 0 && val.format == MPV_FORMAT_INT64) {
                    dh = static_cast<int>(val.u.int64);
                } else if (std::strcmp(key, "aspect") == 0 && val.format == MPV_FORMAT_DOUBLE) {
                    aspect = val.u.double_;
                }
            }

            mpv_free_node_contents(&node);

            if (w > 0 && h > 0) {
                m_mediaInfo.width = w;
                m_mediaInfo.height = h;
                m_mediaInfo.displayWidth = (dw > 0) ? dw : w;
                m_mediaInfo.displayHeight = (dh > 0) ? dh : h;
                m_mediaInfo.aspectRatio = (aspect > 0.01) ? aspect : (static_cast<double>(m_mediaInfo.displayWidth) / m_mediaInfo.displayHeight);
                m_mediaInfo.isAudioOnly = false;
                emit videoParamsChanged(w, h, m_mediaInfo.displayWidth, m_mediaInfo.displayHeight, m_mediaInfo.aspectRatio);
                return;
            }
        } else {
            mpv_free_node_contents(&node);
        }
    }

    // If no video stream, mark as audio-only
    m_mediaInfo.width = 0;
    m_mediaInfo.height = 0;
    m_mediaInfo.displayWidth = 0;
    m_mediaInfo.displayHeight = 0;
    m_mediaInfo.aspectRatio = 16.0 / 9.0;
    m_mediaInfo.isAudioOnly = true;
    emit videoParamsChanged(0, 0, 0, 0, 16.0 / 9.0);
}

void MpvController::updateTracks() {
    if (!m_mpv) return;

    mpv_node node;
    if (mpv_get_property(m_mpv, "track-list", MPV_FORMAT_NODE, &node) >= 0) {
        if (node.format == MPV_FORMAT_NODE_ARRAY) {
            std::vector<TrackInfo> audio;
            std::vector<TrackInfo> video;
            std::vector<TrackInfo> sub;

            for (int i = 0; i < node.u.list->num; ++i) {
                const mpv_node& item = node.u.list->values[i];
                if (item.format == MPV_FORMAT_NODE_MAP) {
                    TrackInfo t;
                    for (int k = 0; k < item.u.list->num; ++k) {
                        const char* key = item.u.list->keys[k];
                        const mpv_node& val = item.u.list->values[k];
                        if (std::strcmp(key, "id") == 0 && val.format == MPV_FORMAT_INT64) {
                            t.id = static_cast<int>(val.u.int64);
                        } else if (std::strcmp(key, "type") == 0 && val.format == MPV_FORMAT_STRING) {
                            t.type = QString::fromUtf8(val.u.string);
                        } else if (std::strcmp(key, "title") == 0 && val.format == MPV_FORMAT_STRING) {
                            t.title = QString::fromUtf8(val.u.string);
                        } else if (std::strcmp(key, "lang") == 0 && val.format == MPV_FORMAT_STRING) {
                            t.lang = QString::fromUtf8(val.u.string);
                        } else if (std::strcmp(key, "codec") == 0 && val.format == MPV_FORMAT_STRING) {
                            t.codec = QString::fromUtf8(val.u.string);
                        } else if (std::strcmp(key, "selected") == 0 && val.format == MPV_FORMAT_FLAG) {
                            t.selected = (val.u.flag != 0);
                        }
                    }

                    if (t.type == QStringLiteral("audio")) {
                        audio.push_back(t);
                    } else if (t.type == QStringLiteral("video")) {
                        video.push_back(t);
                    } else if (t.type == QStringLiteral("sub")) {
                        sub.push_back(t);
                    }
                }
            }

            m_mediaInfo.audioTracks = std::move(audio);
            m_mediaInfo.videoTracks = std::move(video);
            m_mediaInfo.subtitleTracks = std::move(sub);
            emit tracksChanged(m_mediaInfo.audioTracks, m_mediaInfo.videoTracks, m_mediaInfo.subtitleTracks);
        }
        mpv_free_node_contents(&node);
    }
}

void MpvController::setAudioTrack(int id) {
    if (!m_mpv) return;
    QByteArray val = (id > 0) ? QByteArray::number(id) : QByteArray("no");
    mpv_set_property_string(m_mpv, "aid", val.constData());
}

void MpvController::setVideoTrack(int id) {
    if (!m_mpv) return;
    QByteArray val = (id > 0) ? QByteArray::number(id) : QByteArray("no");
    mpv_set_property_string(m_mpv, "vid", val.constData());
}

void MpvController::setSubtitleTrack(int id) {
    if (!m_mpv) return;
    QByteArray val = (id > 0) ? QByteArray::number(id) : QByteArray("no");
    mpv_set_property_string(m_mpv, "sid", val.constData());
}

void MpvController::loadFile(const QString& filePath) {
    if (!m_mpv) return;

    m_mediaInfo = MediaInfo{};
    m_mediaInfo.filePath = filePath;
    QFileInfo fi(filePath);
    m_mediaInfo.fileName = fi.fileName();
    m_mediaInfo.title = fi.completeBaseName();

    QByteArray pathBytes = filePath.toUtf8();
    const char* cmd[] = {"loadfile", pathBytes.constData(), nullptr};
    mpv_command_async(m_mpv, 0, cmd);

    // Auto-play immediately when opening a file
    int pauseFlag = 0;
    mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &pauseFlag);
}

void MpvController::play() {
    if (!m_mpv) return;
    int flag = 0;
    mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &flag);
}

void MpvController::pause() {
    if (!m_mpv) return;
    int flag = 1;
    mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &flag);
}

void MpvController::togglePlayPause() {
    if (!m_mpv) return;
    if (m_isPaused || m_state == PlaybackState::Ended) {
        if (m_state == PlaybackState::Ended) {
            seekAbsolute(0);
        }
        play();
    } else {
        pause();
    }
}

void MpvController::stop() {
    if (!m_mpv) return;
    const char* cmd[] = {"stop", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
    m_state = PlaybackState::Stopped;
    emit playbackStateChanged(m_state);
}

void MpvController::seekRelative(double seconds) {
    if (!m_mpv) return;
    QByteArray secStr = QByteArray::number(seconds, 'f', 2);
    const char* cmd[] = {"seek", secStr.constData(), "relative", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
}

void MpvController::seekAbsolute(double seconds) {
    if (!m_mpv) return;
    double clamped = std::clamp(seconds, 0.0, std::max(0.0, m_duration));
    QByteArray secStr = QByteArray::number(clamped, 'f', 2);
    const char* cmd[] = {"seek", secStr.constData(), "absolute", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
}

void MpvController::setVolume(int percent) {
    if (!m_mpv) return;
    int clamped = std::clamp(percent, 0, 100);
    if (m_volume != clamped) {
        m_volume = clamped;
        emit volumeChanged(m_volume);
    }
    double vol = static_cast<double>(clamped);
    mpv_set_property_async(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE, &vol);
}

void MpvController::adjustVolume(int deltaPercent) {
    setVolume(m_volume + deltaPercent);
}

void MpvController::setMute(bool mute) {
    if (!m_mpv) return;
    if (m_isMuted != mute) {
        m_isMuted = mute;
        emit muteChanged(m_isMuted);
    }
    int flag = mute ? 1 : 0;
    mpv_set_property_async(m_mpv, 0, "mute", MPV_FORMAT_FLAG, &flag);
}

void MpvController::toggleMute() {
    setMute(!m_isMuted);
}

void MpvController::setSpeed(double speed) {
    if (!m_mpv) return;
    double clamped = std::clamp(speed, 0.25, 4.0);
    mpv_set_property_async(m_mpv, 0, "speed", MPV_FORMAT_DOUBLE, &clamped);
}

} // namespace kvikplay
