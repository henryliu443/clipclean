# Clipclean for Windows

Native Win32 (C++17) port of the macOS Clipclean menu bar utility. It watches
the clipboard and, when **Plain Text Mode** is on, strips rich text so every
paste is plain text.

Targets **Windows 7 SP1 and later** (32- or 64-bit), with no .NET runtime and no
installer: a single small executable.

## Features

- Tray icon with a right-click menu: Mode ▸ On / Off, Settings, Quit
- Left-click opens a borderless panel with a large slide switch
- Global hot keys: `F6` turns Plain Text Mode on, `F5` turns it off
- Settings: launch at login, appearance (dark mode + follow system), language
  (English / 中文)
- Clipboard monitoring via `AddClipboardFormatListener`
- Panel follows the system light / dark appearance on Windows 10 and 11; on
  Windows 7 and 8 a manual light / dark switch is always available
- No network access, no clipboard history

## Build

### Visual Studio 2026 (MSVC)

Install the **.NET desktop development** workload is *not* required; you only
need **Desktop development with C++**. Then:

```bat
cmake -S . -B build -G "Visual Studio 18 2026"
cmake --build build --config Release
```

The executable is `build\Release\Clipclean.exe`.

### MinGW-w64

```sh
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Tests

The clipboard text transformer is pure logic and has a small test executable:

```bat
cmake --build build --target plain_text_tests
ctest --test-dir build
```

## Layout

```
src/
  main.cpp                 WinMain and the message loop
  app.{h,cpp}              wiring: tray, hot keys, clipboard, panel
  core/                    platform-independent logic (unit tested)
    plain_text.{h,cpp}     RTF / HTML -> plain text
    strings.{h,cpp}        English / 中文
    settings.h             preference data model
  win32/                   thin wrappers over the Windows API
    tray_icon.{h,cpp}      Shell_NotifyIcon
    hotkeys.{h,cpp}        RegisterHotKey
    clipboard.{h,cpp}      clipboard read / write
    clipboard_monitor.{h,cpp}
    autostart.{h,cpp}      HKCU Run key
    settings_store.{h,cpp} registry persistence
    win_util.{h,cpp}       version, DPI, dark mode, icons
  ui/
    panel_window.{h,cpp}   borderless popup window
    panel_view.{h,cpp}     panel contents and hit testing
    switch_control.{h,cpp} the slide switch
    theme.{h,cpp}          colours and fonts
res/
  app.rc                   icons, version info, manifest
  app.manifest             DPI awareness, supported OS
  resource.h
  *.ico                    generated from the macOS AppIcon set
tests/
  test_plain_text.cpp
```

The macOS sources live in the repository root and are untouched by this port.
