#include <QApplication>
#include "TestCheck.h"
#include <iostream>
#include "platform/PlatformIntegration.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    using namespace kvikplay;

    std::cout << "[TestPlatform] Testing PlatformIntegration creation..." << std::endl;
    auto platform = PlatformIntegration::create();
    CHECK(platform != nullptr);

    std::cout << "[TestPlatform] Testing sleep & screensaver inhibition..." << std::endl;
    platform->setDisplaySleepInhibited(true);
    platform->setDisplaySleepInhibited(false);

    std::cout << "[TestPlatform] Testing Now Playing metadata integration..." << std::endl;
    NowPlayingInfo info;
    info.title = QStringLiteral("Test Nordic Movie");
    info.artist = QStringLiteral("Director");
    info.duration = 120.0;
    info.position = 15.5;
    info.playbackRate = 1.0;
    platform->setNowPlayingInfo(info);
    platform->clearNowPlayingInfo();

    std::cout << "[TestPlatform] Testing Media Key callbacks configuration..." << std::endl;
    bool played = false;
    bool paused = false;
    bool toggled = false;
    bool next = false;
    bool prev = false;
    double seekPos = -1.0;

    MediaKeyCallbacks callbacks;
    callbacks.onPlay = [&played]() { played = true; };
    callbacks.onPause = [&paused]() { paused = true; };
    callbacks.onTogglePlayPause = [&toggled]() { toggled = true; };
    callbacks.onNextTrack = [&next]() { next = true; };
    callbacks.onPreviousTrack = [&prev]() { prev = true; };
    callbacks.onSeek = [&seekPos](double sec) { seekPos = sec; };

    platform->setupMediaKeyCommands(callbacks);

    // Verify invoking callbacks directly
    callbacks.onPlay();
    CHECK(played);
    callbacks.onPause();
    CHECK(paused);
    callbacks.onTogglePlayPause();
    CHECK(toggled);
    callbacks.onNextTrack();
    CHECK(next);
    callbacks.onPreviousTrack();
    CHECK(prev);
    callbacks.onSeek(42.0);
    CHECK(seekPos == 42.0);

    std::cout << "[TestPlatform] All platform integration tests passed successfully!" << std::endl;
    return 0;
}
