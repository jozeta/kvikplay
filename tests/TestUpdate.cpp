#include "TestCheck.h"
#include <QCoreApplication>
#include <iostream>
#include "update/UpdateLogic.h"

using namespace kvikplay;

static QByteArray release(const char* tag, const char* assetName, const char* url,
                          const char* digest, bool prerelease = false) {
    return QStringLiteral(R"({"tag_name":"%1","prerelease":%2,"draft":false,
        "html_url":"https://github.com/jozeta/kvikplay/releases/tag/%1",
        "body":"Fixes <b>things</b>",
        "assets":[{"name":"%3","browser_download_url":"%4","size":123,"digest":"%5"}]})")
        .arg(QString::fromUtf8(tag), prerelease ? QStringLiteral("true") : QStringLiteral("false"),
             QString::fromUtf8(assetName), QString::fromUtf8(url), QString::fromUtf8(digest))
        .toUtf8();
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "[TestUpdate] Version comparison..." << std::endl;
    CHECK(isNewerVersion(QStringLiteral("v1.1.0"), QStringLiteral("1.0.0")));
    CHECK(isNewerVersion(QStringLiteral("1.10.0"), QStringLiteral("1.9.3")));   // numeric, not text
    CHECK(!isNewerVersion(QStringLiteral("v1.0.0"), QStringLiteral("1.0.0")));
    CHECK(!isNewerVersion(QStringLiteral("v0.9.9"), QStringLiteral("1.0.0")));
    CHECK(!isNewerVersion(QStringLiteral("nightly"), QStringLiteral("1.0.0"))); // unparsable: never newer

    const QString mac = QStringLiteral("macOS-arm64");
    const char* sha = "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    std::cout << "[TestUpdate] Release parsing picks this platform's asset..." << std::endl;
    auto r = parseLatestRelease(release("v1.2.0", "Kvikplay-1.2.0-macOS-arm64.zip",
        "https://github.com/jozeta/kvikplay/releases/download/v1.2.0/Kvikplay-1.2.0-macOS-arm64.zip", sha), mac);
    CHECK(r.has_value());
    CHECK(r->version == QStringLiteral("1.2.0"));
    CHECK(r->assetName == QStringLiteral("Kvikplay-1.2.0-macOS-arm64.zip"));
    CHECK(r->sha256 == QStringLiteral("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"));
    CHECK(r->notes == QStringLiteral("Fixes <b>things</b>")); // raw text; shown as plain text

    std::cout << "[TestUpdate] No asset for this platform: release page only..." << std::endl;
    auto w = parseLatestRelease(release("v1.2.0", "Kvikplay-1.2.0-macOS-arm64.zip",
        "https://github.com/jozeta/kvikplay/releases/download/v1.2.0/x.zip", sha), QStringLiteral("Windows-x64"));
    CHECK(w.has_value());
    CHECK(w->assetUrl.isEmpty());
    CHECK(w->pageUrl.startsWith(QStringLiteral("https://github.com/jozeta/kvikplay/")));

    std::cout << "[TestUpdate] Untrusted download URLs and missing checksums are refused..." << std::endl;
    auto evil = parseLatestRelease(release("v1.2.0", "Kvikplay-1.2.0-macOS-arm64.zip",
        "https://evil.example/Kvikplay.zip", sha), mac);
    CHECK(evil.has_value() && evil->assetUrl.isEmpty());
    auto plain = parseLatestRelease(release("v1.2.0", "Kvikplay-1.2.0-macOS-arm64.zip",
        "http://github.com/jozeta/kvikplay/releases/download/v1.2.0/Kvikplay-1.2.0-macOS-arm64.zip", sha), mac);
    CHECK(plain.has_value() && plain->assetUrl.isEmpty());
    auto nodigest = parseLatestRelease(release("v1.2.0", "Kvikplay-1.2.0-macOS-arm64.zip",
        "https://github.com/jozeta/kvikplay/releases/download/v1.2.0/Kvikplay-1.2.0-macOS-arm64.zip", ""), mac);
    CHECK(nodigest.has_value() && nodigest->assetUrl.isEmpty());

    std::cout << "[TestUpdate] Pre-releases and garbage are ignored..." << std::endl;
    CHECK(!parseLatestRelease(release("v2.0.0", "a.zip", "https://github.com/a", sha, true), mac).has_value());
    CHECK(!parseLatestRelease(QByteArray("not json"), mac).has_value());

    std::cout << "[TestUpdate] Team ID from codesign output..." << std::endl;
    CHECK(teamIdentifierFromCodesign(QStringLiteral(
        "Executable=/x\nAuthority=Developer ID Application: A B (6W6TD6P8G3)\nTeamIdentifier=6W6TD6P8G3\n"))
          == QStringLiteral("6W6TD6P8G3"));
    CHECK(teamIdentifierFromCodesign(QStringLiteral("TeamIdentifier=not set\n")).isEmpty());

    std::cout << "[TestUpdate] All tests passed!" << std::endl;
    return 0;
}
