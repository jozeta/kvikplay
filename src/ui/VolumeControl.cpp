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
    , m_sliderOpacity(new QGraphicsOpacityEffect(m_sliderContainer))
    , m_sliderAnim(new QPropertyAnimation(m_sliderOpacity, "opacity", this))
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
    connect(m_iconButton, &QPushButton::clicked, this, &VolumeControl::muteToggled);

    // Setup vertical slider container
    m_sliderContainer->setFixedSize(42, 140);
    m_sliderContainer->move(2, 2);
    m_sliderContainer->setGraphicsEffect(m_sliderOpacity);
    m_sliderOpacity->setOpacity(0.0);
    m_sliderContainer->hide();

    auto* sliderLayout = new QVBoxLayout(m_sliderContainer);
    sliderLayout->setContentsMargins(4, 8, 4, 8);
    sliderLayout->setSpacing(4);

    m_percentLabel->setAlignment(Qt::AlignCenter);
    m_percentLabel->setStyleSheet(QStringLiteral("color: #121316; font-size: 11px; font-weight: bold;"));
    sliderLayout->addWidget(m_percentLabel);

    m_slider->setRange(0, 100);
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

    m_sliderAnim->setDuration(150);
    connect(m_sliderAnim, &QPropertyAnimation::finished, this, [this]() {
        if (m_sliderOpacity->opacity() <= 0.05) {
            m_sliderContainer->hide();
        }
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
    if (m_isMuted || m_volume == 0) {
        m_iconButton->setIcon(QIcon(QStringLiteral(":/icons/volume_mute.svg")));
    } else {
        m_iconButton->setIcon(QIcon(QStringLiteral(":/icons/volume_high.svg")));
    }
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
    m_sliderAnim->stop();
    m_sliderAnim->setStartValue(m_sliderOpacity->opacity());
    m_sliderAnim->setEndValue(1.0);
    m_sliderAnim->start();
}

void VolumeControl::hideSlider() {
    m_sliderAnim->stop();
    m_sliderAnim->setStartValue(m_sliderOpacity->opacity());
    m_sliderAnim->setEndValue(0.0);
    m_sliderAnim->start();
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
