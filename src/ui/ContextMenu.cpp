#include "ContextMenu.h"
#include <QActionGroup>
#include <QFileInfo>
#include <cmath>

namespace kvikplay {

namespace {

// Menu text from untrusted strings (file names, track titles): a single '&' would be
// eaten as an accelerator marker, so it is doubled.
QString menuText(QString text) {
    return text.replace(QLatin1Char('&'), QStringLiteral("&&"));
}

QString speedText(double speed) {
    // 1.0×, 1.5×, 1.25×: as many decimals as needed, at least one.
    const bool twoDecimals = std::lround(speed * 100.0) % 10 != 0;
    return QStringLiteral("%1×").arg(speed, 0, 'f', twoDecimals ? 2 : 1);
}

QString shortTrackName(const TrackInfo& t) {
    if (!t.title.isEmpty()) return t.title;
    if (!t.lang.isEmpty()) return t.lang.toUpper();
    return QStringLiteral("Track %1").arg(t.id);
}

#ifdef Q_OS_MACOS
const QString kFileBrowser = QStringLiteral("Finder");
const QString kQuitText = QStringLiteral("Quit Kvikplay");
const QString kTrackpad = QStringLiteral("Trackpad");
#else
const QString kFileBrowser = QStringLiteral("Explorer");
const QString kQuitText = QStringLiteral("Exit Kvikplay");
const QString kTrackpad = QStringLiteral("Touchpad");
#endif

} // namespace

ContextMenu::ContextMenu(QWidget* parent)
    : QMenu(parent) {
    setStyleSheet(QStringLiteral(
        "QMenu {"
        "  background-color: #1a1c22;"
        "  border: 1px solid #2d313b;"
        "  border-radius: 8px;"
        "  padding: 6px;"
        "  color: #eeeeee;"
        "  font-size: 13px;"
        "}"
        "QMenu::item {"
        "  padding: 6px 28px 6px 20px;"
        "  border-radius: 4px;"
        "}"
        "QMenu::item:selected {"
        "  background-color: #2e3440;"
        "  color: #ffffff;"
        "}"
        "QMenu::item:disabled {"
        "  color: #666c7a;"
        "}"
        "QMenu::separator {"
        "  height: 1px;"
        "  background: #2d313b;"
        "  margin: 4px 8px;"
        "}"
        "QMenu::indicator:checked {"
        "  width: 14px;"
        "  height: 14px;"
        "}"
    ));
    rebuild(MenuState{});
}

QAction* ContextMenu::addHinted(QMenu* menu, const QString& text, const QKeySequence& hint) {
    QAction* a = menu->addAction(text);
    if (!hint.isEmpty()) {
        a->setShortcut(hint);
        a->setShortcutContext(Qt::WidgetShortcut); // only active inside the open menu
        a->setShortcutVisibleInContextMenu(true);    // macOS hides it otherwise
    }
    return a;
}

void ContextMenu::rebuild(const MenuState& s) {
    clear(); // deletes the old actions and submenus parented to them

    if (s.hasMedia) {
        QAction* playPause = addHinted(this, s.playing ? QStringLiteral("Pause") : QStringLiteral("Play"),
                                       QKeySequence(Qt::Key_Space));
        connect(playPause, &QAction::triggered, this, &ContextMenu::playPauseTriggered);
        addSeparator();
    }

    addFileItems(s);

    if (s.hasMedia) {
        addSeparator();
        addPlayingItems(s);
        addSeparator();
        addWindowItems(s);
        addSeparator();
        addToolItems(s);
    }

    addSeparator();
    addSettingsAndHelp(s);
}

void ContextMenu::addFileItems(const MenuState& s) {
    QAction* open = addHinted(this, QStringLiteral("Open File…"), QKeySequence::Open);
    connect(open, &QAction::triggered, this, &ContextMenu::openFileTriggered);

    QMenu* recent = addMenu(QStringLiteral("Open Recent"));
    recent->setEnabled(!s.recentFiles.isEmpty());
    for (const QString& path : s.recentFiles) {
        const QString name = QFileInfo(path).fileName();
        QAction* a = recent->addAction(menuText(name.isEmpty() ? path : name));
        connect(a, &QAction::triggered, this, [this, path]() { emit recentFileSelected(path); });
    }
    if (!s.recentFiles.isEmpty()) {
        recent->addSeparator();
        connect(recent->addAction(QStringLiteral("Clear Recent")), &QAction::triggered,
                this, &ContextMenu::clearRecentTriggered);
    }
}

QMenu* ContextMenu::addTrackMenu(const QString& kind, const std::vector<TrackInfo>& tracks,
                                 bool withOff, void (ContextMenu::*signal)(int)) {
    QString current = QStringLiteral("Off");
    for (const auto& t : tracks) {
        if (t.selected) current = shortTrackName(t);
    }
    QMenu* menu = addMenu(menuText(QStringLiteral("%1: %2").arg(kind, current)));
    auto* group = new QActionGroup(menu);

    auto addChoice = [&](const QString& text, bool checked, int id) {
        QAction* a = menu->addAction(text);
        a->setCheckable(true);
        a->setChecked(checked);
        group->addAction(a);
        connect(a, &QAction::triggered, this, [this, signal, id]() { emit (this->*signal)(id); });
    };

    if (withOff) {
        bool anySelected = false;
        for (const auto& t : tracks) anySelected = anySelected || t.selected;
        addChoice(QStringLiteral("Off"), !anySelected, 0);
        if (!tracks.empty()) menu->addSeparator();
    }
    for (const auto& t : tracks) {
        addChoice(menuText(t.displayName()), t.selected, t.id);
    }
    return menu;
}

void ContextMenu::addPlayingItems(const MenuState& s) {
    // Speed
    QMenu* speed = addMenu(QStringLiteral("Speed: %1").arg(speedText(s.speed)));
    auto* speedGroup = new QActionGroup(speed);
    for (double v : {0.5, 0.75, 1.0, 1.25, 1.5, 2.0}) {
        QAction* a = speed->addAction(speedText(v));
        a->setCheckable(true);
        a->setChecked(std::abs(v - s.speed) < 0.001);
        speedGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, v]() { emit speedSelected(v); });
    }
    speed->addSeparator();
    QAction* slower = addHinted(speed, QStringLiteral("Slower"), QKeySequence(Qt::Key_Minus));
    connect(slower, &QAction::triggered, this, [this, s]() {
        emit speedSelected(std::max(0.25, std::round((s.speed - 0.1) * 10.0) / 10.0));
    });
    QAction* faster = addHinted(speed, QStringLiteral("Faster"), QKeySequence(Qt::Key_Equal));
    connect(faster, &QAction::triggered, this, [this, s]() {
        emit speedSelected(std::min(4.0, std::round((s.speed + 0.1) * 10.0) / 10.0));
    });

    // Tracks: only menus where there is a choice to make.
    if (s.audioTracks.size() > 1) {
        addTrackMenu(QStringLiteral("Audio"), s.audioTracks, false, &ContextMenu::audioTrackSelected);
    }
    if (s.videoTracks.size() > 1) {
        addTrackMenu(QStringLiteral("Video"), s.videoTracks, false, &ContextMenu::videoTrackSelected);
    }
    QMenu* subs = addTrackMenu(QStringLiteral("Subtitles"), s.subtitleTracks, true,
                               &ContextMenu::subtitleTrackSelected);
    subs->addSeparator();
    connect(subs->addAction(QStringLiteral("Add Subtitle File…")), &QAction::triggered,
            this, &ContextMenu::addSubtitleFileTriggered);
    QAction* earlier = addHinted(subs, QStringLiteral("Show 100 ms Earlier"), QKeySequence(Qt::Key_Z));
    connect(earlier, &QAction::triggered, this, [this]() { emit subtitleDelayAdjusted(-0.1); });
    QAction* later = addHinted(subs, QStringLiteral("Show 100 ms Later"), QKeySequence(Qt::Key_X));
    connect(later, &QAction::triggered, this, [this]() { emit subtitleDelayAdjusted(0.1); });
    if (std::abs(s.subtitleDelay) > 0.0005) {
        QAction* reset = subs->addAction(QStringLiteral("Reset Timing (now %1 ms)")
                                             .arg(static_cast<int>(std::round(s.subtitleDelay * 1000.0))));
        connect(reset, &QAction::triggered, this, &ContextMenu::subtitleDelayResetTriggered);
    }

    // Chapters
    if (!s.chapters.empty()) {
        const int n = static_cast<int>(s.chapters.size());
        const int cur = std::clamp(s.currentChapter, 0, n - 1);
        QMenu* chapters = addMenu(menuText(QStringLiteral("Chapter %1/%2: %3")
                                               .arg(cur + 1).arg(n).arg(s.chapters[cur].title)));
        auto* group = new QActionGroup(chapters);
        for (int i = 0; i < n; ++i) {
            const ChapterInfo& ch = s.chapters[i];
            QAction* a = chapters->addAction(menuText(QStringLiteral("%1. %2").arg(i + 1).arg(ch.title)));
            a->setCheckable(true);
            a->setChecked(i == cur);
            group->addAction(a);
            const double start = ch.time;
            connect(a, &QAction::triggered, this, [this, start]() { emit chapterSelected(start); });
        }
    }

    // Folder
    QAction* next = addHinted(this, QStringLiteral("Next in Folder"), QKeySequence(Qt::Key_BracketRight));
    next->setEnabled(s.hasNextInFolder);
    connect(next, &QAction::triggered, this, &ContextMenu::nextInFolderTriggered);
    QAction* prev = addHinted(this, QStringLiteral("Previous in Folder"), QKeySequence(Qt::Key_BracketLeft));
    prev->setEnabled(s.hasPreviousInFolder);
    connect(prev, &QAction::triggered, this, &ContextMenu::previousInFolderTriggered);
}

void ContextMenu::addWindowItems(const MenuState& s) {
    QAction* fullscreen = addHinted(this, QStringLiteral("Fullscreen"), QKeySequence(Qt::Key_F));
    fullscreen->setCheckable(true);
    fullscreen->setChecked(s.fullscreen);
    connect(fullscreen, &QAction::triggered, this, &ContextMenu::fullscreenToggled);

    QAction* pip = addHinted(this, QStringLiteral("Picture-in-Picture"), QKeySequence(Qt::Key_P));
    pip->setCheckable(true);
    pip->setChecked(s.pictureInPicture);
    connect(pip, &QAction::triggered, this, &ContextMenu::pipToggled);

    QAction* onTop = addAction(QStringLiteral("Always on Top"));
    onTop->setCheckable(true);
    onTop->setChecked(s.alwaysOnTop);
    connect(onTop, &QAction::toggled, this, &ContextMenu::alwaysOnTopToggled);

    QMenu* size = addMenu(QStringLiteral("Window Size"));
    const struct { const char* text; double scale; Qt::Key key; } sizes[] = {
        {"Original Size (100%)", 1.0, Qt::Key_1},
        {"Half Size (50%)", 0.5, Qt::Key_2},
        {"Quarter Size (25%)", 0.25, Qt::Key_4},
    };
    for (const auto& sz : sizes) {
        QAction* a = addHinted(size, QString::fromUtf8(sz.text), QKeySequence(sz.key));
        const double scale = sz.scale;
        connect(a, &QAction::triggered, this, [this, scale]() { emit windowScaleSelected(scale); });
    }
}

void ContextMenu::addToolItems(const MenuState& s) {
    QAction* shot = addHinted(this, QStringLiteral("Take Screenshot"), QKeySequence(Qt::Key_S));
    connect(shot, &QAction::triggered, this, &ContextMenu::screenshotTriggered);

    QAction* boost = addHinted(this, QStringLiteral("Dialogue Boost"), QKeySequence(Qt::Key_N));
    boost->setCheckable(true);
    boost->setChecked(s.dialogueBoost);
    connect(boost, &QAction::toggled, this, &ContextMenu::dialogueBoostToggled);

    QAction* inspector = addHinted(this, QStringLiteral("Media Inspector"), QKeySequence(Qt::Key_I));
    inspector->setCheckable(true);
    inspector->setChecked(s.inspectorVisible);
    connect(inspector, &QAction::triggered, this, &ContextMenu::inspectorToggled);
}

void ContextMenu::addSettingsAndHelp(const MenuState& s) {
    QMenu* settings = addMenu(QStringLiteral("Settings"));

    QAction* reverse = settings->addAction(QStringLiteral("Reverse %1 Scrubbing").arg(kTrackpad));
    reverse->setCheckable(true);
    reverse->setChecked(s.reverseTrackpad);
    connect(reverse, &QAction::toggled, this, &ContextMenu::reverseTrackpadToggled);

    QAction* startFs = settings->addAction(QStringLiteral("Start in Fullscreen"));
    startFs->setCheckable(true);
    startFs->setChecked(s.startInFullscreen);
    connect(startFs, &QAction::toggled, this, &ContextMenu::startInFullscreenToggled);

    QMenu* external = settings->addMenu(QStringLiteral("Opening Files from %1").arg(kFileBrowser));
    auto* group = new QActionGroup(external);
    QAction* replace = external->addAction(QStringLiteral("Replace Current Video"));
    QAction* newWindow = external->addAction(QStringLiteral("Open in New Window"));
    for (QAction* a : {replace, newWindow}) {
        a->setCheckable(true);
        group->addAction(a);
    }
    (s.externalOpenNewWindow ? newWindow : replace)->setChecked(true);
    connect(replace, &QAction::triggered, this, [this]() { emit externalOpenNewWindowToggled(false); });
    connect(newWindow, &QAction::triggered, this, [this]() { emit externalOpenNewWindowToggled(true); });

    settings->addSeparator();
    connect(settings->addAction(QStringLiteral("File Associations…")), &QAction::triggered,
            this, &ContextMenu::fileAssociationsTriggered);

    // "&&": a single & would be eaten as an accelerator marker.
    connect(addAction(QStringLiteral("Help && Shortcuts…")), &QAction::triggered,
            this, &ContextMenu::helpTriggered);
    connect(addAction(QStringLiteral("About Kvikplay")), &QAction::triggered,
            this, &ContextMenu::aboutTriggered);
    addSeparator();
    QAction* quit = addHinted(this, kQuitText, QKeySequence(Qt::Key_Q));
    connect(quit, &QAction::triggered, this, &ContextMenu::closeTriggered);
}

} // namespace kvikplay
