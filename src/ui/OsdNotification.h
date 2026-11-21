#pragma once

#include <QWidget>
#include <QIcon>
#include <QPropertyAnimation>
#include <QTimer>

namespace kvikplay {

// Messages in the middle of the picture: a dark capsule for text ("+10 s", "Speed 1.5×")
// or a round ring for the volume level. Paints its own fade instead of using a
// QGraphicsOpacityEffect: it lives inside PlaybackOverlay, which already has one, and
// nested graphics effects make Qt fail to paint.
class OsdNotification : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit OsdNotification(QWidget* parent = nullptr);

    void showMessage(const QString& text, int durationMs = 900);
    void showLevel(int percent, bool muted, int durationMs = 900);

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void present(int durationMs);

    enum class Mode { Text, Level };
    Mode m_mode{Mode::Text};
    QString m_text;   // drawn as plain text: messages may contain file names
    int m_level{0};
    bool m_muted{false};
    QFont m_font;
    qreal m_opacity{0.0};
    QPropertyAnimation* m_fadeAnimation{nullptr};
    QTimer* m_hideTimer{nullptr};
};

} // namespace kvikplay
