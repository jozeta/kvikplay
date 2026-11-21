#include "SingleInstance.h"

#ifdef _WIN32
#include <windows.h>
#include <iterator>
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

// The receiving window must belong to a process running this same Kvikplay
// executable; anyone can register a window class with our name.
bool isOurExecutable(DWORD pid) {
    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc) return false;
    wchar_t theirs[MAX_PATH * 4] = {};
    DWORD len = static_cast<DWORD>(std::size(theirs));
    const bool ok = QueryFullProcessImageNameW(proc, 0, theirs, &len) != 0;
    CloseHandle(proc);
    if (!ok) return false;
    wchar_t ours[MAX_PATH * 4] = {};
    if (GetModuleFileNameW(nullptr, ours, static_cast<DWORD>(std::size(ours))) == 0) return false;
    return CompareStringOrdinal(theirs, static_cast<int>(len), ours, -1, TRUE) == CSTR_EQUAL;
}

bool sendToRunningInstance(const QString& filePath) {
    HWND target = FindWindowExW(HWND_MESSAGE, nullptr, kWindowClass, nullptr);
    if (!target) return false;
    DWORD targetPid = 0;
    GetWindowThreadProcessId(target, &targetPid);
    if (targetPid == 0 || !isOurExecutable(targetPid)) return false; // start normally instead

    const std::wstring path = filePath.toStdWString();
    COPYDATASTRUCT cds{};
    cds.dwData = kMagic;
    cds.cbData = static_cast<DWORD>(path.size() * sizeof(wchar_t));
    cds.lpData = const_cast<wchar_t*>(path.data());

    // Let that instance (and only it) bring its window to the front.
    AllowSetForegroundWindow(targetPid);
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
