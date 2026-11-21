#include "TestCheck.h"
#include <iostream>
#include <QCoreApplication>
#include "config/SettingsManager.h"
#include <QFileInfo>
#include <QTemporaryDir>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    using namespace kvikplay;

    // Item 32: KVIKPLAY_CONFIG_DIR keeps tests (and screenshot runs) away from
    // the user's real config.json. Must be set before the singleton is created.
    QTemporaryDir tmp;
    CHECK(tmp.isValid());
    qputenv("KVIKPLAY_CONFIG_DIR", tmp.path().toUtf8());

    std::cout << "[TestSettings] Checking settings persistence..." << std::endl;

    auto& settings = SettingsManager::instance();
    settings.save();
    CHECK(QFileInfo::exists(tmp.path() + QStringLiteral("/config.json")));

    // New: what opening a file from Finder/Explorer does. Default: replace.
    std::cout << "[TestSettings] Checking external-open mode..." << std::endl;
    CHECK(settings.externalOpenInNewWindow() == false);
    settings.setExternalOpenInNewWindow(true);
    settings.save();
    settings.setExternalOpenInNewWindow(false);
    settings.load();
    CHECK(settings.externalOpenInNewWindow() == true);
    settings.setExternalOpenInNewWindow(false);

    // Update check at startup: on by default, can be turned off (About box).
    CHECK(settings.checkForUpdates() == true);
    settings.setCheckForUpdates(false);
    settings.save();
    settings.setCheckForUpdates(true);
    settings.load();
    CHECK(settings.checkForUpdates() == false);
    settings.setCheckForUpdates(true);

    // Item 11: a recent file that no longer exists can be dropped from the list.
    settings.clearRecentFiles();
    settings.addRecentFile(QStringLiteral("/a/one.mkv"));
    settings.addRecentFile(QStringLiteral("/a/two.mkv"));
    settings.removeRecentFile(QStringLiteral("/a/one.mkv"));
    CHECK(settings.recentFiles() == QStringList{QStringLiteral("/a/two.mkv")});
    settings.clearRecentFiles();

    // Item 10: resume positions are evicted oldest-first, not alphabetically.
    std::cout << "[TestSettings] Checking resume-position eviction order..." << std::endl;
    settings.setResumePosition(QStringLiteral("/z/oldest.mkv"), 42.0);
    for (int i = 0; i < 100; ++i) {
        settings.setResumePosition(QStringLiteral("/m/f%1.mkv").arg(i, 3, 10, QChar('0')), 60.0 + i);
    }
    CHECK(settings.resumePosition(QStringLiteral("/z/oldest.mkv")) == 0.0);   // evicted
    CHECK(settings.resumePosition(QStringLiteral("/m/f000.mkv")) == 60.0);    // kept
    // Updating an entry makes it the newest; order survives save + load.
    settings.setResumePosition(QStringLiteral("/m/f000.mkv"), 61.0);
    settings.save();
    settings.load();
    settings.setResumePosition(QStringLiteral("/new.mkv"), 30.0);
    CHECK(settings.resumePosition(QStringLiteral("/m/f000.mkv")) == 61.0);    // refreshed, kept
    CHECK(settings.resumePosition(QStringLiteral("/m/f001.mkv")) == 0.0);     // now the oldest, evicted
    CHECK(settings.resumePosition(QStringLiteral("/new.mkv")) == 30.0);
    settings.setVolume(85);
    CHECK(settings.volume() == 85);

    settings.setMuted(true);
    CHECK(settings.isMuted() == true);

    settings.setAlwaysOnTop(true);
    CHECK(settings.alwaysOnTop() == true);

    settings.setWindowSize(QSize(1280, 720));
    CHECK(settings.windowSize() == QSize(1280, 720));

    settings.setReverseTrackpadScrubbing(true);
    CHECK(settings.reverseTrackpadScrubbing() == true);

    QStringList exts = {QStringLiteral(".mp4"), QStringLiteral(".mkv")};
    settings.setAssociatedExtensions(exts);
    CHECK(settings.associatedExtensions().size() == 2);

    // Resume positions
    settings.setResumePosition(QStringLiteral("/path/to/ep1.mkv"), 123.45);
    CHECK(std::abs(settings.resumePosition(QStringLiteral("/path/to/ep1.mkv")) - 123.45) < 0.01);
    settings.clearResumePosition(QStringLiteral("/path/to/ep1.mkv"));
    CHECK(settings.resumePosition(QStringLiteral("/path/to/ep1.mkv")) == 0.0);

    // Recent files
    settings.clearRecentFiles();
    CHECK(settings.recentFiles().isEmpty());
    settings.addRecentFile(QStringLiteral("/media/movie1.mp4"));
    settings.addRecentFile(QStringLiteral("/media/movie2.mp4"));
    CHECK(settings.recentFiles().size() == 2);
    CHECK(settings.recentFiles().first() == QStringLiteral("/media/movie2.mp4"));

    // Dialogue boost
    settings.setDialogueBoost(true);
    CHECK(settings.dialogueBoost() == true);

    // Save and reload
    settings.save();
    settings.load();

    CHECK(settings.volume() == 85);
    CHECK(settings.isMuted() == true);
    CHECK(settings.alwaysOnTop() == true);
    CHECK(settings.windowSize() == QSize(1280, 720));
    CHECK(settings.reverseTrackpadScrubbing() == true);
    CHECK(settings.dialogueBoost() == true);
    CHECK(settings.recentFiles().size() == 2);

    // Reset back to defaults
    settings.setReverseTrackpadScrubbing(false);
    settings.setDialogueBoost(false);
    settings.clearRecentFiles();
    settings.save();

    std::cout << "[TestSettings] All tests passed!" << std::endl;
    return 0;
}
