#pragma once

#include <QWidget>
#include <QImage>
#include <QPixmap>

namespace kvikplay {

// Hover preview above the timeline: the frame with a thin white border and rounded
// corners, and a small dark pill underneath with the time and chapter ("3:47 · Intro").
// Painted by hand; the chapter title is untrusted and drawn as plain text.
class TimelinePreviewCard : public QWidget {
    Q_OBJECT

public:
    explicit TimelinePreviewCard(QWidget* parent = nullptr);

    void setTime(double seconds, const QString& chapter = QString());
    void setThumbnail(const QImage& img);
    void clearThumbnail();
    void updatePosition(int overlayX, int timelineTopY, int parentWidth);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_frame;
    QString m_caption;
};

} // namespace kvikplay
