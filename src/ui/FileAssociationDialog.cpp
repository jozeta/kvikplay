#include "FileAssociationDialog.h"
#include "config/SettingsManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QMessageBox>

namespace kvikplay {

FileAssociationDialog::FileAssociationDialog(PlatformIntegration* platform, QWidget* parent)
    : QDialog(parent)
    , m_platform(platform) {

    setWindowTitle(QStringLiteral("File associations"));
    setMinimumSize(480, 520);
    resize(520, 580);

    setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: #16181d;"
        "  color: #eeeeee;"
        "}"
        "QLabel {"
        "  color: #dddddd;"
        "}"
        "QCheckBox {"
        "  color: #cccccc;"
        "  spacing: 8px;"
        "  font-size: 13px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 16px;"
        "  height: 16px;"
        "  border: 1px solid #4a505e;"
        "  border-radius: 4px;"
        "  background: #21252d;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: #4c78dd;"
        "  border-color: #608bee;"
        "}"
        "QPushButton {"
        "  background-color: #272b35;"
        "  border: 1px solid #3d4352;"
        "  border-radius: 6px;"
        "  color: #eeeeee;"
        "  padding: 6px 14px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #323845;"
        "}"
        "QPushButton#primaryBtn {"
        "  background-color: #3b6bd6;"
        "  border-color: #4f7ee8;"
        "}"
        "QPushButton#primaryBtn:hover {"
        "  background-color: #4b7ae2;"
        "}"
        "QScrollArea {"
        "  border: 1px solid #282c37;"
        "  border-radius: 8px;"
        "  background-color: #1a1c22;"
        "}"
    ));

    setupUi();
}

void FileAssociationDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    auto* headerLabel = new QLabel(QStringLiteral("Select formats to open with Kvikplay:"), this);
    headerLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold; color: #ffffff;"));
    mainLayout->addWidget(headerLabel);

    // Scrollable container for format categories
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* scrollContent = new QWidget(scrollArea);
    auto* contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(20);

    contentLayout->addWidget(createCategoryGroup(FormatCategory::CommonVideo, QStringLiteral("Common Video")));
    contentLayout->addWidget(createCategoryGroup(FormatCategory::OtherVideo, QStringLiteral("Other Video")));
    contentLayout->addWidget(createCategoryGroup(FormatCategory::Audio, QStringLiteral("Audio")));
    contentLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // Platform informational note
#ifdef _WIN32
    auto* noteLabel = new QLabel(
        QStringLiteral("Note: Windows 10/11 requires confirming default apps in Windows Settings."), this);
    noteLabel->setStyleSheet(QStringLiteral("color: #8f96a3; font-size: 11px;"));
    noteLabel->setWordWrap(true);
    mainLayout->addWidget(noteLabel);
#endif

    // Button bar
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    auto* selectAllBtn = new QPushButton(QStringLiteral("Select All"), this);
    connect(selectAllBtn, &QPushButton::clicked, this, &FileAssociationDialog::selectAll);

    auto* deselectAllBtn = new QPushButton(QStringLiteral("Deselect All"), this);
    connect(deselectAllBtn, &QPushButton::clicked, this, &FileAssociationDialog::deselectAll);

    bottomLayout->addWidget(selectAllBtn);
    bottomLayout->addWidget(deselectAllBtn);

#ifdef _WIN32
    auto* settingsBtn = new QPushButton(QStringLiteral("Open Windows Settings"), this);
    connect(settingsBtn, &QPushButton::clicked, this, &FileAssociationDialog::openSystemSettings);
    bottomLayout->addWidget(settingsBtn);
#endif

    bottomLayout->addStretch();

    auto* cancelBtn = new QPushButton(QStringLiteral("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto* saveBtn = new QPushButton(QStringLiteral("Save"), this);
    saveBtn->setObjectName(QStringLiteral("primaryBtn"));
    connect(saveBtn, &QPushButton::clicked, this, &FileAssociationDialog::save);

    bottomLayout->addWidget(cancelBtn);
    bottomLayout->addWidget(saveBtn);

    mainLayout->addLayout(bottomLayout);
}

QWidget* FileAssociationDialog::createCategoryGroup(FormatCategory category, const QString& title) {
    auto* group = new QWidget(this);
    auto* layout = new QVBoxLayout(group);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* titleLabel = new QLabel(title, group);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: bold; color: #a4abb8;"));
    layout->addWidget(titleLabel);

    auto* gridWidget = new QWidget(group);
    auto* grid = new QGridLayout(gridWidget);
    grid->setContentsMargins(4, 0, 4, 0);
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(8);

    const auto& formats = FormatRegistry::instance().formatsByCategory(category);
    const QStringList savedExts = SettingsManager::instance().associatedExtensions();

    int row = 0;
    int col = 0;
    const int maxCols = 3;

    for (const auto& fmt : formats) {
        auto* cb = new QCheckBox(fmt.extension, gridWidget);

        // Check if currently associated or saved
        bool isDefault = m_platform ? m_platform->isDefaultPlayerForExtension(fmt.extension) : false;
        bool isSaved = savedExts.contains(fmt.extension);
        cb->setChecked(isDefault || isSaved);

        grid->addWidget(cb, row, col);
        m_checkboxes.push_back(cb);
        m_extensionCheckboxes[fmt.extension] = cb;

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    layout->addWidget(gridWidget);
    return group;
}

void FileAssociationDialog::selectAll() {
    for (auto* cb : m_checkboxes) {
        cb->setChecked(true);
    }
}

void FileAssociationDialog::deselectAll() {
    for (auto* cb : m_checkboxes) {
        cb->setChecked(false);
    }
}

void FileAssociationDialog::openSystemSettings() {
    if (m_platform) {
        m_platform->openSystemDefaultAppsSettings();
    }
}

void FileAssociationDialog::save() {
    QStringList selected;
    for (auto it = m_extensionCheckboxes.cbegin(); it != m_extensionCheckboxes.cend(); ++it) {
        if (it.value()->isChecked()) {
            selected.append(it.key());
        }
    }

    SettingsManager::instance().setAssociatedExtensions(selected);
    SettingsManager::instance().save();

    if (m_platform) {
        m_platform->registerFileAssociations(selected);
    }

    accept();
}

} // namespace kvikplay
