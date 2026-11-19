#include "HelpDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QIcon>

namespace kvikplay {

HelpDialog::HelpDialog(QWidget* parent)
    : QDialog(parent) {

    setWindowTitle(QStringLiteral("Kvikplay Guide & Shortcuts"));
    setModal(true);
    setFixedSize(540, 580);

    setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: #16181d;"
        "  color: #eeeeee;"
        "  border-radius: 12px;"
        "  border: 1px solid rgba(255, 255, 255, 0.16);"
        "}"
        "QLabel {"
        "  color: #e0e2e8;"
        "  background: transparent;"
        "}"
        "QScrollArea {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QScrollBar:vertical {"
        "  border: none;"
        "  background: rgba(255, 255, 255, 0.05);"
        "  width: 6px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgba(255, 255, 255, 0.25);"
        "  border-radius: 3px;"
        "}"
        "QPushButton#closeBtn {"
        "  background-color: rgba(255, 255, 255, 0.12);"
        "  color: #ffffff;"
        "  border: 1px solid rgba(255, 255, 255, 0.25);"
        "  border-radius: 6px;"
        "  padding: 8px 24px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#closeBtn:hover {"
        "  background-color: rgba(255, 255, 255, 0.22);"
        "  border-color: rgba(255, 255, 255, 0.40);"
        "}"
    ));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(14);

    // Header
    auto* headerLayout = new QHBoxLayout();
    auto* iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(QStringLiteral(":/icons/app_icon.svg")).pixmap(36, 36));
    headerLayout->addWidget(iconLabel);

    auto* titleCol = new QVBoxLayout();
    auto* titleLabel = new QLabel(QStringLiteral("Kvikplay Guide & Shortcuts"), this);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 700; color: #ffffff;"));
    auto* subLabel = new QLabel(QStringLiteral("Minimalist, ultra-fast desktop media player"), this);
    subLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #9aa0aa;"));
    titleCol->addWidget(titleLabel);
    titleCol->addWidget(subLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch();
    rootLayout->addLayout(headerLayout);

    // Scrollable content
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    auto* contentWidget = new QWidget(scrollArea);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 4, 10, 4);
    contentLayout->setSpacing(16);

    // Section 1: How Kvikplay Works
    auto* guideBox = new QLabel(contentWidget);
    guideBox->setWordWrap(true);
    guideBox->setText(QStringLiteral(
        "<div style='background-color: rgba(255, 255, 255, 0.05); padding: 12px 14px; border-radius: 8px; font-size: 12px; line-height: 1.5;'>"
        "<b style='color: #ffffff;'>How Kvikplay Works:</b><br/>"
        "• <b>Pure Frameless Display</b>: Video fills 100% of the window without borders or black bars. Resizing automatically preserves native video aspect ratio.<br/>"
        "• <b>Play & Pause</b>: Quick right-click or trackpad two-finger tap toggles Play / Pause.<br/>"
        "• <b>Context Menu</b>: Hold right-click or trackpad click for 0.5s to open the menu.<br/>"
        "• <b>Move & Resize</b>: Drag anywhere with left-click to move. Drag the generous 32px corners to resize.<br/>"
        "• <b>Drag & Drop</b>: Drop any video, audio, or subtitle file (.srt, .vtt, .ass) directly onto the player."
        "</div>"
    ));
    contentLayout->addWidget(guideBox);

    // Section 2: Shortcuts Table
    auto* tableLabel = new QLabel(contentWidget);
    tableLabel->setWordWrap(true);
    tableLabel->setText(QStringLiteral(
        "<table width='100%' cellpadding='5' cellspacing='0' style='font-size: 12px; line-height: 1.4;'>"
        "  <tr style='color: #ffffff; font-weight: 700; border-bottom: 1px solid rgba(255,255,255,0.2);'>"
        "    <th align='left' style='padding-bottom: 6px;'>Action</th>"
        "    <th align='right' style='padding-bottom: 6px;'>Shortcut</th>"
        "  </tr>"
        "  <tr><td>Play / Pause</td><td align='right'><b>Space / Right-Click</b></td></tr>"
        "  <tr><td>Quick 2.0x Speed Boost</td><td align='right'><b>Hold C / Hold &gt;&gt;</b></td></tr>"
        "  <tr><td>Seek &plusmn;10s / &plusmn;30s</td><td align='right'><b>Left/Right / Shift+Left/Right</b></td></tr>"
        "  <tr><td>Volume &plusmn;5% / Mute</td><td align='right'><b>Up / Down / M</b></td></tr>"
        "  <tr><td>Next / Prev File in Folder</td><td align='right'><b>] / [ &nbsp;or&nbsp; PageDown / PageUp</b></td></tr>"
        "  <tr><td>Next / Prev Chapter</td><td align='right'><b>Shift + ] / Shift + [</b></td></tr>"
        "  <tr><td>Subtitle Timing Sync (&plusmn;100ms)</td><td align='right'><b>Z / X</b></td></tr>"
        "  <tr><td>Quiet Dialogue Boost (dynaudnorm)</td><td align='right'><b>N</b></td></tr>"
        "  <tr><td>Clean Screenshot (Clipboard &amp; Desktop)</td><td align='right'><b>S</b></td></tr>"
        "  <tr><td>Screenshot with Subtitles</td><td align='right'><b>Shift + S</b></td></tr>"
        "  <tr><td>Window Scale (100% / 50% / 25%)</td><td align='right'><b>1 / 2 / 4</b></td></tr>"
        "  <tr><td>Picture-in-Picture Snap</td><td align='right'><b>P</b></td></tr>"
        "  <tr><td>Stream &amp; Codec Info Inspector</td><td align='right'><b>I</b></td></tr>"
        "  <tr><td>Fullscreen Toggle</td><td align='right'><b>F / Double-Click</b></td></tr>"
        "  <tr><td>Quit Kvikplay</td><td align='right'><b>Q</b></td></tr>"
        "</table>"
    ));
    contentLayout->addWidget(tableLabel);
    scrollArea->setWidget(contentWidget);
    rootLayout->addWidget(scrollArea, 1);

    // Bottom close button
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    auto* closeBtn = new QPushButton(QStringLiteral("Got it"), this);
    closeBtn->setObjectName(QStringLiteral("closeBtn"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);
    rootLayout->addLayout(bottomLayout);
}

} // namespace kvikplay
