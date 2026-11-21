#pragma once

#include <QString>
#include <functional>

namespace kvikplay {

// Windows: one player window per user session unless the user picks "Open in New
// Window". A second launch sends its file to the running instance and exits.
// macOS gets this from the OS (one process, QFileOpenEvent), so every function is a
// no-op there.
namespace SingleInstance {

// True when another Kvikplay instance is already running (Windows only).
bool anotherInstanceRunning();

// Sends filePath to the running instance. Returns false when there is none or it
// did not answer, so the caller can start normally instead.
bool sendToRunningInstance(const QString& filePath);

// Receives files sent by later launches. The callback runs on the GUI thread.
void listen(std::function<void(const QString&)> onFile);

} // namespace SingleInstance
} // namespace kvikplay
