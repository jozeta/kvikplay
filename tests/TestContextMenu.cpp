#include "TestCheck.h"
#include <QApplication>
#include <QMenu>
#include <QSignalSpy>
#include <iostream>
#include "ui/ContextMenu.h"

using namespace kvikplay;

// Visible top-level entries; separators as "-".
static QStringList topLevel(const QMenu& menu) {
    QStringList out;
    for (QAction* a : menu.actions()) {
        if (!a->isVisible()) continue;
        out << (a->isSeparator() ? QStringLiteral("-") : a->text());
    }
    return out;
}

static QAction* find(const QMenu& menu, const QString& prefix) {
    for (QAction* a : menu.actions()) {
        if (a->isVisible() && a->text().startsWith(prefix)) return a;
    }
    return nullptr;
}

static TrackInfo track(int id, const char* title, bool selected) {
    TrackInfo t;
    t.id = id;
    t.title = QString::fromUtf8(title);
    t.selected = selected;
    return t;
}

int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    ContextMenu menu;

#ifdef Q_OS_MACOS
    const QString quit = QStringLiteral("Quit Kvikplay");
#else
    const QString quit = QStringLiteral("Exit Kvikplay");
#endif

    std::cout << "[TestContextMenu] No video: short menu..." << std::endl;
    MenuState empty;
    menu.rebuild(empty);
    const QStringList shortMenu{QStringLiteral("Open File…"), QStringLiteral("Open Recent"), QStringLiteral("-"),
                                QStringLiteral("Settings"), QStringLiteral("Help && Shortcuts…"),
                                QStringLiteral("About Kvikplay"), QStringLiteral("-"), quit};
    if (topLevel(menu) != shortMenu) std::cerr << "  got: " << topLevel(menu).join(" | ").toStdString() << std::endl;
    CHECK(topLevel(menu) == shortMenu);

    std::cout << "[TestContextMenu] Playing: grouped menu, labels show the current choice..." << std::endl;
    MenuState s;
    s.hasMedia = true;
    s.playing = true;
    s.speed = 1.25;
    s.audioTracks = {track(1, "English Stereo", true)};
    s.subtitleTracks = {track(3, "Swedish", true)};
    s.hasNextInFolder = true;
    menu.rebuild(s);
    CHECK(topLevel(menu).first() == QStringLiteral("Pause"));
    // Shortcut hints are visible (macOS hides context-menu shortcuts unless asked).
    CHECK(find(menu, QStringLiteral("Pause"))->isShortcutVisibleInContextMenu());
    CHECK(find(menu, QStringLiteral("Speed: 1.25×")) != nullptr);
    CHECK(find(menu, QStringLiteral("Audio")) == nullptr);       // one audio track: nothing to choose
    CHECK(find(menu, QStringLiteral("Chapter")) == nullptr);     // no chapters: hidden
    CHECK(find(menu, QStringLiteral("Subtitles: Swedish")) != nullptr);
    CHECK(find(menu, QStringLiteral("Next in Folder"))->isEnabled());
    CHECK(!find(menu, QStringLiteral("Previous in Folder"))->isEnabled());
    CHECK(find(menu, QStringLiteral("Window Size")) != nullptr);
    CHECK(topLevel(menu).last() == quit);

    std::cout << "[TestContextMenu] More tracks and chapters appear when they exist..." << std::endl;
    s.audioTracks.push_back(track(2, "Commentary", false));
    ChapterInfo c1; c1.id = 0; c1.title = QStringLiteral("Opening"); c1.time = 0.0;
    ChapterInfo c2; c2.id = 1; c2.title = QStringLiteral("Intro"); c2.time = 60.0;
    s.chapters = {c1, c2};
    s.currentChapter = 1;
    menu.rebuild(s);
    CHECK(find(menu, QStringLiteral("Audio: English Stereo")) != nullptr);
    CHECK(find(menu, QStringLiteral("Chapter 2/2: Intro")) != nullptr);

    std::cout << "[TestContextMenu] Choosing a speed and a chapter emits them..." << std::endl;
    QSignalSpy speedSpy(&menu, &ContextMenu::speedSelected);
    QMenu* speedMenu = find(menu, QStringLiteral("Speed"))->menu();
    for (QAction* a : speedMenu->actions()) {
        if (a->text() == QStringLiteral("1.5×")) a->trigger();
    }
    CHECK(speedSpy.count() == 1);
    CHECK(speedSpy.at(0).at(0).toDouble() == 1.5);

    QSignalSpy chapterSpy(&menu, &ContextMenu::chapterSelected);
    find(menu, QStringLiteral("Chapter"))->menu()->actions().first()->trigger();
    CHECK(chapterSpy.count() == 1);
    CHECK(chapterSpy.at(0).at(0).toDouble() == 0.0);

    std::cout << "[TestContextMenu] Settings hold the preferences..." << std::endl;
    QMenu* settings = find(menu, QStringLiteral("Settings"))->menu();
    QStringList settingTexts;
    for (QAction* a : settings->actions()) settingTexts << a->text();
#ifdef Q_OS_MACOS
    CHECK(settingTexts.contains(QStringLiteral("Reverse Trackpad Scrubbing")));
#else
    CHECK(settingTexts.contains(QStringLiteral("Reverse Touchpad Scrubbing"))); // Windows wording
#endif
    CHECK(settingTexts.contains(QStringLiteral("Start in Fullscreen")));
    CHECK(settingTexts.contains(QStringLiteral("File Associations…")));

    std::cout << "[TestContextMenu] All tests passed!" << std::endl;
    return 0;
}
