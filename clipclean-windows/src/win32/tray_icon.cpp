#include "win32/tray_icon.h"

#include "win32/win_util.h"

namespace clipclean {
namespace {

DWORD notifyIconSize() {
    // Windows XP's shell rejects the Vista-era struct size, so fall back to the
    // V2 layout there.
#ifdef NOTIFYICONDATA_V2_SIZE
    if (windowsBuild() < 6000) return NOTIFYICONDATA_V2_SIZE;
#endif
    return sizeof(NOTIFYICONDATAW);
}

} // namespace

TrayIcon::~TrayIcon() {
    remove();
}

bool TrayIcon::create(HWND owner, UINT callbackMessage, HICON icon,
                      const wchar_t* tooltip) {
    data_ = {};
    data_.cbSize = notifyIconSize();
    data_.hWnd = owner;
    data_.uID = 1;
    data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data_.uCallbackMessage = callbackMessage;
    data_.hIcon = icon;
    if (tooltip) lstrcpynW(data_.szTip, tooltip, 128);

    added_ = Shell_NotifyIconW(NIM_ADD, &data_) != FALSE;
    return added_;
}

void TrayIcon::setIcon(HICON icon) {
    if (!added_) return;
    data_.hIcon = icon;
    data_.uFlags = NIF_ICON;
    Shell_NotifyIconW(NIM_MODIFY, &data_);
}

void TrayIcon::setTooltip(const wchar_t* tooltip) {
    if (!added_ || !tooltip) return;
    lstrcpynW(data_.szTip, tooltip, 128);
    data_.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &data_);
}

void TrayIcon::showBalloon(const wchar_t* title, const wchar_t* text) {
    if (!added_ || !text) return;
    data_.uFlags = NIF_INFO;
    data_.dwInfoFlags = NIIF_INFO;
    if (title) lstrcpynW(data_.szInfoTitle, title, 64);
    lstrcpynW(data_.szInfo, text, 256);
    Shell_NotifyIconW(NIM_MODIFY, &data_);
}

void TrayIcon::remove() {
    if (!added_) return;
    Shell_NotifyIconW(NIM_DELETE, &data_);
    added_ = false;
}

} // namespace clipclean
