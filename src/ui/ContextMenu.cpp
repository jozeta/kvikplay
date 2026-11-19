#include "ContextMenu.h"

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

} // namespace kvikplay
