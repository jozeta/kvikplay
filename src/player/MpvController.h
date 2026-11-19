#pragma once

#include <QObject>
#include <QString>
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
    void seekAbsolute(double seconds);

    void setVolume(int percent);
    void adjustVolume(int deltaPercent);
    void setMute(bool mute);
    void toggleMute();

    void setSpeed(double speed);

    PlaybackState state() const { return m_state; }
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

private slots:
    void processMpvEvents();

private:
    static void onMpvWakeup(void* ctx);
    void handleMpvEvent(mpv_event* event);
    void handlePropertyChange(mpv_event_property* prop);
    void updateVideoParams();

    mpv_handle* m_mpv{nullptr};
    PlaybackState m_state{PlaybackState::Idle};
    MediaInfo m_mediaInfo;
    double m_position{0.0};
    double m_duration{0.0};
    int m_volume{100};
    bool m_isMuted{false};
    double m_speed{1.0};
    bool m_isPaused{false};
};

} // namespace kvikplay
