#include "PlaybackOverlay.h"
#include "TimeFormat.h"
#include <QDateTime>
#include <QFileInfo>
#include <QIcon>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace kvikplay {

namespace {

// Times use tabular figures so they don't jitter while counting.
QFont timeFont(int pixelSize, QFont::Weight weight) {
    QFont f;
    f.setPixelSize(pixelSize);
    f.setWeight(weight);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    f.setFeature(QFont::Tag("tnum"), 1);
#endif
    return f;
}

} // namespace

// ==========================================
// TimelineSlider
// ==========================================

TimelineSlider::TimelineSlider(QWidget* parent)
    : QSlider(Qt::Horizontal, parent) {
    setRange(0, 10000);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_TranslucentBackground);
    setAccessibleName(QStringLiteral("Timeline"));
}

void TimelineSlider::setDuration(double duration) {
    m_duration = duration;
    update();
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

void TimelineSlider::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal cy = height() / 2.0;
    const qreal trackH = 6.0;
    const qreal w = width();
    const qreal played = w * value() / 10000.0;
    const QRectF track(0, cy - trackH / 2, w, trackH);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 62));
    p.drawRoundedRect(track, trackH / 2, trackH / 2);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(QRectF(0, track.top(), played, trackH), trackH / 2, trackH / 2);

    if (m_duration > 0.0) {
        for (const auto& ch : m_chapters) {
            if (ch.time <= 0.0 || ch.time >= m_duration) continue;
            const qreal x = w * ch.time / m_duration;
            p.setBrush(x <= played ? QColor(20, 20, 24, 200) : QColor(255, 255, 255, 230));
            p.drawRoundedRect(QRectF(x - 1, cy - 6, 2, 12), 1, 1);
        }
    }

    const qreal r = (m_hovered || m_dragging) ? 8.0 : 6.0;
    const qreal hx = std::clamp(played, r, w - r);
    p.setBrush(QColor(0, 0, 0, 70)); // soft shadow under the handle
    p.drawEllipse(QPointF(hx, cy + 1), r + 1, r + 1);
    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(hx, cy), r, r);
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
        update();
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
            m_hovered = false;
            emit hoverLeft();
        }
        update();
        event->accept();
        return;
    }
    QSlider::mouseReleaseEvent(event);
}

void TimelineSlider::enterEvent(QEnterEvent* event) {
    m_hovered = true;
    update();
    QSlider::enterEvent(event);
}

void TimelineSlider::leaveEvent(QEvent* event) {
    m_hovered = false;
    update();
    if (!m_dragging) {
        emit hoverLeft();
    }
    QSlider::leaveEvent(event);
}

// ==========================================
// PlaybackOverlay
// ==========================================

QPushButton* PlaybackOverlay::makeGlyphButton(const QString& icon, int size, int iconSize,
                                              const QString& name, const QString& tip) {
    auto* b = new QPushButton(this);
    b->setFixedSize(size, size);
    b->setCursor(Qt::PointingHandCursor);
    b->setFocusPolicy(Qt::NoFocus);
    b->setIcon(QIcon(icon));
    b->setIconSize(QSize(iconSize, iconSize));
    b->setAccessibleName(name);
    b->setToolTip(tip);
    // Bare glyph; a faint circle only while hovered or pressed.
    b->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; border-radius: %1px; }"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.14); }"
        "QPushButton:pressed { background: rgba(255, 255, 255, 0.24); }").arg(size / 2));
    return b;
}

PlaybackOverlay::PlaybackOverlay(QWidget* parent)
    : QWidget(parent)
    , m_titleLabel(new QLabel(this))
    , m_fileLabel(new QLabel(this))
    , m_timelineSlider(new TimelineSlider(this))
    , m_elapsedLabel(new QLabel(this))
    , m_endTimeButton(new QPushButton(this))
    , m_previewCard(new TimelinePreviewCard(this))
    , m_volumeControl(new VolumeControl(this))
    , m_osdNotification(new OsdNotification(this))
    , m_opacityEffect(new QGraphicsOpacityEffect(this))
    , m_fadeAnimation(new QPropertyAnimation(m_opacityEffect, "opacity", this))
    , m_fwdHoldTimer(new QTimer(this)) {

    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(0.0);

    // Title (top left): media title, file name below. Both are untrusted text.
    for (QLabel* l : {m_titleLabel, m_fileLabel, m_elapsedLabel}) {
        l->setTextFormat(Qt::PlainText);
        l->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    m_titleLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 17px; font-weight: 600; background: transparent;"));
    m_fileLabel->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 0.75); font-size: 12px; background: transparent;"));

    m_closeButton = makeGlyphButton(QStringLiteral(":/icons/osd/close.svg"), 44, 16,
                                    QStringLiteral("Close"), QStringLiteral("Close (Q)"));
    connect(m_closeButton, &QPushButton::clicked, this, &PlaybackOverlay::closeClicked);

    // Times at both ends of the timeline; the right one toggles remaining / total.
    m_elapsedLabel->setFont(timeFont(13, QFont::DemiBold));
    m_elapsedLabel->setStyleSheet(QStringLiteral("color: #ffffff; background: transparent;"));
    m_endTimeButton->setFont(timeFont(13, QFont::DemiBold));
    m_endTimeButton->setCursor(Qt::PointingHandCursor);
    m_endTimeButton->setFocusPolicy(Qt::NoFocus);
    m_endTimeButton->setAccessibleName(QStringLiteral("Remaining time; click to show the total length"));
    m_endTimeButton->setStyleSheet(QStringLiteral(
        "QPushButton { color: rgba(255, 255, 255, 0.78); background: transparent; border: none; padding: 0; text-align: right; }"
        "QPushButton:hover { color: #ffffff; }"));
    connect(m_endTimeButton, &QPushButton::clicked, this, &PlaybackOverlay::toggleTimeMode);

    connect(m_timelineSlider, &TimelineSlider::seekRequested, this, &PlaybackOverlay::seekRequested);
    connect(m_timelineSlider, &TimelineSlider::hoverMoved, this, &PlaybackOverlay::onHoverMoved);
    connect(m_timelineSlider, &TimelineSlider::hoverLeft, this, &PlaybackOverlay::onHoverLeft);
    connect(m_timelineSlider, &TimelineSlider::draggingPositionChanged, this, [this](double pos) {
        m_position = pos;
        updateTimeLabels();
    });

    // Bottom row: play, -10, +10 on the left; volume, subtitles, fullscreen on the right.
    m_playPauseBtn = makeGlyphButton(QStringLiteral(":/icons/osd/play.svg"), 48, 24,
                                     QStringLiteral("Play"), QStringLiteral("Play / Pause (Space)"));
    connect(m_playPauseBtn, &QPushButton::clicked, this, &PlaybackOverlay::playPauseClicked);

    m_seekBackBtn = makeGlyphButton(QStringLiteral(":/icons/osd/back10.svg"), 44, 22,
                                    QStringLiteral("Back 10 seconds"), QStringLiteral("Back 10 s (←)"));
    connect(m_seekBackBtn, &QPushButton::clicked, this, &PlaybackOverlay::seekBackwardClicked);

    m_seekFwdBtn = makeGlyphButton(QStringLiteral(":/icons/osd/forward10.svg"), 44, 22,
                                   QStringLiteral("Forward 10 seconds; hold for double speed"),
                                   QStringLiteral("Forward 10 s (→) · hold for 2× speed"));
    // A tap seeks; only a hold (350 ms) starts the 2x boost.
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

    m_subtitlesBtn = makeGlyphButton(QStringLiteral(":/icons/osd/subtitles.svg"), 44, 21,
                                     QStringLiteral("Subtitles"), QStringLiteral("Subtitles"));
    connect(m_subtitlesBtn, &QPushButton::clicked, this, [this]() {
        emit subtitlesClicked(m_subtitlesBtn->mapToGlobal(QPoint(0, 0)));
    });

    m_fullscreenBtn = makeGlyphButton(QStringLiteral(":/icons/osd/fullscreen.svg"), 44, 21,
                                      QStringLiteral("Fullscreen"), QStringLiteral("Fullscreen (F)"));
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &PlaybackOverlay::fullscreenClicked);

    connect(m_volumeControl, &VolumeControl::volumeChanged, this, &PlaybackOverlay::volumeChanged);
    connect(m_volumeControl, &VolumeControl::muteToggled, this, &PlaybackOverlay::muteToggled);

    m_fadeAnimation->setDuration(150);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_opacityEffect->opacity() <= 0.05) {
            hide();
        }
    });

    setTitle(QString(), QString());
    updateTimeLabels();
    hide();
}

void PlaybackOverlay::setTitle(const QString& title, const QString& fileName) {
    m_titleLabel->setText(title.isEmpty() ? fileName : title);
    m_fileLabel->setText(fileName);
    m_fileLabel->setVisible(fileLineAddsSomething());
    m_titleLabel->adjustSize();
    m_fileLabel->adjustSize();
}

void PlaybackOverlay::setPosition(double seconds) {
    m_position = seconds;
    if (!m_timelineSlider->isDragging() && isVisible()) {
        m_timelineSlider->setPosition(seconds);
        int curSec = static_cast<int>(seconds);
        if (curSec != m_lastReportedSec) {
            m_lastReportedSec = curSec;
            updateTimeLabels();
        }
    }
}

void PlaybackOverlay::setDuration(double seconds) {
    m_duration = seconds;
    m_timelineSlider->setDuration(seconds);
    updateTimeLabels();
}

void PlaybackOverlay::setChapters(const std::vector<ChapterInfo>& chapters) {
    m_chapters = chapters;
    m_timelineSlider->setChapters(chapters);
}

bool PlaybackOverlay::fileLineAddsSomething() const {
    // The file name as a second line only when it says more than the title (a metadata
    // title such as "Big Buck Bunny"), not "movie" over "movie.mkv".
    const QString file = m_fileLabel->text();
    const QString title = m_titleLabel->text();
    return !file.isEmpty() && file != title && QFileInfo(file).completeBaseName() != title && height() >= 300;
}

QString PlaybackOverlay::chapterAt(double seconds) const {
    QString title;
    for (const auto& ch : m_chapters) {
        if (ch.time <= seconds) title = ch.title;
    }
    return title;
}

void PlaybackOverlay::toggleTimeMode() {
    m_showTotalTime = !m_showTotalTime;
    m_endTimeButton->setAccessibleName(m_showTotalTime
        ? QStringLiteral("Total length; click to show the remaining time")
        : QStringLiteral("Remaining time; click to show the total length"));
    updateTimeLabels();
}

void PlaybackOverlay::onHoverMoved(double seconds, int localX) {
    if (m_duration <= 0.0) return;
    QPoint sliderPosInOverlay = m_timelineSlider->mapTo(this, QPoint(localX, 0));
    m_previewCard->setTime(seconds, chapterAt(seconds));
    m_previewCard->updatePosition(sliderPosInOverlay.x(), m_timelineSlider->y(), width());
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

void PlaybackOverlay::updateTimeLabels() {
    m_elapsedLabel->setText(formatClock(m_position));
    m_elapsedLabel->adjustSize();
    const QString end = m_showTotalTime || m_duration <= 0.0 ? formatClock(m_duration)
                                                             : formatRemaining(m_position, m_duration);
    m_endTimeButton->setText(end);
    const int w = m_endTimeButton->fontMetrics().horizontalAdvance(end) + 4;
    m_endTimeButton->setFixedSize(w, 20);
    if (width() > 0) {
        m_endTimeButton->move(m_timelineSlider->geometry().right() + 1 - w, m_endTimeButton->y());
    }
}

void PlaybackOverlay::setPlaying(bool playing) {
    if (playing == m_isPlaying && !m_playPauseBtn->icon().isNull()) return;
    m_isPlaying = playing;
    static const QIcon pauseIcon(QStringLiteral(":/icons/osd/pause.svg"));
    static const QIcon playIcon(QStringLiteral(":/icons/osd/play.svg"));
    m_playPauseBtn->setIcon(m_isPlaying ? pauseIcon : playIcon);
    m_playPauseBtn->setAccessibleName(m_isPlaying ? QStringLiteral("Pause") : QStringLiteral("Play"));
}

void PlaybackOverlay::setVolume(int percent) {
    m_volumeControl->setVolume(percent);
}

void PlaybackOverlay::setMuted(bool muted) {
    m_volumeControl->setMuted(muted);
}

void PlaybackOverlay::setFullscreen(bool fullscreen) {
    static const QIcon enter(QStringLiteral(":/icons/osd/fullscreen.svg"));
    static const QIcon exit(QStringLiteral(":/icons/osd/fullscreen_exit.svg"));
    m_fullscreenBtn->setIcon(fullscreen ? exit : enter);
    m_fullscreenBtn->setAccessibleName(fullscreen ? QStringLiteral("Exit fullscreen") : QStringLiteral("Fullscreen"));
}

void PlaybackOverlay::showOsd(const QString& message) {
    m_osdNotification->showMessage(message);
}

void PlaybackOverlay::showVolumeLevel(int percent, bool muted) {
    m_osdNotification->showLevel(percent, muted);
}

void PlaybackOverlay::showOverlay() {
    m_timelineSlider->setPosition(m_position);
    m_lastReportedSec = static_cast<int>(m_position);
    updateTimeLabels();

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

void PlaybackOverlay::paintEvent(QPaintEvent* /*event*/) {
    // Soft scrims behind the title and the controls, so white text and glyphs stay
    // readable on bright frames without any boxes.
    QPainter p(this);
    const int w = width();
    const int h = height();

    const int topH = std::min(140, static_cast<int>(h * 0.3));
    QLinearGradient top(0, 0, 0, topH);
    top.setColorAt(0.0, QColor(0, 0, 0, 128));
    top.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(QRect(0, 0, w, topH), top);

    const int bottomH = std::min(240, static_cast<int>(h * 0.55));
    QLinearGradient bottom(0, h, 0, h - bottomH);
    bottom.setColorAt(0.0, QColor(0, 0, 0, 178));
    bottom.setColorAt(0.45, QColor(0, 0, 0, 64));
    bottom.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(QRect(0, h - bottomH, w, bottomH), bottom);
}

void PlaybackOverlay::resizeEvent(QResizeEvent* /*event*/) {
    positionControls();
}

void PlaybackOverlay::positionControls() {
    const int w = width();
    const int h = height();
    const int m = w < 600 ? 16 : 28;   // side margin
    const bool compact = w < 480;      // small window: only the essentials

    // Top: title and file name left, close right.
    m_titleLabel->move(m, 20);
    m_titleLabel->setMaximumWidth(std::max(80, w - 2 * m - 60));
    m_fileLabel->move(m, 20 + m_titleLabel->sizeHint().height() + 2);
    m_fileLabel->setMaximumWidth(std::max(80, w - 2 * m - 60));
    m_fileLabel->setVisible(fileLineAddsSomething());
    m_closeButton->move(w - m + 10 - m_closeButton->width(), 10);

    // Bottom row of buttons.
    const int rowY = h - 8 - 48;
    int x = m - 12;
    m_playPauseBtn->move(x, rowY);
    x += m_playPauseBtn->width() + 2;
    m_seekBackBtn->setVisible(!compact);
    m_seekFwdBtn->setVisible(!compact);
    if (!compact) {
        m_seekBackBtn->move(x, rowY + 2);
        x += m_seekBackBtn->width() + 2;
        m_seekFwdBtn->move(x, rowY + 2);
    }

    int rx = w - m + 12;
    rx -= m_fullscreenBtn->width();
    m_fullscreenBtn->move(rx, rowY + 2);
    m_subtitlesBtn->setVisible(!compact);
    if (!compact) {
        rx -= m_subtitlesBtn->width() + 2;
        m_subtitlesBtn->move(rx, rowY + 2);
    }
    rx -= m_volumeControl->width() + 2;
    // VolumeControl is tall (its slider opens upwards); align its button with the row.
    m_volumeControl->move(rx, rowY + 2 + 44 - m_volumeControl->height());

    // Full-width timeline above the buttons, times above its ends.
    const int sliderH = 20;
    const int sliderY = rowY - 6 - sliderH;
    m_timelineSlider->setGeometry(m, sliderY, w - 2 * m, sliderH);
    m_elapsedLabel->move(m, sliderY - 20);
    m_endTimeButton->move(w - m - m_endTimeButton->width(), sliderY - 21);
    updateTimeLabels();
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
