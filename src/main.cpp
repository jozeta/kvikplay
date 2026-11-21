#include "app/Application.h"
#include "config/SettingsManager.h"
#include "platform/SingleInstance.h"
#include "window/MainWindow.h"
#include "version.h"
#include <QIcon>
#include <QSurfaceFormat>
#include <iostream>
#include <clocale>

#ifdef _WIN32
#include <windows.h>
#include <cstdio>
#endif

int main(int argc, char* argv[]) {
    std::setlocale(LC_NUMERIC, "C");

    // Configure high-performance OpenGL surface format before QApplication creation
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    format.setSwapInterval(1);      // Smooth VSync matching display refresh rate (e.g. 120Hz ProMotion)
    format.setDepthBufferSize(0);   // Video surface requires no depth buffer (saves VRAM & memory bandwidth)
    format.setStencilBufferSize(0); // No stencil buffer needed
    QSurfaceFormat::setDefaultFormat(format);

    kvikplay::Application app(argc, argv);

    if (app.versionRequested()) {
#ifdef _WIN32
        // A GUI-subsystem exe has no console; borrow the one it was started from.
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            FILE* out = nullptr;
            freopen_s(&out, "CONOUT$", "w", stdout);
        }
#endif
        std::cout << "Kvikplay " << KVIKPLAY_VERSION << std::endl;
        return 0;
    }

    // Windows: a second launch (double-click in Explorer) hands its file to the running
    // window, unless the user chose "Open in New Window". macOS does this natively
    // (one process, QFileOpenEvent), so these are no-ops there.
    bool cascade = app.cascadeRequested();
    if (kvikplay::SingleInstance::anotherInstanceRunning()) {
        const QString file = app.initialFile();
        if (!file.isEmpty() && !kvikplay::SettingsManager::instance().externalOpenInNewWindow()) {
            if (kvikplay::SingleInstance::sendToRunningInstance(file)) {
                return 0;
            }
        }
        cascade = true;
    }

    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.svg")));

    kvikplay::MainWindow window;
    if (!window.initialize(cascade)) {
        std::cerr << "Error: Failed to initialize Kvikplay media player." << std::endl;
        return 1;
    }

    kvikplay::SingleInstance::listen([&window](const QString& file) { window.openForwardedFile(file); });

    app.setMainWindow(&window);
    window.show();
    window.applyStartupState();

    return app.exec();
}
