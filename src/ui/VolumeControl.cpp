#include "VolumeControl.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QIcon>
#include <algorithm>

namespace kvikplay {

VolumeControl::VolumeControl(QWidget* parent)
    : QWidget(parent)
    , m_iconButton(new QPushButton(this))
    , m_sliderContainer(new QWidget(this))
    , m_slider(new QSlider(Qt::Vertical, m_sliderContainer))
    , m_percentLabel(new QLabel(QStringLiteral("100%"), m_sliderContainer))
    , m_sliderHideTimer(new QTimer(this)) {

    setFixedSize(44, 190);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);

    // Bare white glyph at the bottom; a faint circle only on hover (matches the overlay).
    m_iconButton->setFixedSize(44, 44);
    m_iconButton->setCursor(Qt::PointingHandCursor);
    m_iconButton->setFocusPolicy(Qt::NoFocus);
    m_iconButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; border-radius: 22px; }"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.14); }"
        "QPushButton:pressed { background: rgba(255, 255, 255, 0.24); }"
    ));
    m_iconButton->setIconSize(QSize(21, 21));
    m_iconButton->move(0, 146);
    m_iconButton->setAccessibleName(QStringLiteral("Mute"));
    m_iconButton->setToolTip(QStringLiteral("Mute (M)"));
    connect(m_iconButton, &QPushButton::clicked, this, &VolumeControl::muteToggled);

    // Setup vertical slider container
    m_sliderContainer->setFixedSize(44, 140);
    m_sliderContainer->move(0, 0);
    // Shown and hidden directly: a fade effect here would be nested inside the
    // overlay's own fade effect, which Qt cannot paint.
    m_sliderContainer->hide();

    auto* sliderLayout = new QVBoxLayout(m_sliderContainer);
    sliderLayout->setContentsMargins(4, 8, 4, 8);
    sliderLayout->setSpacing(4);

    m_percentLabel->setAlignment(Qt::AlignCenter);
    m_percentLabel->setTextFormat(Qt::PlainText);
    m_percentLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 11px; font-weight: 600; background: transparent;"));
    sliderLayout->addWidget(m_percentLabel);

    m_slider->setRange(0, 100);
    m_slider->setAccessibleName(QStringLiteral("Volume"));
    m_slider->setValue(m_volume);
    m_slider->setCursor(Qt::PointingHandCursor);
    m_slider->setFocusPolicy(Qt::NoFocus);
    m_slider->setStyleSheet(QStringLiteral(
        "QSlider::groove:vertical {"
        "  background: rgba(255, 255, 255, 0.25);"
        "  width: 4px;"
        "  border-radius: 2px;"
        "}"
        "QSlider::sub-page:vertical {"
        "  background: rgba(255, 255, 255, 0.25);"
        "  border-radius: 2px;"
        "}"
        "QSlider::add-page:vertical {"
        "  background: #ffffff;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:vertical {"
        "  background: #ffffff;"
        "  height: 14px;"
        "  width: 14px;"
        "  margin: 0 -5px;"
        "  border-radius: 7px;"
        "}"
    ));
    sliderLayout->addWidget(m_slider, 1, Qt::AlignHCenter);

    connect(m_slider, &QSlider::valueChanged, this, [this](int val) {
        m_volume = val;
        m_percentLabel->setText(QStringLiteral("%1%").arg(val));
        updateIcon();
        emit volumeChanged(val);
    });

    m_sliderHideTimer->setSingleShot(true);
    connect(m_sliderHideTimer, &QTimer::timeout, this, &VolumeControl::hideSlider);

    updateIcon();
}

void VolumeControl::setVolume(int percent) {
    m_volume = std::clamp(percent, 0, 100);
    m_slider->blockSignals(true);
    m_slider->setValue(m_volume);
    m_percentLabel->setText(QStringLiteral("%1%").arg(m_volume));
    m_slider->blockSignals(false);
    updateIcon();
}

void VolumeControl::setMuted(bool muted) {
    m_isMuted = muted;
    updateIcon();
}

void VolumeControl::updateIcon() {
    // Parsed once, not on every volume step.
    static const QIcon muteIcon(QStringLiteral(":/icons/osd/mute.svg"));
    static const QIcon highIcon(QStringLiteral(":/icons/osd/volume.svg"));
    const bool silent = m_isMuted || m_volume == 0;
    m_iconButton->setIcon(silent ? muteIcon : highIcon);
    m_iconButton->setAccessibleName(m_isMuted ? QStringLiteral("Unmute") : QStringLiteral("Mute"));
}

void VolumeControl::enterEvent(QEnterEvent* /*event*/) {
    m_sliderHideTimer->stop();
    showSlider();
}

void VolumeControl::leaveEvent(QEvent* /*event*/) {
    m_sliderHideTimer->start(300);
}

void VolumeControl::showSlider() {
    m_sliderContainer->show();
    update(); // the rounded background is painted by this widget
}

void VolumeControl::hideSlider() {
    m_sliderContainer->hide();
    update();
}

void VolumeControl::paintEvent(QPaintEvent* /*event*/) {
    if (m_sliderContainer->isVisible()) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Dark panel behind the slider, like the centre messages.
        QRectF sliderRect = QRectF(m_sliderContainer->geometry()).adjusted(2.5, 0.5, -2.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(sliderRect, 19, 19);
        painter.fillPath(path, QColor(12, 12, 14, 185));
        painter.setPen(QPen(QColor(255, 255, 255, 30), 1.0));
        painter.drawPath(path);
    }
}

} // namespace kvikplay
