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

    void setDisplaySleepInhibited(bool inhibit) override;
    void setNowPlayingInfo(const NowPlayingInfo& info) override;
    void clearNowPlayingInfo() override;
    void setupMediaKeyCommands(const MediaKeyCallbacks& callbacks) override;

private:
    unsigned int m_sleepAssertionId{0};
    MediaKeyCallbacks m_mediaCallbacks;
};

} // namespace kvikplay
