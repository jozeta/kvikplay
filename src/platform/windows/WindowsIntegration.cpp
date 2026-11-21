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
#include <shlobj.h> // SHChangeNotify (left out by WIN32_LEAN_AND_MEAN)
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

#include <chrono>
#include <systemmediatransportcontrolsinterop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>

// DWMWA_WINDOW_CORNER_PREFERENCE / DWM_WINDOW_CORNER_PREFERENCE come from dwmapi.h
// (Windows SDK 10.0.22000 and later, and MinGW-w64).
#endif

namespace kvikplay {

#ifdef _WIN32
struct WindowsIntegration::Smtc {
    winrt::Windows::Media::SystemMediaTransportControls controls{nullptr};
    winrt::event_token buttonToken{};
    winrt::event_token seekToken{};
};
#else
struct WindowsIntegration::Smtc {};
#endif

double WindowsIntegration::s_aspectRatio = 16.0 / 9.0;

WindowsIntegration::WindowsIntegration() = default;

WindowsIntegration::~WindowsIntegration() {
#ifdef _WIN32
    if (m_smtc && m_smtc->controls) {
        try {
            m_smtc->controls.ButtonPressed(m_smtc->buttonToken);
            m_smtc->controls.PlaybackPositionChangeRequested(m_smtc->seekToken);
            m_smtc->controls.IsEnabled(false);
        } catch (...) {
        }
    }
#endif
}

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
    m_hwnd = hwnd; // for System Media Transport Controls

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

bool WindowsIntegration::unregisterFileAssociations(const QStringList& extensions) {
#ifdef _WIN32
    const auto& registry = FormatRegistry::instance();
    for (const auto& ext : extensions) {
        auto fmt = registry.findFormat(ext);
        if (!fmt) continue;
        const std::wstring wProgId = fmt->progId.toStdWString();
        const std::wstring wExt = fmt->extension.toStdWString();

        // Remove only what registerFileAssociations() wrote, all under HKCU.
        const std::wstring extKey = L"Software\\Classes\\" + wExt + L"\\OpenWithProgids";
        HKEY hKeyExt = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, extKey.c_str(), 0, KEY_SET_VALUE, &hKeyExt) == ERROR_SUCCESS) {
            RegDeleteValueW(hKeyExt, wProgId.c_str());
            RegCloseKey(hKeyExt);
        }
        RegDeleteTreeW(HKEY_CURRENT_USER, (L"Software\\Classes\\" + wProgId).c_str());
    }
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return true;
#else
    Q_UNUSED(extensions);
    return false;
#endif
}

void WindowsIntegration::setupMediaKeyCommands(const MediaKeyCallbacks& callbacks) {
#ifdef _WIN32
    using namespace winrt::Windows::Media;
    if (!m_hwnd) return;
    try {
        auto interop = winrt::get_activation_factory<SystemMediaTransportControls,
                                                     ISystemMediaTransportControlsInterop>();
        SystemMediaTransportControls controls{nullptr};
        winrt::check_hresult(interop->GetForWindow(static_cast<HWND>(m_hwnd),
                                                   winrt::guid_of<SystemMediaTransportControls>(),
                                                   winrt::put_abi(controls)));
        m_smtc = std::make_unique<Smtc>();
        m_smtc->controls = controls;
        controls.IsPlayEnabled(true);
        controls.IsPauseEnabled(true);
        controls.IsNextEnabled(true);
        controls.IsPreviousEnabled(true);
        controls.IsEnabled(false); // until something is playing

        // Raised on a background thread; the callbacks queue onto the GUI thread.
        m_smtc->buttonToken = controls.ButtonPressed(
            [callbacks](SystemMediaTransportControls const&,
                        SystemMediaTransportControlsButtonPressedEventArgs const& args) {
                switch (args.Button()) {
                case SystemMediaTransportControlsButton::Play:
                    if (callbacks.onPlay) callbacks.onPlay();
                    break;
                case SystemMediaTransportControlsButton::Pause:
                    if (callbacks.onPause) callbacks.onPause();
                    break;
                case SystemMediaTransportControlsButton::Next:
                    if (callbacks.onNextTrack) callbacks.onNextTrack();
                    break;
                case SystemMediaTransportControlsButton::Previous:
                    if (callbacks.onPreviousTrack) callbacks.onPreviousTrack();
                    break;
                default:
                    break;
                }
            });
        m_smtc->seekToken = controls.PlaybackPositionChangeRequested(
            [callbacks](SystemMediaTransportControls const&, PlaybackPositionChangeRequestedEventArgs const& args) {
                if (callbacks.onSeek) {
                    callbacks.onSeek(std::chrono::duration<double>(args.RequestedPlaybackPosition()).count());
                }
            });
    } catch (...) {
        m_smtc.reset(); // SMTC unavailable (e.g. Windows Server without media features)
    }
#else
    Q_UNUSED(callbacks);
#endif
}

void WindowsIntegration::setNowPlayingInfo(const NowPlayingInfo& info) {
#ifdef _WIN32
    using namespace winrt::Windows::Media;
    using winrt::Windows::Foundation::TimeSpan;
    if (!m_smtc || !m_smtc->controls) return;
    try {
        auto& controls = m_smtc->controls;
        controls.IsEnabled(true);
        controls.PlaybackStatus(info.playbackRate > 0.0 ? MediaPlaybackStatus::Playing
                                                        : MediaPlaybackStatus::Paused);

        auto updater = controls.DisplayUpdater();
        updater.Type(MediaPlaybackType::Video);
        updater.VideoProperties().Title(winrt::hstring(info.title.toStdWString()));
        updater.Update();

        const auto toSpan = [](double s) {
            return std::chrono::duration_cast<TimeSpan>(std::chrono::duration<double>(std::max(0.0, s)));
        };
        SystemMediaTransportControlsTimelineProperties timeline;
        timeline.StartTime(TimeSpan{0});
        timeline.MinSeekTime(TimeSpan{0});
        timeline.EndTime(toSpan(info.duration));
        timeline.MaxSeekTime(toSpan(info.duration));
        timeline.Position(toSpan(info.position));
        controls.UpdateTimelineProperties(timeline);
    } catch (...) {
    }
#else
    Q_UNUSED(info);
#endif
}

void WindowsIntegration::clearNowPlayingInfo() {
#ifdef _WIN32
    if (!m_smtc || !m_smtc->controls) return;
    try {
        m_smtc->controls.DisplayUpdater().ClearAll();
        m_smtc->controls.IsEnabled(false);
    } catch (...) {
    }
#endif
}

void WindowsIntegration::openSystemDefaultAppsSettings() {
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps")));
}

void WindowsIntegration::setDisplaySleepInhibited(bool inhibit) {
#ifdef _WIN32
    if (inhibit) {
        SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
    } else {
        SetThreadExecutionState(ES_CONTINUOUS);
    }
#else
    Q_UNUSED(inhibit);
#endif
}

} // namespace kvikplay
