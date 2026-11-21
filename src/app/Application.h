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
    bool versionRequested() const { return m_versionRequested; }
    // Started as an extra window (Finder/Explorer "Open in New Window"): offset it.
    bool cascadeRequested() const { return m_cascade; }

protected:
    bool event(QEvent* event) override;

private:
    MainWindow* m_mainWindow{nullptr};
    QString m_initialFile;
    bool m_versionRequested{false};
    bool m_cascade{false};
};

} // namespace kvikplay
