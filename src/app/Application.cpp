#include "Application.h"
#include "window/MainWindow.h"
#include "formats/FormatRegistry.h"
#include <QFileOpenEvent>
#include <QFileInfo>
#include <QCommandLineParser>
#include <clocale>

namespace kvikplay {

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv) {

    // Ensure LC_NUMERIC is "C" after QApplication initialization for libmpv
    std::setlocale(LC_NUMERIC, "C");

    setApplicationName(QStringLiteral("Kvikplay"));
    setApplicationDisplayName(QStringLiteral("Kvikplay"));
    setOrganizationName(QStringLiteral("Kvikplay"));
    setOrganizationDomain(QStringLiteral("kvikplay.org"));
    setApplicationVersion(QStringLiteral("1.0.0"));

    // Check positional command-line argument for media file
    const QStringList args = arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString& arg = args.at(i);
        if (!arg.startsWith('-')) {
            QFileInfo fi(arg);
            if (fi.exists() && FormatRegistry::instance().isSupported(arg)) {
                m_initialFile = fi.absoluteFilePath();
                break;
            }
        }
    }
}

void Application::setMainWindow(MainWindow* window) {
    m_mainWindow = window;
    if (m_mainWindow && !m_initialFile.isEmpty()) {
        m_mainWindow->openFile(m_initialFile);
    }
}

bool Application::event(QEvent* event) {
    if (event->type() == QEvent::FileOpen) {
        auto* openEvent = static_cast<QFileOpenEvent*>(event);
        QString file = openEvent->file();
        if (m_mainWindow) {
            m_mainWindow->openFile(file);
        } else {
            m_initialFile = file;
        }
        return true;
    }
    return QApplication::event(event);
}

} // namespace kvikplay
