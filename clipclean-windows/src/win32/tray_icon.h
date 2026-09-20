#pragma once

#include <windows.h>

namespace clipclean {

/// Thin wrapper around `Shell_NotifyIcon`. Mouse and menu messages are sent to
/// the owning window using the callback message passed to `create`.
class TrayIcon {
public:
    TrayIcon() = default;
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    bool create(HWND owner, UINT callbackMessage, HICON icon, const wchar_t* tooltip);
    void setIcon(HICON icon);
    void setTooltip(const wchar_t* tooltip);
    /// Shows a balloon / toast from the tray icon.
    void showBalloon(const wchar_t* title, const wchar_t* text);
    void remove();

private:
    NOTIFYICONDATAW data_{};
    bool added_ = false;
};

} // namespace clipclean
