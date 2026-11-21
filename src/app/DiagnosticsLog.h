#pragma once

#include <QString>

namespace kvikplay {

// A small local log for diagnosing problems on a user's machine (e.g. a black picture on
// one PC): mpv's warnings and errors, Qt's warnings and Kvikplay's own key steps, with
// milliseconds since start. Written to kvikplay.log next to config.json, replaced at every
// start, capped in size, never sent anywhere. Thread-safe.
namespace DiagnosticsLog {

void open(const QString& path, qint64 maxBytes = 1024 * 1024);
void write(const char* category, const QString& text);
void flush();
QString path();

} // namespace DiagnosticsLog
} // namespace kvikplay
