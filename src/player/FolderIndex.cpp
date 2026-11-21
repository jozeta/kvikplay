#include "FolderIndex.h"
#include "formats/FormatRegistry.h"
#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QThreadPool>
#include <algorithm>

namespace kvikplay {

QStringList FolderIndex::listMediaFiles(const QString& dirPath) {
    QDir dir(dirPath);
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::NoSort);

    QStringList mediaFiles;
    for (const auto& entry : entries) {
        if (FormatRegistry::instance().isSupported(entry.fileName())) {
            mediaFiles.push_back(entry.absoluteFilePath());
        }
    }

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(mediaFiles.begin(), mediaFiles.end(), [&collator](const QString& a, const QString& b) {
        return collator.compare(QFileInfo(a).fileName(), QFileInfo(b).fileName()) < 0;
    });
    return mediaFiles;
}

QStringList FolderIndex::files(const QString& dirPath) {
    const QString key = QDir(dirPath).absolutePath();
    const QDateTime modified = QFileInfo(key).lastModified(); // one stat, cheap
    {
        QMutexLocker locker(&m_mutex);
        auto it = m_cache.constFind(key);
        if (it != m_cache.constEnd() && it->modified == modified) {
            return it->files;
        }
    }
    QStringList list = listMediaFiles(key);
    QMutexLocker locker(&m_mutex);
    m_cache.insert(key, Entry{modified, list});
    return list;
}

void FolderIndex::prefetch(const QString& dirPath) {
    // Our own pool (see the header): it is waited for before this index is destroyed.
    m_pool.setMaxThreadCount(1);
    m_pool.start([this, dirPath]() { files(dirPath); });
}

QString FolderIndex::next(const QString& filePath) {
    const QFileInfo fi(filePath);
    const QStringList list = files(fi.absolutePath());
    const qsizetype idx = list.indexOf(fi.absoluteFilePath());
    return (idx >= 0 && idx + 1 < list.size()) ? list[idx + 1] : QString();
}

QString FolderIndex::previous(const QString& filePath) {
    const QFileInfo fi(filePath);
    const QStringList list = files(fi.absolutePath());
    const qsizetype idx = list.indexOf(fi.absoluteFilePath());
    return (idx > 0) ? list[idx - 1] : QString();
}

} // namespace kvikplay
