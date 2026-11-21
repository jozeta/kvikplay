#pragma once

#include <QObject>
#include <QPointer>
#include "UpdateLogic.h"

class QNetworkAccessManager;
class QWidget;

namespace kvikplay {

// Checks GitHub for a newer release, asks the user, then downloads, verifies and installs
// it. The automatic check at startup is silent on failure; a check from the About box
// reports every outcome.
//
// Trust: the release JSON comes from api.github.com over TLS; the download must come
// from github.com/<kUpdateRepo>/releases/download/ and match GitHub's SHA-256 digest. On
// macOS the new app must also pass codesign and Gatekeeper and carry the same Developer
// Team ID as the running app, so only builds signed by the same developer install.
class UpdateManager : public QObject {
    Q_OBJECT

public:
    explicit UpdateManager(QWidget* dialogParent, QObject* parent = nullptr);

    void checkForUpdates(bool userInitiated);

signals:
    // Emitted right before the app quits to let the installer replace it.
    void aboutToInstall();

private:
    void onReleaseInfo(const ReleaseInfo& info, bool userInitiated);
    void downloadAndInstall(const ReleaseInfo& info);
    void installDownloaded(const QString& zipPath, const ReleaseInfo& info);
    void showError(const QString& what, const QString& why);
    void openReleasePage(const ReleaseInfo& info);

    QNetworkAccessManager* m_net{nullptr};
    QPointer<QWidget> m_dialogParent;
    bool m_busy{false};
};

} // namespace kvikplay
