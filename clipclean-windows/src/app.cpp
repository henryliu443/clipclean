#include "app.h"

#include <string>

#include "resource.h"
#include "win32/autostart.h"
#include "win32/clipboard.h"
#include "win32/hotkeys.h"
#include "win32/settings_store.h"
#include "win32/win_util.h"

namespace clipclean {
namespace {

const wchar_t* kMessageWindowClass = L"ClipcleanMessageWindow";

constexpr int kHotKeyOff = 1;
constexpr int kHotKeyOn = 2;

constexpr UINT kMenuModeOn = 1001;
constexpr UINT kMenuModeOff = 1002;
constexpr UINT kMenuSettings = 1003;
constexpr UINT kMenuQuit = 1004;

/// Mirrors NOTIFYICONIDENTIFIER, which is not declared when targeting XP.
struct NotifyIconIdentifier {
    DWORD cbSize;
    HWND hwnd;
    UINT uID;
    GUID guidItem;
};

} // namespace

App::App() : panel_(*this) {}

App::~App() {
    clipboardMonitor_.stop();
    if (messageWindow_) {
        unregisterHotKey(messageWindow_, kHotKeyOn);
        unregisterHotKey(messageWindow_, kHotKeyOff);
    }
    tray_.remove();
    panel_.destroy();
    if (messageWindow_) {
        DestroyWindow(messageWindow_);
        messageWindow_ = nullptr;
    }
    if (trayIcon_) {
        DestroyIcon(trayIcon_);
        trayIcon_ = nullptr;
    }
}

bool App::initialize(HINSTANCE instance) {
    instance_ = instance;
    enableDpiAwareness();

    settings_ = loadSettings();
    settings_.launchAtLogin = isAutostartEnabled();

    if (!createMessageWindow()) return false;

    clipboardMonitor_.setHandler([this]() { handleClipboardChange(); });

    trayIcon_ = loadIcon(settings_.plainTextModeEnabled ? IDI_APP_ON : IDI_APP_OFF,
                         GetSystemMetrics(SM_CXSMICON));
    if (!tray_.create(messageWindow_, kTrayCallbackMessage, trayIcon_, L"Clipclean")) {
        return false;
    }

    if (!panel_.create(instance)) return false;

    registerHotKey(messageWindow_, kHotKeyOff, 0, VK_F5);
    registerHotKey(messageWindow_, kHotKeyOn, 0, VK_F6);

    applyModeState();
    refreshTray();

    if (settings_.firstRun) {
        // First launch: greet from the tray only. Never open the panel by
        // itself, and persist now so the balloon shows exactly once.
        const Strings& text = strings();
        tray_.showBalloon(text.firstRunTitle, text.firstRunMessage);
        settings_.firstRun = false;
        saveSettings(settings_);
    }
    return true;
}

int App::run() {
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

bool App::createMessageWindow() {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &App::messageWindowProc;
        wc.hInstance = instance_;
        wc.lpszClassName = kMessageWindowClass;
        if (!RegisterClassExW(&wc)) return false;
        registered = true;
    }

    messageWindow_ = CreateWindowExW(0, kMessageWindowClass, L"Clipclean",
                                     WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
                                     instance_, this);
    return messageWindow_ != nullptr;
}

LRESULT CALLBACK App::messageWindowProc(HWND hwnd, UINT message, WPARAM wParam,
                                        LPARAM lParam) {
    App* self =
        reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<App*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        // Needed while the window is still being created: handleMessage()
        // forwards unhandled messages to DefWindowProcW(messageWindow_, ...).
        self->messageWindow_ = hwnd;
    }
    if (!self) return DefWindowProcW(hwnd, message, wParam, lParam);
    return self->handleMessage(message, wParam, lParam);
}

LRESULT App::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case kTrayCallbackMessage:
            handleTrayMessage(lParam);
            return 0;
        case kShowPanelMessage:
            panel_.show();
            return 0;
        case WM_HOTKEY:
            handleHotKey(static_cast<int>(wParam));
            return 0;
        case WM_CLIPBOARDUPDATE:
            clipboardMonitor_.onClipboardUpdate();
            return 0;
        case WM_DRAWCLIPBOARD:
            clipboardMonitor_.onDrawClipboard();
            return 0;
        case WM_CHANGECBCHAIN:
            clipboardMonitor_.onChangeChain(reinterpret_cast<HWND>(wParam),
                                            reinterpret_cast<HWND>(lParam));
            return 0;
        case WM_CLOSE:
            DestroyWindow(messageWindow_);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(messageWindow_, message, wParam, lParam);
}

void App::handleTrayMessage(LPARAM lParam) {
    switch (LOWORD(lParam)) {
        case WM_LBUTTONUP:
            if (panel_.isVisible()) {
                panel_.hide();
            } else if (GetTickCount() - panel_.lastHiddenTick() > 250) {
                panel_.show();
            }
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            showContextMenu();
            break;
        default:
            break;
    }
}

void App::handleHotKey(int id) {
    if (id == kHotKeyOn) {
        setPlainTextModeEnabled(true);
    } else if (id == kHotKeyOff) {
        setPlainTextModeEnabled(false);
    }
}

void App::handleClipboardChange() {
    if (!settings_.plainTextModeEnabled) return;

    const unsigned long sequence = clipboardSequenceNumber();
    if (sequence == ignoredSequence_) return;

    if (!clipboardRequiresPlainTextRewrite()) return;

    std::wstring text;
    if (!clipboardPlainText(text)) return;

    if (rewriteClipboardAsPlainText(text)) {
        ignoredSequence_ = clipboardSequenceNumber();
    }
}

void App::showContextMenu() {
    const Strings& text = strings();
    const bool enabled = settings_.plainTextModeEnabled;

    HMENU menu = CreatePopupMenu();
    HMENU modeMenu = CreatePopupMenu();
    AppendMenuW(modeMenu, MF_STRING | (enabled ? MF_CHECKED : 0), kMenuModeOn,
                text.on);
    AppendMenuW(modeMenu, MF_STRING | (!enabled ? MF_CHECKED : 0), kMenuModeOff,
                text.off);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(modeMenu), text.mode);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuSettings, text.settingsEllipsis);
    AppendMenuW(menu, MF_STRING, kMenuQuit, text.quit);

    SetForegroundWindow(messageWindow_);
    POINT point{};
    GetCursorPos(&point);
    const int command =
        TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                       point.x, point.y, 0, messageWindow_, nullptr);
    PostMessageW(messageWindow_, WM_NULL, 0, 0);
    DestroyMenu(menu);

    switch (command) {
        case kMenuModeOn:
            setPlainTextModeEnabled(true);
            break;
        case kMenuModeOff:
            setPlainTextModeEnabled(false);
            break;
        case kMenuSettings:
            panel_.showSettings();
            break;
        case kMenuQuit:
            quit();
            break;
        default:
            break;
    }
}

void App::setPlainTextModeEnabled(bool enabled) {
    if (enabled == settings_.plainTextModeEnabled) return;
    settings_.plainTextModeEnabled = enabled;
    saveSettings(settings_);
    applyModeState();
    refreshTray();
    panel_.syncSwitch(true);
    panel_.refresh();
}

void App::setLanguage(Language language) {
    if (language == settings_.language) return;
    settings_.language = language;
    saveSettings(settings_);
    // The language only affects the tooltip, not the icon: skip the (slow)
    // Shell_NotifyIcon icon reload so the switch feels instant.
    refreshTrayTooltip();
    panel_.refresh();
}

void App::setLaunchAtLogin(bool enabled) {
    setAutostartEnabled(enabled);
    settings_.launchAtLogin = isAutostartEnabled();
    panel_.refresh();
}

bool App::effectiveDark() const {
    if (supportsDarkMode() && settings_.followSystemTheme) {
        return systemUsesDarkMode();
    }
    return settings_.darkTheme;
}

void App::setDarkTheme(bool dark) {
    settings_.darkTheme = dark;
    // A manual choice takes over from the system.
    settings_.followSystemTheme = false;
    saveSettings(settings_);
    panel_.setDark(effectiveDark());
    panel_.refresh();
}

void App::setFollowSystemTheme(bool follow) {
    if (settings_.followSystemTheme == follow) return;
    settings_.followSystemTheme = follow;
    saveSettings(settings_);
    panel_.setDark(effectiveDark());
    panel_.refresh();
}

void App::applyModeState() {
    if (settings_.plainTextModeEnabled) {
        if (!clipboardMonitor_.isRunning()) {
            clipboardMonitor_.start(messageWindow_);
        }
    } else {
        clipboardMonitor_.stop();
    }
}

void App::refreshTray() {
    const bool enabled = settings_.plainTextModeEnabled;
    HICON icon = loadIcon(enabled ? IDI_APP_ON : IDI_APP_OFF,
                          GetSystemMetrics(SM_CXSMICON));
    if (icon) {
        tray_.setIcon(icon);
        if (trayIcon_ && trayIcon_ != icon) DestroyIcon(trayIcon_);
        trayIcon_ = icon;
    }

    refreshTrayTooltip();
}

void App::refreshTrayTooltip() {
    const Strings& text = strings();
    const bool enabled = settings_.plainTextModeEnabled;
    const std::wstring tooltip = std::wstring(text.appName) + L" \u2014 " +
                                 text.plainTextMode + L": " +
                                 (enabled ? text.on : text.off);
    tray_.setTooltip(tooltip.c_str());
}

bool App::trayIconRect(RECT& out) const {
    using GetRectFn = HRESULT(WINAPI*)(const NotifyIconIdentifier*, RECT*);
    static GetRectFn fn = reinterpret_cast<GetRectFn>(GetProcAddress(
        GetModuleHandleW(L"user32.dll"), "Shell_NotifyIconGetRect"));
    if (!fn || !messageWindow_) return false;

    NotifyIconIdentifier identifier{};
    identifier.cbSize = sizeof(identifier);
    identifier.hwnd = messageWindow_;
    identifier.uID = 1;
    return fn(&identifier, &out) == S_OK;
}

void App::quit() {
    if (quitting_) return;
    quitting_ = true;
    if (messageWindow_) PostMessageW(messageWindow_, WM_CLOSE, 0, 0);
}

} // namespace clipclean
