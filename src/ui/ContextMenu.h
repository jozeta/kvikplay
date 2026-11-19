#pragma once

#include <QMenu>
#include <QAction>

namespace kvikplay {

class ContextMenu : public QMenu {
    Q_OBJECT

public:
    explicit ContextMenu(QWidget* parent = nullptr);

    void setPlaying(bool playing);
    void setAlwaysOnTop(bool onTop);
    void setFullscreen(bool fullscreen);

signals:
    void playPauseTriggered();
    void alwaysOnTopToggled(bool onTop);
    void fileAssociationsTriggered();
    void fullscreenToggled();
    void aboutTriggered();
    void closeTriggered();

private:
    QAction* m_playPauseAction{nullptr};
    QAction* m_alwaysOnTopAction{nullptr};
    QAction* m_fileAssocAction{nullptr};
    QAction* m_fullscreenAction{nullptr};
    QAction* m_aboutAction{nullptr};
    QAction* m_closeAction{nullptr};
};

} // namespace kvikplay
