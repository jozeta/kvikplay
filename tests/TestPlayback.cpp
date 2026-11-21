#include <QCoreApplication>
#include <QTimer>
#include "TestCheck.h"
#include <iostream>
#include <cmath>
#include <clocale>
#include <QThread>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryDir>
#include <QFile>
#include <functional>
#include "player/MediaPlayer.h"
#include "player/PlaybackState.h"
#include "formats/FormatRegistry.h"

static bool waitFor(QCoreApplication& app, const std::function<bool()>& cond, int ms = 5000) {
    for (int waited = 0; waited < ms; waited += 20) {
        app.processEvents();
        if (cond()) return true;
        QThread::msleep(20);
    }
    app.processEvents();
    return cond();
}

static QString mpvOption(kvikplay::MediaPlayer& p, const char* name) {
    char* v = mpv_get_property_string(p.controller()->handle(), (QByteArray("options/") + name).constData());
    QString out = v ? QString::fromUtf8(v) : QString();
    if (v) mpv_free(v);
    return out;
}

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

    // Item 13 + 16: libmpv is hardened and starts without its built-in Lua scripts.
    std::cout << "[TestPlayback] Testing hardened mpv options..." << std::endl;
    for (const char* opt : {"config", "load-scripts", "ytdl", "access-references", "osc",
                            "load-stats-overlay", "load-console", "load-select", "load-commands",
                            "load-context-menu", "load-positioning", "load-auto-profiles"}) {
        const QString v = mpvOption(player, opt);
        if (v != QStringLiteral("no")) std::cerr << "  option " << opt << " = " << v.toStdString() << std::endl;
        CHECK(v == QStringLiteral("no"));
    }

    // Item 13: only formats from FormatRegistry are opened (no playlists, EDL, ...).
    std::cout << "[TestPlayback] Testing unsupported file rejection..." << std::endl;
    {
        QTemporaryDir tmp;
        const QString playlist = tmp.path() + QStringLiteral("/list.m3u");
        QFile f(playlist);
        CHECK(f.open(QIODevice::WriteOnly));
        f.write("http://example.invalid/stream.mp4\n");
        f.close();
        QString lastError;
        auto c = QObject::connect(&player, &MediaPlayer::errorOccurred, [&](const QString& e) { lastError = e; });
        player.openMedia(playlist);
        app.processEvents();
        QObject::disconnect(c);
        CHECK(lastError.contains(QStringLiteral("not a supported")));
        CHECK(player.mediaInfo().filePath != playlist);
    }

    // Item 3: replay after the end, then pause, must not be stuck in Ended.
    std::cout << "[TestPlayback] Testing replay after end..." << std::endl;
    player.openMedia(findTestMedia(QStringLiteral("test_media/test_16_9.mp4")));
    CHECK(waitFor(app, [&] { return player.state() == PlaybackState::Playing && player.duration() > 1.0; }));
    player.seekAbsolute(player.duration() - 0.3, true);
    CHECK(waitFor(app, [&] { return player.state() == PlaybackState::Ended; }));
    player.togglePlayPause();   // replay from the start
    CHECK(waitFor(app, [&] { return player.state() == PlaybackState::Playing && player.position() < 1.0; }));
    player.togglePlayPause();   // must pause, not restart
    CHECK(waitFor(app, [&] { return player.state() == PlaybackState::Paused; }));

    // Item 5: a screenshot is reported only once the file exists; failures are reported.
    std::cout << "[TestPlayback] Testing screenshot completion..." << std::endl;
    {
        QTemporaryDir shotDir;
        player.controller()->setScreenshotDirectory(shotDir.path());
        QString saved;
        auto c = QObject::connect(&player, &MediaPlayer::screenshotTaken, [&](const QString& p) { saved = p; });
        player.takeScreenshot(false);
        CHECK(waitFor(app, [&] { return !saved.isEmpty(); }));
        CHECK(QFileInfo::exists(saved));
        QObject::disconnect(c);

        player.controller()->setScreenshotDirectory(shotDir.path() + QStringLiteral("/missing/dir"));
        QString err;
        auto c2 = QObject::connect(&player, &MediaPlayer::errorOccurred, [&](const QString& e) { err = e; });
        player.takeScreenshot(false);
        CHECK(waitFor(app, [&] { return !err.isEmpty(); }));
        CHECK(err.contains(QStringLiteral("Screenshot")));
        QObject::disconnect(c2);
    }

    std::cout << "[TestPlayback] All playback tests passed successfully!" << std::endl;
    return 0;
}
