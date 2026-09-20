#include "win32/settings_store.h"

#include <cwchar>

#include <windows.h>

#include "win32/win_util.h"

namespace clipclean {
namespace {

const wchar_t* kKeyPath = L"Software\\Clipclean";
const wchar_t* kValueMode = L"PlainTextModeEnabled";
const wchar_t* kValueLanguage = L"Language";
const wchar_t* kValueFollowSystem = L"FollowSystemTheme";
const wchar_t* kValueDarkTheme = L"DarkTheme";

} // namespace

Settings loadSettings() {
    Settings settings;
    settings.language = detectSystemLanguage();

    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kKeyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        // No settings key yet: this is the first launch.
        settings.firstRun = true;
        return settings;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);
    DWORD type = 0;
    if (RegQueryValueExW(key, kValueMode, nullptr, &type,
                         reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS &&
        type == REG_DWORD) {
        settings.plainTextModeEnabled = value != 0;
    }

    wchar_t language[8] = {};
    size = sizeof(language) - sizeof(wchar_t);
    if (RegQueryValueExW(key, kValueLanguage, nullptr, &type,
                         reinterpret_cast<LPBYTE>(language), &size) == ERROR_SUCCESS &&
        type == REG_SZ) {
        settings.language = (wcscmp(language, L"zh") == 0) ? Language::Chinese
                                                           : Language::English;
    }

    DWORD follow = 1;
    size = sizeof(follow);
    if (RegQueryValueExW(key, kValueFollowSystem, nullptr, &type,
                         reinterpret_cast<LPBYTE>(&follow), &size) == ERROR_SUCCESS &&
        type == REG_DWORD) {
        settings.followSystemTheme = follow != 0;
    }

    DWORD dark = 0;
    size = sizeof(dark);
    if (RegQueryValueExW(key, kValueDarkTheme, nullptr, &type,
                         reinterpret_cast<LPBYTE>(&dark), &size) == ERROR_SUCCESS &&
        type == REG_DWORD) {
        settings.darkTheme = dark != 0;
    }

    RegCloseKey(key);
    return settings;
}

void saveSettings(const Settings& settings) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kKeyPath, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS) {
        return;
    }

    const DWORD mode = settings.plainTextModeEnabled ? 1 : 0;
    RegSetValueExW(key, kValueMode, 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&mode), sizeof(mode));

    const wchar_t* language = (settings.language == Language::Chinese) ? L"zh" : L"en";
    RegSetValueExW(key, kValueLanguage, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(language),
                   static_cast<DWORD>((wcslen(language) + 1) * sizeof(wchar_t)));

    const DWORD follow = settings.followSystemTheme ? 1 : 0;
    RegSetValueExW(key, kValueFollowSystem, 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&follow), sizeof(follow));

    const DWORD dark = settings.darkTheme ? 1 : 0;
    RegSetValueExW(key, kValueDarkTheme, 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&dark), sizeof(dark));

    RegCloseKey(key);
}

} // namespace clipclean
