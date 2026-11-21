#include "DiagnosticsLog.h"
#include <QElapsedTimer>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>

namespace kvikplay::DiagnosticsLog {

namespace {
QMutex g_mutex;
QFile g_file;
QElapsedTimer g_clock;
qint64 g_maxBytes = 0;
bool g_full = false;
QtMessageHandler g_previousHandler = nullptr;

void writeLocked(const char* category, const QString& text) {
    if (!g_file.isOpen() || g_full) return;
    if (g_file.size() >= g_maxBytes) {
        g_file.write("[log] size limit reached, later lines are dropped\n");
        g_full = true;
        return;
    }
    const QString line = QStringLiteral("%1 [%2] %3\n")
                             .arg(g_clock.elapsed(), 7)
                             .arg(QLatin1String(category), text.trimmed());
    g_file.write(line.toUtf8());
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    if (type != QtDebugMsg && type != QtInfoMsg) {
        QMutexLocker lock(&g_mutex);
        writeLocked("qt", msg);
    }
    if (g_previousHandler) g_previousHandler(type, context, msg);
}
} // namespace

void open(const QString& filePath, qint64 maxBytes) {
    {
        QMutexLocker lock(&g_mutex);
        if (g_file.isOpen()) g_file.close();
        g_file.setFileName(filePath);
        g_full = false;
        g_maxBytes = maxBytes;
        g_clock.start();
        if (!g_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;
    }
    if (!g_previousHandler) {
        g_previousHandler = qInstallMessageHandler(qtMessageHandler);
    }
}

void write(const char* category, const QString& text) {
    QMutexLocker lock(&g_mutex);
    writeLocked(category, text);
}

void flush() {
    QMutexLocker lock(&g_mutex);
    if (g_file.isOpen()) g_file.flush();
}

QString path() {
    QMutexLocker lock(&g_mutex);
    return g_file.fileName();
}

} // namespace kvikplay::DiagnosticsLog
