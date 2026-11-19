#include "OsdNotification.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

namespace kvikplay {

OsdNotification::OsdNotification(QWidget* parent)
    : QWidget(parent)
    , m_label(new QLabel(this))
    , m_opacityEffect(new QGraphicsOpacityEffect(this))
    , m_fadeAnimation(new QPropertyAnimation(m_opacityEffect, "opacity", this))
    , m_hideTimer(new QTimer(this)) {

    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(0.0);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(18, 8, 18, 8);
    layout->addWidget(m_label);

    m_label->setStyleSheet(QStringLiteral("color: #121316; font-size: 14px; font-weight: bold; background: transparent;"));
    m_label->setAlignment(Qt::AlignCenter);

    m_fadeAnimation->setDuration(150);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_opacityEffect->opacity() <= 0.01) {
            hide();
        }
    });

    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        m_fadeAnimation->stop();
        m_fadeAnimation->setStartValue(m_opacityEffect->opacity());
        m_fadeAnimation->setEndValue(0.0);
        m_fadeAnimation->start();
    });

    hide();
}

void OsdNotification::showMessage(const QString& text, int durationMs) {
    m_label->setText(text);
    adjustSize();

    if (parentWidget()) {
        int x = (parentWidget()->width() - width()) / 2;
        int y = parentWidget()->height() / 4;
        move(x, y);
    }

    show();
    raise();

    m_hideTimer->stop();
    m_fadeAnimation->stop();
    m_fadeAnimation->setStartValue(m_opacityEffect->opacity());
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();

    m_hideTimer->start(durationMs);
}

void OsdNotification::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Nordic minimalist semi-transparent white pill
    QPainterPath path;
    path.addRoundedRect(rect(), height() / 2.0, height() / 2.0);

    painter.fillPath(path, QColor(255, 255, 255, 220));

    // Subtle edge border
    painter.setPen(QPen(QColor(0, 0, 0, 25), 1.0));
    painter.drawPath(path);
}

} // namespace kvikplay
