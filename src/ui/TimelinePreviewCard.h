#pragma once

#include <QWidget>
#include <QLabel>
#include <QImage>

namespace kvikplay {

class TimelinePreviewCard : public QWidget {
    Q_OBJECT

public:
    explicit TimelinePreviewCard(QWidget* parent = nullptr);

    void setTime(double seconds);
    void setThumbnail(const QImage& img);
    void clearThumbnail();
    void updatePosition(int overlayX, int timelineContainerY, int parentWidth);

private:
    static QString formatTime(double seconds);

    QLabel* m_imageLabel{nullptr};
    QLabel* m_timeLabel{nullptr};
};

} // namespace kvikplay
