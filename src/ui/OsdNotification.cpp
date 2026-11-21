#include "OsdNotification.h"
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

namespace kvikplay {

OsdNotification::OsdNotification(QWidget* parent)
    : QWidget(parent)
    , m_fadeAnimation(new QPropertyAnimation(this, "opacity", this))
    , m_hideTimer(new QTimer(this)) {

    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAccessibleName(QStringLiteral("Notification"));

    m_font = font();
    m_font.setPixelSize(14);
    m_font.setBold(true);

    m_fadeAnimation->setDuration(150);
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
    m_text = text;
    setAccessibleDescription(text);

    const QFontMetrics fm(m_font);
    const int w = fm.horizontalAdvance(m_text) + 36;
    const int h = fm.height() + 16;
    resize(w, h);

    if (parentWidget()) {
        const int x = (parentWidget()->width() - w) / 2;
        const int y = parentWidget()->height() / 4;
        move(x, y);
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

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(m_opacity);

    // Semi-transparent white pill with a subtle edge
    QPainterPath path;
    path.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), height() / 2.0, height() / 2.0);
    painter.fillPath(path, QColor(255, 255, 255, 220));
    painter.setPen(QPen(QColor(0, 0, 0, 25), 1.0));
    painter.drawPath(path);

    painter.setFont(m_font);
    painter.setPen(QColor(0x12, 0x13, 0x16));
    painter.drawText(rect(), Qt::AlignCenter | Qt::TextSingleLine, m_text);
}

} // namespace kvikplay
