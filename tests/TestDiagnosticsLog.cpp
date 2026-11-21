#include "TestCheck.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
#include "app/DiagnosticsLog.h"

using namespace kvikplay;

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("kvikplay.log"));

    std::cout << "[TestDiagnosticsLog] Lines are written with a category..." << std::endl;
    DiagnosticsLog::open(path, 4096);
    DiagnosticsLog::write("app", QStringLiteral("render context ready"));
    DiagnosticsLog::write("mpv", QStringLiteral("vo/libmpv: No render context set."));
    qWarning("a Qt warning");
    DiagnosticsLog::flush();
    QFile f(path);
    CHECK(f.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(f.readAll());
    f.close();
    CHECK(text.contains(QStringLiteral("[app] render context ready")));
    CHECK(text.contains(QStringLiteral("[mpv] vo/libmpv: No render context set.")));
    CHECK(text.contains(QStringLiteral("a Qt warning")));

    std::cout << "[TestDiagnosticsLog] The file never grows past its cap..." << std::endl;
    for (int i = 0; i < 1000; ++i) DiagnosticsLog::write("mpv", QStringLiteral("line %1 of filler text").arg(i));
    DiagnosticsLog::flush();
    CHECK(QFile(path).size() <= 4096 + 200);

    std::cout << "[TestDiagnosticsLog] Opening again starts a fresh file..." << std::endl;
    DiagnosticsLog::open(path, 4096);
    DiagnosticsLog::flush();
    CHECK(QFile(path).size() < 200);

    std::cout << "[TestDiagnosticsLog] All tests passed!" << std::endl;
    return 0;
}
