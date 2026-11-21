#pragma once

#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <vector>
#include "player/MediaInfo.h"

namespace kvikplay {

class ContextMenu : public QMenu {
    Q_OBJECT

public:
    explicit ContextMenu(QWidget* parent = nullptr);

    void setPlaying(bool playing);
    void setAlwaysOnTop(bool onTop);
    void setFullscreen(bool fullscreen);
    void setReverseTrackpad(bool reversed);
    void setDialogueBoost(bool enabled);
    void setExternalOpenNewWindow(bool newWindow);
    void setStartInFullscreen(bool on);
    void setRecentFiles(const QStringList& files);
    void setTracks(const std::vector<TrackInfo>& audio,
                   const std::vector<TrackInfo>& video,
                   const std::vector<TrackInfo>& sub);

signals:
    void openFileTriggered();
    void playPauseTriggered();
    void alwaysOnTopToggled(bool onTop);
    void fileAssociationsTriggered();
    void fullscreenToggled();
    void reverseTrackpadToggled(bool reversed);
    void dialogueBoostToggled(bool enabled);
    void externalOpenNewWindowToggled(bool newWindow);
    void startInFullscreenToggled(bool on);
    void recentFileSelected(const QString& filePath);
    void clearRecentTriggered();
    void screenshotTriggered();
    void pipToggled();
    void inspectorToggled();
    void helpTriggered();
    void aboutTriggered();
    void closeTriggered();
    void audioTrackSelected(int id);
    void videoTrackSelected(int id);
    void subtitleTrackSelected(int id);

private:
    QAction* m_openFileAction{nullptr};
    QAction* m_playPauseAction{nullptr};
    QAction* m_alwaysOnTopAction{nullptr};
    QAction* m_fullscreenAction{nullptr};

    QMenu* m_recentMenu{nullptr};
    QMenu* m_audioMenu{nullptr};
    QMenu* m_videoMenu{nullptr};
    QMenu* m_subtitleMenu{nullptr};

    QAction* m_dialogueBoostAction{nullptr};
    QAction* m_screenshotAction{nullptr};
    QAction* m_pipAction{nullptr};
    QAction* m_inspectorAction{nullptr};
    QAction* m_reverseTrackpadAction{nullptr};
    QAction* m_fileAssocAction{nullptr};
    QAction* m_helpAction{nullptr};
    QAction* m_aboutAction{nullptr};
    QAction* m_closeAction{nullptr};
    QAction* m_externalReplaceAction{nullptr};
    QAction* m_externalNewWindowAction{nullptr};
    QAction* m_startFullscreenAction{nullptr};

    // One group per track menu for the menu's lifetime (a new group per track-list
    // change leaked). Deleted actions leave their group automatically.
    QActionGroup* m_audioGroup{nullptr};
    QActionGroup* m_videoGroup{nullptr};
    QActionGroup* m_subtitleGroup{nullptr};
};

} // namespace kvikplay
