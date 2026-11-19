#include "ContextMenu.h"
#include <QActionGroup>

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

    m_playPauseAction = addAction(QStringLiteral("Play"), this, &ContextMenu::playPauseTriggered);
    m_playPauseAction->setShortcut(QKeySequence(Qt::Key_Space));

    m_alwaysOnTopAction = addAction(QStringLiteral("Always on Top"));
    m_alwaysOnTopAction->setCheckable(true);
    connect(m_alwaysOnTopAction, &QAction::toggled, this, &ContextMenu::alwaysOnTopToggled);

    m_fullscreenAction = addAction(QStringLiteral("Fullscreen"), this, &ContextMenu::fullscreenToggled);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F));

    addSeparator();

    // Track selection submenus
    m_audioMenu = addMenu(QStringLiteral("Audio"));
    m_audioMenu->setEnabled(false);

    m_videoMenu = addMenu(QStringLiteral("Video"));
    m_videoMenu->setEnabled(false);

    m_subtitleMenu = addMenu(QStringLiteral("Subtitles"));
    m_subtitleMenu->setEnabled(false);

    addSeparator();

    m_reverseTrackpadAction = addAction(QStringLiteral("Reverse Trackpad Scrubbing"));
    m_reverseTrackpadAction->setCheckable(true);
    connect(m_reverseTrackpadAction, &QAction::toggled, this, &ContextMenu::reverseTrackpadToggled);

    m_clickToPlayPauseAction = addAction(QStringLiteral("Click Window to Play/Pause"));
    m_clickToPlayPauseAction->setCheckable(true);
    m_clickToPlayPauseAction->setChecked(true);
    connect(m_clickToPlayPauseAction, &QAction::toggled, this, &ContextMenu::clickToPlayPauseToggled);

    addSeparator();

    m_fileAssocAction = addAction(QStringLiteral("File Associations..."), this, &ContextMenu::fileAssociationsTriggered);
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

void ContextMenu::setClickToPlayPause(bool enabled) {
    m_clickToPlayPauseAction->setChecked(enabled);
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
        auto* group = new QActionGroup(m_audioMenu);
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
        auto* group = new QActionGroup(m_videoMenu);
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
        auto* group = new QActionGroup(m_subtitleMenu);

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
