#pragma once

#include <QWidget>
#include <memory>
#include <QPoint>
#include <QTimer>
#include "player/MediaPlayer.h"
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

    bool initialize();
    void openFile(const QString& filePath);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

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

    void toggleFullscreen();
    void setAlwaysOnTop(bool top);
    void showFileAssociationsDialog();
    void showAboutDialog();
    void onOsdInactivityTimeout();
    void applyWindowScale(double scale);

private:
    Qt::Edges hitTestEdges(const QPoint& pos) const;
    void updateCursorForEdges(Qt::Edges edges);
    void updateRendererGeometry();

    std::unique_ptr<MediaPlayer> m_player;
    std::unique_ptr<PlatformIntegration> m_platform;
    AspectRatioManager m_aspectManager;

    VideoRenderer* m_renderer{nullptr};
    PlaybackOverlay* m_overlay{nullptr};
    ContextMenu* m_contextMenu{nullptr};

    QTimer* m_osdTimer{nullptr};
    QTimer* m_scrollResetTimer{nullptr};
    QRect m_normalGeometry;
    QPoint m_pressPos;
    int m_accumulatedScrollX{0};
    bool m_wasActiveOnPress{false};
    bool m_draggingWindow{false};
    bool m_isFullscreen{false};
    bool m_isAlwaysOnTop{false};
    const int m_borderMargin{8};
};

} // namespace kvikplay
