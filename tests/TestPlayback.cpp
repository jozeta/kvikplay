#include <QCoreApplication>
#include <QTimer>
#include "TestCheck.h"
#include <iostream>
#include <cmath>
#include <clocale>
#include <QThread>
#include <QFileInfo>
#include <QDir>
#include "player/MediaPlayer.h"
#include "player/PlaybackState.h"
#include "formats/FormatRegistry.h"

static QString findTestMedia(const QString& relPath) {
    if (QFileInfo::exists(relPath)) return relPath;
    if (QFileInfo::exists(QStringLiteral("../") + relPath)) return QStringLiteral("../") + relPath;
    return relPath;
}

int main(int argc, char* argv[]) {
    std::setlocale(LC_NUMERIC, "C");
    QCoreApplication app(argc, argv);
    using namespace kvikplay;

    std::cout << "[TestPlayback] Initializing MediaPlayer..." << std::endl;
    MediaPlayer player;
    bool initOk = player.initialize();
    CHECK(initOk);

    // 1. Test 16:9 MP4
    std::cout << "[TestPlayback] Testing 16:9 MP4..." << std::endl;
    bool loaded = false;
    bool paramsChanged = false;
    QObject::connect(&player, &MediaPlayer::mediaLoaded, [&]() { loaded = true; });
    QObject::connect(&player, &MediaPlayer::videoParamsChanged, [&]() { paramsChanged = true; });

    player.openMedia(findTestMedia(QStringLiteral("test_media/test_16_9.mp4")));

    // Spin event loop until loaded (up to 3 seconds)
    for (int i = 0; i < 150 && (!loaded || !paramsChanged); ++i) {
        QThread::msleep(20);
        app.processEvents();
    }
    CHECK(loaded);
    CHECK(player.mediaInfo().hasVideo());
    CHECK(player.mediaInfo().width == 640);
    CHECK(player.mediaInfo().height == 360);
    CHECK(std::abs(player.mediaInfo().aspectRatio - (16.0 / 9.0)) < 0.05);

    // 2. Test Seeking and Volume
    std::cout << "[TestPlayback] Testing Seeking & Volume controls..." << std::endl;
    player.setVolume(75);
    CHECK(player.volume() == 75);

    player.adjustVolume(-15);
    CHECK(player.volume() == 60);

    player.toggleMute();
    CHECK(player.isMuted() == true);
    player.toggleMute();
    CHECK(player.isMuted() == false);

    player.seekRelative(1.0);

    // 3. Test Anamorphic Video (720x480 SAR 32/27 => DAR 16/9)
    std::cout << "[TestPlayback] Testing Anamorphic Video..." << std::endl;
    loaded = false;
    player.openMedia(findTestMedia(QStringLiteral("test_media/test_anamorphic.mp4")));
    for (int i = 0; i < 150 && (!loaded || std::abs(player.mediaInfo().aspectRatio - (16.0 / 9.0)) >= 0.1); ++i) {
        QThread::msleep(20);
        app.processEvents();
    }
    CHECK(loaded);
    CHECK(player.mediaInfo().hasVideo());
    CHECK(player.mediaInfo().width == 720);
    CHECK(player.mediaInfo().height == 480);
    std::cout << "Detected Anamorphic DAR: " << player.mediaInfo().aspectRatio << std::endl;
    CHECK(std::abs(player.mediaInfo().aspectRatio - (16.0 / 9.0)) < 0.1);

    // 4. Test 9:16 Vertical Video
    std::cout << "[TestPlayback] Testing 9:16 Vertical Video..." << std::endl;
    loaded = false;
    paramsChanged = false;
    player.openMedia(findTestMedia(QStringLiteral("test_media/test_9_16.mov")));
    for (int i = 0; i < 150 && (!loaded || !paramsChanged); ++i) {
        QThread::msleep(20);
        app.processEvents();
    }
    CHECK(loaded);
    CHECK(player.mediaInfo().hasVideo());
    CHECK(player.mediaInfo().width == 360);
    CHECK(player.mediaInfo().height == 640);
    CHECK(std::abs(player.mediaInfo().aspectRatio - (9.0 / 16.0)) < 0.05);

    // 5. Test Audio-only File (MP3)
    std::cout << "[TestPlayback] Testing Audio-only MP3..." << std::endl;
    loaded = false;
    player.openMedia(findTestMedia(QStringLiteral("test_media/test_audio.mp3")));
    for (int i = 0; i < 150 && !loaded; ++i) {
        QThread::msleep(20);
        app.processEvents();
    }
    CHECK(loaded);
    CHECK(player.mediaInfo().isAudioOnly);
    CHECK(!player.mediaInfo().hasVideo());

    // 6. Test Multi-Track Audio Selection
    std::cout << "[TestPlayback] Testing Multi-Track Media..." << std::endl;
    loaded = false;
    player.openMedia(findTestMedia(QStringLiteral("test_media/test_multi_track.mkv")));
    for (int i = 0; i < 150 && (!loaded || player.audioTracks().size() < 2); ++i) {
        QThread::msleep(20);
        app.processEvents();
    }
    CHECK(loaded);
    CHECK(player.audioTracks().size() >= 2);
    std::cout << "Found audio tracks: " << player.audioTracks().size() << std::endl;
    for (const auto& trk : player.audioTracks()) {
        std::cout << "  Track " << trk.id << ": " << trk.displayName().toStdString() << " (selected=" << trk.selected << ")" << std::endl;
    }
    player.setAudioTrack(2);

    // Test Dialogue Boost
    std::cout << "[TestPlayback] Testing Dialogue Boost..." << std::endl;
    player.setDialogueBoost(true);
    player.setDialogueBoost(false);

    // Test Subtitle addition & delay sync
    std::cout << "[TestPlayback] Testing Subtitle addition & delay..." << std::endl;
    player.addSubtitleTrack(findTestMedia(QStringLiteral("test_media/test_sub.srt")));
    player.setSubtitleDelay(0.15);
    CHECK(std::abs(player.subtitleDelay() - 0.15) < 0.01);
    player.adjustSubtitleDelay(-0.05);
    CHECK(std::abs(player.subtitleDelay() - 0.10) < 0.01);

    // Test Keyframe Seeking & Speed
    std::cout << "[TestPlayback] Testing Keyframe Seeking & Speed..." << std::endl;
    player.seekAbsolute(1.5, false); // keyframe seek
    player.seekAbsolute(2.0, true);  // exact seek
    player.setSpeed(2.0);
    CHECK(player.speed() == 2.0);
    player.setSpeed(1.0);
    CHECK(player.speed() == 1.0);

    // Test Frame Stepping
    std::cout << "[TestPlayback] Testing Frame Stepping..." << std::endl;
    player.stepFrame();
    player.stepFrameBack();

    // Test Stream Stats & Caching
    std::cout << "[TestPlayback] Testing Stream Stats..." << std::endl;
    auto stats = player.streamStats();
    CHECK(stats.width > 0 || stats.fps >= 0.0);
    auto stats2 = player.streamStats();
    CHECK(stats2.fileFormat == stats.fileFormat);

    // 7. Test Corrupt File Error Handling
    std::cout << "[TestPlayback] Testing Corrupt File Handling..." << std::endl;
    bool errorOccurred = false;
    QObject::connect(&player, &MediaPlayer::errorOccurred, [&](const QString& err) {
        std::cout << "Received expected error: " << err.toStdString() << std::endl;
        errorOccurred = true;
    });
    player.openMedia(findTestMedia(QStringLiteral("test_media/corrupt.mp4")));
    for (int i = 0; i < 100 && !errorOccurred; ++i) {
        QThread::msleep(20);
        app.processEvents();
    }
    CHECK(errorOccurred);

    std::cout << "[TestPlayback] All playback tests passed successfully!" << std::endl;
    return 0;
}
