#include "PlaybackOverlay.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QIcon>
#include <QMouseEvent>
#include <QDateTime>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace kvikplay {

// ==========================================
// TimelineSlider Implementation
// ==========================================

TimelineSlider::TimelineSlider(QWidget* parent)
    : QSlider(Qt::Horizontal, parent) {
    setRange(0, 10000);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    setStyleSheet(QStringLiteral(
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: rgba(255, 255, 255, 0.3);"
        "  border-radius: 2px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: rgba(255, 255, 255, 0.95);"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #ffffff;"
        "  width: 12px;"
        "  height: 12px;"
        "  margin: -4px 0;"
        "  border-radius: 6px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "  background: #ffffff;"
        "  width: 14px;"
        "  height: 14px;"
        "  margin: -5px 0;"
        "  border-radius: 7px;"
        "}"
    ));
}

void TimelineSlider::setDuration(double duration) {
    m_duration = duration;
}

void TimelineSlider::setPosition(double position) {
    m_position = position;
    if (!m_dragging && m_duration > 0.0) {
        int val = static_cast<int>(std::clamp(position / m_duration, 0.0, 1.0) * 10000);
        blockSignals(true);
        setValue(val);
        blockSignals(false);
    }
}

double TimelineSlider::ratioFromEvent(QMouseEvent* event) const {
    if (width() <= 0) return 0.0;
    return std::clamp(static_cast<double>(event->pos().x()) / width(), 0.0, 1.0);
}

void TimelineSlider::setChapters(const std::vector<ChapterInfo>& chapters) {
    m_chapters = chapters;
    update();
}

void TimelineSlider::paintEvent(QPaintEvent* event) {
    QSlider::paintEvent(event);

    if (m_duration <= 0.0 || m_chapters.empty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int grooveY = height() / 2;
    const int tickHeight = 6;
    const int tickTop = grooveY - (tickHeight / 2);

    for (const auto& ch : m_chapters) {
        if (ch.time <= 0.0 || ch.time >= m_duration) continue;
        double ratio = ch.time / m_duration;
        int x = static_cast<int>(ratio * width());
        painter.fillRect(x - 1, tickTop, 2, tickHeight, QColor(255, 255, 255, 175));
    }
}

void TimelineSlider::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        double ratio = ratioFromEvent(event);
        setValue(static_cast<int>(ratio * 10000));
        double targetSec = ratio * m_duration;
        emit hoverMoved(targetSec, event->pos().x());
        emit draggingPositionChanged(targetSec);
        emit seekRequested(targetSec, false);
        event->accept();
        return;
    }
    QSlider::mousePressEvent(event);
}

void TimelineSlider::mouseMoveEvent(QMouseEvent* event) {
    double ratio = ratioFromEvent(event);
    double targetSec = ratio * m_duration;
    emit hoverMoved(targetSec, event->pos().x());

    if (m_dragging) {
        setValue(static_cast<int>(ratio * 10000));
        emit draggingPositionChanged(targetSec);

        // Instant keyframe seeking during dragging
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - m_lastSeekDispatchMs >= 25) {
            m_lastSeekDispatchMs = now;
            emit seekRequested(targetSec, false);
        }
        event->accept();
        return;
    }
    QSlider::mouseMoveEvent(event);
}

void TimelineSlider::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        double ratio = ratioFromEvent(event);
        setValue(static_cast<int>(ratio * 10000));
        emit seekRequested(ratio * m_duration, true);
        if (!rect().contains(event->pos())) {
            emit hoverLeft();
        }
        event->accept();
        return;
    }
    QSlider::mouseReleaseEvent(event);
}

void TimelineSlider::leaveEvent(QEvent* event) {
    if (!m_dragging) {
        emit hoverLeft();
    }
    QSlider::leaveEvent(event);
}

// ==========================================
// PlaybackOverlay Implementation
// ==========================================

PlaybackOverlay::PlaybackOverlay(QWidget* parent)
    : QWidget(parent)
    , m_filenameLabel(new QLabel(this))
    , m_closeButton(new QPushButton(this))
    , m_timelineContainer(new QWidget(this))
    , m_timelineSlider(new TimelineSlider(m_timelineContainer))
    , m_timeLabel(new QPushButton(QStringLiteral("00:00 / 00:00"), m_timelineContainer))
    , m_controlsContainer(new QWidget(this))
    , m_seekBackBtn(new QPushButton(m_controlsContainer))
    , m_playPauseBtn(new QPushButton(m_controlsContainer))
    , m_seekFwdBtn(new QPushButton(m_controlsContainer))
    , m_volumeControl(new VolumeControl(this))
    , m_osdNotification(new OsdNotification(this))
    , m_previewCard(new TimelinePreviewCard(this))
    , m_opacityEffect(new QGraphicsOpacityEffect(this))
    , m_fadeAnimation(new QPropertyAnimation(m_opacityEffect, "opacity", this))
    , m_fwdHoldTimer(new QTimer(this)) {

    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(0.0);

    setupUi();

    m_fadeAnimation->setDuration(150);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_opacityEffect->opacity() <= 0.05) {
            hide();
        }
    });
    hide();
}

void PlaybackOverlay::setupUi() {
    // Transparent Nordic minimalist button styling - NO black background
    const QString buttonStyle = QStringLiteral(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.35);"
        "  border: 1px solid rgba(255, 255, 255, 0.45);"
        "  border-radius: %1px;"
        "  color: #121316;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 0.65);"
        "  border-color: rgba(255, 255, 255, 0.75);"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(255, 255, 255, 0.85);"
        "}"
    );

    // 1. Filename Label (top-left in small text)
    m_filenameLabel->setStyleSheet(QStringLiteral(
        "color: rgba(255, 255, 255, 0.95);"
        "font-size: 11px;"
        "font-weight: 600;"
        "background-color: rgba(255, 255, 255, 0.2);"
        "border: 1px solid rgba(255, 255, 255, 0.3);"
        "border-radius: 4px;"
        "padding: 3px 8px;"
    ));
    m_filenameLabel->setTextFormat(Qt::PlainText); // file names are untrusted: never HTML
    m_filenameLabel->setText(QStringLiteral("No media"));
    m_filenameLabel->adjustSize();

    // 2. Close Button (top-right)
    m_closeButton->setFixedSize(34, 34);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFocusPolicy(Qt::NoFocus);
    m_closeButton->setIcon(QIcon(QStringLiteral(":/icons/close.svg")));
    m_closeButton->setIconSize(QSize(16, 16));
    m_closeButton->setStyleSheet(buttonStyle.arg(17));
    m_closeButton->setAccessibleName(QStringLiteral("Close"));
    m_closeButton->setToolTip(QStringLiteral("Close (Q)"));
    connect(m_closeButton, &QPushButton::clicked, this, &PlaybackOverlay::closeClicked);

    // 3. Timeline row (timeline slider + time display on right)
    auto* timelineLayout = new QHBoxLayout(m_timelineContainer);
    timelineLayout->setContentsMargins(0, 0, 0, 0);
    timelineLayout->setSpacing(14);

    m_timeLabel->setCursor(Qt::PointingHandCursor);
    m_timeLabel->setFocusPolicy(Qt::NoFocus);
    m_timeLabel->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  color: #ffffff;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "  background: transparent;"
        "  border: none;"
        "  padding: 2px 4px;"
        "}"
        "QPushButton:hover {"
        "  color: rgba(255, 255, 255, 0.75);"
        "}"
    ));
    m_timeLabel->setAccessibleName(QStringLiteral("Elapsed time; click to show remaining time"));
    connect(m_timeLabel, &QPushButton::clicked, this, &PlaybackOverlay::toggleTimeMode);
    m_timelineSlider->setAccessibleName(QStringLiteral("Timeline"));

    timelineLayout->addWidget(m_timelineSlider, 1);
    timelineLayout->addWidget(m_timeLabel, 0, Qt::AlignVCenter);

    connect(m_timelineSlider, &TimelineSlider::seekRequested, this, &PlaybackOverlay::seekRequested);
    connect(m_timelineSlider, &TimelineSlider::hoverMoved, this, &PlaybackOverlay::onHoverMoved);
    connect(m_timelineSlider, &TimelineSlider::hoverLeft, this, &PlaybackOverlay::onHoverLeft);
    connect(m_timelineSlider, &TimelineSlider::draggingPositionChanged, this, [this](double pos) {
        m_position = pos;
        updateTimeLabel();
    });

    // 4. Playback Controls (center-bottom)
    auto* centerLayout = new QHBoxLayout(m_controlsContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(14);
    centerLayout->setAlignment(Qt::AlignCenter);

    m_seekBackBtn->setFixedSize(40, 40);
    m_seekBackBtn->setCursor(Qt::PointingHandCursor);
    m_seekBackBtn->setFocusPolicy(Qt::NoFocus);
    m_seekBackBtn->setIcon(QIcon(QStringLiteral(":/icons/seek_backward.svg")));
    m_seekBackBtn->setIconSize(QSize(20, 20));
    m_seekBackBtn->setStyleSheet(buttonStyle.arg(20));
    m_seekBackBtn->setAccessibleName(QStringLiteral("Back 10 seconds"));
    m_seekBackBtn->setToolTip(QStringLiteral("Back 10 s (←)"));
    connect(m_seekBackBtn, &QPushButton::clicked, this, &PlaybackOverlay::seekBackwardClicked);

    m_playPauseBtn->setFixedSize(52, 52);
    m_playPauseBtn->setCursor(Qt::PointingHandCursor);
    m_playPauseBtn->setFocusPolicy(Qt::NoFocus);
    m_playPauseBtn->setIcon(QIcon(QStringLiteral(":/icons/play.svg")));
    m_playPauseBtn->setIconSize(QSize(26, 26));
    m_playPauseBtn->setStyleSheet(buttonStyle.arg(26));
    m_playPauseBtn->setAccessibleName(QStringLiteral("Play"));
    m_playPauseBtn->setToolTip(QStringLiteral("Play / Pause (Space)"));
    connect(m_playPauseBtn, &QPushButton::clicked, this, &PlaybackOverlay::playPauseClicked);

    m_seekFwdBtn->setFixedSize(40, 40);
    m_seekFwdBtn->setCursor(Qt::PointingHandCursor);
    m_seekFwdBtn->setFocusPolicy(Qt::NoFocus);
    m_seekFwdBtn->setIcon(QIcon(QStringLiteral(":/icons/seek_forward.svg")));
    m_seekFwdBtn->setIconSize(QSize(20, 20));
    m_seekFwdBtn->setStyleSheet(buttonStyle.arg(20));
    m_seekFwdBtn->setAccessibleName(QStringLiteral("Forward 10 seconds; hold for double speed"));
    m_seekFwdBtn->setToolTip(QStringLiteral("Forward 10 s (→) · hold for 2× speed"));
    // A tap seeks; only a hold (350 ms) starts the 2x boost. Before, every tap also
    // flashed "Speed 2.0x" / "Speed 1.0x" and then seeked.
    m_fwdHoldTimer->setSingleShot(true);
    m_fwdHoldTimer->setInterval(350);
    connect(m_fwdHoldTimer, &QTimer::timeout, this, [this]() {
        m_fwdBoosting = true;
        emit speedBoostRequested(true);
    });
    connect(m_seekFwdBtn, &QPushButton::pressed, this, [this]() {
        m_fwdBoosting = false;
        m_fwdHoldTimer->start();
    });
    connect(m_seekFwdBtn, &QPushButton::released, this, [this]() {
        if (m_fwdHoldTimer->isActive()) {
            m_fwdHoldTimer->stop();
            emit seekForwardClicked();
        } else if (m_fwdBoosting) {
            m_fwdBoosting = false;
            emit speedBoostRequested(false);
        }
    });

    centerLayout->addWidget(m_seekBackBtn);
    centerLayout->addWidget(m_playPauseBtn);
    centerLayout->addWidget(m_seekFwdBtn);
    m_controlsContainer->adjustSize();

    // 5. Volume Control (bottom-right)
    connect(m_volumeControl, &VolumeControl::volumeChanged, this, &PlaybackOverlay::volumeChanged);
    connect(m_volumeControl, &VolumeControl::muteToggled, this, &PlaybackOverlay::muteToggled);
}

void PlaybackOverlay::setFileName(const QString& name) {
    m_filenameLabel->setText(name);
    m_filenameLabel->adjustSize();
}

void PlaybackOverlay::setPosition(double seconds) {
    m_position = seconds;
    if (!m_timelineSlider->isDragging() && isVisible()) {
        m_timelineSlider->setPosition(seconds);
        int curSec = static_cast<int>(seconds);
        if (curSec != m_lastReportedSec) {
            m_lastReportedSec = curSec;
            updateTimeLabel();
        }
    }
}

void PlaybackOverlay::setDuration(double seconds) {
    m_duration = seconds;
    m_timelineSlider->setDuration(seconds);
    updateTimeLabel();
}

void PlaybackOverlay::setChapters(const std::vector<ChapterInfo>& chapters) {
    m_timelineSlider->setChapters(chapters);
}

void PlaybackOverlay::toggleTimeMode() {
    m_showRemainingTime = !m_showRemainingTime;
    updateTimeLabel();
}

void PlaybackOverlay::onHoverMoved(double seconds, int localX) {
    if (m_duration <= 0.0) return;
    QPoint sliderPosInOverlay = m_timelineSlider->mapTo(this, QPoint(localX, 0));
    m_previewCard->updatePosition(sliderPosInOverlay.x(), m_timelineContainer->y(), width());
    m_previewCard->setTime(seconds);
    m_previewCard->show();
    m_previewCard->raise();
    emit thumbnailRequested(seconds);
}

void PlaybackOverlay::onHoverLeft() {
    m_previewCard->hide();
    m_previewCard->clearThumbnail();
}

void PlaybackOverlay::setThumbnail(double /*seconds*/, const QImage& img) {
    if (m_previewCard && m_previewCard->isVisible()) {
        m_previewCard->setThumbnail(img);
    }
}

QString PlaybackOverlay::formatTime(double seconds) {
    if (seconds < 0.0) seconds = 0.0;
    int totalSec = static_cast<int>(std::round(seconds));
    int h = totalSec / 3600;
    int m = (totalSec % 3600) / 60;
    int s = totalSec % 60;
    if (h > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void PlaybackOverlay::updateTimeLabel() {
    QString currentStr;
    if (m_showRemainingTime) {
        double remaining = std::max(0.0, m_duration - m_position);
        currentStr = QStringLiteral("-%1").arg(formatTime(remaining));
    } else {
        currentStr = formatTime(m_position);
    }
    m_timeLabel->setText(QStringLiteral("%1 / %2").arg(currentStr, formatTime(m_duration)));
}

void PlaybackOverlay::setPlaying(bool playing) {
    if (playing == m_isPlaying && !m_playPauseBtn->icon().isNull()) return;
    m_isPlaying = playing;
    // Parsed once, not on every state change.
    static const QIcon pauseIcon(QStringLiteral(":/icons/pause.svg"));
    static const QIcon playIcon(QStringLiteral(":/icons/play.svg"));
    m_playPauseBtn->setIcon(m_isPlaying ? pauseIcon : playIcon);
    m_playPauseBtn->setAccessibleName(m_isPlaying ? QStringLiteral("Pause") : QStringLiteral("Play"));
}

void PlaybackOverlay::setVolume(int percent) {
    m_volumeControl->setVolume(percent);
}

void PlaybackOverlay::setMuted(bool muted) {
    m_volumeControl->setMuted(muted);
}

void PlaybackOverlay::showOsd(const QString& message) {
    m_osdNotification->showMessage(message);
}

void PlaybackOverlay::showOverlay() {
    m_timelineSlider->setPosition(m_position);
    m_lastReportedSec = static_cast<int>(m_position);
    updateTimeLabel();

    if (!isVisible()) {
        show();
    }

    if (m_opacityEffect->opacity() < 0.99 || (m_fadeAnimation->state() == QAbstractAnimation::Running && m_fadeAnimation->endValue().toDouble() != 1.0)) {
        m_fadeAnimation->stop();
        m_fadeAnimation->setStartValue(m_opacityEffect->opacity());
        m_fadeAnimation->setEndValue(1.0);
        m_fadeAnimation->start();
    }
}

void PlaybackOverlay::hideOverlay() {
    if (m_previewCard) {
        m_previewCard->hide();
    }
    m_fadeAnimation->stop();
    m_fadeAnimation->setStartValue(m_opacityEffect->opacity());
    m_fadeAnimation->setEndValue(0.0);
    m_fadeAnimation->start();
}

void PlaybackOverlay::resizeEvent(QResizeEvent* /*event*/) {
    positionControls();
}

void PlaybackOverlay::positionControls() {
    const int margin = 20;

    // Top-left filename label
    m_filenameLabel->move(margin, margin);

    // Top-right close button
    m_closeButton->move(width() - m_closeButton->width() - margin, margin);

    // Bottom center controls
    int centerW = m_controlsContainer->width();
    int centerH = m_controlsContainer->height();
    int controlsY = height() - centerH - 14;
    m_controlsContainer->move((width() - centerW) / 2, controlsY);

    // Timeline row across lower area - with generous 16px space above the controls
    int timelineH = 24;
    int timelineY = controlsY - timelineH - 16;
    m_timelineContainer->setGeometry(margin, timelineY, width() - (margin * 2), timelineH);

    // Bottom right volume control
    int volW = m_volumeControl->width();
    int volH = m_volumeControl->height();
    m_volumeControl->move(width() - volW - margin, height() - volH - 10);
}

void PlaybackOverlay::mousePressEvent(QMouseEvent* event) {
    if (parentWidget()) {
        QMouseEvent parentEvent(event->type(), event->position(), event->globalPosition(),
                                event->button(), event->buttons(), event->modifiers());
        QCoreApplication::sendEvent(parentWidget(), &parentEvent);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void PlaybackOverlay::mouseMoveEvent(QMouseEvent* event) {
    if (parentWidget()) {
        QMouseEvent parentEvent(event->type(), event->position(), event->globalPosition(),
                                event->button(), event->buttons(), event->modifiers());
        QCoreApplication::sendEvent(parentWidget(), &parentEvent);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void PlaybackOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (parentWidget()) {
        QMouseEvent parentEvent(event->type(), event->position(), event->globalPosition(),
                                event->button(), event->buttons(), event->modifiers());
        QCoreApplication::sendEvent(parentWidget(), &parentEvent);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void PlaybackOverlay::mouseDoubleClickEvent(QMouseEvent* event) {
    if (parentWidget()) {
        QMouseEvent parentEvent(event->type(), event->position(), event->globalPosition(),
                                event->button(), event->buttons(), event->modifiers());
        QCoreApplication::sendEvent(parentWidget(), &parentEvent);
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void PlaybackOverlay::wheelEvent(QWheelEvent* event) {
    if (parentWidget()) {
        QWheelEvent parentEvent(event->position(), event->globalPosition(),
                                event->pixelDelta(), event->angleDelta(),
                                event->buttons(), event->modifiers(),
                                event->phase(), event->inverted());
        QCoreApplication::sendEvent(parentWidget(), &parentEvent);
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

void PlaybackOverlay::contextMenuEvent(QContextMenuEvent* event) {
    // Consume context menu event so the menu only appears on a 0.5s right-click hold
    event->accept();
}

} // namespace kvikplay
