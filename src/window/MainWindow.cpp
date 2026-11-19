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
    , m_renderer(new VideoRenderer(this))
    , m_overlay(new PlaybackOverlay(this))
    , m_contextMenu(new ContextMenu(this))
    , m_cursorTimer(new QTimer(this)) {

    // Frameless window setup
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setAcceptDrops(true);

    setStyleSheet(QStringLiteral("background-color: #000000;"));

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

    // Player signal connections
    connect(m_player.get(), &MediaPlayer::stateChanged, this, &MainWindow::onPlaybackStateChanged);
    connect(m_player.get(), &MediaPlayer::mediaLoaded, this, &MainWindow::onMediaLoaded);
    connect(m_player.get(), &MediaPlayer::videoParamsChanged, this, &MainWindow::onVideoParamsChanged);
    connect(m_player.get(), &MediaPlayer::volumeChanged, this, &MainWindow::onVolumeChanged);
    connect(m_player.get(), &MediaPlayer::muteChanged, this, &MainWindow::onMuteChanged);
    connect(m_player.get(), &MediaPlayer::mediaEnded, this, &MainWindow::onMediaEnded);
    connect(m_player.get(), &MediaPlayer::errorOccurred, this, &MainWindow::onErrorOccurred);

    // Context menu connections
    connect(m_contextMenu, &ContextMenu::playPauseTriggered, m_player.get(), &MediaPlayer::togglePlayPause);
    connect(m_contextMenu, &ContextMenu::alwaysOnTopToggled, this, &MainWindow::setAlwaysOnTop);
    connect(m_contextMenu, &ContextMenu::fullscreenToggled, this, &MainWindow::toggleFullscreen);
    connect(m_contextMenu, &ContextMenu::fileAssociationsTriggered, this, &MainWindow::showFileAssociationsDialog);
    connect(m_contextMenu, &ContextMenu::aboutTriggered, this, &MainWindow::showAboutDialog);
    connect(m_contextMenu, &ContextMenu::closeTriggered, this, &QWidget::close);

    // Fullscreen auto-hide cursor timer
    m_cursorTimer->setSingleShot(true);
    connect(m_cursorTimer, &QTimer::timeout, this, &MainWindow::onCursorInactivityTimeout);
}

MainWindow::~MainWindow() = default;

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

    if (settings.alwaysOnTop()) {
        setAlwaysOnTop(true);
    }

    m_overlay->setVolume(settings.volume());
    m_overlay->setMuted(settings.isMuted());

    // Native platform window setup
    if (m_platform) {
        m_platform->setupFramelessWindow(this);
        m_platform->setWindowAspectRatio(this, m_aspectManager.aspectRatio(), savedSize.width(), savedSize.height());
    }

    return true;
}

void MainWindow::openFile(const QString& filePath) {
    m_player->openMedia(filePath);
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
    }

    updateRendererGeometry();
}

void MainWindow::onMediaLoaded(const MediaInfo& info) {
    m_renderer->setMediaInfo(info);
    updateRendererGeometry();
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

void MainWindow::onMediaEnded() {
    m_overlay->setPlaying(false);
    m_contextMenu->setPlaying(false);
}

void MainWindow::onErrorOccurred(const QString& error) {
    m_overlay->showOsd(error);
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
        // Exit fullscreen
        m_isFullscreen = false;
        m_contextMenu->setFullscreen(false);
        showNormal();
        if (m_normalGeometry.isValid()) {
            setGeometry(m_normalGeometry);
        }
        setCursor(Qt::ArrowCursor);
        m_cursorTimer->stop();
    } else {
        // Enter fullscreen
        m_normalGeometry = geometry();
        m_isFullscreen = true;
        m_contextMenu->setFullscreen(true);
        showFullScreen();
        m_cursorTimer->start(2500);
    }
    updateRendererGeometry();
}

void MainWindow::onCursorInactivityTimeout() {
    if (m_isFullscreen) {
        setCursor(Qt::BlankCursor);
        m_overlay->hideOverlay();
    }
}

void MainWindow::updateRendererGeometry() {
    if (m_isFullscreen) {
        // Center video preserving aspect ratio on the fullscreen black canvas
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
}

void MainWindow::resizeEvent(QResizeEvent* /*event*/) {
    updateRendererGeometry();
    if (!m_isFullscreen) {
        SettingsManager::instance().setWindowSize(size());
        SettingsManager::instance().setWindowPosition(pos());
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!m_isFullscreen) {
        SettingsManager::instance().setWindowSize(size());
        SettingsManager::instance().setWindowPosition(pos());
    }
    SettingsManager::instance().save();
    QWidget::closeEvent(event);
}

Qt::Edges MainWindow::hitTestEdges(const QPoint& pos) const {
    if (m_isFullscreen) return Qt::Edges();

    Qt::Edges edges;
    if (pos.x() <= m_borderMargin) edges |= Qt::LeftEdge;
    if (pos.x() >= width() - m_borderMargin) edges |= Qt::RightEdge;
    if (pos.y() <= m_borderMargin) edges |= Qt::TopEdge;
    if (pos.y() >= height() - m_borderMargin) edges |= Qt::BottomEdge;

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
    if (event->button() == Qt::LeftButton) {
        Qt::Edges edges = hitTestEdges(event->pos());
        if (edges != Qt::Edges() && windowHandle()) {
            windowHandle()->startSystemResize(edges);
            event->accept();
            return;
        }

        // Click on client area moves the frameless window
        if (!m_isFullscreen && windowHandle()) {
            windowHandle()->startSystemMove();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    // Show overlay and handle cursors
    m_overlay->showOverlay();

    if (m_isFullscreen) {
        setCursor(Qt::ArrowCursor);
        m_cursorTimer->start(2500);
    } else {
        Qt::Edges edges = hitTestEdges(event->pos());
        updateCursorForEdges(edges);
    }

    QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_player->togglePlayPause();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent* event) {
    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;
    int delta = !numDegrees.isNull() ? numDegrees.y() : numPixels.y();

    if (delta == 0) {
        event->ignore();
        return;
    }

    Qt::KeyboardModifiers mods = event->modifiers();

    if (mods & Qt::ControlModifier) {
        // Ctrl + Wheel: Volume adjustment (+/- 5%)
        int step = (delta > 0) ? 5 : -5;
        int newVol = std::clamp(m_player->volume() + step, 0, 100);
        m_player->setVolume(newVol);
        m_overlay->showOsd(QStringLiteral("Volume %1%").arg(newVol));
    } else if (mods & Qt::ShiftModifier) {
        // Shift + Wheel: Seek +/- 30s
        double seekAmount = (delta > 0) ? 30.0 : -30.0;
        m_player->seekRelative(seekAmount);
        m_overlay->showOsd((delta > 0) ? QStringLiteral("+30s") : QStringLiteral("-30s"));
    } else {
        // Regular Wheel: Seek +/- 10s
        double seekAmount = (delta > 0) ? 10.0 : -10.0;
        m_player->seekRelative(seekAmount);
        m_overlay->showOsd((delta > 0) ? QStringLiteral("+10s") : QStringLiteral("-10s"));
    }

    m_overlay->showOverlay();
    event->accept();
}

void MainWindow::contextMenuEvent(QContextMenuEvent* event) {
    m_contextMenu->exec(event->globalPos());
}

void MainWindow::enterEvent(QEnterEvent* /*event*/) {
    m_overlay->showOverlay();
}

void MainWindow::leaveEvent(QEvent* /*event*/) {
    m_overlay->hideOverlay();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        const auto& urls = event->mimeData()->urls();
        for (const auto& url : urls) {
            if (url.isLocalFile()) {
                QString localPath = url.toLocalFile();
                if (FormatRegistry::instance().isSupported(localPath)) {
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
                if (FormatRegistry::instance().isSupported(localPath)) {
                    openFile(localPath);
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
        break;
    }
    case Qt::Key_Down: {
        int newVol = std::clamp(m_player->volume() - 5, 0, 100);
        m_player->setVolume(newVol);
        m_overlay->showOsd(QStringLiteral("Volume %1%").arg(newVol));
        break;
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

    if (msg->message == WM_NCHITTEST && !m_isFullscreen) {
        POINT pt = {GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
        ScreenToClient(reinterpret_cast<HWND>(winId()), &pt);

        int x = pt.x;
        int y = pt.y;
        int w = width();
        int h = height();
        int m = m_borderMargin;

        if (x <= m && y <= m) { *result = HTTOPLEFT; return true; }
        if (x >= w - m && y <= m) { *result = HTTOPRIGHT; return true; }
        if (x <= m && y >= h - m) { *result = HTBOTTOMLEFT; return true; }
        if (x >= w - m && y >= h - m) { *result = HTBOTTOMRIGHT; return true; }
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
