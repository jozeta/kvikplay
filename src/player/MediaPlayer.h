#pragma once

#include <QObject>
#include <memory>
#include <vector>
#include "MpvController.h"
#include "PlaybackState.h"
#include "MediaInfo.h"

namespace kvikplay {

class MediaPlayer : public QObject {
    Q_OBJECT

public:
    explicit MediaPlayer(QObject* parent = nullptr);
    ~MediaPlayer() override = default;

    bool initialize();

    MpvController* controller() { return m_controller.get(); }
    const MpvController* controller() const { return m_controller.get(); }

    void openMedia(const QString& filePath);
    void play();
    void pause();
    void togglePlayPause();
    void stop();

    void seekRelative(double seconds);
    void seekAbsolute(double seconds);

    void setVolume(int volumePercent);
    void adjustVolume(int deltaPercent);
    void toggleMute();

    void setSpeed(double speed);

    // Track controls
    void setAudioTrack(int id);
    void setVideoTrack(int id);
    void setSubtitleTrack(int id);
    void addSubtitleTrack(const QString& filePath);

    // Screenshot and Audio enhancement
    void takeScreenshot(bool includeSubtitles = false);
    void setDialogueBoost(bool enabled);

    const std::vector<TrackInfo>& audioTracks() const { return m_controller->audioTracks(); }
    const std::vector<TrackInfo>& videoTracks() const { return m_controller->videoTracks(); }
    const std::vector<TrackInfo>& subtitleTracks() const { return m_controller->subtitleTracks(); }

    PlaybackState state() const { return m_controller->state(); }
    bool hasMedia() const { return m_controller ? m_controller->hasMedia() : false; }
    const MediaInfo& mediaInfo() const { return m_controller->mediaInfo(); }
    double position() const { return m_controller->position(); }
    double duration() const { return m_controller->duration(); }
    int volume() const { return m_controller->volume(); }
    bool isMuted() const { return m_controller->isMuted(); }

signals:
    void stateChanged(PlaybackState state);
    void mediaLoaded(const MediaInfo& info);
    void videoParamsChanged(int width, int height, int displayWidth, int displayHeight, double dar);
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void volumeChanged(int percent);
    void muteChanged(bool muted);
    void speedChanged(double speed);
    void mediaEnded();
    void errorOccurred(const QString& error);
    void tracksChanged(const std::vector<TrackInfo>& audio, const std::vector<TrackInfo>& video, const std::vector<TrackInfo>& sub);
    void screenshotTaken(const QString& filePath);
    void subtitleAdded(const QString& fileName);

private:
    std::unique_ptr<MpvController> m_controller;
};

} // namespace kvikplay
