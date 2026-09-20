#pragma once

#include <string>

#include <windows.h>

#include "core/settings.h"

namespace clipclean {

/// Coarse Windows version, good enough for feature gating.
enum class WindowsVersion {
    Win7,
    Win8,
    Win8_1,
    Win10,
    Win11,
    Unknown,
};

/// The running Windows build number (for example 7601 on Windows 7 SP1,
/// 22621 on Windows 11 22H2). Uses RtlGetVersion so it is not affected by
/// manifest-based version lies.
DWORD windowsBuild();

/// Detects the running Windows version.
WindowsVersion detectWindowsVersion();

/// Whether the OS has a system light / dark app appearance. Only Windows 10
/// and 11 do; Windows 7 and 8 are always light.
bool supportsDarkMode();

/// Opts the process into DPI awareness when the OS supports it. No-op on XP.
void enableDpiAwareness();

/// DPI of the primary display (96 when unknown). Use this to scale the fixed
/// pixel metrics of the UI so it is the same physical size at any scale factor.
int systemDpi();

/// DPI of the monitor a window is on. Falls back to `systemDpi()`.
int windowDpi(HWND hwnd);

/// Language the user interface should default to on first launch.
Language detectSystemLanguage();

/// Whether applications should use their dark appearance.
bool systemUsesDarkMode();

std::wstring executablePath();

std::wstring executableDirectory();

/// Loads an icon from the executable at the requested size.
HICON loadIcon(int resourceId, int size);

/// Loads the main application icon at the requested size.
HICON loadAppIcon(int size);

} // namespace clipclean
