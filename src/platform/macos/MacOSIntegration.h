#pragma once

#include "platform/PlatformIntegration.h"

namespace kvikplay {

class MacOSIntegration : public PlatformIntegration {
public:
    MacOSIntegration();
    ~MacOSIntegration() override;

    void setupFramelessWindow(QWidget* widget) override;
    void setWindowRounded(QWidget* widget, bool rounded) override;
    void setWindowAspectRatio(QWidget* widget, double ratio, int displayWidth, int displayHeight) override;
    bool isDefaultPlayerForExtension(const QString& ext) override;
    bool registerFileAssociations(const QStringList& extensions) override;
    void openSystemDefaultAppsSettings() override;
    void installActivationObserver(QWidget* widget, std::function<void(bool activatedByMouse)> callback) override;
    void setAlwaysOnTop(QWidget* widget, bool top) override;
    void installHoverObserver(QWidget* widget, std::function<void()> onHover) override;

    void setDisplaySleepInhibited(bool inhibit) override;
    void setNowPlayingInfo(const NowPlayingInfo& info) override;
    void clearNowPlayingInfo() override;
    void setupMediaKeyCommands(const MediaKeyCallbacks& callbacks) override;

private:
    unsigned int m_sleepAssertionId{0};
    MediaKeyCallbacks m_mediaCallbacks;
    // Objective-C objects kept as opaque pointers (this header is also read by C++).
    void* m_activationObserver{nullptr}; // id<NSObject> from NSNotificationCenter, retained
    void* m_hoverTarget{nullptr};        // KvikplayHoverTarget, retained
    void* m_hoverArea{nullptr};          // NSTrackingArea, retained
    void* m_hoverView{nullptr};          // NSView the area belongs to (not retained)
};

} // namespace kvikplay
