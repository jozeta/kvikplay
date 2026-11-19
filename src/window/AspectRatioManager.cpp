#include "AspectRatioManager.h"
#include <algorithm>
#include <cmath>

namespace kvikplay {

AspectRatioManager::AspectRatioManager() = default;

void AspectRatioManager::setAspectRatio(double ratio) {
    if (ratio > 0.05 && ratio < 20.0) {
        m_aspectRatio = ratio;
        // Keep minimum dimensions aligned with ratio
        int minH = static_cast<int>(std::round(m_minSize.width() / m_aspectRatio));
        if (minH < 180) {
            minH = 180;
            m_minSize.setWidth(static_cast<int>(std::round(minH * m_aspectRatio)));
            m_minSize.setHeight(minH);
        } else {
            m_minSize.setHeight(minH);
        }
    }
}

void AspectRatioManager::setMinimumSize(const QSize& minSize) {
    if (minSize.width() > 100 && minSize.height() > 100) {
        m_minSize = minSize;
    }
}

QSize AspectRatioManager::calculateSizeForWidth(int width) const {
    int w = std::max(width, m_minSize.width());
    int h = static_cast<int>(std::round(w / m_aspectRatio));
    if (h < m_minSize.height()) {
        h = m_minSize.height();
        w = static_cast<int>(std::round(h * m_aspectRatio));
    }
    return QSize(w, h);
}

QSize AspectRatioManager::calculateSizeForHeight(int height) const {
    int h = std::max(height, m_minSize.height());
    int w = static_cast<int>(std::round(h * m_aspectRatio));
    if (w < m_minSize.width()) {
        w = m_minSize.width();
        h = static_cast<int>(std::round(w / m_aspectRatio));
    }
    return QSize(w, h);
}

QRect AspectRatioManager::constrainResizeRect(const QRect& initialRect, const QRect& proposedRect, Qt::Edges edges) const {
    int newWidth = proposedRect.width();
    int newHeight = proposedRect.height();

    bool isCorner = ((edges & (Qt::LeftEdge | Qt::RightEdge)) && (edges & (Qt::TopEdge | Qt::BottomEdge)));
    bool isVerticalEdge = (edges == Qt::TopEdge || edges == Qt::BottomEdge);
    bool isHorizontalEdge = (edges == Qt::LeftEdge || edges == Qt::RightEdge);

    if (isVerticalEdge) {
        // Height drove the change, calculate width
        newHeight = std::max(newHeight, m_minSize.height());
        newWidth = static_cast<int>(std::round(newHeight * m_aspectRatio));
        if (newWidth < m_minSize.width()) {
            newWidth = m_minSize.width();
            newHeight = static_cast<int>(std::round(newWidth / m_aspectRatio));
        }
    } else if (isHorizontalEdge) {
        // Width drove the change, calculate height
        newWidth = std::max(newWidth, m_minSize.width());
        newHeight = static_cast<int>(std::round(newWidth / m_aspectRatio));
        if (newHeight < m_minSize.height()) {
            newHeight = m_minSize.height();
            newWidth = static_cast<int>(std::round(newHeight * m_aspectRatio));
        }
    } else if (isCorner) {
        // Dragging a corner: pick dimension with the greater relative change
        double widthRatioChange = std::abs(static_cast<double>(newWidth) - initialRect.width()) / std::max(1, initialRect.width());
        double heightRatioChange = std::abs(static_cast<double>(newHeight) - initialRect.height()) / std::max(1, initialRect.height());

        if (widthRatioChange >= heightRatioChange) {
            newWidth = std::max(newWidth, m_minSize.width());
            newHeight = static_cast<int>(std::round(newWidth / m_aspectRatio));
            if (newHeight < m_minSize.height()) {
                newHeight = m_minSize.height();
                newWidth = static_cast<int>(std::round(newHeight * m_aspectRatio));
            }
        } else {
            newHeight = std::max(newHeight, m_minSize.height());
            newWidth = static_cast<int>(std::round(newHeight * m_aspectRatio));
            if (newWidth < m_minSize.width()) {
                newWidth = m_minSize.width();
                newHeight = static_cast<int>(std::round(newWidth / m_aspectRatio));
            }
        }
    } else {
        // Default
        newWidth = std::max(newWidth, m_minSize.width());
        newHeight = static_cast<int>(std::round(newWidth / m_aspectRatio));
    }

    // Anchor the stationary corner/edge
    int left = initialRect.left();
    int top = initialRect.top();

    if (edges & Qt::LeftEdge) {
        left = initialRect.right() - newWidth + 1;
    } else if (edges & Qt::RightEdge) {
        left = initialRect.left();
    } else {
        // Horizontally centered if dragging top/bottom only
        left = initialRect.center().x() - newWidth / 2;
    }

    if (edges & Qt::TopEdge) {
        top = initialRect.bottom() - newHeight + 1;
    } else if (edges & Qt::BottomEdge) {
        top = initialRect.top();
    } else {
        // Vertically centered if dragging left/right only
        top = initialRect.center().y() - newHeight / 2;
    }

    return QRect(left, top, newWidth, newHeight);
}

QSize AspectRatioManager::calculateInitialWindowSize(int mediaWidth, int mediaHeight, const QSize& maxScreenBounds) const {
    if (mediaWidth <= 0 || mediaHeight <= 0) {
        return QSize(960, 540);
    }

    int targetW = mediaWidth;
    int targetH = mediaHeight;

    int maxW = static_cast<int>(maxScreenBounds.width() * 0.85);
    int maxH = static_cast<int>(maxScreenBounds.height() * 0.85);

    if (targetW > maxW || targetH > maxH) {
        double scaleW = static_cast<double>(maxW) / targetW;
        double scaleH = static_cast<double>(maxH) / targetH;
        double scale = std::min(scaleW, scaleH);
        targetW = static_cast<int>(std::round(targetW * scale));
        targetH = static_cast<int>(std::round(targetH * scale));
    }

    // Ensure within minimums
    if (targetW < m_minSize.width()) {
        targetW = m_minSize.width();
        targetH = static_cast<int>(std::round(targetW / m_aspectRatio));
    }
    if (targetH < m_minSize.height()) {
        targetH = m_minSize.height();
        targetW = static_cast<int>(std::round(targetH * m_aspectRatio));
    }

    return QSize(targetW, targetH);
}

} // namespace kvikplay
