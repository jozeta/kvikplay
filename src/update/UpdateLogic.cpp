#include "UpdateLogic.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSysInfo>
#include <QVersionNumber>

namespace kvikplay {

namespace {
std::optional<QVersionNumber> parseVersion(QString text) {
    if (text.startsWith(QLatin1Char('v')) || text.startsWith(QLatin1Char('V'))) text.remove(0, 1);
    static const QRegularExpression strict(QStringLiteral("^\\d+\\.\\d+\\.\\d+$"));
    if (!strict.match(text).hasMatch()) return std::nullopt;
    return QVersionNumber::fromString(text);
}
} // namespace

bool isNewerVersion(const QString& tag, const QString& current) {
    const auto remote = parseVersion(tag);
    const auto local = parseVersion(current);
    return remote && local && *remote > *local;
}

std::optional<ReleaseInfo> parseLatestRelease(const QByteArray& json, const QString& platformSuffix) {
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return std::nullopt;
    const QJsonObject rel = doc.object();
    if (rel.value(QStringLiteral("draft")).toBool() || rel.value(QStringLiteral("prerelease")).toBool()) {
        return std::nullopt;
    }
    const auto version = parseVersion(rel.value(QStringLiteral("tag_name")).toString());
    if (!version) return std::nullopt;

    ReleaseInfo info;
    info.version = version->toString();
    info.notes = rel.value(QStringLiteral("body")).toString();
    const QString repoPage = QStringLiteral("https://github.com/%1/").arg(QLatin1String(kUpdateRepo));
    const QString page = rel.value(QStringLiteral("html_url")).toString();
    info.pageUrl = page.startsWith(repoPage) ? page : repoPage + QStringLiteral("releases/latest");

    if (platformSuffix.isEmpty()) return info;
    const QString wanted = QStringLiteral("Kvikplay-%1-%2.zip").arg(info.version, platformSuffix);
    const QString trustedPrefix =
        QStringLiteral("https://github.com/%1/releases/download/").arg(QLatin1String(kUpdateRepo));
    static const QRegularExpression hex64(QStringLiteral("^[0-9a-f]{64}$"));

    for (const QJsonValue& v : rel.value(QStringLiteral("assets")).toArray()) {
        const QJsonObject a = v.toObject();
        if (a.value(QStringLiteral("name")).toString() != wanted) continue;
        const QString url = a.value(QStringLiteral("browser_download_url")).toString();
        QString digest = a.value(QStringLiteral("digest")).toString().toLower();
        if (!digest.startsWith(QStringLiteral("sha256:"))) break;
        digest.remove(0, 7);
        if (!url.startsWith(trustedPrefix) || !hex64.match(digest).hasMatch()) break;
        info.assetName = wanted;
        info.assetUrl = url;
        info.sha256 = digest;
        break;
    }
    return info;
}

QString teamIdentifierFromCodesign(const QString& codesignOutput) {
    static const QRegularExpression re(QStringLiteral("^TeamIdentifier=([A-Z0-9]{10})$"),
                                       QRegularExpression::MultilineOption);
    const auto m = re.match(codesignOutput);
    return m.hasMatch() ? m.captured(1) : QString();
}

QString currentPlatformSuffix() {
#if defined(Q_OS_MACOS)
    return QSysInfo::currentCpuArchitecture() == QStringLiteral("arm64") ? QStringLiteral("macOS-arm64")
                                                                         : QString();
#elif defined(Q_OS_WIN)
    return QSysInfo::currentCpuArchitecture() == QStringLiteral("x86_64") ? QStringLiteral("Windows-x64")
                                                                          : QString();
#else
    return QString();
#endif
}

} // namespace kvikplay
