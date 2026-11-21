#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

namespace kvikplay {

// Where releases are published. Only downloads from this repository are accepted.
inline constexpr const char* kUpdateRepo = "jozeta/kvikplay";

struct ReleaseInfo {
    QString version;   // "1.2.0" (tag without the leading v)
    QString notes;     // release body, raw: always show as plain text
    QString pageUrl;   // release page on github.com
    QString assetName; // empty when there is no trusted asset for this platform
    QString assetUrl;  // https://github.com/<repo>/releases/download/... or empty
    QString sha256;    // lowercase hex from GitHub's asset digest
};

// True when tag (e.g. "v1.2.0") is a newer x.y.z version than current. Unparsable
// tags are never newer.
bool isNewerVersion(const QString& tag, const QString& current);

// Parses the GitHub "latest release" JSON. Drafts, pre-releases and invalid JSON give
// nullopt. The asset for platformSuffix ("macOS-arm64", "Windows-x64") is only filled in
// when its URL is an https download from kUpdateRepo and GitHub supplied a SHA-256 digest;
// otherwise assetUrl stays empty and the caller can only offer the release page.
std::optional<ReleaseInfo> parseLatestRelease(const QByteArray& json, const QString& platformSuffix);

// "TeamIdentifier=XXXX" from `codesign -dv` output; empty when absent or "not set".
QString teamIdentifierFromCodesign(const QString& codesignOutput);

// The asset suffix for automatic installs ("macOS-arm64"); empty where updates can't be
// verified and installed automatically (Windows, Intel Macs): the release page opens instead.
QString currentPlatformSuffix();

} // namespace kvikplay
