#pragma once

#include <QString>
#include <cmath>

namespace kvikplay {

// "1:35", "9:56", "1:02:03": minutes without a leading zero, as players show it.
inline QString formatClock(double seconds) {
    const int total = static_cast<int>(std::lround(std::max(0.0, seconds)));
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    if (h > 0) {
        return QStringLiteral("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    }
    return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
}

// "−8:21" (U+2212 minus sign); empty when the length is unknown.
inline QString formatRemaining(double position, double duration) {
    if (duration <= 0.0) return QString();
    return QStringLiteral("−") + formatClock(std::max(0.0, duration - position));
}

} // namespace kvikplay
