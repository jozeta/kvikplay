#pragma once

#include <QApplication>
#include <QString>

namespace kvikplay {

class MainWindow;

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application() override = default;

    void setMainWindow(MainWindow* window);
    QString initialFile() const { return m_initialFile; }

protected:
    bool event(QEvent* event) override;

private:
    MainWindow* m_mainWindow{nullptr};
    QString m_initialFile;
};

} // namespace kvikplay
