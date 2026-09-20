#pragma once

#include <windows.h>

#include "core/settings.h"
#include "core/strings.h"
#include "ui/panel_window.h"
#include "win32/clipboard_monitor.h"
#include "win32/tray_icon.h"

namespace clipclean {

constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kShowPanelMessage = WM_APP + 2;

/// Wires the tray icon, clipboard monitor, global hot keys and panel together
/// and owns the application state.
class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool initialize(HINSTANCE instance);
    int run();

    bool plainTextModeEnabled() const { return settings_.plainTextModeEnabled; }
    void setPlainTextModeEnabled(bool enabled);
    void togglePlainTextMode() { setPlainTextModeEnabled(!plainTextModeEnabled()); }

    Language language() const { return settings_.language; }
    void setLanguage(Language language);

    bool launchAtLogin() const { return settings_.launchAtLogin; }
    void setLaunchAtLogin(bool enabled);

    bool darkTheme() const { return settings_.darkTheme; }
    bool followSystemTheme() const { return settings_.followSystemTheme; }
    void setDarkTheme(bool dark);
    void setFollowSystemTheme(bool follow);

    /// Appearance the panel should actually use: the system theme when
    /// following (Windows 10/11 only), otherwise the manual choice.
    bool effectiveDark() const;

    const Strings& strings() const { return stringsFor(settings_.language); }

    /// Screen rectangle of the tray icon, if the OS can report it.
    bool trayIconRect(RECT& out) const;

    void quit();

private:
    static LRESULT CALLBACK messageWindowProc(HWND hwnd, UINT message,
                                              WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    bool createMessageWindow();
    void handleTrayMessage(LPARAM lParam);
    void handleHotKey(int id);
    void handleClipboardChange();
    void showContextMenu();
    void applyModeState();
    void refreshTray();
    void refreshTrayTooltip();

    HINSTANCE instance_ = nullptr;
    Settings settings_;
    HWND messageWindow_ = nullptr;
    TrayIcon tray_;
    ClipboardMonitor clipboardMonitor_;
    PanelWindow panel_;
    HICON trayIcon_ = nullptr;
    unsigned long ignoredSequence_ = 0;
    bool quitting_ = false;
};

} // namespace clipclean
