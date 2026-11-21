#include "SingleInstance.h"

#ifdef _WIN32
#include <windows.h>
#include <string>
#endif

namespace kvikplay::SingleInstance {

#ifdef _WIN32

namespace {
constexpr wchar_t kMutexName[] = L"Local\\Kvikplay.SingleInstance";
constexpr wchar_t kWindowClass[] = L"KvikplayIpcWindow";
constexpr ULONG_PTR kMagic = 0x4B565046; // 'KVPF': "Kvikplay file path"
constexpr DWORD kMaxPathChars = 32768;

HANDLE g_mutex = nullptr;
std::function<void(const QString&)> g_onFile;

LRESULT CALLBACK ipcWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COPYDATA) {
        const auto* cds = reinterpret_cast<const COPYDATASTRUCT*>(lParam);
        // Only well-formed path messages: our tag, UTF-16, bounded length. The path is
        // validated again (exists, supported format) by MainWindow::openFile.
        if (cds && cds->dwData == kMagic && cds->lpData && cds->cbData > 0 &&
            cds->cbData % sizeof(wchar_t) == 0 && cds->cbData / sizeof(wchar_t) <= kMaxPathChars) {
            const QString path = QString::fromWCharArray(static_cast<const wchar_t*>(cds->lpData),
                                                         static_cast<int>(cds->cbData / sizeof(wchar_t)));
            if (g_onFile) g_onFile(path);
            return TRUE;
        }
        return FALSE;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
} // namespace

bool anotherInstanceRunning() {
    if (!g_mutex) {
        g_mutex = CreateMutexW(nullptr, FALSE, kMutexName); // held until the process exits
    }
    return g_mutex && GetLastError() == ERROR_ALREADY_EXISTS;
}

bool sendToRunningInstance(const QString& filePath) {
    HWND target = FindWindowExW(HWND_MESSAGE, nullptr, kWindowClass, nullptr);
    if (!target) return false;

    const std::wstring path = filePath.toStdWString();
    COPYDATASTRUCT cds{};
    cds.dwData = kMagic;
    cds.cbData = static_cast<DWORD>(path.size() * sizeof(wchar_t));
    cds.lpData = const_cast<wchar_t*>(path.data());

    // Let the running instance bring its window to the front.
    AllowSetForegroundWindow(ASFW_ANY);
    DWORD_PTR result = 0;
    const LRESULT ok = SendMessageTimeoutW(target, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds),
                                           SMTO_ABORTIFHUNG, 3000, &result);
    return ok != 0 && result == TRUE;
}

void listen(std::function<void(const QString&)> onFile) {
    g_onFile = std::move(onFile);
    HINSTANCE inst = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ipcWndProc;
    wc.hInstance = inst;
    wc.lpszClassName = kWindowClass;
    RegisterClassExW(&wc);
    // Message-only window: invisible, receives WM_COPYDATA through Qt's event loop.
    CreateWindowExW(0, kWindowClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, inst, nullptr);
}

#else

bool anotherInstanceRunning() { return false; }
bool sendToRunningInstance(const QString&) { return false; }
void listen(std::function<void(const QString&)>) {}

#endif

} // namespace kvikplay::SingleInstance
