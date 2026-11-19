#include "PlatformIntegration.h"

#if defined(Q_OS_MACOS)
#include "macos/MacOSIntegration.h"
#elif defined(Q_OS_WIN)
#include "windows/WindowsIntegration.h"
#endif

namespace kvikplay {

#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
class FallbackPlatformIntegration : public PlatformIntegration {
public:
    void setupFramelessWindow(QWidget*) override {}
    void setWindowAspectRatio(QWidget*, double, int, int) override {}
    bool isDefaultPlayerForExtension(const QString&) override { return false; }
    bool registerFileAssociations(const QStringList&) override { return false; }
    void openSystemDefaultAppsSettings() override {}
};
#endif

std::unique_ptr<PlatformIntegration> PlatformIntegration::create() {
#if defined(Q_OS_MACOS)
    return std::make_unique<MacOSIntegration>();
#elif defined(Q_OS_WIN)
    return std::make_unique<WindowsIntegration>();
#else
    return std::make_unique<FallbackPlatformIntegration>();
#endif
}

} // namespace kvikplay
