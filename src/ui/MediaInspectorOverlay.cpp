#include "MediaInspectorOverlay.h"
#include <QVBoxLayout>

namespace kvikplay {

MediaInspectorOverlay::MediaInspectorOverlay(QWidget* parent)
    : QWidget(parent)
    , m_statsLabel(new QLabel(this)) {

    setAttribute(Qt::WA_TransparentForMouseEvents);
    setStyleSheet(QStringLiteral(
        "kvikplay--MediaInspectorOverlay {"
        "  background-color: rgba(16, 18, 22, 0.90);"
        "  border: 1px solid rgba(255, 255, 255, 0.20);"
        "  border-radius: 8px;"
        "}"
        "QLabel {"
        "  color: #d2d6e0;"
        "  font-family: ui-monospace, Menlo, Consolas, Monaco, monospace;"
        "  font-size: 11px;"
        "  line-height: 1.4;"
        "  background: transparent;"
        "}"
    ));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->addWidget(m_statsLabel);

    hide();
}

void MediaInspectorOverlay::updateStats(const StreamStats& stats) {
    QString text;
    text += QStringLiteral("<b>FORMAT</b>:  %1\n").arg(stats.fileFormat.isEmpty() ? QStringLiteral("-") : stats.fileFormat);
    text += QStringLiteral("<b>VIDEO</b>:   %1 (%2x%3)\n")
                .arg(stats.videoCodec.isEmpty() ? QStringLiteral("None") : stats.videoCodec)
                .arg(stats.width)
                .arg(stats.height);
    text += QStringLiteral("<b>DISPLAY</b>: %1x%2 @ %3 fps\n")
                .arg(stats.displayWidth)
                .arg(stats.displayHeight)
                .arg(stats.fps, 0, 'f', 2);
    text += QStringLiteral("<b>BITRATE</b>: %1 kbps\n").arg(static_cast<int>(stats.videoBitrate));
    text += QStringLiteral("<b>DECODER</b>: %1 (drops: %2)\n").arg(stats.hwdec).arg(stats.dropCount);
    text += QStringLiteral("<b>AUDIO</b>:   %1 (%2 ch, %3 Hz, %4 kbps)")
                .arg(stats.audioCodec.isEmpty() ? QStringLiteral("None") : stats.audioCodec)
                .arg(stats.audioChannels)
                .arg(stats.audioSampleRate)
                .arg(static_cast<int>(stats.audioBitrate));

    m_statsLabel->setText(text);
    adjustSize();
}

void MediaInspectorOverlay::toggleInspector() {
    setVisible(!isVisible());
    if (isVisible()) {
        raise();
    }
}

} // namespace kvikplay
