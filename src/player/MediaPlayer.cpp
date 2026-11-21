#include "MediaPlayer.h"
#include "config/SettingsManager.h"
#include "formats/FormatRegistry.h"
#include <QFileInfo>

namespace kvikplay {

MediaPlayer::MediaPlayer(QObject* parent)
    : QObject(parent)
    , m_controller(std::make_unique<MpvController>(this)) {

    connect(m_controller.get(), &MpvController::playbackStateChanged, this, &MediaPlayer::stateChanged);
    connect(m_controller.get(), &MpvController::mediaLoaded, this, &MediaPlayer::mediaLoaded);
    connect(m_controller.get(), &MpvController::videoParamsChanged, this, &MediaPlayer::videoParamsChanged);
    connect(m_controller.get(), &MpvController::positionChanged, this, &MediaPlayer::positionChanged);
    connect(m_controller.get(), &MpvController::durationChanged, this, &MediaPlayer::durationChanged);
    connect(m_controller.get(), &MpvController::volumeChanged, this, [this](int vol) {
        SettingsManager::instance().setVolume(vol);
        emit volumeChanged(vol);
    });
    connect(m_controller.get(), &MpvController::muteChanged, this, [this](bool muted) {
        SettingsManager::instance().setMuted(muted);
        emit muteChanged(muted);
    });
    connect(m_controller.get(), &MpvController::speedChanged, this, &MediaPlayer::speedChanged);
    connect(m_controller.get(), &MpvController::playbackEnded, this, &MediaPlayer::mediaEnded);
    connect(m_controller.get(), &MpvController::errorOccurred, this, &MediaPlayer::errorOccurred);
    connect(m_controller.get(), &MpvController::tracksChanged, this, &MediaPlayer::tracksChanged);
    connect(m_controller.get(), &MpvController::screenshotTaken, this, &MediaPlayer::screenshotTaken);
    connect(m_controller.get(), &MpvController::subtitleAdded, this, &MediaPlayer::subtitleAdded);
    connect(m_controller.get(), &MpvController::subtitleDelayChanged, this, &MediaPlayer::subtitleDelayChanged);
    connect(m_controller.get(), &MpvController::chaptersChanged, this, &MediaPlayer::chaptersChanged);
    connect(m_controller.get(), &MpvController::chapterChanged, this, &MediaPlayer::chapterChanged);
}

bool MediaPlayer::initialize(VideoOutput output) {
    // Exactly "libmpv", never a list like "libmpv,null": mpv probes list entries, and
    // outside macOS vo_libmpv refuses to be probed (mpv video/out/vo_libmpv.c preinit), so
    // mpv silently fell back to "null" on Windows: sound but a black window (1.0.0-1.2.2).
    if (!m_controller->initialize(output == VideoOutput::Window ? "libmpv" : "null")) {
        return false;
    }

    // Apply saved settings
    auto& settings = SettingsManager::instance();
    m_controller->setVolume(settings.volume());
    m_controller->setMute(settings.isMuted());
    m_controller->setSpeed(settings.playbackSpeed());
    m_controller->setDialogueBoost(settings.dialogueBoost());

    return true;
}

void MediaPlayer::openMedia(const QString& filePath) {
    if (filePath.isEmpty()) return;

    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        emit errorOccurred(QStringLiteral("File not found: %1").arg(fi.fileName()));
        return;
    }
    // Only open formats Kvikplay lists. Playlists, EDL and other container-of-
    // references formats could make libmpv read other files or reach the network.
    if (!FormatRegistry::instance().isSupported(filePath)) {
        emit errorOccurred(QStringLiteral("%1 is not a supported media file").arg(fi.fileName()));
        return;
    }

    m_controller->loadFile(fi.absoluteFilePath());
}

void MediaPlayer::play() {
    m_controller->play();
}

void MediaPlayer::pause() {
    m_controller->pause();
}

void MediaPlayer::togglePlayPause() {
    m_controller->togglePlayPause();
}

void MediaPlayer::stop() {
    m_controller->stop();
}

void MediaPlayer::seekRelative(double seconds) {
    m_controller->seekRelative(seconds);
}

void MediaPlayer::seekAbsolute(double seconds, bool exact) {
    m_controller->seekAbsolute(seconds, exact);
}

void MediaPlayer::setSubtitleDelay(double seconds) {
    m_controller->setSubtitleDelay(seconds);
}

void MediaPlayer::adjustSubtitleDelay(double deltaSeconds) {
    m_controller->adjustSubtitleDelay(deltaSeconds);
}

void MediaPlayer::nextChapter() {
    m_controller->nextChapter();
}

void MediaPlayer::previousChapter() {
    m_controller->previousChapter();
}

void MediaPlayer::setVolume(int volumePercent) {
    m_controller->setVolume(volumePercent);
}

void MediaPlayer::adjustVolume(int deltaPercent) {
    m_controller->adjustVolume(deltaPercent);
}

void MediaPlayer::toggleMute() {
    m_controller->toggleMute();
}

void MediaPlayer::setSpeed(double speed) {
    m_controller->setSpeed(speed);
}

void MediaPlayer::setAudioTrack(int id) {
    m_controller->setAudioTrack(id);
}

void MediaPlayer::setVideoTrack(int id) {
    m_controller->setVideoTrack(id);
}

void MediaPlayer::setSubtitleTrack(int id) {
    m_controller->setSubtitleTrack(id);
}

void MediaPlayer::addSubtitleTrack(const QString& filePath) {
    m_controller->addSubtitleTrack(filePath);
}

void MediaPlayer::takeScreenshot(bool includeSubtitles) {
    m_controller->takeScreenshot(includeSubtitles);
}

void MediaPlayer::setDialogueBoost(bool enabled) {
    m_controller->setDialogueBoost(enabled);
}

} // namespace kvikplay
