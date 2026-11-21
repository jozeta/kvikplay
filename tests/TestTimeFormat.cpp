#include "TestCheck.h"
#include <iostream>
#include "ui/TimeFormat.h"

using namespace kvikplay;

int main() {
    std::cout << "[TestTimeFormat] Clock text..." << std::endl;
    CHECK(formatClock(0.0) == QStringLiteral("0:00"));
    CHECK(formatClock(95.4) == QStringLiteral("1:35"));
    CHECK(formatClock(596.0) == QStringLiteral("9:56"));
    CHECK(formatClock(3723.0) == QStringLiteral("1:02:03"));
    CHECK(formatClock(-5.0) == QStringLiteral("0:00"));

    std::cout << "[TestTimeFormat] Remaining time uses a real minus sign..." << std::endl;
    CHECK(formatRemaining(95.0, 596.0) == QStringLiteral("−8:21"));
    CHECK(formatRemaining(700.0, 596.0) == QStringLiteral("−0:00"));
    CHECK(formatRemaining(10.0, 0.0) == QString()); // unknown length: nothing to show

    std::cout << "[TestTimeFormat] All tests passed!" << std::endl;
    return 0;
}
