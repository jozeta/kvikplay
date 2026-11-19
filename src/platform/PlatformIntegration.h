#pragma once

#include <QString>
#include <QStringList>
#include <memory>
#include <functional>

class QWindow;
class QWidget;

namespace kvikplay {

struct NowPlayingInfo {
    QString title;
    QString artist;
    double duration{0.0};
    double position{0.0};
    double playbackRate{1.0};
};

struct MediaKeyCallbacks {
    std::function<void()> onPlay;
    std::function<void()> onPause;
    std::function<void()> onTogglePlayPause;
    std::function<void()> onNextTrack;
    std::function<void()> onPreviousTrack;
    std::function<void(double seconds)> onSeek;
};

class PlatformIntegration {
public:
    virtual ~PlatformIntegration() = default;

    static std::unique_ptr<PlatformIntegration> create();

    virtual void setupFramelessWindow(QWidget* widget) = 0;
    virtual void setWindowRounded(QWidget* widget, bool rounded) = 0;
    virtual void setWindowAspectRatio(QWidget* widget, double ratio, int displayWidth, int displayHeight) = 0;
    virtual bool isDefaultPlayerForExtension(const QString& ext) = 0;
    virtual bool registerFileAssociations(const QStringList& extensions) = 0;
    virtual void openSystemDefaultAppsSettings() = 0;
    virtual void installActivationObserver(QWidget* widget, std::function<void(bool activatedByMouse)> callback) {
        (void)widget;
        (void)callback;
    }

    // Power & Sleep management
    virtual void setDisplaySleepInhibited(bool inhibit) { (void)inhibit; }

    // System Media Transport & Now Playing
    virtual void setNowPlayingInfo(const NowPlayingInfo& info) { (void)info; }
    virtual void clearNowPlayingInfo() {}
    virtual void setupMediaKeyCommands(const MediaKeyCallbacks& callbacks) { (void)callbacks; }
};

} // namespace kvikplay
