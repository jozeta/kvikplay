#include "PlaybackOverlay.h"
#include <QHBoxLayout>
#include <QIcon>
#include <QMouseEvent>

namespace kvikplay {

PlaybackOverlay::PlaybackOverlay(QWidget* parent)
    : QWidget(parent)
    , m_closeButton(new QPushButton(this))
    , m_controlsContainer(new QWidget(this))
    , m_seekBackBtn(new QPushButton(m_controlsContainer))
    , m_playPauseBtn(new QPushButton(m_controlsContainer))
    , m_seekFwdBtn(new QPushButton(m_controlsContainer))
    , m_volumeControl(new VolumeControl(this))
    , m_osdNotification(new OsdNotification(this))
    , m_opacityEffect(new QGraphicsOpacityEffect(this))
    , m_fadeAnimation(new QPropertyAnimation(m_opacityEffect, "opacity", this))
    , m_autoHideTimer(new QTimer(this)) {

    setAttribute(Qt::WA_NoSystemBackground);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(0.0);

    setupUi();

    m_fadeAnimation->setDuration(150);

    m_autoHideTimer->setSingleShot(true);
    connect(m_autoHideTimer, &QTimer::timeout, this, &PlaybackOverlay::hideOverlay);

    hide();
}

void PlaybackOverlay::setupUi() {
    // Styles for minimal Nordic semi-transparent white buttons
    const QString buttonStyle = QStringLiteral(
        "QPushButton {"
        "  background-color: rgba(255, 255, 255, 0.85);"
        "  border: 1px solid rgba(0, 0, 0, 0.1);"
        "  border-radius: %1px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 1.0);"
        "  border-color: rgba(0, 0, 0, 0.2);"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(230, 230, 230, 1.0);"
        "}"
    );

    // 1. Close Button (top-right)
    m_closeButton->setFixedSize(36, 36);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setIcon(QIcon(QStringLiteral(":/icons/close.svg")));
    m_closeButton->setIconSize(QSize(18, 18));
    m_closeButton->setStyleSheet(buttonStyle.arg(18));
    connect(m_closeButton, &QPushButton::clicked, this, &PlaybackOverlay::closeClicked);

    // 2. Playback Controls (center-bottom)
    auto* centerLayout = new QHBoxLayout(m_controlsContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(14);
    centerLayout->setAlignment(Qt::AlignCenter);

    m_seekBackBtn->setFixedSize(40, 40);
    m_seekBackBtn->setCursor(Qt::PointingHandCursor);
    m_seekBackBtn->setIcon(QIcon(QStringLiteral(":/icons/seek_backward.svg")));
    m_seekBackBtn->setIconSize(QSize(20, 20));
    m_seekBackBtn->setStyleSheet(buttonStyle.arg(20));
    connect(m_seekBackBtn, &QPushButton::clicked, this, &PlaybackOverlay::seekBackwardClicked);

    m_playPauseBtn->setFixedSize(52, 52);
    m_playPauseBtn->setCursor(Qt::PointingHandCursor);
    m_playPauseBtn->setIcon(QIcon(QStringLiteral(":/icons/play.svg")));
    m_playPauseBtn->setIconSize(QSize(26, 26));
    m_playPauseBtn->setStyleSheet(buttonStyle.arg(26));
    connect(m_playPauseBtn, &QPushButton::clicked, this, &PlaybackOverlay::playPauseClicked);

    m_seekFwdBtn->setFixedSize(40, 40);
    m_seekFwdBtn->setCursor(Qt::PointingHandCursor);
    m_seekFwdBtn->setIcon(QIcon(QStringLiteral(":/icons/seek_forward.svg")));
    m_seekFwdBtn->setIconSize(QSize(20, 20));
    m_seekFwdBtn->setStyleSheet(buttonStyle.arg(20));
    connect(m_seekFwdBtn, &QPushButton::clicked, this, &PlaybackOverlay::seekForwardClicked);

    centerLayout->addWidget(m_seekBackBtn);
    centerLayout->addWidget(m_playPauseBtn);
    centerLayout->addWidget(m_seekFwdBtn);
    m_controlsContainer->adjustSize();

    // 3. Volume Control (bottom-right)
    connect(m_volumeControl, &VolumeControl::volumeChanged, this, &PlaybackOverlay::volumeChanged);
    connect(m_volumeControl, &VolumeControl::muteToggled, this, &PlaybackOverlay::muteToggled);
}

void PlaybackOverlay::setPlaying(bool playing) {
    m_isPlaying = playing;
    if (m_isPlaying) {
        m_playPauseBtn->setIcon(QIcon(QStringLiteral(":/icons/pause.svg")));
    } else {
        m_playPauseBtn->setIcon(QIcon(QStringLiteral(":/icons/play.svg")));
    }
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
    show();
    m_fadeAnimation->stop();
    m_fadeAnimation->setStartValue(m_opacityEffect->opacity());
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
    restartAutoHideTimer();
}

void PlaybackOverlay::hideOverlay() {
    m_fadeAnimation->stop();
    m_fadeAnimation->setStartValue(m_opacityEffect->opacity());
    m_fadeAnimation->setEndValue(0.0);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_opacityEffect->opacity() <= 0.05) {
            hide();
        }
    });
    m_fadeAnimation->start();
}

void PlaybackOverlay::restartAutoHideTimer() {
    m_autoHideTimer->start(1800);
}

void PlaybackOverlay::resizeEvent(QResizeEvent* /*event*/) {
    positionControls();
}

void PlaybackOverlay::positionControls() {
    const int margin = 20;

    // Top-right close button
    m_closeButton->move(width() - m_closeButton->width() - margin, margin);

    // Center bottom playback controls
    int centerW = m_controlsContainer->width();
    int centerH = m_controlsContainer->height();
    m_controlsContainer->move((width() - centerW) / 2, height() - centerH - margin);

    // Bottom right volume control
    int volW = m_volumeControl->width();
    int volH = m_volumeControl->height();
    m_volumeControl->move(width() - volW - margin, height() - volH - margin);
}

} // namespace kvikplay
