#include "win32/autostart.h"

#include <cwchar>

#include <windows.h>

#include "win32/win_util.h"

namespace clipclean {
namespace {

const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* kValueName = L"Clipclean";

std::wstring quotedExecutable() {
    return L"\"" + executablePath() + L"\"";
}

} // namespace

bool isAutostartEnabled() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t value[1024] = {};
    DWORD size = sizeof(value) - sizeof(wchar_t);
    DWORD type = 0;
    const LONG result = RegQueryValueExW(key, kValueName, nullptr, &type,
                                         reinterpret_cast<LPBYTE>(value), &size);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_SZ) return false;
    return _wcsicmp(value, quotedExecutable().c_str()) == 0;
}

void setAutostartEnabled(bool enabled) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS) {
        return;
    }

    if (enabled) {
        const std::wstring command = quotedExecutable();
        RegSetValueExW(key, kValueName, 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(command.c_str()),
                       static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(key, kValueName);
    }

    RegCloseKey(key);
}

} // namespace clipclean
