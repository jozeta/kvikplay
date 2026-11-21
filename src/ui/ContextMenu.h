#pragma once

#include <QMenu>
#include <QAction>
#include <QStringList>
#include <vector>
#include "player/MediaInfo.h"

namespace kvikplay {

// Everything the right-click menu shows, captured when it opens. The menu is rebuilt
// from this each time, so it can never show a stale state.
struct MenuState {
    bool hasMedia{false};
    bool playing{false};
    double speed{1.0};
    std::vector<TrackInfo> audioTracks;
    std::vector<TrackInfo> videoTracks;
    std::vector<TrackInfo> subtitleTracks;
    double subtitleDelay{0.0};
    std::vector<ChapterInfo> chapters;
    int currentChapter{-1};
    bool hasNextInFolder{false};
    bool hasPreviousInFolder{false};
    bool fullscreen{false};
    bool pictureInPicture{false};
    bool alwaysOnTop{false};
    bool dialogueBoost{false};
    bool inspectorVisible{false};
    bool reverseTrackpad{false};
    bool startInFullscreen{false};
    bool externalOpenNewWindow{false};
    QStringList recentFiles;
};

// The right-click (hold) menu, grouped by task: playback, file, what's playing,
// window, tools, then settings and help. Items that don't apply are hidden (one audio
// track, no chapters) and labels show the current choice ("Speed: 1.25×").
class ContextMenu : public QMenu {
    Q_OBJECT

public:
    explicit ContextMenu(QWidget* parent = nullptr);

    void rebuild(const MenuState& state);

signals:
    // File
    void openFileTriggered();
    void recentFileSelected(const QString& filePath);
    void clearRecentTriggered();
    // Playback
    void playPauseTriggered();
    void speedSelected(double speed);
    void audioTrackSelected(int id);
    void videoTrackSelected(int id);
    void subtitleTrackSelected(int id);
    void addSubtitleFileTriggered();
    void subtitleDelayAdjusted(double deltaSeconds);
    void subtitleDelayResetTriggered();
    void chapterSelected(double startSeconds);
    void nextInFolderTriggered();
    void previousInFolderTriggered();
    // Window
    void fullscreenToggled();
    void pipToggled();
    void alwaysOnTopToggled(bool onTop);
    void windowScaleSelected(double scale);
    // Tools
    void screenshotTriggered();
    void dialogueBoostToggled(bool enabled);
    void inspectorToggled();
    // Settings
    void reverseTrackpadToggled(bool reversed);
    void startInFullscreenToggled(bool on);
    void externalOpenNewWindowToggled(bool newWindow);
    void fileAssociationsTriggered();
    // Help
    void helpTriggered();
    void aboutTriggered();
    void closeTriggered();

private:
    void addFileItems(const MenuState& s);
    void addPlayingItems(const MenuState& s);
    void addWindowItems(const MenuState& s);
    void addToolItems(const MenuState& s);
    void addSettingsAndHelp(const MenuState& s);
    QMenu* addTrackMenu(const QString& kind, const std::vector<TrackInfo>& tracks,
                        bool withOff, void (ContextMenu::*signal)(int));

    // An action that shows a shortcut as a hint only; the key itself is handled by the
    // main window (a live menu shortcut would fire twice or while the menu is closed).
    QAction* addHinted(QMenu* menu, const QString& text, const QKeySequence& hint);
};

} // namespace kvikplay
