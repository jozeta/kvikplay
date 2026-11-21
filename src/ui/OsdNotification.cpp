#include "OsdNotification.h"
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

namespace kvikplay {

namespace {
constexpr int kRing = 132; // level ring diameter
}

OsdNotification::OsdNotification(QWidget* parent)
    : QWidget(parent)
    , m_fadeAnimation(new QPropertyAnimation(this, "opacity", this))
    , m_hideTimer(new QTimer(this)) {

    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAccessibleName(QStringLiteral("Notification"));

    m_font = font();
    m_font.setPixelSize(15);
    m_font.setWeight(QFont::DemiBold);

    m_fadeAnimation->setDuration(160);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_opacity <= 0.01) {
            hide();
        }
    });

    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        m_fadeAnimation->stop();
        m_fadeAnimation->setStartValue(m_opacity);
        m_fadeAnimation->setEndValue(0.0);
        m_fadeAnimation->start();
    });

    hide();
}

void OsdNotification::setOpacity(qreal opacity) {
    m_opacity = opacity;
    update();
}

void OsdNotification::showMessage(const QString& text, int durationMs) {
    m_mode = Mode::Text;
    m_text = text;
    setAccessibleDescription(text);
    const QFontMetrics fm(m_font);
    resize(fm.horizontalAdvance(m_text) + 44, fm.height() + 24);
    present(durationMs);
}

void OsdNotification::showLevel(int percent, bool muted, int durationMs) {
    m_mode = Mode::Level;
    m_level = std::clamp(percent, 0, 100);
    m_muted = muted;
    setAccessibleDescription(muted ? QStringLiteral("Muted") : QStringLiteral("Volume %1%").arg(m_level));
    resize(kRing + 8, kRing + 8);
    present(durationMs);
}

void OsdNotification::present(int durationMs) {
    if (parentWidget()) {
        // A little above the middle, where the eye already is.
        move((parentWidget()->width() - width()) / 2, parentWidget()->height() * 45 / 100 - height() / 2);
    }
    show();
    raise();
    update();

    m_hideTimer->stop();
    m_fadeAnimation->stop();
    m_fadeAnimation->setStartValue(m_opacity);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
    m_hideTimer->start(durationMs);
}

void OsdNotification::paintEvent(QPaintEvent* /*event*/) {
    if (m_opacity <= 0.0) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(m_opacity);
    p.setPen(Qt::NoPen);

    if (m_mode == Mode::Text) {
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        p.setBrush(QColor(12, 12, 14, 150));
        p.drawRoundedRect(r, r.height() / 2, r.height() / 2);
        p.setFont(m_font);
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter | Qt::TextSingleLine, m_text);
        return;
    }

    // Level ring: dark disc, faint full ring, white arc for the level.
    const QRectF disc(4, 4, kRing, kRing);
    p.setBrush(QColor(12, 12, 14, 135));
    p.drawEllipse(disc);
    const QRectF ring = disc.adjusted(5, 5, -5, -5);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(255, 255, 255, 46), 4));
    p.drawEllipse(ring);
    if (!m_muted && m_level > 0) {
        p.setPen(QPen(Qt::white, 4, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(ring, 90 * 16, -static_cast<int>(360 * 16 * m_level / 100.0));
    }

    static const QIcon volumeIcon(QStringLiteral(":/icons/osd/volume.svg"));
    static const QIcon muteIcon(QStringLiteral(":/icons/osd/mute.svg"));
    const QIcon& icon = (m_muted || m_level == 0) ? muteIcon : volumeIcon;
    icon.paint(&p, QRect(static_cast<int>(disc.center().x()) - 15, static_cast<int>(disc.center().y()) - 26, 30, 30));

    p.setFont(m_font);
    p.setPen(Qt::white);
    p.drawText(QRectF(disc.left(), disc.center().y() + 8, disc.width(), 22), Qt::AlignCenter,
               m_muted ? QStringLiteral("Muted") : QStringLiteral("%1%").arg(m_level));
}

} // namespace kvikplay
