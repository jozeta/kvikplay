#pragma once

#include <QString>
#include <QStringList>
#include <memory>
#include <functional>

#include <QWidget>
#include <QWindow>

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

    // Keep the window above others without recreating the native window (a QWidget
    // setWindowFlags() call would drop the native customisations above).
    virtual void setAlwaysOnTop(QWidget* widget, bool top) {
        if (widget && widget->windowHandle()) {
            widget->windowHandle()->setFlag(Qt::WindowStaysOnTopHint, top);
        }
    }

    // Mouse movement over the window while the app is in the background, so the
    // controls appear on hover without a click first. Windows already delivers it.
    virtual void installHoverObserver(QWidget* widget, std::function<void()> onHover) {
        (void)widget;
        (void)onHover;
    }

    // Undo registerFileAssociations() for these extensions, where the OS allows it.
    virtual bool canUnregisterFileAssociations() const { return false; }
    virtual bool unregisterFileAssociations(const QStringList& extensions) {
        (void)extensions;
        return false;
    }

    // Power & Sleep management
    virtual void setDisplaySleepInhibited(bool inhibit) { (void)inhibit; }

    // System Media Transport & Now Playing
    virtual void setNowPlayingInfo(const NowPlayingInfo& info) { (void)info; }
    virtual void clearNowPlayingInfo() {}
    virtual void setupMediaKeyCommands(const MediaKeyCallbacks& callbacks) { (void)callbacks; }
};

} // namespace kvikplay
