#include "WindowsIntegration.h"
#include "formats/FormatRegistry.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#include <shellapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

enum DWM_WINDOW_CORNER_PREFERENCE {
    DWMWCP_DEFAULT = 0,
    DWMWCP_DONOTROUND = 1,
    DWMWCP_ROUND = 2,
    DWMWCP_ROUNDSMALL = 3
};
#endif

namespace kvikplay {

double WindowsIntegration::s_aspectRatio = 16.0 / 9.0;

WindowsIntegration::WindowsIntegration() = default;
WindowsIntegration::~WindowsIntegration() = default;

double WindowsIntegration::currentAspectRatio() {
    return s_aspectRatio;
}

void WindowsIntegration::setGlobalAspectRatio(double ratio) {
    if (ratio > 0.05) {
        s_aspectRatio = ratio;
    }
}

void WindowsIntegration::setupFramelessWindow(QWidget* widget) {
    if (!widget) return;
#ifdef _WIN32
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (!hwnd) return;

    // Enable standard DWM drop shadow for frameless window
    MARGINS margins = {1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    SetWindowLong(hwnd, GWL_STYLE, style);

    // Request rounded window corners on Windows 11
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));

    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
#else
    Q_UNUSED(widget);
#endif
}

void WindowsIntegration::setWindowRounded(QWidget* widget, bool rounded) {
#ifdef _WIN32
    if (!widget) return;
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (!hwnd) return;

    DWM_WINDOW_CORNER_PREFERENCE preference = rounded ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
#else
    Q_UNUSED(widget);
    Q_UNUSED(rounded);
#endif
}

void WindowsIntegration::setWindowAspectRatio(QWidget* widget, double ratio, int displayWidth, int displayHeight) {
    Q_UNUSED(widget);
    Q_UNUSED(displayWidth);
    Q_UNUSED(displayHeight);
    setGlobalAspectRatio(ratio);
}

bool WindowsIntegration::isDefaultPlayerForExtension(const QString& ext) {
#ifdef _WIN32
    QString cleanExt = ext.startsWith('.') ? ext : (QStringLiteral(".") + ext);
    std::wstring wExt = cleanExt.toStdWString();

    wchar_t progIdBuf[256] = {0};
    DWORD bufSize = sizeof(progIdBuf);
    LSTATUS status = RegGetValueW(
        HKEY_CURRENT_USER,
        (L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\" + wExt + L"\\UserChoice").c_str(),
        L"ProgId",
        RRF_RT_REG_SZ,
        nullptr,
        progIdBuf,
        &bufSize
    );

    if (status == ERROR_SUCCESS) {
        QString currentProgId = QString::fromWCharArray(progIdBuf);
        return currentProgId.startsWith(QStringLiteral("Kvikplay."));
    }
#else
    Q_UNUSED(ext);
#endif
    return false;
}

bool WindowsIntegration::registerFileAssociations(const QStringList& extensions) {
#ifdef _WIN32
    QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    std::wstring wExePath = exePath.toStdWString();
    std::wstring openCmd = L"\"" + wExePath + L"\" \"%1\"";
    std::wstring iconCmd = L"\"" + wExePath + L"\",0";

    const auto& registry = FormatRegistry::instance();

    for (const auto& ext : extensions) {
        auto fmt = registry.findFormat(ext);
        if (!fmt) continue;

        std::wstring wProgId = fmt->progId.toStdWString();
        std::wstring wExt = fmt->extension.toStdWString();
        std::wstring wDesc = fmt->name.toStdWString();

        // 1. HKCU\Software\Classes\<ProgID>
        std::wstring progKey = L"Software\\Classes\\" + wProgId;
        HKEY hKeyProg = nullptr;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, progKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKeyProg, nullptr) == ERROR_SUCCESS) {
            RegSetValueExW(hKeyProg, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(wDesc.c_str()), (wDesc.size() + 1) * sizeof(wchar_t));

            HKEY hKeyCmd = nullptr;
            std::wstring cmdKey = progKey + L"\\shell\\open\\command";
            if (RegCreateKeyExW(HKEY_CURRENT_USER, cmdKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKeyCmd, nullptr) == ERROR_SUCCESS) {
                RegSetValueExW(hKeyCmd, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(openCmd.c_str()), (openCmd.size() + 1) * sizeof(wchar_t));
                RegCloseKey(hKeyCmd);
            }

            HKEY hKeyIcon = nullptr;
            std::wstring iconKey = progKey + L"\\DefaultIcon";
            if (RegCreateKeyExW(HKEY_CURRENT_USER, iconKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKeyIcon, nullptr) == ERROR_SUCCESS) {
                RegSetValueExW(hKeyIcon, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(iconCmd.c_str()), (iconCmd.size() + 1) * sizeof(wchar_t));
                RegCloseKey(hKeyIcon);
            }

            RegCloseKey(hKeyProg);
        }

        // 2. HKCU\Software\Classes\<.ext>\OpenWithProgids
        std::wstring extKey = L"Software\\Classes\\" + wExt + L"\\OpenWithProgids";
        HKEY hKeyExt = nullptr;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, extKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKeyExt, nullptr) == ERROR_SUCCESS) {
            RegSetValueExW(hKeyExt, wProgId.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(L""), sizeof(wchar_t));
            RegCloseKey(hKeyExt);
        }
    }

    // Notify shell of association changes
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return true;
#else
    Q_UNUSED(extensions);
    return false;
#endif
}

void WindowsIntegration::openSystemDefaultAppsSettings() {
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps")));
}

} // namespace kvikplay
