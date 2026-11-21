#include "FormatRegistry.h"
#include <QFileInfo>

namespace kvikplay {

QString categoryToString(FormatCategory category) {
    switch (category) {
    case FormatCategory::CommonVideo: return QStringLiteral("Common Video");
    case FormatCategory::OtherVideo:  return QStringLiteral("Other Video");
    case FormatCategory::Audio:       return QStringLiteral("Audio");
    }
    return QStringLiteral("Unknown");
}

const FormatRegistry& FormatRegistry::instance() {
    static const FormatRegistry s_instance;
    return s_instance;
}

FormatRegistry::FormatRegistry() {
    // Common video formats
    m_formats.push_back({QStringLiteral(".mp4"),  QStringLiteral("MPEG-4 Video"),            FormatCategory::CommonVideo, QStringLiteral("video/mp4"),                 QStringLiteral("public.mpeg-4"),                QStringLiteral("Kvikplay.mp4")});
    m_formats.push_back({QStringLiteral(".m4v"),  QStringLiteral("Apple MPEG-4 Video"),      FormatCategory::CommonVideo, QStringLiteral("video/x-m4v"),              QStringLiteral("com.apple.m4v-video"),          QStringLiteral("Kvikplay.m4v")});
    m_formats.push_back({QStringLiteral(".mkv"),  QStringLiteral("Matroska Video"),          FormatCategory::CommonVideo, QStringLiteral("video/x-matroska"),          QStringLiteral("org.matroska.mkv"),             QStringLiteral("Kvikplay.mkv")});
    m_formats.push_back({QStringLiteral(".avi"),  QStringLiteral("Audio Video Interleave"),  FormatCategory::CommonVideo, QStringLiteral("video/x-msvideo"),           QStringLiteral("public.avi"),                   QStringLiteral("Kvikplay.avi")});
    m_formats.push_back({QStringLiteral(".mov"),  QStringLiteral("QuickTime Movie"),         FormatCategory::CommonVideo, QStringLiteral("video/quicktime"),           QStringLiteral("com.apple.quicktime-movie"),    QStringLiteral("Kvikplay.mov")});
    m_formats.push_back({QStringLiteral(".webm"), QStringLiteral("WebM Video"),              FormatCategory::CommonVideo, QStringLiteral("video/webm"),                QStringLiteral("org.webmproject.webm"),         QStringLiteral("Kvikplay.webm")});

    // Other video formats
    m_formats.push_back({QStringLiteral(".ogv"),  QStringLiteral("Ogg Video"),               FormatCategory::OtherVideo,  QStringLiteral("video/ogg"),                 QStringLiteral("org.xiph.ogv"),                 QStringLiteral("Kvikplay.ogv")});
    m_formats.push_back({QStringLiteral(".3gp"),  QStringLiteral("3GPP Multimedia"),         FormatCategory::OtherVideo,  QStringLiteral("video/3gpp"),                QStringLiteral("public.3gpp"),                  QStringLiteral("Kvikplay.3gp")});
    m_formats.push_back({QStringLiteral(".3g2"),  QStringLiteral("3GPP2 Multimedia"),        FormatCategory::OtherVideo,  QStringLiteral("video/3gpp2"),               QStringLiteral("public.3gpp2"),                 QStringLiteral("Kvikplay.3g2")});
    m_formats.push_back({QStringLiteral(".mts"),  QStringLiteral("AVCHD Video"),             FormatCategory::OtherVideo,  QStringLiteral("video/mp2t"),                QStringLiteral("public.avchd-transport-stream"),QStringLiteral("Kvikplay.mts")});
    m_formats.push_back({QStringLiteral(".m2ts"), QStringLiteral("BDAV MPEG-2 Video"),       FormatCategory::OtherVideo,  QStringLiteral("video/mp2t"),                QStringLiteral("public.avchd-transport-stream"),QStringLiteral("Kvikplay.m2ts")});
    m_formats.push_back({QStringLiteral(".ts"),   QStringLiteral("MPEG Transport Stream"),   FormatCategory::OtherVideo,  QStringLiteral("video/mp2t"),                QStringLiteral("public.mpeg-ts"),               QStringLiteral("Kvikplay.ts")});
    m_formats.push_back({QStringLiteral(".m2v"),  QStringLiteral("MPEG-2 Video"),            FormatCategory::OtherVideo,  QStringLiteral("video/mpeg"),                QStringLiteral("public.mpeg-2-video"),          QStringLiteral("Kvikplay.m2v")});
    m_formats.push_back({QStringLiteral(".vob"),  QStringLiteral("DVD Video Object"),        FormatCategory::OtherVideo,  QStringLiteral("video/x-ms-vob"),            QStringLiteral("org.videolan.vob"),             QStringLiteral("Kvikplay.vob")});
    m_formats.push_back({QStringLiteral(".mpg"),  QStringLiteral("MPEG-1/2 Video"),          FormatCategory::OtherVideo,  QStringLiteral("video/mpeg"),                QStringLiteral("public.mpeg"),                  QStringLiteral("Kvikplay.mpg")});
    m_formats.push_back({QStringLiteral(".mpeg"), QStringLiteral("MPEG-1/2 Video"),          FormatCategory::OtherVideo,  QStringLiteral("video/mpeg"),                QStringLiteral("public.mpeg"),                  QStringLiteral("Kvikplay.mpeg")});
    m_formats.push_back({QStringLiteral(".wmv"),  QStringLiteral("Windows Media Video"),     FormatCategory::OtherVideo,  QStringLiteral("video/x-ms-wmv"),            QStringLiteral("com.microsoft.windows-media-wmv"), QStringLiteral("Kvikplay.wmv")});
    m_formats.push_back({QStringLiteral(".asf"),  QStringLiteral("Advanced Systems Format"), FormatCategory::OtherVideo,  QStringLiteral("video/x-ms-asf"),            QStringLiteral("com.microsoft.windows-media-asf"), QStringLiteral("Kvikplay.asf")});
    m_formats.push_back({QStringLiteral(".flv"),  QStringLiteral("Flash Video"),             FormatCategory::OtherVideo,  QStringLiteral("video/x-flv"),               QStringLiteral("com.adobe.flash.video"),        QStringLiteral("Kvikplay.flv")});

    // Audio formats
    m_formats.push_back({QStringLiteral(".mp3"),  QStringLiteral("MP3 Audio"),               FormatCategory::Audio,       QStringLiteral("audio/mpeg"),                QStringLiteral("public.mp3"),                   QStringLiteral("Kvikplay.mp3")});
    m_formats.push_back({QStringLiteral(".wav"),  QStringLiteral("Waveform Audio"),          FormatCategory::Audio,       QStringLiteral("audio/wav"),                 QStringLiteral("com.microsoft.waveform-audio"), QStringLiteral("Kvikplay.wav")});
    m_formats.push_back({QStringLiteral(".flac"), QStringLiteral("Free Lossless Audio"),     FormatCategory::Audio,       QStringLiteral("audio/flac"),                QStringLiteral("org.xiph.flac"),                QStringLiteral("Kvikplay.flac")});
    m_formats.push_back({QStringLiteral(".aac"),  QStringLiteral("Advanced Audio Coding"),   FormatCategory::Audio,       QStringLiteral("audio/aac"),                 QStringLiteral("public.aac-audio"),             QStringLiteral("Kvikplay.aac")});
    m_formats.push_back({QStringLiteral(".m4a"),  QStringLiteral("Apple MPEG-4 Audio"),      FormatCategory::Audio,       QStringLiteral("audio/mp4"),                 QStringLiteral("public.m4a-audio"),             QStringLiteral("Kvikplay.m4a")});
    m_formats.push_back({QStringLiteral(".ogg"),  QStringLiteral("Ogg Vorbis Audio"),        FormatCategory::Audio,       QStringLiteral("audio/ogg"),                 QStringLiteral("org.xiph.ogg-audio"),           QStringLiteral("Kvikplay.ogg")});
    m_formats.push_back({QStringLiteral(".opus"), QStringLiteral("Opus Audio"),              FormatCategory::Audio,       QStringLiteral("audio/opus"),                QStringLiteral("org.xiph.opus"),                QStringLiteral("Kvikplay.opus")});
    m_formats.push_back({QStringLiteral(".wma"),  QStringLiteral("Windows Media Audio"),     FormatCategory::Audio,       QStringLiteral("audio/x-ms-wma"),            QStringLiteral("com.microsoft.windows-media-wma"), QStringLiteral("Kvikplay.wma")});

    for (const auto& fmt : m_formats) {
        m_supportedExtensions.insert(fmt.extension.toLower());
    }
}

QString FormatRegistry::extractExtension(const QString& filePathOrExt) {
    if (filePathOrExt.isEmpty()) {
        return QString();
    }
    if (filePathOrExt.startsWith('.')) {
        return filePathOrExt.toLower();
    }
    QFileInfo fi(filePathOrExt);
    QString s = fi.suffix();
    if (s.isEmpty()) {
        return QString();
    }
    return QStringLiteral(".") + s.toLower();
}

std::vector<FormatInfo> FormatRegistry::formatsByCategory(FormatCategory category) const {
    std::vector<FormatInfo> res;
    for (const auto& item : m_formats) {
        if (item.category == category) {
            res.push_back(item);
        }
    }
    return res;
}

bool FormatRegistry::isSupported(const QString& filePathOrExt) const {
    // A NUL would pass the extension check but reach mpv as a C string cut short
    // at the NUL ("x.m3u\0.mkv" -> "x.m3u"), so such a path is never supported.
    if (filePathOrExt.contains(QChar(0))) return false;
    return m_supportedExtensions.contains(extractExtension(filePathOrExt));
}

bool FormatRegistry::isVideo(const QString& filePathOrExt) const {
    auto fmt = findFormat(filePathOrExt);
    return fmt.has_value() && (fmt->category == FormatCategory::CommonVideo || fmt->category == FormatCategory::OtherVideo);
}

bool FormatRegistry::isAudio(const QString& filePathOrExt) const {
    auto fmt = findFormat(filePathOrExt);
    return fmt.has_value() && fmt->category == FormatCategory::Audio;
}

std::optional<FormatInfo> FormatRegistry::findFormat(const QString& filePathOrExt) const {
    QString ext = extractExtension(filePathOrExt);
    if (ext.isEmpty()) return std::nullopt;

    for (const auto& item : m_formats) {
        if (item.extension.compare(ext, Qt::CaseInsensitive) == 0) {
            return item;
        }
    }
    return std::nullopt;
}

QStringList FormatRegistry::allExtensions() const {
    QStringList exts;
    for (const auto& item : m_formats) {
        exts.append(item.extension);
    }
    return exts;
}

QString FormatRegistry::fileFilterString() const {
    QStringList allGlobs;
    QStringList videoGlobs;
    QStringList audioGlobs;

    for (const auto& item : m_formats) {
        QString glob = QStringLiteral("*") + item.extension;
        allGlobs.append(glob);
        if (item.category == FormatCategory::Audio) {
            audioGlobs.append(glob);
        } else {
            videoGlobs.append(glob);
        }
    }

    return QStringLiteral("All Supported Media (%1);;Video Files (%2);;Audio Files (%3);;All Files (*.*)")
        .arg(allGlobs.join(QStringLiteral(" ")),
             videoGlobs.join(QStringLiteral(" ")),
             audioGlobs.join(QStringLiteral(" ")));
}

} // namespace kvikplay
