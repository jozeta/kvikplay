#pragma once

#include <QString>
#include <QStringList>
#include <vector>
#include <optional>

namespace kvikplay {

enum class FormatCategory {
    CommonVideo,
    OtherVideo,
    Audio
};

QString categoryToString(FormatCategory category);

struct FormatInfo {
    QString extension;       // With dot, e.g. ".mp4"
    QString name;            // User-friendly name, e.g. "MPEG-4 Video"
    FormatCategory category;
    QString mimeType;        // e.g. "video/mp4"
    QString uti;             // macOS Uniform Type Identifier, e.g. "public.mpeg-4"
    QString progId;          // Windows ProgID, e.g. "Kvikplay.mp4"
};

class FormatRegistry {
public:
    static const FormatRegistry& instance();

    const std::vector<FormatInfo>& allFormats() const { return m_formats; }
    std::vector<FormatInfo> formatsByCategory(FormatCategory category) const;

    bool isSupported(const QString& filePathOrExt) const;
    bool isVideo(const QString& filePathOrExt) const;
    bool isAudio(const QString& filePathOrExt) const;

    std::optional<FormatInfo> findFormat(const QString& filePathOrExt) const;

    QString fileFilterString() const;
    QStringList allExtensions() const;

private:
    FormatRegistry();
    static QString extractExtension(const QString& filePathOrExt);

    std::vector<FormatInfo> m_formats;
};

} // namespace kvikplay
