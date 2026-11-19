#pragma once

#include <QString>
#include <vector>

namespace kvikplay {

struct TrackInfo {
    int id{0};
    QString type;    // "video", "audio", "sub"
    QString title;
    QString lang;
    QString codec;
    bool selected{false};

    QString displayName() const {
        QString name;
        if (!title.isEmpty()) {
            name = title;
        } else if (!lang.isEmpty()) {
            name = lang.toUpper();
        } else {
            name = QStringLiteral("Track %1").arg(id);
        }
        if (!codec.isEmpty()) {
            name += QStringLiteral(" (%1)").arg(codec);
        }
        return name;
    }
};

struct MediaInfo {
    QString filePath;
    QString fileName;
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

    std::vector<TrackInfo> audioTracks;
    std::vector<TrackInfo> videoTracks;
    std::vector<TrackInfo> subtitleTracks;

    bool hasVideo() const {
        return !isAudioOnly && width > 0 && height > 0;
    }
};

} // namespace kvikplay
