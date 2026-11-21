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

    setFixedSize(46, 190);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);

    // Setup icon button at bottom - transparent Nordic glass
    m_iconButton->setFixedSize(38, 38);
    m_iconButton->setCursor(Qt::PointingHandCursor);
    m_iconButton->setFocusPolicy(Qt::NoFocus);
    m_iconButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.35);"
        "  border: 1px solid rgba(255, 255, 255, 0.45);"
        "  border-radius: 19px;"
        "  color: #121316;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 0.65);"
        "}"
    ));
    m_iconButton->setIconSize(QSize(20, 20));
    m_iconButton->move(4, 146);
    m_iconButton->setAccessibleName(QStringLiteral("Mute"));
    m_iconButton->setToolTip(QStringLiteral("Mute (M)"));
    connect(m_iconButton, &QPushButton::clicked, this, &VolumeControl::muteToggled);

    // Setup vertical slider container
    m_sliderContainer->setFixedSize(42, 140);
    m_sliderContainer->move(2, 2);
    // Shown and hidden directly: a fade effect here would be nested inside the
    // overlay's own fade effect, which Qt cannot paint.
    m_sliderContainer->hide();

    auto* sliderLayout = new QVBoxLayout(m_sliderContainer);
    sliderLayout->setContentsMargins(4, 8, 4, 8);
    sliderLayout->setSpacing(4);

    m_percentLabel->setAlignment(Qt::AlignCenter);
    m_percentLabel->setTextFormat(Qt::PlainText);
    m_percentLabel->setStyleSheet(QStringLiteral("color: #121316; font-size: 11px; font-weight: bold;"));
    sliderLayout->addWidget(m_percentLabel);

    m_slider->setRange(0, 100);
    m_slider->setAccessibleName(QStringLiteral("Volume"));
    m_slider->setValue(m_volume);
    m_slider->setCursor(Qt::PointingHandCursor);
    m_slider->setFocusPolicy(Qt::NoFocus);
    m_slider->setStyleSheet(QStringLiteral(
        "QSlider::groove:vertical {"
        "  background: rgba(0, 0, 0, 0.15);"
        "  width: 4px;"
        "  border-radius: 2px;"
        "}"
        "QSlider::sub-page:vertical {"
        "  background: rgba(0, 0, 0, 0.15);"
        "  border-radius: 2px;"
        "}"
        "QSlider::add-page:vertical {"
        "  background: #121316;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:vertical {"
        "  background: #121316;"
        "  height: 14px;"
        "  width: 14px;"
        "  margin: 0 -5px;"
        "  border-radius: 7px;"
        "}"
        "QSlider::handle:vertical:hover {"
        "  background: #000000;"
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
    static const QIcon muteIcon(QStringLiteral(":/icons/volume_mute.svg"));
    static const QIcon highIcon(QStringLiteral(":/icons/volume_high.svg"));
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

        QRect sliderRect = m_sliderContainer->geometry();
        QPainterPath path;
        path.addRoundedRect(sliderRect, 20, 20);
        painter.fillPath(path, QColor(255, 255, 255, 150));
        painter.setPen(QPen(QColor(255, 255, 255, 120), 1.0));
        painter.drawPath(path);
    }
}

} // namespace kvikplay
