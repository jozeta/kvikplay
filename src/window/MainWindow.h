#pragma once

#include <QWidget>
#include <memory>
#include <QPoint>
#include <QTimer>
#include "player/MediaPlayer.h"
#include "player/FolderIndex.h"
#include "update/UpdateManager.h"
#include "player/ThumbnailWorker.h"
#include "rendering/VideoRenderer.h"
#include "window/AspectRatioManager.h"
#include "ui/PlaybackOverlay.h"
#include "ui/ContextMenu.h"
#include "platform/PlatformIntegration.h"

namespace kvikplay {

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    bool initialize(bool cascade = false);
    // Called once after show(): applies "Start in Fullscreen".
    void applyStartupState();

    // Open in this window (drag-and-drop, Open File, recent files, next/previous).
    void openFile(const QString& filePath);
    // Opened from Finder/Explorer: replaces the video or opens a new window,
    // depending on the "Opening from Finder/Explorer" menu choice.
    void openExternalFile(const QString& filePath);
    // A file handed over by a second launch (Windows single instance).
    void openForwardedFile(const QString& filePath);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

    // Mouse & Window interactions
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

    // Drag and drop
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    // Keyboard shortcuts
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

#ifdef _WIN32
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private slots:
    void onVideoParamsChanged(int width, int height, int displayWidth, int displayHeight, double dar);
    void onMediaLoaded(const MediaInfo& info);
    void onPlaybackStateChanged(PlaybackState state);
    void onVolumeChanged(int percent);
    void onMuteChanged(bool muted);
    void onPositionChanged(double seconds);
    void onDurationChanged(double seconds);
    void onTracksChanged(const std::vector<TrackInfo>& audio,
                         const std::vector<TrackInfo>& video,
                         const std::vector<TrackInfo>& sub);
    void onMediaEnded();
    void onErrorOccurred(const QString& error);
    void onScreenshotTaken(const QString& filePath);
    void onThumbnailRequested(double seconds);
    void onThumbnailReady(double seconds, const QImage& image);
    void playNextInFolder();
    void playPreviousInFolder();
    void toggleDialogueBoost();
    void togglePictureInPicture();
    void toggleMediaInspector();
    void showHelpDialog();
    void onChaptersChanged(const std::vector<ChapterInfo>& chapters);
    void onChapterChanged(int index, const QString& title);

    void toggleFullscreen();
    void setAlwaysOnTop(bool top);
    void showFileAssociationsDialog();
    void showAboutDialog();
    void onOsdInactivityTimeout();
    void onRightClickHoldTimeout();
    void applyWindowScale(double scale);

private:
    Qt::Edges hitTestEdges(const QPoint& pos) const;
    void updateCursorForEdges(Qt::Edges edges);
    void updateRendererGeometry();
    void fitAndPositionWindow(const QSize& targetSize, const QPoint& preferredCenter);
    void launchNewWindow(const QString& filePath);
    void beginSpeedBoost();
    void endSpeedBoost();
    void setSpeedAndRemember(double speed);
    void updateNowPlayingMetadata();
    void openFileDialog();
    MenuState buildMenuState();
    void showContextMenu(const QPoint& globalPos);
    void showSubtitleMenu(const QPoint& buttonGlobalPos);

    std::unique_ptr<MediaPlayer> m_player;
    std::unique_ptr<PlatformIntegration> m_platform;
    std::unique_ptr<ThumbnailWorker> m_thumbnailWorker;
    AspectRatioManager m_aspectManager;
    FolderIndex m_folderIndex;
    UpdateManager* m_updates{nullptr};
    bool m_isExtraWindow{false}; // started with --cascade: no update prompt here

    QString m_currentFilePath;
    QString m_pendingOpen; // opened once the video surface is ready

    VideoRenderer* m_renderer{nullptr};
    PlaybackOverlay* m_overlay{nullptr};
    ContextMenu* m_contextMenu{nullptr};
    class MediaInspectorOverlay* m_inspector{nullptr};

    QTimer* m_osdTimer{nullptr};
    QTimer* m_scrollResetTimer{nullptr};
    QTimer* m_rightClickHoldTimer{nullptr};
    QTimer* m_inspectorTimer{nullptr};
    QRect m_normalGeometry;
    QRect m_prePiPGeometry;
    QPoint m_pressPos;
    QPoint m_rightPressPos;
    QPoint m_rightPressGlobalPos;
    int m_accumulatedScrollX{0};
    qint64 m_lastActivationTime{0};
    bool m_activatedByMouseClick{false};
    bool m_wasActiveOnRightPress{false};
    bool m_rightClickMenuShown{false};
    bool m_draggingWindow{false};
    bool m_isFullscreen{false};
    bool m_isAlwaysOnTop{false};
    bool m_isPiP{false};
    bool m_prePiPAlwaysOnTop{false};
    bool m_speedBoostActive{false};
    double m_preBoostSpeed{1.0};
    double m_lastNowPlayingPosition{-1.0};
    qint64 m_lastNowPlayingSyncTimeMs{0};
    const int m_borderMargin{16};
    const int m_cornerMargin{32};
};

} // namespace kvikplay
