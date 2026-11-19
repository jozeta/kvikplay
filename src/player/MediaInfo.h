#pragma once

#include <QString>

namespace kvikplay {

struct MediaInfo {
    QString filePath;
    QString title;
    double duration{0.0};
    int width{0};
    int height{0};
    int displayWidth{0};
    int displayHeight{0};
    double aspectRatio{16.0 / 9.0};
    bool isAudioOnly{false};
    QString videoCodec;
    QString audioCodec;

    bool hasVideo() const {
        return !isAudioOnly && width > 0 && height > 0;
    }
};

} // namespace kvikplay
