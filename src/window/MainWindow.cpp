#include "MainWindow.h"
#include "config/SettingsManager.h"
#include "formats/FormatRegistry.h"
#include "ui/FileAssociationDialog.h"
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
#include <QCollator>
#include <QDir>
#include <QClipboard>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
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
    , m_osdTimer(new QTimer(this))
    , m_scrollResetTimer(new QTimer(this))
    , m_rightClickHoldTimer(new QTimer(this)) {

    // Frameless window setup with transparency for rounded corners
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, true);
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
    connect(m_overlay, &PlaybackOverlay::volumeChanged, this, [this](int vol) {
        m_player->setVolume(vol);
        m_overlay->showOsd(QStringLiteral("Volume %1%").arg(vol));
    });
    connect(m_overlay, &PlaybackOverlay::muteToggled, m_player.get(), &MediaPlayer::toggleMute);
    connect(m_overlay, &PlaybackOverlay::seekRequested, m_player.get(), &MediaPlayer::seekAbsolute);
    connect(m_overlay, &PlaybackOverlay::thumbnailRequested, this, &MainWindow::onThumbnailRequested);

    if (m_thumbnailWorker) {
        connect(m_thumbnailWorker.get(), &ThumbnailWorker::thumbnailReady, this, &MainWindow::onThumbnailReady);
    }

    // Player signal connections
    connect(m_player.get(), &MediaPlayer::stateChanged, this, &MainWindow::onPlaybackStateChanged);
    connect(m_player.get(), &MediaPlayer::mediaLoaded, this, &MainWindow::onMediaLoaded);
    connect(m_player.get(), &MediaPlayer::videoParamsChanged, this, &MainWindow::onVideoParamsChanged);
    connect(m_player.get(), &MediaPlayer::volumeChanged, this, &MainWindow::onVolumeChanged);
    connect(m_player.get(), &MediaPlayer::muteChanged, this, &MainWindow::onMuteChanged);
    connect(m_player.get(), &MediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player.get(), &MediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player.get(), &MediaPlayer::tracksChanged, this, &MainWindow::onTracksChanged);
    connect(m_player.get(), &MediaPlayer::mediaEnded, this, &MainWindow::onMediaEnded);
    connect(m_player.get(), &MediaPlayer::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(m_player.get(), &MediaPlayer::screenshotTaken, this, &MainWindow::onScreenshotTaken);

    // Context menu connections
    connect(m_contextMenu, &ContextMenu::playPauseTriggered, m_player.get(), &MediaPlayer::togglePlayPause);
    connect(m_contextMenu, &ContextMenu::alwaysOnTopToggled, this, &MainWindow::setAlwaysOnTop);
    connect(m_contextMenu, &ContextMenu::fullscreenToggled, this, &MainWindow::toggleFullscreen);
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
    connect(m_contextMenu, &ContextMenu::clearRecentTriggered, this, [this]() {
        SettingsManager::instance().clearRecentFiles();
        SettingsManager::instance().save();
        m_contextMenu->setRecentFiles({});
    });
    connect(m_contextMenu, &ContextMenu::screenshotTriggered, this, [this]() {
        m_player->takeScreenshot(false);
    });
    connect(m_contextMenu, &ContextMenu::fileAssociationsTriggered, this, &MainWindow::showFileAssociationsDialog);
    connect(m_contextMenu, &ContextMenu::aboutTriggered, this, &MainWindow::showAboutDialog);
    connect(m_contextMenu, &ContextMenu::closeTriggered, this, &QWidget::close);
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
}

MainWindow::~MainWindow() {
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

bool MainWindow::initialize() {
    if (!m_player->initialize()) {
        return false;
    }

    m_renderer->setMpvHandle(m_player->controller()->handle());

    // Restore saved settings
    auto& settings = SettingsManager::instance();
    QSize savedSize = settings.windowSize();
    QPoint savedPos = settings.windowPosition();

    m_aspectManager.setAspectRatio(static_cast<double>(savedSize.width()) / savedSize.height());
    resize(savedSize);

    if (savedPos.x() >= 0 && savedPos.y() >= 0) {
        move(savedPos);
    } else {
        // Center on screen
        if (QScreen* screen = QGuiApplication::primaryScreen()) {
            QRect scrGeo = screen->availableGeometry();
            move(scrGeo.center().x() - savedSize.width() / 2, scrGeo.center().y() - savedSize.height() / 2);
        }
    }

    m_normalGeometry = geometry();

    if (settings.alwaysOnTop()) {
        setAlwaysOnTop(true);
    }

    m_overlay->setVolume(settings.volume());
    m_overlay->setMuted(settings.isMuted());
    m_contextMenu->setReverseTrackpad(settings.reverseTrackpadScrubbing());
    m_contextMenu->setRecentFiles(settings.recentFiles());

    bool dBoost = settings.dialogueBoost();
    m_player->setDialogueBoost(dBoost);
    m_contextMenu->setDialogueBoost(dBoost);

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
    }

    return true;
}

void MainWindow::openFile(const QString& filePath) {
    if (!m_currentFilePath.isEmpty() && m_player->hasMedia()) {
        double curPos = m_player->position();
        double dur = m_player->duration();
        if (curPos > 5.0 && (dur <= 0.0 || curPos < dur - 5.0)) {
            SettingsManager::instance().setResumePosition(m_currentFilePath, curPos);
        }
    }

    m_currentFilePath = filePath;
    SettingsManager::instance().addRecentFile(filePath);
    SettingsManager::instance().save();
    m_contextMenu->setRecentFiles(SettingsManager::instance().recentFiles());

    if (m_thumbnailWorker) {
        m_thumbnailWorker->openFile(filePath);
    }

    m_player->openMedia(filePath);
    m_player->play(); // Auto-play straight away
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
        if (!scr) scr = QGuiApplication::primaryScreen();
        QSize maxBounds = scr ? scr->availableGeometry().size() : QSize(1920, 1080);

        QSize newSize = m_aspectManager.calculateInitialWindowSize(displayWidth, displayHeight, maxBounds);
        QPoint oldCenter = geometry().center();
        resize(newSize);
        move(oldCenter.x() - newSize.width() / 2, oldCenter.y() - newSize.height() / 2);
        m_normalGeometry = geometry();
    }

    updateRendererGeometry();
}

void MainWindow::onMediaLoaded(const MediaInfo& info) {
    m_renderer->setMediaInfo(info);
    m_overlay->setFileName(info.fileName);
    m_overlay->setDuration(info.duration);
    m_contextMenu->setTracks(info.audioTracks, info.videoTracks, info.subtitleTracks);
    m_player->play(); // Ensure immediate playback on open

    double resumePos = SettingsManager::instance().resumePosition(info.filePath);
    if (resumePos > 5.0 && (info.duration <= 0.0 || resumePos < info.duration - 5.0)) {
        m_player->seekAbsolute(resumePos);
        int m = static_cast<int>(resumePos) / 60;
        int s = static_cast<int>(resumePos) % 60;
        m_overlay->showOsd(QStringLiteral("Resumed at %1:%2")
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0')));
    }

    updateRendererGeometry();
}

void MainWindow::onTracksChanged(const std::vector<TrackInfo>& audio,
                                 const std::vector<TrackInfo>& video,
                                 const std::vector<TrackInfo>& sub) {
    m_contextMenu->setTracks(audio, video, sub);
}

void MainWindow::onPlaybackStateChanged(PlaybackState state) {
    bool isPlaying = (state == PlaybackState::Playing);
    m_overlay->setPlaying(isPlaying);
    m_contextMenu->setPlaying(isPlaying);
}

void MainWindow::onVolumeChanged(int percent) {
    m_overlay->setVolume(percent);
}

void MainWindow::onMuteChanged(bool muted) {
    m_overlay->setMuted(muted);
    m_overlay->showOsd(muted ? QStringLiteral("Muted") : QStringLiteral("Unmuted"));
}

void MainWindow::onPositionChanged(double seconds) {
    m_overlay->setPosition(seconds);
}

void MainWindow::onDurationChanged(double seconds) {
    m_overlay->setDuration(seconds);
}

void MainWindow::onMediaEnded() {
    SettingsManager::instance().clearResumePosition(m_currentFilePath);
    SettingsManager::instance().save();
    m_overlay->setPlaying(false);
    m_contextMenu->setPlaying(false);
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
    if (m_thumbnailWorker) {
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
    m_contextMenu->setDialogueBoost(next);
    m_overlay->showOsd(next ? QStringLiteral("Dialogue Boost: ON") : QStringLiteral("Dialogue Boost: OFF"));
}

QStringList MainWindow::getFolderMediaFiles() const {
    if (m_currentFilePath.isEmpty()) return {};
    QFileInfo curFi(m_currentFilePath);
    QDir dir = curFi.dir();
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::NoSort);

    QStringList mediaFiles;
    for (const auto& entry : entries) {
        if (FormatRegistry::instance().isSupported(entry.fileName())) {
            mediaFiles.push_back(entry.absoluteFilePath());
        }
    }

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(mediaFiles.begin(), mediaFiles.end(), [&collator](const QString& a, const QString& b) {
        return collator.compare(a, b) < 0;
    });

    return mediaFiles;
}

void MainWindow::playNextInFolder() {
    QStringList files = getFolderMediaFiles();
    if (files.size() <= 1) return;

    int idx = files.indexOf(m_currentFilePath);
    if (idx >= 0 && idx + 1 < files.size()) {
        QString nextFile = files[idx + 1];
        openFile(nextFile);
        m_overlay->showOsd(QStringLiteral("Next: %1").arg(QFileInfo(nextFile).fileName()));
    }
}

void MainWindow::playPreviousInFolder() {
    QStringList files = getFolderMediaFiles();
    if (files.size() <= 1) return;

    int idx = files.indexOf(m_currentFilePath);
    if (idx > 0) {
        QString prevFile = files[idx - 1];
        openFile(prevFile);
        m_overlay->showOsd(QStringLiteral("Prev: %1").arg(QFileInfo(prevFile).fileName()));
    }
}

void MainWindow::setAlwaysOnTop(bool top) {
    m_isAlwaysOnTop = top;
    m_contextMenu->setAlwaysOnTop(top);
    SettingsManager::instance().setAlwaysOnTop(top);

    Qt::WindowFlags flags = windowFlags();
    if (top) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
    show();
}

void MainWindow::toggleFullscreen() {
    if (m_isFullscreen) {
        // Exit fullscreen -> restore windowed mode size and position
        m_isFullscreen = false;
        m_contextMenu->setFullscreen(false);
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
        m_contextMenu->setFullscreen(true);
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

    QSize minSize = m_aspectManager.minimumSize();
    if (targetW < minSize.width()) {
        targetW = minSize.width();
        targetH = static_cast<int>(std::round(targetW / m_aspectManager.aspectRatio()));
    }
    if (targetH < minSize.height()) {
        targetH = minSize.height();
        targetW = static_cast<int>(std::round(targetH * m_aspectManager.aspectRatio()));
    }

    QPoint oldCenter = geometry().center();
    resize(targetW, targetH);
    move(oldCenter.x() - targetW / 2, oldCenter.y() - targetH / 2);
    m_normalGeometry = geometry();

    m_overlay->showOsd(QStringLiteral("Size: %1%").arg(static_cast<int>(scale * 100)));
    m_overlay->showOverlay();
    m_osdTimer->start(5000);
}

void MainWindow::onOsdInactivityTimeout() {
    m_overlay->hideOverlay();
    if (m_isFullscreen) {
        setCursor(Qt::BlankCursor);
    }
}

void MainWindow::onRightClickHoldTimeout() {
    m_rightClickMenuShown = true;
    m_contextMenu->setReverseTrackpad(SettingsManager::instance().reverseTrackpadScrubbing());
    m_contextMenu->exec(m_rightPressGlobalPos);
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
        }
    }
    QWidget::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
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
    setCursor(Qt::ArrowCursor);

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
            m_overlay->showOsd(QStringLiteral("Volume %1%").arg(newVol));
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
        m_contextMenu->setReverseTrackpad(SettingsManager::instance().reverseTrackpadScrubbing());
        m_contextMenu->exec(event->globalPos());
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
                if (isSubtitleFile(localPath) || FormatRegistry::instance().isSupported(localPath)) {
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
                if (isSubtitleFile(localPath)) {
                    m_player->addSubtitleTrack(localPath);
                    m_overlay->showOsd(QStringLiteral("Subtitles: %1").arg(QFileInfo(localPath).fileName()));
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
    case Qt::Key_BracketRight:
    case Qt::Key_PageDown:
        playNextInFolder();
        break;
    case Qt::Key_BracketLeft:
    case Qt::Key_PageUp:
        playPreviousInFolder();
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
        m_overlay->showOsd(QStringLiteral("Volume %1%").arg(newVol));
        m_overlay->showOverlay();
        m_osdTimer->start(5000);
        event->accept();
        return;
    }
    case Qt::Key_Down: {
        int newVol = std::clamp(m_player->volume() - 5, 0, 100);
        m_player->setVolume(newVol);
        m_overlay->showOsd(QStringLiteral("Volume %1%").arg(newVol));
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
        "QPushButton { background-color: #272b35; color: #ffffff; border: 1px solid #3d4352; border-radius: 6px; padding: 6px 16px; }"
    ));
    aboutBox.setText(QStringLiteral(
        "<h3>Kvikplay</h3>"
        "<p>Fast, minimalistic cross-platform media player.</p>"
        "<p>Built with Nordic minimalism: zero chrome, instant playback, precise aspect-ratio locking.</p>"
        "<p>Engine: <b>libmpv</b> &bull; UI: <b>Qt 6</b></p>"
    ));
    aboutBox.setIconPixmap(QIcon(QStringLiteral(":/icons/app_icon.svg")).pixmap(48, 48));
    aboutBox.exec();
}

#ifdef _WIN32
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    MSG* msg = static_cast<MSG*>(message);

    if (msg->message == WM_MOUSEACTIVATE) {
        m_activatedByMouseClick = true;
        m_lastActivationTime = QDateTime::currentMSecsSinceEpoch();
        *result = MA_ACTIVATEANDEAT;
        return true;
    }

    if (msg->message == WM_NCHITTEST && !m_isFullscreen) {
        POINT pt = {GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
        ScreenToClient(reinterpret_cast<HWND>(winId()), &pt);

        int x = pt.x;
        int y = pt.y;
        int w = width();
        int h = height();
        int m = m_borderMargin;   // 16px
        int c = m_cornerMargin;   // 32px

        if (x <= c && y <= c) { *result = HTTOPLEFT; return true; }
        if (x >= w - c && y <= c) { *result = HTTOPRIGHT; return true; }
        if (x <= c && y >= h - c) { *result = HTBOTTOMLEFT; return true; }
        if (x >= w - c && y >= h - c) { *result = HTBOTTOMRIGHT; return true; }
        if (x <= m) { *result = HTLEFT; return true; }
        if (x >= w - m) { *result = HTRIGHT; return true; }
        if (y <= m) { *result = HTTOP; return true; }
        if (y >= h - m) { *result = HTBOTTOM; return true; }
    } else if (msg->message == WM_SIZING && !m_isFullscreen) {
        RECT* rect = reinterpret_cast<RECT*>(msg->lParam);
        double ratio = m_aspectManager.aspectRatio();

        int w = rect->right - rect->left;
        int h = rect->bottom - rect->top;

        switch (msg->wParam) {
        case WMSZ_LEFT:
        case WMSZ_RIGHT:
            h = static_cast<int>(std::round(w / ratio));
            rect->bottom = rect->top + h;
            break;
        case WMSZ_TOP:
        case WMSZ_BOTTOM:
            w = static_cast<int>(std::round(h * ratio));
            rect->right = rect->left + w;
            break;
        case WMSZ_TOPLEFT:
        case WMSZ_BOTTOMRIGHT:
        case WMSZ_TOPRIGHT:
        case WMSZ_BOTTOMLEFT:
            h = static_cast<int>(std::round(w / ratio));
            if (msg->wParam == WMSZ_TOPLEFT || msg->wParam == WMSZ_TOPRIGHT) {
                rect->top = rect->bottom - h;
            } else {
                rect->bottom = rect->top + h;
            }
            break;
        default:
            break;
        }
        *result = TRUE;
        return true;
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif

} // namespace kvikplay
