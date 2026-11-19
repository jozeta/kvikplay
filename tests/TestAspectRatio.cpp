#include <cassert>
#include <iostream>
#include <cmath>
#include "window/AspectRatioManager.h"

int main() {
    using namespace kvikplay;
    AspectRatioManager manager;

    std::cout << "[TestAspectRatio] Checking aspect ratio calculations..." << std::endl;

    // Default 16:9
    manager.setAspectRatio(16.0 / 9.0);
    assert(std::abs(manager.aspectRatio() - (16.0 / 9.0)) < 0.001);

    // Size calculation for width
    QSize s1 = manager.calculateSizeForWidth(1600);
    assert(s1.width() == 1600);
    assert(s1.height() == 900);

    // Size calculation for height
    QSize s2 = manager.calculateSizeForHeight(1080);
    assert(s2.width() == 1920);
    assert(s2.height() == 1080);

    // Respect minimum size
    QSize sMin = manager.calculateSizeForWidth(50);
    assert(sMin.width() >= manager.minimumSize().width());
    assert(sMin.height() >= manager.minimumSize().height());

    // 4:3 Aspect ratio
    manager.setAspectRatio(4.0 / 3.0);
    QSize s43 = manager.calculateSizeForWidth(800);
    assert(s43.width() == 800);
    assert(s43.height() == 600);

    // 9:16 Vertical Video
    manager.setAspectRatio(9.0 / 16.0);
    QSize sVert = manager.calculateSizeForHeight(1600);
    assert(sVert.width() == 900);
    assert(sVert.height() == 1600);

    // Constrain resize rect (dragging bottom-right corner)
    QRect initRect(100, 100, 960, 540);
    manager.setAspectRatio(16.0 / 9.0);
    QRect propRect(100, 100, 1200, 700);
    QRect constrained = manager.constrainResizeRect(initRect, propRect, Qt::RightEdge | Qt::BottomEdge);
    assert(constrained.left() == 100);
    assert(constrained.top() == 100);
    double ratio = static_cast<double>(constrained.width()) / constrained.height();
    assert(std::abs(ratio - (16.0 / 9.0)) < 0.05);

    std::cout << "[TestAspectRatio] All tests passed!" << std::endl;
    return 0;
}
