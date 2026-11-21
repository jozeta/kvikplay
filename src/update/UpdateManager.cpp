#include "UpdateManager.h"
#include "version.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>
#include <memory>

namespace kvikplay {

namespace {

QNetworkRequest githubRequest(const QUrl& url) {
    QNetworkRequest req(url);
    // GitHub requires a User-Agent; a generic one, nothing about the user.
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Kvikplay/" KVIKPLAY_VERSION));
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(30000); // 30 s without data
    return req;
}

#ifdef Q_OS_MACOS
// Runs a tool and returns its exit code; output (stdout+stderr) in *out.
int run(const QString& program, const QStringList& args, QString* out = nullptr) {
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(program, args);
    if (!p.waitForFinished(120000)) return -1;
    if (out) *out = QString::fromUtf8(p.readAll());
    return p.exitStatus() == QProcess::NormalExit ? p.exitCode() : -1;
}

QString teamIdOf(const QString& bundle) {
    QString out;
    run(QStringLiteral("/usr/bin/codesign"), {QStringLiteral("-dv"), QStringLiteral("--verbose=2"), bundle}, &out);
    return teamIdentifierFromCodesign(out);
}

QString runningBundlePath() {
    QDir dir(QCoreApplication::applicationDirPath()); // .../kvikplay.app/Contents/MacOS
    dir.cdUp();
    dir.cdUp();
    return dir.absolutePath().endsWith(QStringLiteral(".app")) ? dir.absolutePath() : QString();
}
#endif

} // namespace

UpdateManager::UpdateManager(QWidget* dialogParent, QObject* parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
    , m_dialogParent(dialogParent) {}

void UpdateManager::checkForUpdates(bool userInitiated) {
    if (m_busy) return;
    m_busy = true;
    const QUrl api(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(QLatin1String(kUpdateRepo)));
    QNetworkReply* reply = m_net->get(githubRequest(api));
    connect(reply, &QNetworkReply::finished, this, [this, reply, userInitiated]() {
        reply->deleteLater();
        m_busy = false;
        if (reply->error() != QNetworkReply::NoError) {
            if (userInitiated) {
                showError(QStringLiteral("Couldn't check for updates."),
                          QStringLiteral("%1\n\nCheck your internet connection and try again.").arg(reply->errorString()));
            }
            return;
        }
        const auto info = parseLatestRelease(reply->readAll(), currentPlatformSuffix());
        if (!info) {
            if (userInitiated) {
                showError(QStringLiteral("Couldn't check for updates."),
                          QStringLiteral("GitHub sent an answer Kvikplay doesn't understand. Try again later."));
            }
            return;
        }
        onReleaseInfo(*info, userInitiated);
    });
}

void UpdateManager::onReleaseInfo(const ReleaseInfo& info, bool userInitiated) {
    if (!isNewerVersion(info.version, QStringLiteral(KVIKPLAY_VERSION))) {
        if (userInitiated) {
            QMessageBox::information(m_dialogParent, QStringLiteral("No update available"),
                                     QStringLiteral("You're using the latest version of Kvikplay (%1).")
                                         .arg(QStringLiteral(KVIKPLAY_VERSION)));
        }
        return;
    }

    QMessageBox box(m_dialogParent);
    box.setWindowTitle(QStringLiteral("Update available"));
    box.setTextFormat(Qt::PlainText);
    box.setText(QStringLiteral("Kvikplay %1 is available. You have %2.")
                    .arg(info.version, QStringLiteral(KVIKPLAY_VERSION)));
    if (!info.notes.trimmed().isEmpty()) {
        box.setDetailedText(info.notes); // plain text: release notes are not rendered as HTML
    }

    QPushButton* primary = nullptr;
    if (!info.assetUrl.isEmpty()) {
        box.setInformativeText(QStringLiteral("Do you want to download and install it now? "
                                              "Kvikplay restarts when the update is installed."));
        primary = box.addButton(QStringLiteral("Install Update"), QMessageBox::AcceptRole);
    } else {
        box.setInformativeText(QStringLiteral("There is no automatic update for this computer yet. "
                                              "Open the download page to get it."));
        primary = box.addButton(QStringLiteral("Open Download Page"), QMessageBox::AcceptRole);
    }
    box.addButton(QStringLiteral("Not Now"), QMessageBox::RejectRole);
    box.setDefaultButton(primary);
    box.exec();
    if (box.clickedButton() != primary) return;

    if (info.assetUrl.isEmpty()) {
        openReleasePage(info);
    } else {
        downloadAndInstall(info);
    }
}

void UpdateManager::downloadAndInstall(const ReleaseInfo& info) {
    auto tmp = std::make_shared<QTemporaryDir>(QDir::tempPath() + QStringLiteral("/kvikplay-update-XXXXXX"));
    if (!tmp->isValid()) {
        showError(QStringLiteral("Couldn't download the update."), QStringLiteral("No temporary folder is available."));
        return;
    }
    tmp->setAutoRemove(false); // the installer helper cleans it up after the swap
    const QString zipPath = tmp->filePath(info.assetName);
    auto file = std::make_shared<QFile>(zipPath);
    if (!file->open(QIODevice::WriteOnly)) {
        showError(QStringLiteral("Couldn't download the update."), file->errorString());
        return;
    }
    auto hash = std::make_shared<QCryptographicHash>(QCryptographicHash::Sha256);

    auto* progress = new QProgressDialog(QStringLiteral("Downloading Kvikplay %1…").arg(info.version),
                                         QStringLiteral("Cancel"), 0, 100, m_dialogParent);
    progress->setWindowTitle(QStringLiteral("Updating Kvikplay"));
    progress->setMinimumDuration(0);
    progress->setAutoClose(false);
    progress->setValue(0);

    m_busy = true;
    QNetworkReply* reply = m_net->get(githubRequest(QUrl(info.assetUrl)));
    connect(reply, &QNetworkReply::readyRead, this, [reply, file, hash]() {
        const QByteArray chunk = reply->readAll();
        hash->addData(chunk);
        file->write(chunk);
    });
    connect(reply, &QNetworkReply::downloadProgress, progress, [progress](qint64 got, qint64 total) {
        if (total > 0) progress->setValue(static_cast<int>(got * 100 / total));
    });
    connect(progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, hash, progress, info, zipPath, tmp]() {
        reply->deleteLater();
        progress->deleteLater();
        m_busy = false;
        const QByteArray rest = reply->readAll();
        hash->addData(rest);
        file->write(rest);
        file->close();

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            QDir(tmp->path()).removeRecursively();
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            QDir(tmp->path()).removeRecursively();
            showError(QStringLiteral("Couldn't download the update."),
                      QStringLiteral("%1\n\nTry again later.").arg(reply->errorString()));
            return;
        }
        if (QString::fromLatin1(hash->result().toHex()) != info.sha256) {
            QDir(tmp->path()).removeRecursively();
            showError(QStringLiteral("The update was not installed."),
                      QStringLiteral("The downloaded file doesn't match the checksum GitHub published for it, "
                                     "so it may be damaged or tampered with. Nothing was changed."));
            return;
        }
        installDownloaded(zipPath, info);
    });
}

void UpdateManager::installDownloaded(const QString& zipPath, const ReleaseInfo& info) {
    const QString workDir = QFileInfo(zipPath).absolutePath();
    const QString pid = QString::number(QCoreApplication::applicationPid());

#ifdef Q_OS_MACOS
    const QString target = runningBundlePath();
    if (target.isEmpty() || !QFileInfo(QFileInfo(target).absolutePath()).isWritable()) {
        QDir(workDir).removeRecursively();
        showError(QStringLiteral("The update can't be installed automatically."),
                  QStringLiteral("Kvikplay can't write to the folder it is in. "
                                 "The download page opens so you can install it yourself."));
        openReleasePage(info);
        return;
    }
    const QString ownTeam = teamIdOf(target);
    if (ownTeam.isEmpty()) {
        QDir(workDir).removeRecursively();
        showError(QStringLiteral("The update can't be installed automatically."),
                  QStringLiteral("This copy of Kvikplay isn't signed by a developer, so it can't verify "
                                 "the update. The download page opens instead."));
        openReleasePage(info);
        return;
    }

    const QString unpacked = workDir + QStringLiteral("/unpacked");
    QString out;
    if (run(QStringLiteral("/usr/bin/ditto"), {QStringLiteral("-x"), QStringLiteral("-k"), zipPath, unpacked}, &out) != 0) {
        QDir(workDir).removeRecursively();
        showError(QStringLiteral("The update was not installed."), QStringLiteral("The download couldn't be unpacked."));
        return;
    }
    QString newApp;
    QDirIterator it(unpacked, {QStringLiteral("*.app")}, QDir::Dirs, QDirIterator::NoIteratorFlags);
    if (it.hasNext()) newApp = it.next();

    // Only an intact app signed by the same developer team and accepted by Gatekeeper.
    const bool signatureOk = !newApp.isEmpty() &&
        run(QStringLiteral("/usr/bin/codesign"), {QStringLiteral("--verify"), QStringLiteral("--deep"),
                                                  QStringLiteral("--strict"), newApp}) == 0 &&
        teamIdOf(newApp) == ownTeam &&
        run(QStringLiteral("/usr/sbin/spctl"), {QStringLiteral("--assess"), QStringLiteral("--type"),
                                                QStringLiteral("execute"), newApp}) == 0;
    if (!signatureOk) {
        QDir(workDir).removeRecursively();
        showError(QStringLiteral("The update was not installed."),
                  QStringLiteral("The downloaded app isn't signed by the Kvikplay developer or isn't approved "
                                 "by macOS. Nothing was changed."));
        return;
    }

    // After Kvikplay has quit: move the old app aside, copy the new one into place,
    // restore the old one if anything fails, relaunch, clean up. Paths are passed as
    // arguments ($1..$4), never pasted into the script.
    const QString script = QStringLiteral(
        "pid=\"$1\"; old=\"$2\"; new=\"$3\"; work=\"$4\"\n"
        "while kill -0 \"$pid\" 2>/dev/null; do sleep 0.2; done\n"
        "backup=\"$old.update-backup\"\n"
        "rm -rf \"$backup\"\n"
        "if mv \"$old\" \"$backup\" && /usr/bin/ditto \"$new\" \"$old\"; then\n"
        "  rm -rf \"$backup\"\n"
        "else\n"
        "  rm -rf \"$old\"; mv \"$backup\" \"$old\"\n"
        "fi\n"
        "rm -rf \"$work\"\n"
        "/usr/bin/open \"$old\"\n");
    const bool started = QProcess::startDetached(QStringLiteral("/bin/sh"),
        {QStringLiteral("-c"), script, QStringLiteral("kvikplay-update"), pid, target, newApp, workDir});
#elif defined(Q_OS_WIN)
    const QString installDir = QCoreApplication::applicationDirPath();
    if (!QFileInfo(installDir).isWritable()) {
        QDir(workDir).removeRecursively();
        showError(QStringLiteral("The update can't be installed automatically."),
                  QStringLiteral("Kvikplay can't write to the folder it is in. "
                                 "The download page opens so you can install it yourself."));
        openReleasePage(info);
        return;
    }
    // Runs after Kvikplay has quit: unpack, copy over the install folder, relaunch.
    const QString ps1 = workDir + QStringLiteral("/install-update.ps1");
    QFile f(ps1);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        showError(QStringLiteral("The update was not installed."), f.errorString());
        return;
    }
    f.write(
        "param([int]$ProcId, [string]$Zip, [string]$Dest, [string]$Exe, [string]$Work)\n"
        "try { Wait-Process -Id $ProcId -Timeout 60 -ErrorAction SilentlyContinue } catch {}\n"
        "$tmp = Join-Path $Work 'unpacked'\n"
        "Expand-Archive -LiteralPath $Zip -DestinationPath $tmp -Force\n"
        "$src = Get-ChildItem -LiteralPath $tmp -Recurse -Filter 'kvikplay.exe' | Select-Object -First 1\n"
        "if ($src) { Copy-Item -Path (Join-Path $src.DirectoryName '*') -Destination $Dest -Recurse -Force }\n"
        "Start-Process -FilePath $Exe\n"
        "Remove-Item -LiteralPath $Work -Recurse -Force -ErrorAction SilentlyContinue\n");
    f.close();
    const bool started = QProcess::startDetached(QStringLiteral("powershell.exe"),
        {QStringLiteral("-NoProfile"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
         QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"), QStringLiteral("-File"), ps1,
         QStringLiteral("-ProcId"), pid, QStringLiteral("-Zip"), zipPath, QStringLiteral("-Dest"), installDir,
         QStringLiteral("-Exe"), QCoreApplication::applicationFilePath(), QStringLiteral("-Work"), workDir});
#else
    const bool started = false;
#endif

    if (!started) {
        QDir(workDir).removeRecursively();
        showError(QStringLiteral("The update was not installed."), QStringLiteral("The installer couldn't be started."));
        return;
    }
    emit aboutToInstall();
    QApplication::quit(); // the helper waits for this process to exit
}

void UpdateManager::showError(const QString& what, const QString& why) {
    QMessageBox box(m_dialogParent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("Kvikplay update"));
    box.setTextFormat(Qt::PlainText);
    box.setText(what);
    box.setInformativeText(why);
    box.exec();
}

void UpdateManager::openReleasePage(const ReleaseInfo& info) {
    QDesktopServices::openUrl(QUrl(info.pageUrl));
}

} // namespace kvikplay
