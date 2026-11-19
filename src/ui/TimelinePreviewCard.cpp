#include "TimelinePreviewCard.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace kvikplay {

TimelinePreviewCard::TimelinePreviewCard(QWidget* parent)
    : QWidget(parent)
    , m_imageLabel(new QLabel(this))
    , m_timeLabel(new QLabel(this)) {

    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFixedSize(168, 120);

    setStyleSheet(QStringLiteral(
        "kvikplay--TimelinePreviewCard {"
        "  background-color: rgba(20, 20, 20, 0.92);"
        "  border: 1px solid rgba(255, 255, 255, 0.20);"
        "  border-radius: 8px;"
        "}"
    ));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_imageLabel->setFixedSize(160, 90);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  background-color: #0d0d0d;"
        "  border-radius: 5px;"
        "}"
    ));
    layout->addWidget(m_imageLabel, 0, Qt::AlignCenter);

    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  color: #ffffff;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "  background: transparent;"
        "}"
    ));
    layout->addWidget(m_timeLabel, 0, Qt::AlignCenter);

    hide();
}

QString TimelinePreviewCard::formatTime(double seconds) {
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

void TimelinePreviewCard::setTime(double seconds) {
    m_timeLabel->setText(formatTime(seconds));
}

void TimelinePreviewCard::setThumbnail(const QImage& img) {
    if (img.isNull()) {
        clearThumbnail();
        return;
    }

    QPixmap pix = QPixmap::fromImage(img);
    if (pix.width() != 160 || pix.height() != 90) {
        pix = pix.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    m_imageLabel->setPixmap(pix);
}

void TimelinePreviewCard::clearThumbnail() {
    m_imageLabel->clear();
}

void TimelinePreviewCard::updatePosition(int overlayX, int timelineContainerY, int parentWidth) {
    int cardW = width();
    int cardH = height();
    int targetX = overlayX - (cardW / 2);
    targetX = std::clamp(targetX, 8, std::max(8, parentWidth - cardW - 8));
    int targetY = timelineContainerY - cardH - 8;
    move(targetX, targetY);
}

} // namespace kvikplay
