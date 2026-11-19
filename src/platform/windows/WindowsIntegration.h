#pragma once

#include "platform/PlatformIntegration.h"

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
    void openSystemDefaultAppsSettings() override;

    void setDisplaySleepInhibited(bool inhibit) override;

    static double currentAspectRatio();
    static void setGlobalAspectRatio(double ratio);

private:
    static double s_aspectRatio;
};

} // namespace kvikplay
