#include "app/Application.h"
#include "window/MainWindow.h"
#include <QIcon>
#include <iostream>
#include <clocale>

int main(int argc, char* argv[]) {
    std::setlocale(LC_NUMERIC, "C");
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
