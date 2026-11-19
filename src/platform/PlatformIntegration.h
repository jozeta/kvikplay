#pragma once

#include <QString>
#include <QStringList>
#include <memory>

class QWindow;
class QWidget;

namespace kvikplay {

class PlatformIntegration {
public:
    virtual ~PlatformIntegration() = default;

    static std::unique_ptr<PlatformIntegration> create();

    virtual void setupFramelessWindow(QWidget* widget) = 0;
    virtual void setWindowAspectRatio(QWidget* widget, double ratio, int displayWidth, int displayHeight) = 0;
    virtual bool isDefaultPlayerForExtension(const QString& ext) = 0;
    virtual bool registerFileAssociations(const QStringList& extensions) = 0;
    virtual void openSystemDefaultAppsSettings() = 0;
};

} // namespace kvikplay
