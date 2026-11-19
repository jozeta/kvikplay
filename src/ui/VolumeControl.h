#pragma once

#include <QWidget>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace kvikplay {

class VolumeControl : public QWidget {
    Q_OBJECT

public:
    explicit VolumeControl(QWidget* parent = nullptr);

    void setVolume(int percent);
    int volume() const { return m_volume; }

    void setMuted(bool muted);
    bool isMuted() const { return m_isMuted; }

signals:
    void volumeChanged(int percent);
    void muteToggled();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateIcon();
    void showSlider();
    void hideSlider();

    QPushButton* m_iconButton{nullptr};
    QWidget* m_sliderContainer{nullptr};
    QSlider* m_slider{nullptr};
    QLabel* m_percentLabel{nullptr};

    QGraphicsOpacityEffect* m_sliderOpacity{nullptr};
    QPropertyAnimation* m_sliderAnim{nullptr};
    QTimer* m_sliderHideTimer{nullptr};

    int m_volume{100};
    bool m_isMuted{false};
};

} // namespace kvikplay
