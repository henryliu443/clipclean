#pragma once

#include <functional>

#include <windows.h>

namespace clipclean {

/// Watches the clipboard for changes.
///
/// Uses `AddClipboardFormatListener` (Vista+) when available and falls back to
/// the legacy `SetClipboardViewer` chain on XP. The message window owning the
/// monitor forwards `WM_DRAWCLIPBOARD` / `WM_CHANGECBCHAIN` / `WM_CLIPBOARDUPDATE`
/// here.
class ClipboardMonitor {
public:
    using Handler = std::function<void()>;

    void setHandler(Handler handler) { handler_ = std::move(handler); }

    bool start(HWND owner);
    void stop();
    bool isRunning() const { return owner_ != nullptr; }

    void onChangeChain(HWND remove, HWND next);
    void onDrawClipboard();
    void onClipboardUpdate();

private:
    HWND owner_ = nullptr;
    HWND nextViewer_ = nullptr;
    bool usingListener_ = false;
    Handler handler_;
};

} // namespace clipclean
