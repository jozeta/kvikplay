#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <mpv/client.h>
#include "PlaybackState.h"
#include "MediaInfo.h"

namespace kvikplay {

class MpvController : public QObject {
    Q_OBJECT

public:
    explicit MpvController(QObject* parent = nullptr);
    ~MpvController() override;

    bool initialize();

    mpv_handle* handle() const { return m_mpv; }

    void loadFile(const QString& filePath);
    void play();
    void pause();
    void togglePlayPause();
    void stop();

    void seekRelative(double seconds);
    void seekAbsolute(double seconds, bool exact = true);
    void stepFrame();
    void stepFrameBack();

    void setVolume(int percent);
    void adjustVolume(int deltaPercent);
    void setMute(bool mute);
    void toggleMute();

    void setSpeed(double speed);
    double speed() const { return m_speed; }

    // Subtitle delay
    double subtitleDelay() const { return m_subtitleDelay; }
    void setSubtitleDelay(double seconds);
    void adjustSubtitleDelay(double deltaSeconds);

    // Chapter controls
    void nextChapter();
    void previousChapter();
    int currentChapter() const { return m_currentChapter; }
    int chapterCount() const { return static_cast<int>(m_mediaInfo.chapters.size()); }
    QString currentChapterTitle() const;

    // Technical stats
    StreamStats streamStats() const;

    // Track controls
    void setAudioTrack(int id);
    void setVideoTrack(int id);
    void setSubtitleTrack(int id);
    void addSubtitleTrack(const QString& filePath);

    // Screenshot and Audio enhancement
    void takeScreenshot(bool includeSubtitles = false);
    // Folder for screenshots; empty = the Desktop (or home if there is none).
    void setScreenshotDirectory(const QString& dir) { m_screenshotDir = dir; }
    void setDialogueBoost(bool enabled);

    const std::vector<TrackInfo>& audioTracks() const { return m_mediaInfo.audioTracks; }
    const std::vector<TrackInfo>& videoTracks() const { return m_mediaInfo.videoTracks; }
    const std::vector<TrackInfo>& subtitleTracks() const { return m_mediaInfo.subtitleTracks; }

    PlaybackState state() const { return m_state; }
    bool hasMedia() const { return !m_mediaInfo.filePath.isEmpty(); }
    const MediaInfo& mediaInfo() const { return m_mediaInfo; }
    double position() const { return m_position; }
    double duration() const { return m_duration; }
    int volume() const { return m_volume; }
    bool isMuted() const { return m_isMuted; }

signals:
    void playbackStateChanged(PlaybackState state);
    void mediaLoaded(const MediaInfo& info);
    void videoParamsChanged(int width, int height, int displayWidth, int displayHeight, double dar);
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void volumeChanged(int percent);
    void muteChanged(bool muted);
    void speedChanged(double speed);
    void playbackEnded();
    void errorOccurred(const QString& errorMessage);
    void tracksChanged(const std::vector<TrackInfo>& audio, const std::vector<TrackInfo>& video, const std::vector<TrackInfo>& sub);
    void screenshotTaken(const QString& filePath);
    void subtitleAdded(const QString& fileName);
    void subtitleDelayChanged(double seconds);
    void chaptersChanged(const std::vector<ChapterInfo>& chapters);
    void chapterChanged(int chapterIndex, const QString& title);

private slots:
    void processMpvEvents();

private:
    static void onMpvWakeup(void* ctx);
    void handleMpvEvent(mpv_event* event);
    void handlePropertyChange(uint64_t id, mpv_event_property* prop);
    void handleCommandReply(uint64_t id, int error);

    // Async commands whose completion we report (screenshot saved, subtitle loaded).
    enum class PendingKind { Screenshot, SubtitleAdd };
    struct PendingCommand { PendingKind kind; QString path; };
    uint64_t registerPending(PendingKind kind, const QString& path);
    void updateVideoParams();
    void updateTracks();
    void updateChapters();

    mpv_handle* m_mpv{nullptr};
    PlaybackState m_state{PlaybackState::Idle};
    MediaInfo m_mediaInfo;
    double m_position{0.0};
    double m_duration{0.0};
    int m_volume{100};
    bool m_isMuted{false};
    double m_speed{1.0};
    bool m_isPaused{false};
    double m_subtitleDelay{0.0};
    int m_currentChapter{0};
    QString m_screenshotDir;
    std::unordered_map<uint64_t, PendingCommand> m_pending;
    uint64_t m_nextReplyId{1000};
    std::atomic<bool> m_wakeupPending{false};
    mutable StreamStats m_cachedStats;
    mutable bool m_staticStatsCached{false};
};

} // namespace kvikplay
