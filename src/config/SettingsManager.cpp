#include "SettingsManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
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
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
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

    if (obj.contains(QStringLiteral("recentFiles")) && obj[QStringLiteral("recentFiles")].isArray()) {
        m_recentFiles.clear();
        for (const auto& v : obj[QStringLiteral("recentFiles")].toArray()) {
            if (v.isString()) m_recentFiles.append(v.toString());
        }
    }

    if (obj.contains(QStringLiteral("resumePositions")) && obj[QStringLiteral("resumePositions")].isObject()) {
        m_resumePositions.clear();
        QJsonObject resObj = obj[QStringLiteral("resumePositions")].toObject();
        for (auto it = resObj.begin(); it != resObj.end(); ++it) {
            m_resumePositions.insert(it.key(), it.value().toDouble(0.0));
        }
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

    QJsonArray recArr;
    for (const auto& f : m_recentFiles) recArr.append(f);
    obj[QStringLiteral("recentFiles")] = recArr;

    QJsonObject resObj;
    for (auto it = m_resumePositions.begin(); it != m_resumePositions.end(); ++it) {
        resObj.insert(it.key(), it.value());
    }
    obj[QStringLiteral("resumePositions")] = resObj;

    QJsonArray arr;
    for (const auto& ext : m_associatedExtensions) {
        arr.append(ext);
    }
    obj[QStringLiteral("associations")] = arr;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Compact));
        file.close();
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
    return m_resumePositions.value(filePath, 0.0);
}

void SettingsManager::setResumePosition(const QString& filePath, double pos) {
    if (filePath.isEmpty()) return;
    if (pos > 5.0) {
        m_resumePositions.insert(filePath, pos);
        if (m_resumePositions.size() > 100) {
            m_resumePositions.erase(m_resumePositions.begin());
        }
    } else {
        m_resumePositions.remove(filePath);
    }
}

void SettingsManager::clearResumePosition(const QString& filePath) {
    m_resumePositions.remove(filePath);
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
