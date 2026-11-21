#include "MainWindow.h"
#include "config/SettingsManager.h"
#include "formats/FormatRegistry.h"
#include "ui/FileAssociationDialog.h"
#include "ui/MediaInspectorOverlay.h"
#include "ui/HelpDialog.h"
#include <QApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QWindow>
#include <QPainter>
#include <QDateTime>
#include <QDir>
#include <QClipboard>
#include <QFileDialog>
#include <QStandardPaths>
#include <QProcess>
#include <QPointer>
#include <QMenu>
#include <QActionGroup>
#include <QCheckBox>
#include <QPushButton>
#include <algorithm>
#include <utility>
#include <cmath>
#include "version.h"

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h> // GET_X_LPARAM / GET_Y_LPARAM
#endif

namespace kvikplay {

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
    , m_player(std::make_unique<MediaPlayer>(this))
    , m_platform(PlatformIntegration::create())
    , m_thumbnailWorker(std::make_unique<ThumbnailWorker>())
    , m_renderer(new VideoRenderer(this))
    , m_overlay(new PlaybackOverlay(this))
    , m_contextMenu(new ContextMenu(this))
    , m_inspector(new MediaInspectorOverlay(this))
    , m_osdTimer(new QTimer(this))
    , m_scrollResetTimer(new QTimer(this))
    , m_rightClickHoldTimer(new QTimer(this))
    , m_inspectorTimer(new QTimer(this))
    , m_updates(new UpdateManager(this, this)) {

    // Frameless window setup with transparency for rounded corners
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
#ifndef _WIN32
    // macOS draws the rounded corners itself through a translucent window. Windows 11
    // rounds frameless windows in DWM, and a translucent (layered) window there would
    // push every video frame through a slow composition path, so it stays opaque.
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);

    setStyleSheet(QStringLiteral("background-color: transparent;"));

    // Overlay signal connections
    connect(m_overlay, &PlaybackOverlay::playPauseClicked, m_player.get(), &MediaPlayer::togglePlayPause);
    connect(m_overlay, &PlaybackOverlay::seekBackwardClicked, this, [this]() {
        m_player->seekRelative(-10.0);
        m_overlay->showOsd(QStringLiteral("-10s"));
    });
    connect(m_overlay, &PlaybackOverlay::seekForwardClicked, this, [this]() {
        m_player->seekRelative(10.0);
        m_overlay->showOsd(QStringLiteral("+10s"));
    });
    connect(m_overlay, &PlaybackOverlay::closeClicked, this, &QWidget::close);
    connect(m_overlay, &PlaybackOverlay::fullscreenClicked, this, &MainWindow::toggleFullscreen);
    connect(m_overlay, &PlaybackOverlay::subtitlesClicked, this, &MainWindow::showSubtitleMenu);
    connect(m_overlay, &PlaybackOverlay::volumeChanged, this, [this](int vol) {
        m_player->setVolume(vol);
        m_overlay->showVolumeLevel(vol, m_player->isMuted());
    });
    connect(m_overlay, &PlaybackOverlay::muteToggled, m_player.get(), &MediaPlayer::toggleMute);
    connect(m_overlay, &PlaybackOverlay::seekRequested, m_player.get(), &MediaPlayer::seekAbsolute);
    connect(m_overlay, &PlaybackOverlay::thumbnailRequested, this, &MainWindow::onThumbnailRequested);
    connect(m_overlay, &PlaybackOverlay::speedBoostRequested, this, [this](bool active) {
        active ? beginSpeedBoost() : endSpeedBoost();
    });

    if (m_thumbnailWorker) {
        connect(m_thumbnailWorker.get(), &ThumbnailWorker::thumbnailReady, this, &MainWindow::onThumbnailReady);
    }

    // A file opened at launch (command line, Finder) waits for the video surface.
    connect(m_renderer, &VideoRenderer::renderSetupFinished, this, [this](bool) {
        if (!m_pendingOpen.isEmpty()) {
            const QString file = std::exchange(m_pendingOpen, QString());
            openFile(file);
        }
    }, Qt::QueuedConnection);

    // Player signal connections
    connect(m_player.get(), &MediaPlayer::stateChanged, this, &MainWindow::onPlaybackStateChanged);
    connect(m_player.get(), &MediaPlayer::mediaLoaded, this, &MainWindow::onMediaLoaded);
    connect(m_player.get(), &MediaPlayer::videoParamsChanged, this, &MainWindow::onVideoParamsChanged);
    connect(m_player.get(), &MediaPlayer::volumeChanged, this, &MainWindow::onVolumeChanged);
    connect(m_player.get(), &MediaPlayer::muteChanged, this, &MainWindow::onMuteChanged);
    connect(m_player.get(), &MediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player.get(), &MediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player.get(), &MediaPlayer::speedChanged, this, [this](double) {
        updateNowPlayingMetadata();
    });
    connect(m_player.get(), &MediaPlayer::tracksChanged, this, &MainWindow::onTracksChanged);
    connect(m_player.get(), &MediaPlayer::mediaEnded, this, &MainWindow::onMediaEnded);
    connect(m_player.get(), &MediaPlayer::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(m_player.get(), &MediaPlayer::screenshotTaken, this, &MainWindow::onScreenshotTaken);
    connect(m_player.get(), &MediaPlayer::subtitleAdded, this, [this](const QString& name) {
        m_overlay->showOsd(QStringLiteral("Subtitles: %1").arg(name));
    });
    connect(m_player.get(), &MediaPlayer::chaptersChanged, this, &MainWindow::onChaptersChanged);
    connect(m_player.get(), &MediaPlayer::chapterChanged, this, &MainWindow::onChapterChanged);

    // Context menu connections
    connect(m_contextMenu, &ContextMenu::openFileTriggered, this, &MainWindow::openFileDialog);
    connect(m_contextMenu, &ContextMenu::playPauseTriggered, m_player.get(), &MediaPlayer::togglePlayPause);
    connect(m_contextMenu, &ContextMenu::alwaysOnTopToggled, this, &MainWindow::setAlwaysOnTop);
    connect(m_contextMenu, &ContextMenu::fullscreenToggled, this, &MainWindow::toggleFullscreen);
    connect(m_contextMenu, &ContextMenu::pipToggled, this, &MainWindow::togglePictureInPicture);
    connect(m_contextMenu, &ContextMenu::inspectorToggled, this, &MainWindow::toggleMediaInspector);
    connect(m_contextMenu, &ContextMenu::helpTriggered, this, &MainWindow::showHelpDialog);
    connect(m_contextMenu, &ContextMenu::reverseTrackpadToggled, this, [](bool rev) {
        SettingsManager::instance().setReverseTrackpadScrubbing(rev);
        SettingsManager::instance().save();
    });
    connect(m_contextMenu, &ContextMenu::dialogueBoostToggled, this, [this](bool enabled) {
        SettingsManager::instance().setDialogueBoost(enabled);
        SettingsManager::instance().save();
        m_player->setDialogueBoost(enabled);
        m_overlay->showOsd(enabled ? QStringLiteral("Dialogue Boost: ON") : QStringLiteral("Dialogue Boost: OFF"));
    });
    connect(m_contextMenu, &ContextMenu::recentFileSelected, this, &MainWindow::openFile);
    connect(m_contextMenu, &ContextMenu::externalOpenNewWindowToggled, this, [](bool newWindow) {
        SettingsManager::instance().setExternalOpenInNewWindow(newWindow);
        SettingsManager::instance().save();
    });
    connect(m_contextMenu, &ContextMenu::startInFullscreenToggled, this, [](bool on) {
        SettingsManager::instance().setStartInFullscreen(on);
        SettingsManager::instance().save();
    });
    connect(m_contextMenu, &ContextMenu::clearRecentTriggered, this, [this]() {
        SettingsManager::instance().clearRecentFiles();
        SettingsManager::instance().save();
    });
    connect(m_contextMenu, &ContextMenu::screenshotTriggered, this, [this]() {
        m_player->takeScreenshot(false);
    });
    connect(m_contextMenu, &ContextMenu::fileAssociationsTriggered, this, &MainWindow::showFileAssociationsDialog);
    connect(m_contextMenu, &ContextMenu::aboutTriggered, this, &MainWindow::showAboutDialog);
    connect(m_contextMenu, &ContextMenu::closeTriggered, this, &QWidget::close);
    connect(m_contextMenu, &ContextMenu::speedSelected, this, [this](double speed) {
        setSpeedAndRemember(speed);
        m_overlay->showOsd(QStringLiteral("Speed %1x").arg(speed, 0, 'f', speed * 10 == std::round(speed * 10) ? 1 : 2));
    });
    connect(m_contextMenu, &ContextMenu::chapterSelected, this, [this](double start) {
        m_player->seekAbsolute(start, true);
    });
    connect(m_contextMenu, &ContextMenu::nextInFolderTriggered, this, &MainWindow::playNextInFolder);
    connect(m_contextMenu, &ContextMenu::previousInFolderTriggered, this, &MainWindow::playPreviousInFolder);
    connect(m_contextMenu, &ContextMenu::windowScaleSelected, this, &MainWindow::applyWindowScale);
    connect(m_contextMenu, &ContextMenu::addSubtitleFileTriggered, this, [this]() {
        const QString dir = m_currentFilePath.isEmpty() ? QDir::homePath() : QFileInfo(m_currentFilePath).absolutePath();
        const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("Add Subtitle File"), dir,
            QStringLiteral("Subtitles (*.srt *.ass *.ssa *.vtt *.sub *.idx)"));
        if (!file.isEmpty()) {
            m_player->addSubtitleTrack(file); // OSD once mpv has loaded it
        }
    });
    connect(m_contextMenu, &ContextMenu::subtitleDelayAdjusted, this, [this](double delta) {
        m_player->adjustSubtitleDelay(delta);
        m_overlay->showOsd(QStringLiteral("Sub Delay: %1ms").arg(static_cast<int>(std::round(m_player->subtitleDelay() * 1000))));
    });
    connect(m_contextMenu, &ContextMenu::subtitleDelayResetTriggered, this, [this]() {
        m_player->setSubtitleDelay(0.0);
        m_overlay->showOsd(QStringLiteral("Sub Delay: 0ms"));
    });
    connect(m_contextMenu, &ContextMenu::audioTrackSelected, m_player.get(), &MediaPlayer::setAudioTrack);
    connect(m_contextMenu, &ContextMenu::videoTrackSelected, m_player.get(), &MediaPlayer::setVideoTrack);
    connect(m_contextMenu, &ContextMenu::subtitleTrackSelected, m_player.get(), &MediaPlayer::setSubtitleTrack);

    // OSD 5-second inactivity timer
    m_osdTimer->setSingleShot(true);
    m_osdTimer->setInterval(5000);
    connect(m_osdTimer, &QTimer::timeout, this, &MainWindow::onOsdInactivityTimeout);

    // Right-click hold timer (right click menu appears if held for longer than 0.5s)
    m_rightClickHoldTimer->setSingleShot(true);
    m_rightClickHoldTimer->setInterval(500);
    connect(m_rightClickHoldTimer, &QTimer::timeout, this, &MainWindow::onRightClickHoldTimeout);

    // Trackpad scroll accumulator reset timer
    m_scrollResetTimer->setSingleShot(true);
    m_scrollResetTimer->setInterval(250);
    connect(m_scrollResetTimer, &QTimer::timeout, this, [this]() {
        m_accumulatedScrollX = 0;
    });

    // Inspector refresh timer (500ms)
    m_inspectorTimer->setInterval(500);
    connect(m_inspectorTimer, &QTimer::timeout, this, [this]() {
        if (m_inspector && m_inspector->isVisible()) {
            m_inspector->updateStats(inspectorStats());
        }
    });
}

MainWindow::~MainWindow() {
    if (m_platform) {
        m_platform->setDisplaySleepInhibited(false);
        m_platform->clearNowPlayingInfo();
    }
    if (m_thumbnailWorker) {
        m_thumbnailWorker->stop();
        m_thumbnailWorker.reset();
    }
    // Free render context before mpv core is destroyed
    if (m_renderer) {
        delete m_renderer;
        m_renderer = nullptr;
    }
    m_player.reset();
}

bool MainWindow::initialize(bool cascade) {
    m_isExtraWindow = cascade;
    if (!m_player->initialize()) {
        return false;
    }

    m_renderer->setMpvHandle(m_player->controller()->handle());

    // Restore saved settings
    auto& settings = SettingsManager::instance();
    QSize savedSize = settings.windowSize();
    QPoint savedPos = settings.windowPosition();

    m_aspectManager.setAspectRatio(static_cast<double>(savedSize.width()) / savedSize.height());

    // Ensure initial window geometry is strictly within available monitor space
    QPoint initialCenter;
    if (savedPos.x() >= 0 && savedPos.y() >= 0) {
        initialCenter = QPoint(savedPos.x() + savedSize.width() / 2, savedPos.y() + savedSize.height() / 2);
    } else {
        if (QScreen* screen = QGuiApplication::primaryScreen()) {
            initialCenter = screen->availableGeometry().center();
        } else {
            initialCenter = QPoint(960, 540);
        }
    }
    if (cascade) {
        // An extra player window: offset it so it doesn't hide the first one.
        initialCenter += QPoint(40, 40);
    }
    fitAndPositionWindow(savedSize, initialCenter);

    m_overlay->setVolume(settings.volume());
    m_overlay->setMuted(settings.isMuted());

    bool dBoost = settings.dialogueBoost();
    m_player->setDialogueBoost(dBoost);

    // Native platform window setup
    if (m_platform) {
        m_platform->setupFramelessWindow(this);
        m_platform->setWindowRounded(this, true);
        m_platform->setWindowAspectRatio(this, m_aspectManager.aspectRatio(), savedSize.width(), savedSize.height());
        m_platform->installActivationObserver(this, [this](bool activatedByMouse) {
            m_lastActivationTime = QDateTime::currentMSecsSinceEpoch();
            if (activatedByMouse) {
                m_activatedByMouseClick = true;
            }
        });
        m_platform->installHoverObserver(this, [this]() {
            // Hover while the app is in the background: same as a mouse move.
            m_overlay->showOverlay();
            m_osdTimer->start(5000);
        });

        // Media keys can arrive on another thread (SMTC) and, in theory, during
        // shutdown: the QPointer makes a late one a no-op instead of a use-after-free.
        QPointer<MainWindow> self(this);
        auto onGui = [self](auto fn) {
            QMetaObject::invokeMethod(qApp, [self, fn]() {
                if (self) fn(self.data());
            }, Qt::QueuedConnection);
        };
        MediaKeyCallbacks callbacks;
        callbacks.onPlay = [onGui]() { onGui([](MainWindow* w) { w->m_player->play(); }); };
        callbacks.onPause = [onGui]() { onGui([](MainWindow* w) { w->m_player->pause(); }); };
        callbacks.onTogglePlayPause = [onGui]() { onGui([](MainWindow* w) { w->m_player->togglePlayPause(); }); };
        callbacks.onNextTrack = [onGui]() { onGui([](MainWindow* w) { w->playNextInFolder(); }); };
        callbacks.onPreviousTrack = [onGui]() { onGui([](MainWindow* w) { w->playPreviousInFolder(); }); };
        callbacks.onSeek = [onGui](double seconds) {
            onGui([seconds](MainWindow* w) { w->m_player->seekAbsolute(seconds, true); });
        };
        m_platform->setupMediaKeyCommands(callbacks);
    }

    // After the native window is set up, so the platform can apply it directly.
    if (settings.alwaysOnTop()) {
        setAlwaysOnTop(true);
    }

    return true;
}

void MainWindow::applyStartupState() {
    if (SettingsManager::instance().startInFullscreen() && !m_isFullscreen) {
        toggleFullscreen();
    }
    // Look for a newer version shortly after start, in the background, so it never
    // delays opening the video. Extra windows don't ask again.
    if (SettingsManager::instance().checkForUpdates() && !m_isExtraWindow) {
        QTimer::singleShot(3000, this, [this]() { m_updates->checkForUpdates(false); });
    }
}

void MainWindow::openFileDialog() {
    QString filter = FormatRegistry::instance().fileFilterString();
    QString startDir;
    if (!m_currentFilePath.isEmpty()) {
        startDir = QFileInfo(m_currentFilePath).absolutePath();
    } else {
        startDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        if (startDir.isEmpty()) {
            startDir = QDir::homePath();
        }
    }
    QString selected = QFileDialog::getOpenFileName(this, QStringLiteral("Open Video or Audio"), startDir, filter);
    if (!selected.isEmpty()) {
        openFile(selected);
    }
}

void MainWindow::openFile(const QString& filePath) {
    const QFileInfo fi(filePath);
    if (!fi.isFile()) {
        // A recent file that was moved or deleted: drop it from the list.
        SettingsManager::instance().removeRecentFile(filePath);
        SettingsManager::instance().save();
        m_overlay->showOsd(QStringLiteral("File not found: %1").arg(fi.fileName()));
        return;
    }
    if (!FormatRegistry::instance().isSupported(filePath)) {
        m_overlay->showOsd(QStringLiteral("%1 is not a supported media file").arg(fi.fileName()));
        return;
    }
    if (!m_renderer->isRenderSetupDone()) {
        // Loading now would race the OpenGL setup and mpv would pick its "null"
        // video output (black window). Open it as soon as the surface is ready.
        m_pendingOpen = fi.absoluteFilePath();
        return;
    }

    if (!m_currentFilePath.isEmpty() && m_player->hasMedia()) {
        double curPos = m_player->position();
        double dur = m_player->duration();
        if (curPos > 5.0 && (dur <= 0.0 || curPos < dur - 5.0)) {
            SettingsManager::instance().setResumePosition(m_currentFilePath, curPos);
        }
    }

    m_currentFilePath = fi.absoluteFilePath();
    SettingsManager::instance().addRecentFile(m_currentFilePath);
    SettingsManager::instance().save();

    if (m_thumbnailWorker) {
        m_thumbnailWorker->openFile(m_currentFilePath);
    }

    m_player->openMedia(m_currentFilePath); // starts playing as soon as it loads
    m_folderIndex.prefetch(fi.absolutePath()); // next/previous without blocking later
}

void MainWindow::openExternalFile(const QString& filePath) {
    if (SettingsManager::instance().externalOpenInNewWindow() && m_player->hasMedia()) {
        launchNewWindow(filePath);
        return;
    }
    openFile(filePath);
}

void MainWindow::openForwardedFile(const QString& filePath) {
    openFile(filePath);
    if (isMinimized()) {
        showNormal();
    }
    raise();
    activateWindow();
}

void MainWindow::launchNewWindow(const QString& filePath) {
    // A separate player process: its own playback, volume and system media controls.
#ifdef Q_OS_MACOS
    // Inside an .app bundle, ask LaunchServices for a new instance (-n).
    QDir bundle(QCoreApplication::applicationDirPath());
    bundle.cdUp();
    bundle.cdUp();
    if (bundle.absolutePath().endsWith(QStringLiteral(".app"))) {
        QProcess::startDetached(QStringLiteral("/usr/bin/open"),
                                {QStringLiteral("-n"), bundle.absolutePath(), QStringLiteral("--args"),
                                 QStringLiteral("--cascade"), filePath});
        return;
    }
#endif
    QProcess::startDetached(QCoreApplication::applicationFilePath(), {QStringLiteral("--cascade"), filePath});
}

void MainWindow::onVideoParamsChanged(int width, int height, int displayWidth, int displayHeight, double dar) {
    Q_UNUSED(width);
    Q_UNUSED(height);

    double ratio = (dar > 0.05) ? dar : 16.0 / 9.0;
    m_aspectManager.setAspectRatio(ratio);

    if (m_platform) {
        m_platform->setWindowAspectRatio(this, ratio, displayWidth, displayHeight);
    }

    if (!m_isFullscreen) {
        QScreen* scr = screen();
        if (!scr) scr = QGuiApplication::screenAt(geometry().center());
        if (!scr) scr = QGuiApplication::primaryScreen();
        QSize maxBounds = scr ? scr->availableGeometry().size() : QSize(1920, 1080);

        QSize newSize = m_aspectManager.calculateInitialWindowSize(displayWidth, displayHeight, maxBounds);
        fitAndPositionWindow(newSize, geometry().center());
    }

    updateRendererGeometry();
}

void MainWindow::onMediaLoaded(const MediaInfo& info) {
    m_renderer->setMediaInfo(info);
    m_overlay->setTitle(info.title, info.fileName);
    m_overlay->setDuration(info.duration);
    // Track menus are filled by tracksChanged, which mpv sends just before this.

    double resumePos = SettingsManager::instance().resumePosition(info.filePath);
    if (resumePos > 5.0 && (info.duration <= 0.0 || resumePos < info.duration - 5.0)) {
        m_player->seekAbsolute(resumePos);
        int m = static_cast<int>(resumePos) / 60;
        int s = static_cast<int>(resumePos) % 60;
        m_overlay->showOsd(QStringLiteral("Resumed at %1:%2")
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
    }

    m_lastNowPlayingPosition = -1.0;
    m_lastNowPlayingSyncTimeMs = 0;
    updateNowPlayingMetadata();

    updateRendererGeometry();
}

void MainWindow::onTracksChanged(const std::vector<TrackInfo>& audio,
                                 const std::vector<TrackInfo>& video,
                                 const std::vector<TrackInfo>& sub) {
}

void MainWindow::onPlaybackStateChanged(PlaybackState state) {
    bool isPlaying = (state == PlaybackState::Playing);
    m_overlay->setPlaying(isPlaying);

    if (m_platform) {
        m_platform->setDisplaySleepInhibited(isPlaying);
    }
    updateNowPlayingMetadata();
}

void MainWindow::onVolumeChanged(int percent) {
    m_overlay->setVolume(percent);
}

void MainWindow::onMuteChanged(bool muted) {
    m_overlay->setMuted(muted);
    m_overlay->showVolumeLevel(m_player->volume(), muted);
}

void MainWindow::onPositionChanged(double seconds) {
    m_overlay->setPosition(seconds);

    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    bool isDiscontinuous = (m_lastNowPlayingPosition >= 0.0) && (std::abs(seconds - m_lastNowPlayingPosition) > 2.0);
    bool periodicTimeout = (nowMs - m_lastNowPlayingSyncTimeMs > 5000);

    if (m_lastNowPlayingPosition < 0.0 || isDiscontinuous || periodicTimeout) {
        m_lastNowPlayingPosition = seconds;
        m_lastNowPlayingSyncTimeMs = nowMs;
        updateNowPlayingMetadata();
    }
}

void MainWindow::onDurationChanged(double seconds) {
    m_overlay->setDuration(seconds);
    updateNowPlayingMetadata();
}

void MainWindow::onMediaEnded() {
    if (m_platform) {
        m_platform->setDisplaySleepInhibited(false);
    }
    updateNowPlayingMetadata();

    SettingsManager::instance().clearResumePosition(m_currentFilePath);
    SettingsManager::instance().save();
    m_overlay->setPlaying(false);
    playNextInFolder();
}

void MainWindow::onErrorOccurred(const QString& error) {
    m_overlay->showOsd(error);
}

void MainWindow::onScreenshotTaken(const QString& filePath) {
    QImage img(filePath);
    if (!img.isNull()) {
        QGuiApplication::clipboard()->setImage(img);
    }
    m_overlay->showOsd(QStringLiteral("Screenshot saved & copied"));
}

void MainWindow::onThumbnailRequested(double seconds) {
    if (m_thumbnailWorker && m_player->mediaInfo().hasVideo()) {
        m_thumbnailWorker->requestThumbnail(seconds);
    }
}

void MainWindow::onThumbnailReady(double seconds, const QImage& image) {
    m_overlay->setThumbnail(seconds, image);
}

void MainWindow::toggleDialogueBoost() {
    bool next = !SettingsManager::instance().dialogueBoost();
    SettingsManager::instance().setDialogueBoost(next);
    SettingsManager::instance().save();
    m_player->setDialogueBoost(next);
    m_overlay->showOsd(next ? QStringLiteral("Dialogue Boost: ON") : QStringLiteral("Dialogue Boost: OFF"));
}

void MainWindow::playNextInFolder() {
    if (m_currentFilePath.isEmpty()) return;
    const QString nextFile = m_folderIndex.next(m_currentFilePath);
    if (!nextFile.isEmpty()) {
        openFile(nextFile);
        m_overlay->showOsd(QStringLiteral("Next: %1").arg(QFileInfo(nextFile).fileName()));
    }
}

void MainWindow::playPreviousInFolder() {
    if (m_currentFilePath.isEmpty()) return;
    const QString prevFile = m_folderIndex.previous(m_currentFilePath);
    if (!prevFile.isEmpty()) {
        openFile(prevFile);
        m_overlay->showOsd(QStringLiteral("Previous: %1").arg(QFileInfo(prevFile).fileName()));
    }
}

void MainWindow::beginSpeedBoost() {
    if (m_speedBoostActive) return;
    m_speedBoostActive = true;
    m_preBoostSpeed = m_player->speed();
    m_player->setSpeed(2.0);
    m_overlay->showOsd(QStringLiteral("Speed 2.0x"));
}

void MainWindow::endSpeedBoost() {
    if (!m_speedBoostActive) return;
    m_speedBoostActive = false;
    m_player->setSpeed(m_preBoostSpeed);
    m_overlay->showOsd(QStringLiteral("Speed %1x").arg(m_preBoostSpeed, 0, 'f', 1));
}

void MainWindow::setSpeedAndRemember(double speed) {
    // A deliberate speed change (not the temporary 2x boost) is kept for next time.
    if (m_speedBoostActive) {
        m_speedBoostActive = false; // the new speed replaces the boost
    }
    m_player->setSpeed(speed);
    SettingsManager::instance().setPlaybackSpeed(speed);
}

void MainWindow::onChaptersChanged(const std::vector<ChapterInfo>& chapters) {
    m_overlay->setChapters(chapters);
}

void MainWindow::onChapterChanged(int index, const QString& title) {
    m_overlay->showOsd(QStringLiteral("Chapter %1/%2: %3")
        .arg(index + 1)
        .arg(m_player->chapterCount())
        .arg(title));
}

void MainWindow::togglePictureInPicture() {
    if (m_isPiP) {
        m_isPiP = false;
        if (m_prePiPGeometry.isValid()) {
            setGeometry(m_prePiPGeometry);
        }
        setAlwaysOnTop(m_prePiPAlwaysOnTop);
        m_overlay->showOsd(QStringLiteral("Exit Picture-in-Picture"));
    } else {
        m_prePiPGeometry = geometry();
        m_prePiPAlwaysOnTop = m_isAlwaysOnTop;
        m_isPiP = true;

        double dar = m_aspectManager.aspectRatio();
        int pipW = 380;
        int pipH = static_cast<int>(std::round(pipW / dar));

        QScreen* scr = screen();
        if (!scr) scr = QGuiApplication::primaryScreen();
        QRect scrRect = scr ? scr->availableGeometry() : QRect(0, 0, 1920, 1080);

        int targetX = scrRect.right() - pipW - 20;
        int targetY = scrRect.bottom() - pipH - 20;

        setGeometry(targetX, targetY, pipW, pipH);
        setAlwaysOnTop(true);
        m_overlay->showOsd(QStringLiteral("Picture-in-Picture"));
    }
}

void MainWindow::toggleMediaInspector() {
    if (!m_inspector) return;
    m_inspector->toggleInspector();
    if (m_inspector->isVisible()) {
        // Below the title block, on the overlay's left margin.
        m_inspector->move(width() < 600 ? 16 : 28, 84);
        m_inspector->updateStats(inspectorStats());
        m_inspectorTimer->start();
    } else {
        m_inspectorTimer->stop();
    }
}

void MainWindow::showHelpDialog() {
    HelpDialog dlg(this);
    dlg.exec();
}

void MainWindow::setAlwaysOnTop(bool top) {
    m_isAlwaysOnTop = top;
    SettingsManager::instance().setAlwaysOnTop(top);

    // Changed on the existing native window: QWidget::setWindowFlags() would destroy
    // and recreate it, losing the rounded corners, aspect lock and focus tracking.
    if (m_platform) {
        m_platform->setAlwaysOnTop(this, top);
    } else if (windowHandle()) {
        windowHandle()->setFlag(Qt::WindowStaysOnTopHint, top);
    }
}

void MainWindow::toggleFullscreen() {
    if (m_isFullscreen) {
        // Exit fullscreen -> restore windowed mode size and position
        m_isFullscreen = false;
        m_overlay->setFullscreen(false);
        showNormal();
        if (m_normalGeometry.isValid()) {
            setGeometry(m_normalGeometry);
        }
        if (m_platform) {
            m_platform->setWindowRounded(this, true);
        }
        setCursor(Qt::ArrowCursor);
    } else {
        // Enter fullscreen -> remember windowed size & position
        m_normalGeometry = geometry();
        m_isFullscreen = true;
        m_overlay->setFullscreen(true);
        if (m_platform) {
            m_platform->setWindowRounded(this, false);
        }
        showFullScreen();
    }
    m_osdTimer->start(5000);
    updateRendererGeometry();
}

void MainWindow::applyWindowScale(double scale) {
    if (m_isFullscreen) {
        toggleFullscreen();
    }

    int dw = m_player->mediaInfo().displayWidth;
    int dh = m_player->mediaInfo().displayHeight;
    if (dw <= 0 || dh <= 0) {
        dw = 960;
        dh = 540;
    }

    int targetW = static_cast<int>(std::round(dw * scale));
    int targetH = static_cast<int>(std::round(dh * scale));

    fitAndPositionWindow(QSize(targetW, targetH), geometry().center());

    m_overlay->showOsd(QStringLiteral("Size: %1%").arg(static_cast<int>(scale * 100)));
    m_overlay->showOverlay();
    m_osdTimer->start(5000);
}

void MainWindow::fitAndPositionWindow(const QSize& targetSize, const QPoint& preferredCenter) {
    // The screen under the preferred centre first: before the window is shown,
    // screen() is always the primary one, which pulled a window saved on a second
    // monitor back onto the main screen.
    QScreen* scr = QGuiApplication::screenAt(preferredCenter);
    if (!scr) {
        scr = screen();
    }
    if (!scr) {
        scr = QGuiApplication::primaryScreen();
    }

    QRect availGeo = scr ? scr->availableGeometry() : QRect(0, 0, 1920, 1080);

    // Keep a comfortable 16px margin from all display boundaries
    const int margin = 16;
    int maxW = std::max(200, availGeo.width() - (margin * 2));
    int maxH = std::max(150, availGeo.height() - (margin * 2));

    int w = targetSize.width();
    int h = targetSize.height();

    // Scale down proportionally if larger than the available monitor bounds
    if (w > maxW || h > maxH) {
        double scaleW = static_cast<double>(maxW) / w;
        double scaleH = static_cast<double>(maxH) / h;
        double scale = std::min(scaleW, scaleH);
        w = static_cast<int>(std::round(w * scale));
        h = static_cast<int>(std::round(h * scale));
    }

    // Clamp against minimum allowed window size
    QSize minSize = m_aspectManager.minimumSize();
    if (w < minSize.width() && maxW >= minSize.width()) {
        w = minSize.width();
        h = static_cast<int>(std::round(w / m_aspectManager.aspectRatio()));
    }
    if (h < minSize.height() && maxH >= minSize.height()) {
        h = minSize.height();
        w = static_cast<int>(std::round(h * m_aspectManager.aspectRatio()));
    }

    // Target top-left derived from preferred center
    int targetX = preferredCenter.x() - w / 2;
    int targetY = preferredCenter.y() - h / 2;

    // Strict boundary clamping so the entire window is 100% inside the monitor area
    int minX = availGeo.left() + margin;
    int maxX = availGeo.right() - w - margin;
    int minY = availGeo.top() + margin;
    int maxY = availGeo.bottom() - h - margin;

    if (maxX >= minX) {
        targetX = std::clamp(targetX, minX, maxX);
    } else {
        targetX = availGeo.left() + (availGeo.width() - w) / 2;
    }

    if (maxY >= minY) {
        targetY = std::clamp(targetY, minY, maxY);
    } else {
        targetY = availGeo.top() + (availGeo.height() - h) / 2;
    }

    resize(w, h);
    move(targetX, targetY);
    m_normalGeometry = geometry();
}

StreamStats MainWindow::inspectorStats() const {
    StreamStats stats = m_player->streamStats();
    stats.renderer = m_renderer ? m_renderer->diagnostics() : QStringLiteral("-");
    return stats;
}

MenuState MainWindow::buildMenuState() {
    auto& settings = SettingsManager::instance();
    MenuState s;
    s.hasMedia = m_player->hasMedia();
    s.playing = m_player->state() == PlaybackState::Playing;
    s.speed = m_player->speed();
    s.audioTracks = m_player->audioTracks();
    s.videoTracks = m_player->videoTracks();
    s.subtitleTracks = m_player->subtitleTracks();
    s.subtitleDelay = m_player->subtitleDelay();
    s.chapters = m_player->mediaInfo().chapters;
    s.currentChapter = m_player->currentChapter();
    if (!m_currentFilePath.isEmpty()) {
        // Cached by FolderIndex (prefetched when the file was opened): no disk scan here.
        s.hasNextInFolder = !m_folderIndex.next(m_currentFilePath).isEmpty();
        s.hasPreviousInFolder = !m_folderIndex.previous(m_currentFilePath).isEmpty();
    }
    s.fullscreen = m_isFullscreen;
    s.pictureInPicture = m_isPiP;
    s.alwaysOnTop = m_isAlwaysOnTop;
    s.dialogueBoost = settings.dialogueBoost();
    s.inspectorVisible = m_inspector && m_inspector->isVisible();
    s.reverseTrackpad = settings.reverseTrackpadScrubbing();
    s.startInFullscreen = settings.startInFullscreen();
    s.externalOpenNewWindow = settings.externalOpenInNewWindow();
    s.recentFiles = settings.recentFiles();
    return s;
}

void MainWindow::showSubtitleMenu(const QPoint& buttonGlobalPos) {
    // The subtitle part of the right-click menu, opened above the overlay's button.
    QMenu menu(this);
    menu.setStyleSheet(m_contextMenu->styleSheet());
    auto* group = new QActionGroup(&menu);
    bool anySelected = false;
    for (const auto& t : m_player->subtitleTracks()) anySelected = anySelected || t.selected;
    QAction* off = menu.addAction(QStringLiteral("Off"));
    off->setCheckable(true);
    off->setChecked(!anySelected);
    group->addAction(off);
    connect(off, &QAction::triggered, this, [this]() { m_player->setSubtitleTrack(0); });
    if (!m_player->subtitleTracks().empty()) menu.addSeparator();
    for (const auto& t : m_player->subtitleTracks()) {
        QAction* a = menu.addAction(t.displayName().replace(QLatin1Char('&'), QStringLiteral("&&")));
        a->setCheckable(true);
        a->setChecked(t.selected);
        group->addAction(a);
        const int id = t.id;
        connect(a, &QAction::triggered, this, [this, id]() { m_player->setSubtitleTrack(id); });
    }
    menu.addSeparator();
    connect(menu.addAction(QStringLiteral("Add Subtitle File…")), &QAction::triggered,
            m_contextMenu, &ContextMenu::addSubtitleFileTriggered);
    connect(menu.addAction(QStringLiteral("Show 100 ms Earlier")), &QAction::triggered, this,
            [this]() { emit m_contextMenu->subtitleDelayAdjusted(-0.1); });
    connect(menu.addAction(QStringLiteral("Show 100 ms Later")), &QAction::triggered, this,
            [this]() { emit m_contextMenu->subtitleDelayAdjusted(0.1); });
    menu.exec(buttonGlobalPos - QPoint(0, menu.sizeHint().height() + 6));
}

void MainWindow::showContextMenu(const QPoint& globalPos) {
    m_contextMenu->rebuild(buildMenuState());
    m_contextMenu->exec(globalPos);
}

void MainWindow::onOsdInactivityTimeout() {
    m_overlay->hideOverlay();
    if (m_isFullscreen) {
        setCursor(Qt::BlankCursor);
    }
}

void MainWindow::onRightClickHoldTimeout() {
    m_rightClickMenuShown = true;
    showContextMenu(m_rightPressGlobalPos);
}

void MainWindow::updateRendererGeometry() {
    if (m_isFullscreen) {
        // Center video preserving aspect ratio on fullscreen black canvas
        double ratio = m_aspectManager.aspectRatio();
        int screenW = width();
        int screenH = height();

        int targetW = screenW;
        int targetH = static_cast<int>(std::round(targetW / ratio));

        if (targetH > screenH) {
            targetH = screenH;
            targetW = static_cast<int>(std::round(targetH * ratio));
        }

        int x = (screenW - targetW) / 2;
        int y = (screenH - targetH) / 2;
        m_renderer->setGeometry(x, y, targetW, targetH);
    } else {
        // Normal mode: video fills entire window without black bars
        m_renderer->setGeometry(rect());
    }

    m_overlay->setGeometry(rect());
    m_overlay->raise(); // Ensure overlay is strictly above video and subtitles
}

void MainWindow::resizeEvent(QResizeEvent* /*event*/) {
    updateRendererGeometry();
    if (!m_isFullscreen) {
        m_normalGeometry = geometry();
        SettingsManager::instance().setWindowSize(size());
        SettingsManager::instance().setWindowPosition(pos());
    }
}

void MainWindow::moveEvent(QMoveEvent* /*event*/) {
    if (!m_isFullscreen) {
        m_normalGeometry = geometry();
        SettingsManager::instance().setWindowPosition(pos());
    }
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            m_lastActivationTime = QDateTime::currentMSecsSinceEpoch();
            if (QApplication::mouseButtons() != Qt::NoButton) {
                m_activatedByMouseClick = true;
            }
        } else {
            // C held while switching apps: its key release goes elsewhere.
            endSpeedBoost();
        }
    }
    QWidget::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_platform) {
        m_platform->setDisplaySleepInhibited(false);
        m_platform->clearNowPlayingInfo();
    }
    if (!m_currentFilePath.isEmpty() && m_player->hasMedia()) {
        double curPos = m_player->position();
        double dur = m_player->duration();
        if (curPos > 5.0 && (dur <= 0.0 || curPos < dur - 5.0)) {
            SettingsManager::instance().setResumePosition(m_currentFilePath, curPos);
        }
    }
    if (!m_isFullscreen) {
        SettingsManager::instance().setWindowSize(size());
        SettingsManager::instance().setWindowPosition(pos());
    }
    SettingsManager::instance().save();
    if (m_thumbnailWorker) {
        m_thumbnailWorker->stop();
    }
    m_player->stop();
    QWidget::closeEvent(event);
}

Qt::Edges MainWindow::hitTestEdges(const QPoint& pos) const {
    if (m_isFullscreen) return Qt::Edges();

    int x = pos.x();
    int y = pos.y();
    int w = width();
    int h = height();
    int m = m_borderMargin;   // 16px border
    int c = m_cornerMargin;   // 32px generous corner

    // 1. Check 4 corners first with generous 32px hit zones
    if (x <= c && y <= c) return Qt::TopEdge | Qt::LeftEdge;
    if (x >= w - c && y <= c) return Qt::TopEdge | Qt::RightEdge;
    if (x <= c && y >= h - c) return Qt::BottomEdge | Qt::LeftEdge;
    if (x >= w - c && y >= h - c) return Qt::BottomEdge | Qt::RightEdge;

    // 2. Check 4 edges with 16px hit zones
    Qt::Edges edges;
    if (x <= m) edges |= Qt::LeftEdge;
    if (x >= w - m) edges |= Qt::RightEdge;
    if (y <= m) edges |= Qt::TopEdge;
    if (y >= h - m) edges |= Qt::BottomEdge;

    return edges;
}

void MainWindow::updateCursorForEdges(Qt::Edges edges) {
    if (edges == (Qt::TopEdge | Qt::LeftEdge) || edges == (Qt::BottomEdge | Qt::RightEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (edges == (Qt::TopEdge | Qt::RightEdge) || edges == (Qt::BottomEdge | Qt::LeftEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (edges & (Qt::LeftEdge | Qt::RightEdge)) {
        setCursor(Qt::SizeHorCursor);
    } else if (edges & (Qt::TopEdge | Qt::BottomEdge)) {
        setCursor(Qt::SizeVerCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        // Record focus state at the exact moment of right click / two-finger click
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        bool wasJustActivated = (now - m_lastActivationTime < 350) || m_activatedByMouseClick;
        m_wasActiveOnRightPress = isActiveWindow() && !wasJustActivated;

        // If not in focus, bring window to front and don't trigger action
        if (!m_wasActiveOnRightPress) {
            activateWindow();
            raise();
        }

        m_rightPressPos = event->pos();
        m_rightPressGlobalPos = event->globalPosition().toPoint();
        m_rightClickMenuShown = false;
        m_rightClickHoldTimer->start(500); // 0.5s hold required for context menu
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_rightClickHoldTimer->stop();

        Qt::Edges edges = hitTestEdges(event->pos());
        if (edges != Qt::Edges() && windowHandle()) {
            windowHandle()->startSystemResize(edges);
            event->accept();
            return;
        }

        m_pressPos = event->pos();
        m_draggingWindow = false;

        if (!isActiveWindow()) {
            activateWindow();
            raise();
        }

        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    // Show overlay immediately on mouse movement
    m_overlay->showOverlay();
    m_overlay->raise();
    if (m_isFullscreen) {
        setCursor(Qt::ArrowCursor); // un-hide after the inactivity timeout
    }

    // Reset 5s inactivity timer
    m_osdTimer->start(5000);

    // If right mouse button was pressed, check for drag movement to cancel context menu hold
    if ((event->buttons() & Qt::RightButton) && m_rightClickHoldTimer->isActive()) {
        if ((event->pos() - m_rightPressPos).manhattanLength() > 6) {
            m_rightClickHoldTimer->stop();
        }
    }

    if (!m_isFullscreen) {
        Qt::Edges edges = hitTestEdges(event->pos());
        updateCursorForEdges(edges);

        // If left button held down, check if user started dragging window
        if ((event->buttons() & Qt::LeftButton) && !m_draggingWindow) {
            if ((event->pos() - m_pressPos).manhattanLength() > 6) {
                m_draggingWindow = true;
                if (windowHandle()) {
                    windowHandle()->startSystemMove();
                }
            }
        }
    }

    QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        bool holdTimerWasActive = m_rightClickHoldTimer->isActive();
        m_rightClickHoldTimer->stop();

        qint64 now = QDateTime::currentMSecsSinceEpoch();
        bool wasJustActivated = (now - m_lastActivationTime < 350) || m_activatedByMouseClick;
        m_activatedByMouseClick = false;

        // If right-click / two-finger click was released before 0.5s (timer was still running),
        // the menu was not shown, window was already focused, and mouse didn't drag:
        // TOGGLE PLAY / PAUSE!
        if (holdTimerWasActive && !m_rightClickMenuShown && m_wasActiveOnRightPress && !wasJustActivated) {
            if ((event->pos() - m_rightPressPos).manhattanLength() <= 6) {
                m_player->togglePlayPause();
            }
        }
        m_rightClickMenuShown = false;
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_draggingWindow = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Double left-click toggles fullscreen <-> windowed mode, remembering window size
        toggleFullscreen();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent* event) {
    int dx = !event->angleDelta().isNull() ? event->angleDelta().x() : event->pixelDelta().x();
    int dy = !event->angleDelta().isNull() ? event->angleDelta().y() : event->pixelDelta().y();

    Qt::KeyboardModifiers mods = event->modifiers();

    // 1. Horizontal scrubbing (Mac trackpad two-finger swipe right/left)
    if (std::abs(dx) > std::abs(dy) && dx != 0) {
        bool reversed = SettingsManager::instance().reverseTrackpadScrubbing();
        int effectiveDx = reversed ? -dx : dx;

        m_accumulatedScrollX += effectiveDx;
        m_scrollResetTimer->start(250);

        if (std::abs(m_accumulatedScrollX) >= 40) {
            // Dragging right -> scrub forward (or reversed if enabled)
            // Dragging left -> scrub backward (or reversed if enabled)
            double seekAmount = (m_accumulatedScrollX > 0) ? (mods & Qt::ShiftModifier ? 30.0 : 10.0)
                                                           : (mods & Qt::ShiftModifier ? -30.0 : -10.0);
            m_player->seekRelative(seekAmount);
            m_overlay->showOsd((seekAmount > 0) ? QStringLiteral("+%1s").arg(static_cast<int>(seekAmount))
                                                : QStringLiteral("%1s").arg(static_cast<int>(seekAmount)));
            m_accumulatedScrollX = 0;
        }

        m_overlay->showOverlay();
        m_overlay->raise();
        m_osdTimer->start(5000);
        event->accept();
        return;
    }

    // 2. Vertical scroll (mouse wheel / trackpad vertical)
    if (dy != 0) {
        if (mods & Qt::ControlModifier) {
            // Ctrl + Wheel: Volume adjustment (+/- 5%)
            int step = (dy > 0) ? 5 : -5;
            int newVol = std::clamp(m_player->volume() + step, 0, 100);
            m_player->setVolume(newVol);
            m_overlay->showVolumeLevel(newVol, m_player->isMuted());
        } else {
            // Vertical wheel seeking
            double seekAmount = (dy > 0) ? (mods & Qt::ShiftModifier ? 30.0 : 10.0)
                                         : (mods & Qt::ShiftModifier ? -30.0 : -10.0);
            m_player->seekRelative(seekAmount);
            m_overlay->showOsd((seekAmount > 0) ? QStringLiteral("+%1s").arg(static_cast<int>(seekAmount))
                                                : QStringLiteral("%1s").arg(static_cast<int>(seekAmount)));
        }

        m_overlay->showOverlay();
        m_overlay->raise();
        m_osdTimer->start(5000);
        event->accept();
    }
}

void MainWindow::contextMenuEvent(QContextMenuEvent* event) {
    if (event->reason() == QContextMenuEvent::Keyboard) {
        showContextMenu(event->globalPos());
    }
    event->accept();
}

void MainWindow::enterEvent(QEnterEvent* /*event*/) {
    m_overlay->showOverlay();
    m_overlay->raise();
    m_osdTimer->start(5000);
}

void MainWindow::leaveEvent(QEvent* /*event*/) {
    m_osdTimer->stop();
    m_overlay->hideOverlay();
}

static bool isSubtitleFile(const QString& path) {
    QString ext = QFileInfo(path).suffix().toLower();
    return (ext == "srt" || ext == "vtt" || ext == "ass" || ext == "sub" || ext == "idx" || ext == "ssa");
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        const auto& urls = event->mimeData()->urls();
        for (const auto& url : urls) {
            if (url.isLocalFile()) {
                QString localPath = url.toLocalFile();
                QFileInfo fi(localPath);
                if (fi.isDir() || isSubtitleFile(localPath) || FormatRegistry::instance().isSupported(localPath)) {
                    event->acceptProposedAction();
                    return;
                }
            }
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        const auto& urls = event->mimeData()->urls();
        for (const auto& url : urls) {
            if (url.isLocalFile()) {
                QString localPath = url.toLocalFile();
                QFileInfo fi(localPath);
                if (fi.isDir()) {
                    // First media file of the folder, in natural order
                    const QStringList mediaFiles = m_folderIndex.files(localPath);
                    if (!mediaFiles.isEmpty()) {
                        openFile(mediaFiles.first());
                        event->acceptProposedAction();
                        return;
                    }
                } else if (isSubtitleFile(localPath)) {
                    m_player->addSubtitleTrack(localPath); // OSD once mpv has loaded it
                    event->acceptProposedAction();
                    return;
                } else if (FormatRegistry::instance().isSupported(localPath)) {
                    openFile(localPath); // Automatically starts playing
                    event->acceptProposedAction();
                    return;
                }
            }
        }
    }
    event->ignore();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->matches(QKeySequence::Open) || ((event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) && event->key() == Qt::Key_O)) {
        openFileDialog();
        return;
    }

    switch (event->key()) {
    case Qt::Key_1:
        applyWindowScale(1.0);
        break;
    case Qt::Key_2:
        applyWindowScale(0.5);
        break;
    case Qt::Key_4:
        applyWindowScale(0.25);
        break;
    case Qt::Key_C:
        if (!event->isAutoRepeat()) {
            beginSpeedBoost();
        }
        break;
    case Qt::Key_Z:
        m_player->adjustSubtitleDelay(-0.1);
        m_overlay->showOsd(QStringLiteral("Sub Delay: %1ms").arg(static_cast<int>(std::round(m_player->subtitleDelay() * 1000))));
        break;
    case Qt::Key_X:
        m_player->adjustSubtitleDelay(0.1);
        m_overlay->showOsd(QStringLiteral("Sub Delay: %1ms").arg(static_cast<int>(std::round(m_player->subtitleDelay() * 1000))));
        break;
    case Qt::Key_P:
        togglePictureInPicture();
        break;
    case Qt::Key_I:
        toggleMediaInspector();
        break;
    case Qt::Key_BracketRight:
    case Qt::Key_PageDown:
        if (event->modifiers() & Qt::ShiftModifier) {
            m_player->nextChapter();
        } else {
            playNextInFolder();
        }
        break;
    case Qt::Key_BracketLeft:
    case Qt::Key_PageUp:
        if (event->modifiers() & Qt::ShiftModifier) {
            m_player->previousChapter();
        } else {
            playPreviousInFolder();
        }
        break;
    case Qt::Key_S:
        if (event->modifiers() & Qt::ShiftModifier) {
            m_player->takeScreenshot(true);
        } else {
            m_player->takeScreenshot(false);
        }
        break;
    case Qt::Key_N:
        toggleDialogueBoost();
        break;
    case Qt::Key_Period:
        m_player->stepFrame();
        m_overlay->showOsd(QStringLiteral("Frame Step +1"));
        break;
    case Qt::Key_Comma:
        m_player->stepFrameBack();
        m_overlay->showOsd(QStringLiteral("Frame Step -1"));
        break;
    case Qt::Key_Minus: {
        double curSpd = m_player->speed();
        double newSpd = std::clamp(std::round((curSpd - 0.1) * 10.0) / 10.0, 0.25, 4.0);
        setSpeedAndRemember(newSpd);
        m_overlay->showOsd(QStringLiteral("Speed %1x").arg(newSpd, 0, 'f', 1));
        break;
    }
    case Qt::Key_Equal:
    case Qt::Key_Plus: {
        double curSpd = m_player->speed();
        double newSpd = std::clamp(std::round((curSpd + 0.1) * 10.0) / 10.0, 0.25, 4.0);
        setSpeedAndRemember(newSpd);
        m_overlay->showOsd(QStringLiteral("Speed %1x").arg(newSpd, 0, 'f', 1));
        break;
    }
    case Qt::Key_Backspace:
        setSpeedAndRemember(1.0);
        m_overlay->showOsd(QStringLiteral("Speed 1.0x"));
        break;
    case Qt::Key_Space:
        m_player->togglePlayPause();
        break;
    case Qt::Key_Left:
        if (event->modifiers() & Qt::ShiftModifier) {
            m_player->seekRelative(-30.0);
            m_overlay->showOsd(QStringLiteral("-30s"));
        } else {
            m_player->seekRelative(-10.0);
            m_overlay->showOsd(QStringLiteral("-10s"));
        }
        break;
    case Qt::Key_Right:
        if (event->modifiers() & Qt::ShiftModifier) {
            m_player->seekRelative(30.0);
            m_overlay->showOsd(QStringLiteral("+30s"));
        } else {
            m_player->seekRelative(10.0);
            m_overlay->showOsd(QStringLiteral("+10s"));
        }
        break;
    case Qt::Key_Up: {
        int newVol = std::clamp(m_player->volume() + 5, 0, 100);
        m_player->setVolume(newVol);
        m_overlay->showVolumeLevel(newVol, m_player->isMuted());
        m_overlay->showOverlay();
        m_osdTimer->start(5000);
        event->accept();
        return;
    }
    case Qt::Key_Down: {
        int newVol = std::clamp(m_player->volume() - 5, 0, 100);
        m_player->setVolume(newVol);
        m_overlay->showVolumeLevel(newVol, m_player->isMuted());
        m_overlay->showOverlay();
        m_osdTimer->start(5000);
        event->accept();
        return;
    }
    case Qt::Key_M:
        m_player->toggleMute();
        break;
    case Qt::Key_F:
        toggleFullscreen();
        break;
    case Qt::Key_Escape:
        if (m_isFullscreen) {
            toggleFullscreen();
        }
        break;
    case Qt::Key_Q:
        close();
        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    m_overlay->showOverlay();
    m_osdTimer->start(5000);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_C && !event->isAutoRepeat()) {
        endSpeedBoost();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void MainWindow::showFileAssociationsDialog() {
    FileAssociationDialog dialog(m_platform.get(), this);
    dialog.exec();
}

void MainWindow::showAboutDialog() {
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle(QStringLiteral("About Kvikplay"));
    aboutBox.setStyleSheet(QStringLiteral(
        "QMessageBox { background-color: #1a1c22; color: #eeeeee; }"
        "QLabel { color: #dddddd; font-size: 13px; }"
        "QCheckBox { color: #dddddd; font-size: 12px; }"
        "QPushButton { background-color: #272b35; color: #ffffff; border: 1px solid #3d4352; border-radius: 6px; padding: 6px 16px; }"
    ));
    aboutBox.setTextFormat(Qt::RichText); // static text only, no media data
    aboutBox.setText(QStringLiteral(
        "<h3>Kvikplay</h3>"
        "<p>Version " KVIKPLAY_VERSION "</p>"
        "<p>Fast, minimalistic media player for macOS and Windows.</p>"
        "<p>Engine: <b>libmpv</b> &bull; UI: <b>Qt 6</b></p>"
    ));
    aboutBox.setIconPixmap(QIcon(QStringLiteral(":/icons/app_icon.svg")).pixmap(48, 48));

    auto* autoCheck = new QCheckBox(QStringLiteral("Check for updates when Kvikplay starts"), &aboutBox);
    autoCheck->setChecked(SettingsManager::instance().checkForUpdates());
    connect(autoCheck, &QCheckBox::toggled, this, [](bool on) {
        SettingsManager::instance().setCheckForUpdates(on);
        SettingsManager::instance().save();
    });
    aboutBox.setCheckBox(autoCheck);
    QPushButton* checkNow = aboutBox.addButton(QStringLiteral("Check for Updates…"), QMessageBox::ActionRole);
    aboutBox.addButton(QMessageBox::Close);
    aboutBox.exec();
    if (aboutBox.clickedButton() == checkNow) {
        m_updates->checkForUpdates(true);
    }
}

#ifdef _WIN32
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    MSG* msg = static_cast<MSG*>(message);

    switch (msg->message) {
    case WM_MOUSEACTIVATE:
        // The click that focuses the window must not also play/pause.
        m_activatedByMouseClick = true;
        m_lastActivationTime = QDateTime::currentMSecsSinceEpoch();
        *result = MA_ACTIVATEANDEAT;
        return true;

    case WM_NCCALCSIZE:
        // WS_THICKFRAME (added for resize, snap and the DWM shadow) brings back a
        // non-client frame; without this a white strip shows on Windows 10.
        if (msg->wParam == TRUE) {
            if (IsZoomed(msg->hwnd)) {
                // Maximized windows extend past the screen by the frame width; trim it.
                auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
                const UINT dpi = GetDpiForWindow(msg->hwnd);
                const int frame = GetSystemMetricsForDpi(SM_CXFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
                params->rgrc[0].left += frame;
                params->rgrc[0].top += frame;
                params->rgrc[0].right -= frame;
                params->rgrc[0].bottom -= frame;
            }
            *result = 0;
            return true;
        }
        break;

    case WM_NCHITTEST: {
        if (m_isFullscreen) break;
        POINT pt = {GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
        ScreenToClient(msg->hwnd, &pt);

        // Windows gives physical pixels; width()/height() and the margins are logical
        // (Qt) pixels. Convert, or at 150 % scaling a third of the window would act
        // as a resize border.
        const qreal dpr = devicePixelRatioF();
        const int x = static_cast<int>(pt.x / dpr);
        const int y = static_cast<int>(pt.y / dpr);
        const int w = width();
        const int h = height();
        const int m = m_borderMargin;   // 16px
        const int c = m_cornerMargin;   // 32px

        if (x <= c && y <= c) { *result = HTTOPLEFT; return true; }
        if (x >= w - c && y <= c) { *result = HTTOPRIGHT; return true; }
        if (x <= c && y >= h - c) { *result = HTBOTTOMLEFT; return true; }
        if (x >= w - c && y >= h - c) { *result = HTBOTTOMRIGHT; return true; }
        if (x <= m) { *result = HTLEFT; return true; }
        if (x >= w - m) { *result = HTRIGHT; return true; }
        if (y <= m) { *result = HTTOP; return true; }
        if (y >= h - m) { *result = HTBOTTOM; return true; }
        break;
    }

    case WM_SIZING: {
        if (m_isFullscreen) break;
        RECT* rect = reinterpret_cast<RECT*>(msg->lParam);
        const double ratio = m_aspectManager.aspectRatio();
        const qreal dpr = devicePixelRatioF();
        const int minW = static_cast<int>(std::round(m_aspectManager.minimumSize().width() * dpr));

        // The rect is in physical pixels; keep the aspect ratio and the minimum size.
        int w = std::max(static_cast<int>(rect->right - rect->left), minW);
        int h = static_cast<int>(std::round(w / ratio));

        switch (msg->wParam) {
        case WMSZ_TOP:
        case WMSZ_BOTTOM: {
            h = std::max(static_cast<int>(rect->bottom - rect->top),
                         static_cast<int>(std::round(minW / ratio)));
            w = static_cast<int>(std::round(h * ratio));
            if (msg->wParam == WMSZ_TOP) rect->top = rect->bottom - h; else rect->bottom = rect->top + h;
            rect->right = rect->left + w;
            break;
        }
        case WMSZ_LEFT:
        case WMSZ_TOPLEFT:
        case WMSZ_BOTTOMLEFT:
            rect->left = rect->right - w;
            if (msg->wParam == WMSZ_TOPLEFT) rect->top = rect->bottom - h; else rect->bottom = rect->top + h;
            break;
        default: // WMSZ_RIGHT, WMSZ_TOPRIGHT, WMSZ_BOTTOMRIGHT
            rect->right = rect->left + w;
            if (msg->wParam == WMSZ_TOPRIGHT) rect->top = rect->bottom - h; else rect->bottom = rect->top + h;
            break;
        }
        *result = TRUE;
        return true;
    }

    default:
        break;
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::updateNowPlayingMetadata() {
    if (!m_platform) return;
    if (!m_player || !m_player->hasMedia()) {
        m_platform->clearNowPlayingInfo();
        return;
    }
    NowPlayingInfo info;
    info.title = m_player->mediaInfo().title.isEmpty() ? m_player->mediaInfo().fileName : m_player->mediaInfo().title;
    info.duration = m_player->duration();
    info.position = m_player->position();
    info.playbackRate = (m_player->state() == PlaybackState::Playing) ? m_player->speed() : 0.0;
    m_platform->setNowPlayingInfo(info);
}

} // namespace kvikplay
