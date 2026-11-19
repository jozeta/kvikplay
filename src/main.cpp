#include "app/Application.h"
#include "window/MainWindow.h"
#include <QIcon>
#include <QSurfaceFormat>
#include <iostream>
#include <clocale>

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
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app_icon.svg")));

    kvikplay::MainWindow window;
    if (!window.initialize()) {
        std::cerr << "Error: Failed to initialize Kvikplay media player." << std::endl;
        return 1;
    }

    app.setMainWindow(&window);
    window.show();

    return app.exec();
}
