#pragma once

#include <QWidget>
#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include "VolumeControl.h"
#include "OsdNotification.h"

namespace kvikplay {

class PlaybackOverlay : public QWidget {
    Q_OBJECT

public:
    explicit PlaybackOverlay(QWidget* parent = nullptr);

    void setPlaying(bool playing);
    void setVolume(int percent);
    void setMuted(bool muted);

    void showOsd(const QString& message);

    void showOverlay();
    void hideOverlay();
    void restartAutoHideTimer();

signals:
    void playPauseClicked();
    void seekBackwardClicked();
    void seekForwardClicked();
    void closeClicked();
    void volumeChanged(int percent);
    void muteToggled();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUi();
    void positionControls();

    QPushButton* m_closeButton{nullptr};

    QWidget* m_controlsContainer{nullptr};
    QPushButton* m_seekBackBtn{nullptr};
    QPushButton* m_playPauseBtn{nullptr};
    QPushButton* m_seekFwdBtn{nullptr};

    VolumeControl* m_volumeControl{nullptr};
    OsdNotification* m_osdNotification{nullptr};

    QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    QPropertyAnimation* m_fadeAnimation{nullptr};
    QTimer* m_autoHideTimer{nullptr};

    bool m_isPlaying{false};
};

} // namespace kvikplay
