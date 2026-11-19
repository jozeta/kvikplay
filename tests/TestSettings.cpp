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

    QStringList exts = {QStringLiteral(".mp4"), QStringLiteral(".mkv")};
    settings.setAssociatedExtensions(exts);
    assert(settings.associatedExtensions().size() == 2);

    // Resume positions
    settings.setResumePosition(QStringLiteral("/path/to/ep1.mkv"), 123.45);
    assert(std::abs(settings.resumePosition(QStringLiteral("/path/to/ep1.mkv")) - 123.45) < 0.01);
    settings.clearResumePosition(QStringLiteral("/path/to/ep1.mkv"));
    assert(settings.resumePosition(QStringLiteral("/path/to/ep1.mkv")) == 0.0);

    // Recent files
    settings.clearRecentFiles();
    assert(settings.recentFiles().isEmpty());
    settings.addRecentFile(QStringLiteral("/media/movie1.mp4"));
    settings.addRecentFile(QStringLiteral("/media/movie2.mp4"));
    assert(settings.recentFiles().size() == 2);
    assert(settings.recentFiles().first() == QStringLiteral("/media/movie2.mp4"));

    // Dialogue boost
    settings.setDialogueBoost(true);
    assert(settings.dialogueBoost() == true);

    // Save and reload
    settings.save();
    settings.load();

    assert(settings.volume() == 85);
    assert(settings.isMuted() == true);
    assert(settings.alwaysOnTop() == true);
    assert(settings.windowSize() == QSize(1280, 720));
    assert(settings.reverseTrackpadScrubbing() == true);
    assert(settings.dialogueBoost() == true);
    assert(settings.recentFiles().size() == 2);

    // Reset back to defaults
    settings.setReverseTrackpadScrubbing(false);
    settings.setDialogueBoost(false);
    settings.clearRecentFiles();
    settings.save();

    std::cout << "[TestSettings] All tests passed!" << std::endl;
    return 0;
}
