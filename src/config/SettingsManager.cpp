#include "SettingsManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

namespace kvikplay {

SettingsManager& SettingsManager::instance() {
    static SettingsManager s_instance;
    return s_instance;
}

SettingsManager::SettingsManager() {
    load();
}

QString SettingsManager::configFilePath() const {
    // KVIKPLAY_CONFIG_DIR overrides the location (tests, screenshot runs, portable use).
    QString configDir = qEnvironmentVariable("KVIKPLAY_CONFIG_DIR");
    if (configDir.isEmpty()) {
        configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    return configDir + QStringLiteral("/config.json");
}

void SettingsManager::load() {
    QString path = configFilePath();
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return; // Use defaults
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return; // Gracefully keep defaults on corrupted JSON
    }

    QJsonObject obj = doc.object();

    if (obj.contains(QStringLiteral("windowWidth")) && obj.contains(QStringLiteral("windowHeight"))) {
        int w = obj[QStringLiteral("windowWidth")].toInt(960);
        int h = obj[QStringLiteral("windowHeight")].toInt(540);
        if (w >= 160 && h >= 90) {
            m_windowSize = QSize(w, h);
        }
    }

    if (obj.contains(QStringLiteral("windowX")) && obj.contains(QStringLiteral("windowY"))) {
        int x = obj[QStringLiteral("windowX")].toInt(-1);
        int y = obj[QStringLiteral("windowY")].toInt(-1);
        m_windowPosition = QPoint(x, y);
    }

    if (obj.contains(QStringLiteral("volume"))) {
        m_volume = std::clamp(obj[QStringLiteral("volume")].toInt(100), 0, 100);
    }

    if (obj.contains(QStringLiteral("muted"))) {
        m_muted = obj[QStringLiteral("muted")].toBool(false);
    }

    if (obj.contains(QStringLiteral("alwaysOnTop"))) {
        m_alwaysOnTop = obj[QStringLiteral("alwaysOnTop")].toBool(false);
    }

    if (obj.contains(QStringLiteral("playbackSpeed"))) {
        double spd = obj[QStringLiteral("playbackSpeed")].toDouble(1.0);
        if (spd >= 0.25 && spd <= 4.0) {
            m_playbackSpeed = spd;
        }
    }

    if (obj.contains(QStringLiteral("startInFullscreen"))) {
        m_startInFullscreen = obj[QStringLiteral("startInFullscreen")].toBool(false);
    }

    if (obj.contains(QStringLiteral("reverseTrackpadScrubbing"))) {
        m_reverseTrackpadScrubbing = obj[QStringLiteral("reverseTrackpadScrubbing")].toBool(false);
    }

    if (obj.contains(QStringLiteral("dialogueBoost"))) {
        m_dialogueBoost = obj[QStringLiteral("dialogueBoost")].toBool(false);
    }

    if (obj.contains(QStringLiteral("checkForUpdates"))) {
        m_checkForUpdates = obj[QStringLiteral("checkForUpdates")].toBool(true);
    }

    if (obj.contains(QStringLiteral("externalOpen"))) {
        m_externalOpenInNewWindow = obj[QStringLiteral("externalOpen")].toString() == QStringLiteral("newWindow");
    }

    if (obj.contains(QStringLiteral("recentFiles")) && obj[QStringLiteral("recentFiles")].isArray()) {
        m_recentFiles.clear();
        for (const auto& v : obj[QStringLiteral("recentFiles")].toArray()) {
            if (v.isString()) m_recentFiles.append(v.toString());
        }
    }

    const QJsonValue resVal = obj[QStringLiteral("resumePositions")];
    if (resVal.isArray()) {
        // Current format: [{"path": ..., "pos": ...}, ...], oldest first.
        m_resumePositions.clear();
        for (const auto& v : resVal.toArray()) {
            const QJsonObject e = v.toObject();
            const QString path = e[QStringLiteral("path")].toString();
            const double pos = e[QStringLiteral("pos")].toDouble(0.0);
            if (!path.isEmpty() && pos > 0.0) {
                m_resumePositions.emplace_back(path, pos);
            }
        }
    } else if (resVal.isObject()) {
        // 1.0.0 format: {path: pos}. Its order was lost, so keep it as-is.
        m_resumePositions.clear();
        const QJsonObject resObj = resVal.toObject();
        for (auto it = resObj.begin(); it != resObj.end(); ++it) {
            m_resumePositions.emplace_back(it.key(), it.value().toDouble(0.0));
        }
    }
    while (static_cast<int>(m_resumePositions.size()) > kMaxResumePositions) {
        m_resumePositions.erase(m_resumePositions.begin());
    }

    if (obj.contains(QStringLiteral("associations")) && obj[QStringLiteral("associations")].isArray()) {
        m_associatedExtensions.clear();
        QJsonArray arr = obj[QStringLiteral("associations")].toArray();
        for (const auto& val : arr) {
            if (val.isString()) {
                m_associatedExtensions.append(val.toString());
            }
        }
    }
}

void SettingsManager::save() const {
    QString path = configFilePath();
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    QJsonObject obj;
    obj[QStringLiteral("windowWidth")] = m_windowSize.width();
    obj[QStringLiteral("windowHeight")] = m_windowSize.height();
    obj[QStringLiteral("windowX")] = m_windowPosition.x();
    obj[QStringLiteral("windowY")] = m_windowPosition.y();
    obj[QStringLiteral("volume")] = m_volume;
    obj[QStringLiteral("muted")] = m_muted;
    obj[QStringLiteral("alwaysOnTop")] = m_alwaysOnTop;
    obj[QStringLiteral("playbackSpeed")] = m_playbackSpeed;
    obj[QStringLiteral("startInFullscreen")] = m_startInFullscreen;
    obj[QStringLiteral("reverseTrackpadScrubbing")] = m_reverseTrackpadScrubbing;
    obj[QStringLiteral("dialogueBoost")] = m_dialogueBoost;
    obj[QStringLiteral("checkForUpdates")] = m_checkForUpdates;
    obj[QStringLiteral("externalOpen")] = m_externalOpenInNewWindow ? QStringLiteral("newWindow") : QStringLiteral("replace");

    QJsonArray recArr;
    for (const auto& f : m_recentFiles) recArr.append(f);
    obj[QStringLiteral("recentFiles")] = recArr;

    QJsonArray resArr;
    for (const auto& [path, pos] : m_resumePositions) {
        resArr.append(QJsonObject{{QStringLiteral("path"), path}, {QStringLiteral("pos"), pos}});
    }
    obj[QStringLiteral("resumePositions")] = resArr;

    QJsonArray arr;
    for (const auto& ext : m_associatedExtensions) {
        arr.append(ext);
    }
    obj[QStringLiteral("associations")] = arr;

    // Write to a temporary file and rename it into place, so a crash or a
    // second instance can never leave a half-written config.json behind.
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        file.commit();
    }
}

void SettingsManager::setWindowSize(const QSize& size) {
    if (size.isValid() && size != m_windowSize) {
        m_windowSize = size;
    }
}

void SettingsManager::setWindowPosition(const QPoint& pos) {
    m_windowPosition = pos;
}

void SettingsManager::setVolume(int vol) {
    int clamped = std::clamp(vol, 0, 100);
    if (m_volume != clamped) {
        m_volume = clamped;
        emit volumeChanged(m_volume);
    }
}

void SettingsManager::setMuted(bool muted) {
    if (m_muted != muted) {
        m_muted = muted;
        emit muteChanged(m_muted);
    }
}

void SettingsManager::setAlwaysOnTop(bool top) {
    if (m_alwaysOnTop != top) {
        m_alwaysOnTop = top;
        emit alwaysOnTopChanged(m_alwaysOnTop);
    }
}

void SettingsManager::setPlaybackSpeed(double speed) {
    if (speed >= 0.25 && speed <= 4.0) {
        m_playbackSpeed = speed;
    }
}

void SettingsManager::setStartInFullscreen(bool fullscreen) {
    m_startInFullscreen = fullscreen;
}

void SettingsManager::setReverseTrackpadScrubbing(bool reverse) {
    if (m_reverseTrackpadScrubbing != reverse) {
        m_reverseTrackpadScrubbing = reverse;
        emit reverseTrackpadScrubbingChanged(m_reverseTrackpadScrubbing);
    }
}

double SettingsManager::resumePosition(const QString& filePath) const {
    for (const auto& [path, pos] : m_resumePositions) {
        if (path == filePath) return pos;
    }
    return 0.0;
}

void SettingsManager::setResumePosition(const QString& filePath, double pos) {
    if (filePath.isEmpty()) return;
    clearResumePosition(filePath);
    if (pos > 5.0) {
        m_resumePositions.emplace_back(filePath, pos); // newest last
        while (static_cast<int>(m_resumePositions.size()) > kMaxResumePositions) {
            m_resumePositions.erase(m_resumePositions.begin());
        }
    }
}

void SettingsManager::clearResumePosition(const QString& filePath) {
    std::erase_if(m_resumePositions, [&](const auto& e) { return e.first == filePath; });
}

void SettingsManager::addRecentFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    m_recentFiles.removeAll(filePath);
    m_recentFiles.prepend(filePath);
    while (m_recentFiles.size() > 8) {
        m_recentFiles.removeLast();
    }
    emit recentFilesChanged();
}

void SettingsManager::removeRecentFile(const QString& filePath) {
    if (m_recentFiles.removeAll(filePath) > 0) {
        emit recentFilesChanged();
    }
}

void SettingsManager::clearRecentFiles() {
    m_recentFiles.clear();
    emit recentFilesChanged();
}

void SettingsManager::setDialogueBoost(bool boost) {
    if (m_dialogueBoost != boost) {
        m_dialogueBoost = boost;
        emit dialogueBoostChanged(m_dialogueBoost);
    }
}

void SettingsManager::setAssociatedExtensions(const QStringList& extensions) {
    m_associatedExtensions = extensions;
}

} // namespace kvikplay
