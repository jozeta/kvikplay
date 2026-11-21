#pragma once

#include "platform/PlatformIntegration.h"
#include <memory>

namespace kvikplay {

class WindowsIntegration : public PlatformIntegration {
public:
    WindowsIntegration();
    ~WindowsIntegration() override;

    void setupFramelessWindow(QWidget* widget) override;
    void setWindowRounded(QWidget* widget, bool rounded) override;
    void setWindowAspectRatio(QWidget* widget, double ratio, int displayWidth, int displayHeight) override;
    bool isDefaultPlayerForExtension(const QString& ext) override;
    bool registerFileAssociations(const QStringList& extensions) override;
    bool canUnregisterFileAssociations() const override { return true; }
    bool unregisterFileAssociations(const QStringList& extensions) override;
    void openSystemDefaultAppsSettings() override;

    void setDisplaySleepInhibited(bool inhibit) override;

    // System Media Transport Controls: the Windows media flyout, lock screen and the
    // keyboard's media keys (the macOS equivalent is Now Playing + MPRemoteCommandCenter).
    void setupMediaKeyCommands(const MediaKeyCallbacks& callbacks) override;
    void setNowPlayingInfo(const NowPlayingInfo& info) override;
    void clearNowPlayingInfo() override;

    static double currentAspectRatio();
    static void setGlobalAspectRatio(double ratio);

private:
    struct Smtc; // C++/WinRT state, defined in the .cpp only
    std::unique_ptr<Smtc> m_smtc;
    void* m_hwnd{nullptr};
    static double s_aspectRatio;
};

} // namespace kvikplay
