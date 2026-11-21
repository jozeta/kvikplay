#pragma once

#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QString>
#include <QStringList>

namespace kvikplay {

// Supported media files of a folder in natural order ("Episode 2" before "Episode 10"),
// cached per folder and refreshed when the folder's modification time changes.
// prefetch() fills the cache on a background thread so next/previous never list a
// (possibly network) folder on the GUI thread.
class FolderIndex {
public:
    static QStringList listMediaFiles(const QString& dirPath);

    QStringList files(const QString& dirPath);
    void prefetch(const QString& dirPath);

    // Neighbours of filePath in its folder; empty when there is none.
    QString next(const QString& filePath);
    QString previous(const QString& filePath);

private:
    struct Entry {
        QDateTime modified;
        QStringList files;
    };
    QMutex m_mutex;
    QHash<QString, Entry> m_cache;
};

} // namespace kvikplay
