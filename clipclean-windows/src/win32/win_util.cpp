#include "win32/win_util.h"

#include "resource.h"

namespace clipclean {
namespace {

using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);

struct ApiVersion {
    DWORD build = 0;
    DWORD major = 0;
    DWORD minor = 0;
};

ApiVersion queryVersion() {
    ApiVersion version;
    if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
        auto rtlGetVersion =
            reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (rtlGetVersion) {
            RTL_OSVERSIONINFOW info{};
            info.dwOSVersionInfoSize = sizeof(info);
            if (rtlGetVersion(&info) == 0) {
                version.major = info.dwMajorVersion;
                version.minor = info.dwMinorVersion;
                version.build = info.dwBuildNumber;
                return version;
            }
        }
    }
    return version;
}

} // namespace

DWORD windowsBuild() {
    return queryVersion().build;
}

WindowsVersion detectWindowsVersion() {
    const ApiVersion version = queryVersion();

    if (version.major == 6) {
        switch (version.minor) {
            case 1: return WindowsVersion::Win7;
            case 2: return WindowsVersion::Win8;
            case 3: return WindowsVersion::Win8_1;
            default: break;
        }
    }
    if (version.major == 10) {
        return version.build >= 22000 ? WindowsVersion::Win11
                                      : WindowsVersion::Win10;
    }
    if (version.major > 10) return WindowsVersion::Win11;
    return WindowsVersion::Unknown;
}

bool supportsDarkMode() {
    const WindowsVersion version = detectWindowsVersion();
    return version == WindowsVersion::Win10 || version == WindowsVersion::Win11;
}

void enableDpiAwareness() {
    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        using SetContextFn = BOOL(WINAPI*)(HANDLE);
        auto setContext = reinterpret_cast<SetContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setContext) {
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
            if (setContext(reinterpret_cast<HANDLE>(-4))) return;
        }
        using SetDpiAwareFn = BOOL(WINAPI*)();
        auto setDpiAware =
            reinterpret_cast<SetDpiAwareFn>(GetProcAddress(user32, "SetProcessDPIAware"));
        if (setDpiAware) setDpiAware();
    }
}

int systemDpi() {
    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        using GetDpiForSystemFn = UINT(WINAPI*)();
        auto getDpi = reinterpret_cast<GetDpiForSystemFn>(
            GetProcAddress(user32, "GetDpiForSystem"));
        if (getDpi) {
            const UINT dpi = getDpi();
            if (dpi) return static_cast<int>(dpi);
        }
    }
    // Windows 7 / 8: read the OS DPI. When the process is DPI aware this is the
    // real value; when it is not, the OS virtualises it to 96 (blurry but the
    // correct size), which is the best we can do without awareness support.
    HDC dc = GetDC(nullptr);
    const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc) ReleaseDC(nullptr, dc);
    return dpi > 0 ? dpi : 96;
}

int windowDpi(HWND hwnd) {
    if (hwnd) {
        if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
            using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
            auto getDpi = reinterpret_cast<GetDpiForWindowFn>(
                GetProcAddress(user32, "GetDpiForWindow"));
            if (getDpi) {
                const UINT dpi = getDpi(hwnd);
                if (dpi) return static_cast<int>(dpi);
            }
        }
    }
    return systemDpi();
}

Language detectSystemLanguage() {
    const LANGID langid = GetUserDefaultUILanguage();
    return PRIMARYLANGID(langid) == LANG_CHINESE ? Language::Chinese : Language::English;
}

bool systemUsesDarkMode() {
    if (!supportsDarkMode()) return false;

    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }

    DWORD value = 1;
    DWORD size = sizeof(value);
    DWORD type = 0;
    const LONG result = RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, &type,
                                         reinterpret_cast<LPBYTE>(&value), &size);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_DWORD) return false;
    return value == 0;
}

std::wstring executablePath() {
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD written =
            GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (written == 0) return {};
        if (written < buffer.size() - 1) {
            buffer.resize(written);
            return buffer;
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::wstring executableDirectory() {
    std::wstring path = executablePath();
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return path;
    return path.substr(0, slash);
}

HICON loadIcon(int resourceId, int size) {
    return static_cast<HICON>(LoadImageW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(resourceId),
        IMAGE_ICON,
        size,
        size,
        LR_DEFAULTCOLOR));
}

HICON loadAppIcon(int size) {
    return loadIcon(IDI_APP, size);
}

} // namespace clipclean
