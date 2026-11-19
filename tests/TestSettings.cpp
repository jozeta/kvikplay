#include <cassert>
#include <iostream>
#include <QCoreApplication>
#include "config/SettingsManager.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    using namespace kvikplay;

    std::cout << "[TestSettings] Checking settings persistence..." << std::endl;

    auto& settings = SettingsManager::instance();
    settings.setVolume(85);
    assert(settings.volume() == 85);

    settings.setMuted(true);
    assert(settings.isMuted() == true);

    settings.setAlwaysOnTop(true);
    assert(settings.alwaysOnTop() == true);

    settings.setWindowSize(QSize(1280, 720));
    assert(settings.windowSize() == QSize(1280, 720));

    settings.setReverseTrackpadScrubbing(true);
    assert(settings.reverseTrackpadScrubbing() == true);

    settings.setClickToPlayPause(false);
    assert(settings.clickToPlayPause() == false);

    QStringList exts = {QStringLiteral(".mp4"), QStringLiteral(".mkv")};
    settings.setAssociatedExtensions(exts);
    assert(settings.associatedExtensions().size() == 2);

    // Save and reload
    settings.save();
    settings.load();

    assert(settings.volume() == 85);
    assert(settings.isMuted() == true);
    assert(settings.alwaysOnTop() == true);
    assert(settings.windowSize() == QSize(1280, 720));
    assert(settings.reverseTrackpadScrubbing() == true);
    assert(settings.clickToPlayPause() == false);

    // Reset back to defaults
    settings.setReverseTrackpadScrubbing(false);
    settings.setClickToPlayPause(true);
    settings.save();

    std::cout << "[TestSettings] All tests passed!" << std::endl;
    return 0;
}
