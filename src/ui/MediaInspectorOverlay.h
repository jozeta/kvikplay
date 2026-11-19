#pragma once

#include <QWidget>
#include <QLabel>
#include "player/MediaInfo.h"

namespace kvikplay {

class MediaInspectorOverlay : public QWidget {
    Q_OBJECT

public:
    explicit MediaInspectorOverlay(QWidget* parent = nullptr);

    void updateStats(const StreamStats& stats);
    void toggleInspector();

private:
    QLabel* m_statsLabel{nullptr};
};

} // namespace kvikplay
