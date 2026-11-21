#include "MpvController.h"
#include "MpvOptions.h"
#include "app/DiagnosticsLog.h"
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <algorithm>
#include <cmath>
#include <clocale>
#include <cstring>
#include <string_view>

namespace kvikplay {

namespace {
// reply_userdata ids for observed properties; dispatched with a switch instead of
// string compares (time-pos arrives once per video frame).
enum PropId : uint64_t {
    PropTimePos = 1, PropDuration, PropPause, PropVolume, PropMute, PropSpeed,
    PropVideoParams, PropEofReached, PropTrackList, PropSubDelay, PropChapter, PropChapterList
};
} // namespace

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

bool MpvController::initialize(const char* videoOutput) {
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
    applyBaseMpvOptions(m_mpv);
    mpv_set_option_string(m_mpv, "vo", videoOutput);
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    mpv_set_option_string(m_mpv, "idle", "yes");
    mpv_set_option_string(m_mpv, "msg-level", "all=warn");
    mpv_set_option_string(m_mpv, "audio-client-name", "Kvikplay");

    // Super fast seeking & low latency
    mpv_set_option_string(m_mpv, "hr-seek", "default");
    mpv_set_option_string(m_mpv, "hr-seek-framedrop", "yes");
    mpv_set_option_string(m_mpv, "video-latency-hacks", "yes");

    // Fast in-memory cache for seamless jumping back/forth
    mpv_set_option_string(m_mpv, "demuxer-readahead-secs", "15");
    mpv_set_option_string(m_mpv, "demuxer-max-bytes", "134217728"); // 128 MiB
    mpv_set_option_string(m_mpv, "demuxer-lavf-o", "probesize=500000,analyzeduration=500000");

    // Audio pitch correction
    mpv_set_option_string(m_mpv, "audio-pitch-correction", "yes");

    // Hardware decoding & rendering optimization:
    // VideoToolbox on macOS (zero-copy into OpenGL). On Windows the renderer is
    // OpenGL, where plain d3d11va has no zero-copy interop and silently falls back
    // to software decoding; auto-safe picks a working method (interop or -copy).
#if defined(Q_OS_MACOS)
    mpv_set_option_string(m_mpv, "hwdec", "videotoolbox");
    mpv_set_option_string(m_mpv, "videotoolbox-format", "auto");
    mpv_set_option_string(m_mpv, "opengl-pbo", "yes");
#elif defined(Q_OS_WIN)
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");
#else
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");
#endif
    mpv_set_option_string(m_mpv, "hwdec-codecs", "all");
    mpv_set_option_string(m_mpv, "video-sync", "audio");

    // HDR Tone-Mapping, Wide Gamut & Color Accuracy
    mpv_set_option_string(m_mpv, "target-colorspace-hint", "yes");
    mpv_set_option_string(m_mpv, "tone-mapping", "auto");
    mpv_set_option_string(m_mpv, "gamut-clipping", "yes");
    mpv_set_option_string(m_mpv, "hdr-compute-peak", "yes");

    // Subtitle auto-loading matching files in same folder & auto character encoding detection
    mpv_set_option_string(m_mpv, "sub-auto", "fuzzy");
    mpv_set_option_string(m_mpv, "sub-codepage", "auto");

    // Position subtitles cleanly above the bottom controls area
    mpv_set_option_string(m_mpv, "sub-margin-y", "70");

    if (mpv_initialize(m_mpv) < 0) {
        qWarning() << "Failed to initialize mpv context";
        mpv_destroy(m_mpv);
        m_mpv = nullptr;
        return false;
    }

    // Property observation
    mpv_observe_property(m_mpv, PropTimePos, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, PropDuration, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, PropPause, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, PropVolume, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, PropMute, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, PropSpeed, "speed", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, PropVideoParams, "video-params", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, PropEofReached, "eof-reached", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, PropTrackList, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, PropSubDelay, "sub-delay", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, PropChapter, "chapter", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpv, PropChapterList, "chapter-list", MPV_FORMAT_NODE);

    // Verbose, so the diagnostics log can show why a video output was skipped while
    // mpv probes them (that reason is only printed at verbose level). Filtered below.
    mpv_request_log_messages(m_mpv, "v");

    mpv_set_wakeup_callback(m_mpv, onMpvWakeup, this);
    return true;
}

void MpvController::onMpvWakeup(void* ctx) {
    auto* self = static_cast<MpvController*>(ctx);
    if (!self->m_wakeupPending.exchange(true)) {
        QMetaObject::invokeMethod(self, &MpvController::processMpvEvents, Qt::QueuedConnection);
    }
}

void MpvController::processMpvEvents() {
    m_wakeupPending.store(false);
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

    case MPV_EVENT_FILE_LOADED: {
        // The title from the file's metadata (falls back to the file name in mpv).
        // Untrusted text: the overlay shows it as plain text.
        if (char* title = mpv_get_property_string(m_mpv, "media-title")) {
            const QString t = QString::fromUtf8(title).trimmed();
            if (!t.isEmpty() && t != m_mediaInfo.fileName) m_mediaInfo.title = t.left(200);
            mpv_free(title);
        }
        m_state = m_isPaused ? PlaybackState::Paused : PlaybackState::Playing;
        if (char* vo = mpv_get_property_string(m_mpv, "current-vo")) {
            DiagnosticsLog::write("app", QStringLiteral("file loaded; video output: %1").arg(QString::fromUtf8(vo)));
            mpv_free(vo);
        }
        updateVideoParams();
        updateTracks();
        updateChapters();
        emit playbackStateChanged(m_state);
        emit mediaLoaded(m_mediaInfo);
        break;
    }

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
            handlePropertyChange(event->reply_userdata, prop);
        }
        break;
    }

    case MPV_EVENT_LOG_MESSAGE: {
        auto* msg = static_cast<mpv_event_log_message*>(event->data);
        if (!msg) break;
        const std::string_view prefix(msg->prefix ? msg->prefix : "");
        const bool important = msg->log_level <= MPV_LOG_LEVEL_WARN;
        const bool videoPath = prefix == "vo" || prefix.starts_with("vo/") || prefix == "libmpv_render" ||
                               prefix == "vd" || prefix.starts_with("hwdec") || prefix == "cplayer";
        if (important || (msg->log_level <= MPV_LOG_LEVEL_V && videoPath)) {
            DiagnosticsLog::write("mpv", QStringLiteral("%1 %2: %3").arg(
                QString::fromUtf8(msg->level), QString::fromUtf8(msg->prefix), QString::fromUtf8(msg->text)));
        }
        break;
    }

    case MPV_EVENT_COMMAND_REPLY:
        handleCommandReply(event->reply_userdata, event->error);
        break;

    default:
        break;
    }
}

void MpvController::handlePropertyChange(uint64_t id, mpv_event_property* prop) {
    const bool isDouble = prop->format == MPV_FORMAT_DOUBLE;
    const bool isFlag = prop->format == MPV_FORMAT_FLAG;

    switch (id) {
    case PropTimePos:
        if (isDouble) {
            m_position = *static_cast<double*>(prop->data);
            emit positionChanged(m_position);
        }
        break;
    case PropDuration:
        if (isDouble) {
            m_duration = *static_cast<double*>(prop->data);
            m_mediaInfo.duration = m_duration;
            emit durationChanged(m_duration);
        }
        break;
    case PropPause:
        if (isFlag) {
            m_isPaused = (*static_cast<int*>(prop->data) != 0);
            if (m_state == PlaybackState::Playing || m_state == PlaybackState::Paused) {
                m_state = m_isPaused ? PlaybackState::Paused : PlaybackState::Playing;
                emit playbackStateChanged(m_state);
            }
        }
        break;
    case PropVolume:
        if (isDouble) {
            m_volume = static_cast<int>(std::round(*static_cast<double*>(prop->data)));
            emit volumeChanged(m_volume);
        }
        break;
    case PropMute:
        if (isFlag) {
            m_isMuted = (*static_cast<int*>(prop->data) != 0);
            emit muteChanged(m_isMuted);
        }
        break;
    case PropSpeed:
        if (isDouble) {
            const double speed = *static_cast<double*>(prop->data);
            if (speed != m_speed) {
                m_speed = speed;
                emit speedChanged(m_speed);
            }
        }
        break;
    case PropVideoParams:
        updateVideoParams();
        break;
    case PropTrackList:
        updateTracks();
        break;
    case PropSubDelay:
        if (isDouble) {
            m_subtitleDelay = *static_cast<double*>(prop->data);
            emit subtitleDelayChanged(m_subtitleDelay);
        }
        break;
    case PropChapter:
        if (prop->format == MPV_FORMAT_INT64) {
            m_currentChapter = static_cast<int>(*static_cast<int64_t*>(prop->data));
            emit chapterChanged(m_currentChapter, currentChapterTitle());
        }
        break;
    case PropChapterList:
        updateChapters();
        break;
    case PropEofReached:
        if (isFlag) {
            if (*static_cast<int*>(prop->data) != 0) {
                m_state = PlaybackState::Ended;
                emit playbackStateChanged(m_state);
                emit playbackEnded();
            } else if (m_state == PlaybackState::Ended) {
                // Left the end (replay or seek back): playing or paused again.
                m_state = m_isPaused ? PlaybackState::Paused : PlaybackState::Playing;
                emit playbackStateChanged(m_state);
            }
        }
        break;
    default:
        break;
    }
}

uint64_t MpvController::registerPending(PendingKind kind, const QString& path) {
    const uint64_t id = m_nextReplyId++;
    m_pending.emplace(id, PendingCommand{kind, path});
    return id;
}

void MpvController::handleCommandReply(uint64_t id, int error) {
    auto it = m_pending.find(id);
    if (it == m_pending.end()) return;
    const PendingCommand cmd = it->second;
    m_pending.erase(it);

    const QString fileName = QFileInfo(cmd.path).fileName();
    switch (cmd.kind) {
    case PendingKind::Screenshot:
        if (error < 0) {
            emit errorOccurred(QStringLiteral("Screenshot failed: %1").arg(QString::fromUtf8(mpv_error_string(error))));
        } else {
            emit screenshotTaken(cmd.path);
        }
        break;
    case PendingKind::SubtitleAdd:
        if (error < 0) {
            emit errorOccurred(QStringLiteral("Could not load subtitles: %1").arg(fileName));
        } else {
            emit subtitleAdded(fileName);
        }
        break;
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
            m_staticStatsCached = false;
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
    m_staticStatsCached = false;
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

void MpvController::seekAbsolute(double seconds, bool exact) {
    if (!m_mpv) return;
    double clamped = std::clamp(seconds, 0.0, std::max(0.0, m_duration));
    QByteArray secStr = QByteArray::number(clamped, 'f', 2);
    const char* mode = exact ? "absolute+exact" : "absolute+keyframes";
    const char* cmd[] = {"seek", secStr.constData(), mode, nullptr};
    mpv_command_async(m_mpv, 0, cmd);
}

void MpvController::stepFrame() {
    if (!m_mpv) return;
    const char* cmd[] = {"frame-step", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
}

void MpvController::stepFrameBack() {
    if (!m_mpv) return;
    const char* cmd[] = {"frame-back-step", nullptr};
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
    // Update the cached value now (like volume) so callers read what they just set;
    // mpv's property observer confirms it asynchronously.
    if (m_speed != clamped) {
        m_speed = clamped;
        emit speedChanged(m_speed);
    }
    mpv_set_property_async(m_mpv, 0, "speed", MPV_FORMAT_DOUBLE, &clamped);
}

void MpvController::takeScreenshot(bool includeSubtitles) {
    if (!m_mpv) return;
    QString dir = m_screenshotDir;
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }
    if (dir.isEmpty()) {
        dir = QDir::homePath();
    }
    // Milliseconds in the name: two screenshots in the same second must not overwrite.
    QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss_zzz"));
    QString filePath = QStringLiteral("%1/Kvikplay_%2.png").arg(dir, timeStr);
    QByteArray filePathUtf8 = filePath.toUtf8();
    const char* mode = includeSubtitles ? "subtitles" : "video";
    const char* cmd[] = {"screenshot-to-file", filePathUtf8.constData(), mode, nullptr};
    // Reported from handleCommandReply() once mpv has actually written the file.
    mpv_command_async(m_mpv, registerPending(PendingKind::Screenshot, filePath), cmd);
}

void MpvController::setDialogueBoost(bool enabled) {
    if (!m_mpv) return;
    if (enabled) {
        mpv_set_property_string(m_mpv, "af", "dynaudnorm=f=75:g=25:p=0.55");
    } else {
        mpv_set_property_string(m_mpv, "af", "");
    }
}

void MpvController::addSubtitleTrack(const QString& filePath) {
    if (!m_mpv || filePath.isEmpty()) return;
    QByteArray pathUtf8 = filePath.toUtf8();
    const char* cmd[] = {"sub-add", pathUtf8.constData(), "select", nullptr};
    mpv_command_async(m_mpv, registerPending(PendingKind::SubtitleAdd, filePath), cmd);
}

void MpvController::setSubtitleDelay(double seconds) {
    if (!m_mpv) return;
    m_subtitleDelay = seconds;
    mpv_set_property_async(m_mpv, 0, "sub-delay", MPV_FORMAT_DOUBLE, &seconds);
    emit subtitleDelayChanged(m_subtitleDelay);
}

void MpvController::adjustSubtitleDelay(double deltaSeconds) {
    setSubtitleDelay(m_subtitleDelay + deltaSeconds);
}

void MpvController::nextChapter() {
    if (!m_mpv) return;
    const char* cmd[] = {"add", "chapter", "1", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
}

void MpvController::previousChapter() {
    if (!m_mpv) return;
    const char* cmd[] = {"add", "chapter", "-1", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
}

QString MpvController::currentChapterTitle() const {
    if (m_currentChapter >= 0 && m_currentChapter < static_cast<int>(m_mediaInfo.chapters.size())) {
        return m_mediaInfo.chapters[m_currentChapter].title;
    }
    return QStringLiteral("Chapter %1").arg(m_currentChapter + 1);
}

void MpvController::updateChapters() {
    if (!m_mpv) return;
    mpv_node node;
    if (mpv_get_property(m_mpv, "chapter-list", MPV_FORMAT_NODE, &node) >= 0) {
        if (node.format == MPV_FORMAT_NODE_ARRAY) {
            std::vector<ChapterInfo> list;
            for (int i = 0; i < node.u.list->num; ++i) {
                const mpv_node& item = node.u.list->values[i];
                if (item.format == MPV_FORMAT_NODE_MAP) {
                    ChapterInfo ch;
                    ch.id = i;
                    for (int k = 0; k < item.u.list->num; ++k) {
                        const char* key = item.u.list->keys[k];
                        const mpv_node& val = item.u.list->values[k];
                        if (std::strcmp(key, "title") == 0 && val.format == MPV_FORMAT_STRING) {
                            ch.title = QString::fromUtf8(val.u.string);
                        } else if (std::strcmp(key, "time") == 0 && val.format == MPV_FORMAT_DOUBLE) {
                            ch.time = val.u.double_;
                        }
                    }
                    if (ch.title.isEmpty()) {
                        ch.title = QStringLiteral("Chapter %1").arg(i + 1);
                    }
                    list.push_back(ch);
                }
            }
            m_mediaInfo.chapters = list;
            emit chaptersChanged(m_mediaInfo.chapters);
        }
        mpv_free_node_contents(&node);
    }
}

StreamStats MpvController::streamStats() const {
    StreamStats stats;
    if (!m_mpv) return stats;

    stats.width = m_mediaInfo.width;
    stats.height = m_mediaInfo.height;
    stats.displayWidth = m_mediaInfo.displayWidth;
    stats.displayHeight = m_mediaInfo.displayHeight;

    if (!m_staticStatsCached) {
        char* fmt = mpv_get_property_string(m_mpv, "file-format");
        if (fmt) { m_cachedStats.fileFormat = QString::fromUtf8(fmt).toUpper(); mpv_free(fmt); }

        char* vcodec = mpv_get_property_string(m_mpv, "video-codec");
        if (vcodec) { m_cachedStats.videoCodec = QString::fromUtf8(vcodec); mpv_free(vcodec); }

        char* hw = mpv_get_property_string(m_mpv, "hwdec-current");
        if (hw) { m_cachedStats.hwdec = QString::fromUtf8(hw); mpv_free(hw); }
        if (m_cachedStats.hwdec.isEmpty() || m_cachedStats.hwdec == "no") {
            m_cachedStats.hwdec = QStringLiteral("Software");
        }

        char* acodec = mpv_get_property_string(m_mpv, "audio-codec-name");
        if (acodec) { m_cachedStats.audioCodec = QString::fromUtf8(acodec); mpv_free(acodec); }

        int64_t ach = 0;
        if (mpv_get_property(m_mpv, "audio-params/channel-count", MPV_FORMAT_INT64, &ach) >= 0) {
            m_cachedStats.audioChannels = static_cast<int>(ach);
        }

        int64_t samplerate = 0;
        if (mpv_get_property(m_mpv, "audio-params/samplerate", MPV_FORMAT_INT64, &samplerate) >= 0) {
            m_cachedStats.audioSampleRate = static_cast<int>(samplerate);
        }

        m_staticStatsCached = true;
    }

    stats.fileFormat = m_cachedStats.fileFormat;
    stats.videoCodec = m_cachedStats.videoCodec;
    stats.hwdec = m_cachedStats.hwdec;
    stats.audioCodec = m_cachedStats.audioCodec;
    stats.audioChannels = m_cachedStats.audioChannels;
    stats.audioSampleRate = m_cachedStats.audioSampleRate;

    // Dynamically query real-time performance properties
    double fps = 0.0;
    if (mpv_get_property(m_mpv, "container-fps", MPV_FORMAT_DOUBLE, &fps) >= 0 && fps > 0.0) {
        stats.fps = fps;
    } else if (mpv_get_property(m_mpv, "estimated-vf-fps", MPV_FORMAT_DOUBLE, &fps) >= 0) {
        stats.fps = fps;
    }

    double vbitrate = 0.0;
    if (mpv_get_property(m_mpv, "video-bitrate", MPV_FORMAT_DOUBLE, &vbitrate) >= 0) {
        stats.videoBitrate = vbitrate / 1000.0; // kbps
    }

    int64_t drops = 0;
    if (mpv_get_property(m_mpv, "frame-drop-count", MPV_FORMAT_INT64, &drops) >= 0) {
        stats.dropCount = static_cast<int>(drops);
    }

    if (char* vo = mpv_get_property_string(m_mpv, "current-vo")) {
        stats.videoOutput = QString::fromUtf8(vo);
        mpv_free(vo);
    } else {
        stats.videoOutput = QStringLiteral("none");
    }

    double abitrate = 0.0;
    if (mpv_get_property(m_mpv, "audio-bitrate", MPV_FORMAT_DOUBLE, &abitrate) >= 0) {
        stats.audioBitrate = abitrate / 1000.0; // kbps
    }

    return stats;
}

} // namespace kvikplay
