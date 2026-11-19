#pragma once

#include <QDialog>

namespace kvikplay {

class HelpDialog : public QDialog {
    Q_OBJECT

public:
    explicit HelpDialog(QWidget* parent = nullptr);
};

} // namespace kvikplay
