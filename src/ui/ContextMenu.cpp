#include "ContextMenu.h"
#include <QActionGroup>
#include <QFileInfo>

namespace kvikplay {

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

    m_openFileAction = addAction(QStringLiteral("Open File..."), this, &ContextMenu::openFileTriggered);
    m_openFileAction->setShortcut(QKeySequence::Open);

    m_playPauseAction = addAction(QStringLiteral("Play"), this, &ContextMenu::playPauseTriggered);
    m_playPauseAction->setShortcut(QKeySequence(Qt::Key_Space));

    m_recentMenu = addMenu(QStringLiteral("Open Recent"));
    m_recentMenu->setEnabled(false);

    m_alwaysOnTopAction = addAction(QStringLiteral("Always on Top"));
    m_alwaysOnTopAction->setCheckable(true);
    connect(m_alwaysOnTopAction, &QAction::toggled, this, &ContextMenu::alwaysOnTopToggled);

    m_fullscreenAction = addAction(QStringLiteral("Fullscreen"), this, &ContextMenu::fullscreenToggled);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F));

    m_pipAction = addAction(QStringLiteral("Picture-in-Picture"), this, &ContextMenu::pipToggled);
    m_pipAction->setShortcut(QKeySequence(Qt::Key_P));

    addSeparator();

    // Track selection submenus
    m_audioMenu = addMenu(QStringLiteral("Audio"));
    m_audioMenu->setEnabled(false);

    m_videoMenu = addMenu(QStringLiteral("Video"));
    m_videoMenu->setEnabled(false);

    m_subtitleMenu = addMenu(QStringLiteral("Subtitles"));
    m_subtitleMenu->setEnabled(false);

    addSeparator();

    m_dialogueBoostAction = addAction(QStringLiteral("Quiet Dialogue Boost"));
    m_dialogueBoostAction->setCheckable(true);
    m_dialogueBoostAction->setShortcut(QKeySequence(Qt::Key_N));
    connect(m_dialogueBoostAction, &QAction::toggled, this, &ContextMenu::dialogueBoostToggled);

    m_screenshotAction = addAction(QStringLiteral("Take Screenshot"), this, &ContextMenu::screenshotTriggered);
    m_screenshotAction->setShortcut(QKeySequence(Qt::Key_S));

    m_inspectorAction = addAction(QStringLiteral("Media Inspector"), this, &ContextMenu::inspectorToggled);
    m_inspectorAction->setShortcut(QKeySequence(Qt::Key_I));

    addSeparator();

    m_reverseTrackpadAction = addAction(QStringLiteral("Reverse Trackpad Scrubbing"));
    m_reverseTrackpadAction->setCheckable(true);
    connect(m_reverseTrackpadAction, &QAction::toggled, this, &ContextMenu::reverseTrackpadToggled);

    m_startFullscreenAction = addAction(QStringLiteral("Start in Fullscreen"));
    m_startFullscreenAction->setCheckable(true);
    connect(m_startFullscreenAction, &QAction::toggled, this, &ContextMenu::startInFullscreenToggled);

#ifdef Q_OS_MACOS
    auto* externalMenu = addMenu(QStringLiteral("Opening Files from Finder"));
#else
    auto* externalMenu = addMenu(QStringLiteral("Opening Files from Explorer"));
#endif
    auto* externalGroup = new QActionGroup(externalMenu);
    m_externalReplaceAction = externalMenu->addAction(QStringLiteral("Replace Current Video"));
    m_externalNewWindowAction = externalMenu->addAction(QStringLiteral("Open in New Window"));
    for (QAction* a : {m_externalReplaceAction, m_externalNewWindowAction}) {
        a->setCheckable(true);
        externalGroup->addAction(a);
    }
    m_externalReplaceAction->setChecked(true);
    connect(m_externalNewWindowAction, &QAction::toggled, this, &ContextMenu::externalOpenNewWindowToggled);

    addSeparator();

    m_fileAssocAction = addAction(QStringLiteral("File Associations..."), this, &ContextMenu::fileAssociationsTriggered);
    // "&&": a single & would be eaten as a keyboard-accelerator marker.
    m_helpAction = addAction(QStringLiteral("Help && Shortcuts..."), this, &ContextMenu::helpTriggered);
    m_aboutAction = addAction(QStringLiteral("About"), this, &ContextMenu::aboutTriggered);

    addSeparator();

    m_closeAction = addAction(QStringLiteral("Close"), this, &ContextMenu::closeTriggered);
    m_closeAction->setShortcut(QKeySequence(Qt::Key_Q));
}

void ContextMenu::setPlaying(bool playing) {
    m_playPauseAction->setText(playing ? QStringLiteral("Pause") : QStringLiteral("Play"));
}

void ContextMenu::setAlwaysOnTop(bool onTop) {
    m_alwaysOnTopAction->setChecked(onTop);
}

void ContextMenu::setFullscreen(bool fullscreen) {
    m_fullscreenAction->setText(fullscreen ? QStringLiteral("Exit Fullscreen") : QStringLiteral("Fullscreen"));
}

void ContextMenu::setReverseTrackpad(bool reversed) {
    m_reverseTrackpadAction->setChecked(reversed);
}

void ContextMenu::setDialogueBoost(bool enabled) {
    if (m_dialogueBoostAction) {
        m_dialogueBoostAction->setChecked(enabled);
    }
}

void ContextMenu::setExternalOpenNewWindow(bool newWindow) {
    QSignalBlocker block(m_externalNewWindowAction); // reflect the setting, don't re-emit
    (newWindow ? m_externalNewWindowAction : m_externalReplaceAction)->setChecked(true);
}

void ContextMenu::setStartInFullscreen(bool on) {
    QSignalBlocker block(m_startFullscreenAction);
    m_startFullscreenAction->setChecked(on);
}

void ContextMenu::setRecentFiles(const QStringList& files) {
    m_recentMenu->clear();
    if (files.isEmpty()) {
        m_recentMenu->setEnabled(false);
        return;
    }

    m_recentMenu->setEnabled(true);
    for (const auto& filePath : files) {
        QFileInfo fi(filePath);
        QString display = fi.fileName();
        if (display.isEmpty()) {
            display = filePath;
        }
        auto* act = m_recentMenu->addAction(display);
        act->setToolTip(filePath);
        connect(act, &QAction::triggered, this, [this, filePath]() {
            emit recentFileSelected(filePath);
        });
    }

    m_recentMenu->addSeparator();
    auto* clearAct = m_recentMenu->addAction(QStringLiteral("Clear Recent"));
    connect(clearAct, &QAction::triggered, this, &ContextMenu::clearRecentTriggered);
}

void ContextMenu::setTracks(const std::vector<TrackInfo>& audio,
                            const std::vector<TrackInfo>& video,
                            const std::vector<TrackInfo>& sub) {
    // 1. Audio tracks
    m_audioMenu->clear();
    if (audio.size() <= 1) {
        m_audioMenu->setEnabled(false);
    } else {
        m_audioMenu->setEnabled(true);
        if (!m_audioGroup) m_audioGroup = new QActionGroup(this);
        auto* group = m_audioGroup;
        for (const auto& t : audio) {
            auto* act = m_audioMenu->addAction(t.displayName());
            act->setCheckable(true);
            act->setChecked(t.selected);
            group->addAction(act);
            int trackId = t.id;
            connect(act, &QAction::triggered, this, [this, trackId]() {
                emit audioTrackSelected(trackId);
            });
        }
    }

    // 2. Video tracks
    m_videoMenu->clear();
    if (video.size() <= 1) {
        m_videoMenu->setEnabled(false);
    } else {
        m_videoMenu->setEnabled(true);
        if (!m_videoGroup) m_videoGroup = new QActionGroup(this);
        auto* group = m_videoGroup;
        for (const auto& t : video) {
            auto* act = m_videoMenu->addAction(t.displayName());
            act->setCheckable(true);
            act->setChecked(t.selected);
            group->addAction(act);
            int trackId = t.id;
            connect(act, &QAction::triggered, this, [this, trackId]() {
                emit videoTrackSelected(trackId);
            });
        }
    }

    // 3. Subtitles
    m_subtitleMenu->clear();
    if (sub.empty()) {
        m_subtitleMenu->setEnabled(false);
    } else {
        m_subtitleMenu->setEnabled(true);
        if (!m_subtitleGroup) m_subtitleGroup = new QActionGroup(this);
        auto* group = m_subtitleGroup;

        // Disabled option
        bool anySelected = false;
        for (const auto& t : sub) {
            if (t.selected) anySelected = true;
        }

        auto* offAct = m_subtitleMenu->addAction(QStringLiteral("Off"));
        offAct->setCheckable(true);
        offAct->setChecked(!anySelected);
        group->addAction(offAct);
        connect(offAct, &QAction::triggered, this, [this]() {
            emit subtitleTrackSelected(0);
        });

        m_subtitleMenu->addSeparator();

        for (const auto& t : sub) {
            auto* act = m_subtitleMenu->addAction(t.displayName());
            act->setCheckable(true);
            act->setChecked(t.selected);
            group->addAction(act);
            int trackId = t.id;
            connect(act, &QAction::triggered, this, [this, trackId]() {
                emit subtitleTrackSelected(trackId);
            });
        }
    }
}

} // namespace kvikplay
