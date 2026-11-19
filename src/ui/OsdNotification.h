#pragma once

#include <QWidget>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>

namespace kvikplay {

class OsdNotification : public QWidget {
    Q_OBJECT

public:
    explicit OsdNotification(QWidget* parent = nullptr);

    void showMessage(const QString& text, int durationMs = 800);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_label{nullptr};
    QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    QPropertyAnimation* m_fadeAnimation{nullptr};
    QTimer* m_hideTimer{nullptr};
};

} // namespace kvikplay
