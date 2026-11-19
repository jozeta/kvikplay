#pragma once

#include <QRect>
#include <QSize>

namespace kvikplay {

class AspectRatioManager {
public:
    AspectRatioManager();

    void setAspectRatio(double ratio);
    double aspectRatio() const { return m_aspectRatio; }

    void setMinimumSize(const QSize& minSize);
    QSize minimumSize() const { return m_minSize; }

    // Fit a desired size to the aspect ratio respecting bounds
    QSize calculateSizeForWidth(int width) const;
    QSize calculateSizeForHeight(int height) const;

    // Constrain a proposed window rect during interactive resizing
    // edges: combination of Qt::Edge flags
    QRect constrainResizeRect(const QRect& initialRect, const QRect& proposedRect, Qt::Edges edges) const;

    // Calculate window size when new media is loaded
    QSize calculateInitialWindowSize(int mediaWidth, int mediaHeight, const QSize& maxScreenBounds) const;

private:
    double m_aspectRatio{16.0 / 9.0};
    QSize m_minSize{320, 180};
};

} // namespace kvikplay
