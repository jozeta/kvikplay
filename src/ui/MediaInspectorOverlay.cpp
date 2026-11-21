#include "MediaInspectorOverlay.h"
#include <QVBoxLayout>
#include <QFontDatabase>

namespace kvikplay {

MediaInspectorOverlay::MediaInspectorOverlay(QWidget* parent)
    : QWidget(parent)
    , m_statsLabel(new QLabel(this)) {

    setAttribute(Qt::WA_TransparentForMouseEvents);
    // Without this a plain QWidget ignores its own stylesheet background, so the
    // panel was transparent and its text unreadable over bright video.
    setAttribute(Qt::WA_StyledBackground, true);
    // The platform's real monospace family; "ui-monospace" does not exist in Qt and
    // cost a 32 ms font-alias scan at startup.
    const QString mono = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    setStyleSheet(QStringLiteral(
        "kvikplay--MediaInspectorOverlay {"
        "  background-color: rgba(16, 18, 22, 0.90);"
        "  border: 1px solid rgba(255, 255, 255, 0.20);"
        "  border-radius: 8px;"
        "}"
        "QLabel {"
        "  color: #d2d6e0;"
        "  font-family: '%1';"
        "  font-size: 11px;"
        "  background: transparent;"
        "}"
    ).arg(mono));
    m_statsLabel->setTextFormat(Qt::RichText); // our table; every value is escaped below
    setAccessibleName(QStringLiteral("Media inspector"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->addWidget(m_statsLabel);

    hide();
}

void MediaInspectorOverlay::updateStats(const StreamStats& stats) {
    QString html = QStringLiteral("<table cellpadding='2' cellspacing='0' style='font-size: 11px;'>");

    auto addRow = [&](const QString& label, const QString& val) {
        html += QStringLiteral(
            "<tr>"
            "  <td style='color: #8b92a2; font-weight: 600; padding-right: 14px;'>%1:</td>"
            "  <td style='color: #ffffff;'>%2</td>"
            "</tr>"
        ).arg(label.toHtmlEscaped(), val.toHtmlEscaped()); // values come from the media file
    };

    addRow(QStringLiteral("Format"), stats.fileFormat.isEmpty() ? QStringLiteral("-") : stats.fileFormat);
    addRow(QStringLiteral("Video Codec"), stats.videoCodec.isEmpty() ? QStringLiteral("None") : stats.videoCodec);
    addRow(QStringLiteral("Native Resolution"), QStringLiteral("%1 × %2").arg(stats.width).arg(stats.height));
    addRow(QStringLiteral("Display Size"), QStringLiteral("%1 × %2").arg(stats.displayWidth).arg(stats.displayHeight));
    addRow(QStringLiteral("Frame Rate"), QStringLiteral("%1 fps").arg(stats.fps, 0, 'f', 2));
    addRow(QStringLiteral("Video Bitrate"), QStringLiteral("%1 kbps").arg(static_cast<int>(stats.videoBitrate)));
    addRow(QStringLiteral("HW Decoder"), stats.hwdec);
    addRow(QStringLiteral("Dropped Frames"), QString::number(stats.dropCount));
    addRow(QStringLiteral("Video Output"), stats.videoOutput.isEmpty() ? QStringLiteral("-") : stats.videoOutput);
    addRow(QStringLiteral("Renderer"), stats.renderer.isEmpty() ? QStringLiteral("-") : stats.renderer);
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
