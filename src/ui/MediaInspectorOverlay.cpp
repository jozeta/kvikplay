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
    QString html = QStringLiteral("<table cellpadding='2' cellspacing='0' style='font-family: ui-monospace, Menlo, Consolas, Monaco, monospace; font-size: 11px;'>");

    auto addRow = [&](const QString& label, const QString& val) {
        html += QStringLiteral(
            "<tr>"
            "  <td style='color: #8b92a2; font-weight: 600; padding-right: 14px;'>%1:</td>"
            "  <td style='color: #ffffff;'>%2</td>"
            "</tr>"
        ).arg(label, val);
    };

    addRow(QStringLiteral("Format"), stats.fileFormat.isEmpty() ? QStringLiteral("-") : stats.fileFormat);
    addRow(QStringLiteral("Video Codec"), stats.videoCodec.isEmpty() ? QStringLiteral("None") : stats.videoCodec);
    addRow(QStringLiteral("Native Resolution"), QStringLiteral("%1 × %2").arg(stats.width).arg(stats.height));
    addRow(QStringLiteral("Display Size"), QStringLiteral("%1 × %2").arg(stats.displayWidth).arg(stats.displayHeight));
    addRow(QStringLiteral("Frame Rate"), QStringLiteral("%1 fps").arg(stats.fps, 0, 'f', 2));
    addRow(QStringLiteral("Video Bitrate"), QStringLiteral("%1 kbps").arg(static_cast<int>(stats.videoBitrate)));
    addRow(QStringLiteral("HW Decoder"), stats.hwdec);
    addRow(QStringLiteral("Dropped Frames"), QString::number(stats.dropCount));
    addRow(QStringLiteral("Audio Codec"), stats.audioCodec.isEmpty() ? QStringLiteral("None") : stats.audioCodec);

    QString chStr = QString::number(stats.audioChannels) + QStringLiteral(" ch");
    if (stats.audioChannels == 1) chStr += QStringLiteral(" (Mono)");
    else if (stats.audioChannels == 2) chStr += QStringLiteral(" (Stereo)");
    else if (stats.audioChannels == 6) chStr += QStringLiteral(" (5.1)");
    else if (stats.audioChannels == 8) chStr += QStringLiteral(" (7.1)");
    addRow(QStringLiteral("Audio Channels"), chStr);

    addRow(QStringLiteral("Sample Rate"), QStringLiteral("%1 Hz").arg(stats.audioSampleRate));
    addRow(QStringLiteral("Audio Bitrate"), QStringLiteral("%1 kbps").arg(static_cast<int>(stats.audioBitrate)));

    html += QStringLiteral("</table>");

    m_statsLabel->setText(html);
    adjustSize();
}

void MediaInspectorOverlay::toggleInspector() {
    setVisible(!isVisible());
    if (isVisible()) {
        raise();
    }
}

} // namespace kvikplay
