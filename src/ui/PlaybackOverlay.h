#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include "VolumeControl.h"
#include "OsdNotification.h"
#include "TimelinePreviewCard.h"

namespace kvikplay {

class TimelineSlider : public QSlider {
    Q_OBJECT

public:
    explicit TimelineSlider(QWidget* parent = nullptr);

    bool isDragging() const { return m_dragging; }

signals:
    void seekRequested(double seconds);
    void draggingPositionChanged(double seconds);
    void hoverMoved(double seconds, int localX);
    void hoverLeft();

public slots:
    void setDuration(double duration);
    void setPosition(double position);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    double ratioFromEvent(QMouseEvent* event) const;

    double m_duration{0.0};
    double m_position{0.0};
    bool m_dragging{false};
    qint64 m_lastSeekDispatchMs{0};
};

class PlaybackOverlay : public QWidget {
    Q_OBJECT

public:
    explicit PlaybackOverlay(QWidget* parent = nullptr);

    void setPlaying(bool playing);
    void setVolume(int percent);
    void setMuted(bool muted);
    void setFileName(const QString& name);
    void setPosition(double seconds);
    void setDuration(double seconds);
    void setThumbnail(double seconds, const QImage& img);

    void showOsd(const QString& message);

    void showOverlay();
    void hideOverlay();

signals:
    void playPauseClicked();
    void seekBackwardClicked();
    void seekForwardClicked();
    void closeClicked();
    void volumeChanged(int percent);
    void muteToggled();
    void seekRequested(double seconds);
    void thumbnailRequested(double seconds);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void toggleTimeMode();
    void onHoverMoved(double seconds, int localX);
    void onHoverLeft();

private:
    void setupUi();
    void positionControls();
    void updateTimeLabel();
    static QString formatTime(double seconds);

    QLabel* m_filenameLabel{nullptr};
    QPushButton* m_closeButton{nullptr};

    // Timeline row
    QWidget* m_timelineContainer{nullptr};
    TimelineSlider* m_timelineSlider{nullptr};
    QPushButton* m_timeLabel{nullptr};
    TimelinePreviewCard* m_previewCard{nullptr};

    // Central buttons
    QWidget* m_controlsContainer{nullptr};
    QPushButton* m_seekBackBtn{nullptr};
    QPushButton* m_playPauseBtn{nullptr};
    QPushButton* m_seekFwdBtn{nullptr};

    VolumeControl* m_volumeControl{nullptr};
    OsdNotification* m_osdNotification{nullptr};

    QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    QPropertyAnimation* m_fadeAnimation{nullptr};

    double m_position{0.0};
    double m_duration{0.0};
    int m_lastReportedSec{-1};
    bool m_isPlaying{false};
    bool m_showRemainingTime{false};
};

} // namespace kvikplay
