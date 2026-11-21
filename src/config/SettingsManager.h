#pragma once

#include <QObject>
#include <QSize>
#include <QPoint>
#include <QStringList>
#include <utility>
#include <vector>

namespace kvikplay {

class SettingsManager : public QObject {
    Q_OBJECT

public:
    static SettingsManager& instance();

    void load();
    void save() const;

    // Window geometry
    QSize windowSize() const { return m_windowSize; }
    void setWindowSize(const QSize& size);

    QPoint windowPosition() const { return m_windowPosition; }
    void setWindowPosition(const QPoint& pos);

    // Preferences
    int volume() const { return m_volume; }
    void setVolume(int vol);

    bool isMuted() const { return m_muted; }
    void setMuted(bool muted);

    bool alwaysOnTop() const { return m_alwaysOnTop; }
    void setAlwaysOnTop(bool top);

    double playbackSpeed() const { return m_playbackSpeed; }
    void setPlaybackSpeed(double speed);

    bool startInFullscreen() const { return m_startInFullscreen; }
    void setStartInFullscreen(bool fullscreen);

    bool reverseTrackpadScrubbing() const { return m_reverseTrackpadScrubbing; }
    void setReverseTrackpadScrubbing(bool reverse);

    // Resume position
    double resumePosition(const QString& filePath) const;
    void setResumePosition(const QString& filePath, double pos);
    void clearResumePosition(const QString& filePath);

    // Recent files (max 8)
    QStringList recentFiles() const { return m_recentFiles; }
    void addRecentFile(const QString& filePath);
    void removeRecentFile(const QString& filePath);
    void clearRecentFiles();

    // Dialogue Boost (Dynamic Audio Normalizer)
    bool dialogueBoost() const { return m_dialogueBoost; }
    void setDialogueBoost(bool boost);

    // Opening a file from Finder / Explorer while a video plays: replace it (default)
    // or open a separate player window. Drag-and-drop and Open File always replace.
    bool externalOpenInNewWindow() const { return m_externalOpenInNewWindow; }
    void setExternalOpenInNewWindow(bool newWindow) { m_externalOpenInNewWindow = newWindow; }

    QStringList associatedExtensions() const { return m_associatedExtensions; }
    void setAssociatedExtensions(const QStringList& extensions);

signals:
    void alwaysOnTopChanged(bool top);
    void volumeChanged(int vol);
    void muteChanged(bool muted);
    void reverseTrackpadScrubbingChanged(bool reverse);
    void dialogueBoostChanged(bool boost);
    void recentFilesChanged();

private:
    SettingsManager();
    QString configFilePath() const;
    static constexpr int kMaxResumePositions = 100;

    QSize m_windowSize{960, 540};
    QPoint m_windowPosition{-1, -1};
    int m_volume{100};
    bool m_muted{false};
    bool m_alwaysOnTop{false};
    double m_playbackSpeed{1.0};
    bool m_startInFullscreen{false};
    bool m_reverseTrackpadScrubbing{false};
    bool m_dialogueBoost{false};
    bool m_externalOpenInNewWindow{false};
    QStringList m_recentFiles;
    // Oldest first; capped at kMaxResumePositions (least recently saved is dropped).
    std::vector<std::pair<QString, double>> m_resumePositions;
    QStringList m_associatedExtensions;
};

} // namespace kvikplay
