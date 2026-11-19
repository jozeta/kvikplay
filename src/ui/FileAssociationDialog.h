#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QMap>
#include <vector>
#include "formats/FormatRegistry.h"
#include "platform/PlatformIntegration.h"

namespace kvikplay {

class FileAssociationDialog : public QDialog {
    Q_OBJECT

public:
    explicit FileAssociationDialog(PlatformIntegration* platform, QWidget* parent = nullptr);

private slots:
    void selectAll();
    void deselectAll();
    void save();
    void openSystemSettings();

private:
    void setupUi();
    QWidget* createCategoryGroup(FormatCategory category, const QString& title);

    PlatformIntegration* m_platform{nullptr};
    std::vector<QCheckBox*> m_checkboxes;
    QMap<QString, QCheckBox*> m_extensionCheckboxes;
};

} // namespace kvikplay
