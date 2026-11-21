#include "TimelinePreviewCard.h"
#include "TimeFormat.h"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

namespace kvikplay {

namespace {
constexpr int kFrameW = 208;
constexpr int kFrameH = 117;
constexpr int kPillH = 22;
constexpr int kGap = 8;
constexpr int kPad = 4; // room for the border and shadow
}

TimelinePreviewCard::TimelinePreviewCard(QWidget* parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFixedSize(kFrameW + 2 * kPad, kFrameH + kGap + kPillH + 2 * kPad);
    hide();
}

void TimelinePreviewCard::setTime(double seconds, const QString& chapter) {
    m_caption = chapter.isEmpty() ? formatClock(seconds)
                                  : QStringLiteral("%1 · %2").arg(formatClock(seconds), chapter);
    update();
}

void TimelinePreviewCard::setThumbnail(const QImage& img) {
    m_frame = img.isNull() ? QPixmap() : QPixmap::fromImage(img);
    update();
}

void TimelinePreviewCard::clearThumbnail() {
    m_frame = QPixmap();
    update();
}

void TimelinePreviewCard::updatePosition(int overlayX, int timelineTopY, int parentWidth) {
    const int x = std::clamp(overlayX - width() / 2, 8, std::max(8, parentWidth - width() - 8));
    move(x, timelineTopY - height() - 6);
}

void TimelinePreviewCard::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Frame: dark placeholder until the thumbnail arrives, then the picture.
    const QRectF frame(kPad, kPad, kFrameW, kFrameH);
    QPainterPath clip;
    clip.addRoundedRect(frame, 10, 10);
    p.fillPath(clip.translated(0, 3), QColor(0, 0, 0, 90)); // soft shadow
    p.fillPath(clip, QColor(18, 18, 22));
    if (!m_frame.isNull()) {
        p.save();
        p.setClipPath(clip);
        // Fit inside the frame, keeping the picture's shape (vertical video: bars).
        const QSize s = m_frame.size().scaled(kFrameW, kFrameH, Qt::KeepAspectRatio);
        const QRectF target(frame.center().x() - s.width() / 2.0, frame.center().y() - s.height() / 2.0,
                            s.width(), s.height());
        p.drawPixmap(target, m_frame, m_frame.rect());
        p.restore();
    }
    p.setPen(QPen(QColor(255, 255, 255, 230), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(clip);

    // Caption pill, centred under the frame.
    QFont f = font();
    f.setPixelSize(12);
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    const QFontMetrics fm(f);
    const QString text = fm.elidedText(m_caption, Qt::ElideRight, kFrameW - 16);
    const int pillW = fm.horizontalAdvance(text) + 18;
    const QRectF pill(width() / 2.0 - pillW / 2.0, frame.bottom() + kGap, pillW, kPillH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(12, 12, 14, 185));
    p.drawRoundedRect(pill, kPillH / 2.0, kPillH / 2.0);
    p.setPen(Qt::white);
    p.drawText(pill, Qt::AlignCenter | Qt::TextSingleLine, text);
}

} // namespace kvikplay
