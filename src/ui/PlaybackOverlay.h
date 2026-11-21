#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include "VolumeControl.h"
#include "OsdNotification.h"
#include "TimelinePreviewCard.h"
#include "player/MediaInfo.h"

namespace kvikplay {

// Full-width timeline, painted by hand (no QSlider style): a 6 px track, the played
// part in white, chapter ticks and a round handle that grows while hovered.
class TimelineSlider : public QSlider {
    Q_OBJECT

public:
    explicit TimelineSlider(QWidget* parent = nullptr);

    bool isDragging() const { return m_dragging; }
    void setChapters(const std::vector<ChapterInfo>& chapters);

signals:
    void seekRequested(double seconds, bool exact);
    void draggingPositionChanged(double seconds);
    void hoverMoved(double seconds, int localX);
    void hoverLeft();

public slots:
    void setDuration(double duration);
    void setPosition(double position);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    double ratioFromEvent(QMouseEvent* event) const;

    double m_duration{0.0};
    double m_position{0.0};
    bool m_dragging{false};
    bool m_hovered{false};
    qint64 m_lastSeekDispatchMs{0};
    std::vector<ChapterInfo> m_chapters;
};

// The "cinematic" overlay: no boxes, just soft scrims at the top and bottom, the title
// top-left, a full-width timeline with times at both ends, and bare white glyph buttons.
class PlaybackOverlay : public QWidget {
    Q_OBJECT

public:
    explicit PlaybackOverlay(QWidget* parent = nullptr);

    void setPlaying(bool playing);
    void setVolume(int percent);
    void setMuted(bool muted);
    void setFullscreen(bool fullscreen);
    void setTitle(const QString& title, const QString& fileName);
    void setPosition(double seconds);
    void setDuration(double seconds);
    void setChapters(const std::vector<ChapterInfo>& chapters);
    void setThumbnail(double seconds, const QImage& img);

    // Short message in the centre ("+10 s", "Speed 1.5×").
    void showOsd(const QString& message);
    // Volume in the centre as a ring.
    void showVolumeLevel(int percent, bool muted);

    void showOverlay();
    void hideOverlay();

signals:
    void playPauseClicked();
    void seekBackwardClicked();
    void seekForwardClicked();
    void closeClicked();
    void fullscreenClicked();
    void subtitlesClicked(const QPoint& globalPos);
    void volumeChanged(int percent);
    void muteToggled();
    void seekRequested(double seconds, bool exact);
    void speedBoostRequested(bool active);
    void thumbnailRequested(double seconds);

protected:
    void paintEvent(QPaintEvent* event) override;
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
    QPushButton* makeGlyphButton(const QString& icon, int size, int iconSize, const QString& name,
                                 const QString& tip);
    void positionControls();
    void updateTimeLabels();
    QString chapterAt(double seconds) const;
    bool fileLineAddsSomething() const;

    QLabel* m_titleLabel{nullptr};
    QLabel* m_fileLabel{nullptr};
    QPushButton* m_closeButton{nullptr};

    TimelineSlider* m_timelineSlider{nullptr};
    QLabel* m_elapsedLabel{nullptr};
    QPushButton* m_endTimeButton{nullptr};
    TimelinePreviewCard* m_previewCard{nullptr};

    QPushButton* m_playPauseBtn{nullptr};
    QPushButton* m_seekBackBtn{nullptr};
    QPushButton* m_seekFwdBtn{nullptr};
    VolumeControl* m_volumeControl{nullptr};
    QPushButton* m_subtitlesBtn{nullptr};
    QPushButton* m_fullscreenBtn{nullptr};

    OsdNotification* m_osdNotification{nullptr};

    QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    QPropertyAnimation* m_fadeAnimation{nullptr};

    std::vector<ChapterInfo> m_chapters;
    double m_position{0.0};
    double m_duration{0.0};
    int m_lastReportedSec{-1};
    bool m_isPlaying{false};
    bool m_showTotalTime{false}; // right-hand time: remaining (default) or total

    // ⏩: a tap seeks +10 s, holding it plays at 2x until released.
    QTimer* m_fwdHoldTimer{nullptr};
    bool m_fwdBoosting{false};
};

} // namespace kvikplay
