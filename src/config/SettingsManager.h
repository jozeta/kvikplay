#pragma once

#include <QObject>
#include <QSize>
#include <QPoint>
#include <QStringList>

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

    bool clickToPlayPause() const { return m_clickToPlayPause; }
    void setClickToPlayPause(bool enable);

    QStringList associatedExtensions() const { return m_associatedExtensions; }
    void setAssociatedExtensions(const QStringList& extensions);

signals:
    void alwaysOnTopChanged(bool top);
    void volumeChanged(int vol);
    void muteChanged(bool muted);
    void reverseTrackpadScrubbingChanged(bool reverse);
    void clickToPlayPauseChanged(bool enable);

private:
    SettingsManager();
    QString configFilePath() const;

    QSize m_windowSize{960, 540};
    QPoint m_windowPosition{-1, -1};
    int m_volume{100};
    bool m_muted{false};
    bool m_alwaysOnTop{false};
    double m_playbackSpeed{1.0};
    bool m_startInFullscreen{false};
    bool m_reverseTrackpadScrubbing{false};
    bool m_clickToPlayPause{true};
    QStringList m_associatedExtensions;
};

} // namespace kvikplay
