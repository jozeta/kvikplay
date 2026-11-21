#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>

namespace kvikplay {

// Short on-screen message ("+10s", "Volume 75%"). Paints its own fade instead of using
// a QGraphicsOpacityEffect: it lives inside PlaybackOverlay, which already has one, and
// nested graphics effects make Qt fail to paint ("Painter not active").
class OsdNotification : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit OsdNotification(QWidget* parent = nullptr);

    void showMessage(const QString& text, int durationMs = 800);

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_text;   // drawn as plain text: messages may contain file names
    QFont m_font;
    qreal m_opacity{0.0};
    QPropertyAnimation* m_fadeAnimation{nullptr};
    QTimer* m_hideTimer{nullptr};
};

} // namespace kvikplay
