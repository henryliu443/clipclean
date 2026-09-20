#include "ui/panel_window.h"

#include <windowsx.h>

#include "app.h"
#include "ui/theme.h"
#include "win32/win_util.h"

namespace clipclean {
namespace {

const wchar_t* kWindowClass = L"ClipcleanPanelWindow";
constexpr int kCornerRadius = 14;
constexpr int kMainWidth = 240;
constexpr int kMainHeight = 300;
constexpr int kSettingsWidth = 300;
constexpr int kSettingsHeight = 400;

void clampToWorkArea(int& x, int& y, int width, int height, const RECT& work) {
    const int margin = 8;
    if (x + width > work.right - margin) x = work.right - margin - width;
    if (x < work.left + margin) x = work.left + margin;
    if (y + height > work.bottom - margin) y = work.bottom - margin - height;
    if (y < work.top + margin) y = work.top + margin;
}

} // namespace

PanelWindow::PanelWindow(App& app) : app_(app), view_(app) {}

PanelWindow::~PanelWindow() {
    destroy();
}

bool PanelWindow::create(HINSTANCE instance) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &PanelWindow::windowProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = kWindowClass;
        if (!RegisterClassExW(&wc)) return false;
        registered = true;
    }

    hwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST, kWindowClass, L"Clipclean",
        WS_POPUP, 0, 0, MulDiv(kMainWidth, dpi_, 96), MulDiv(kMainHeight, dpi_, 96),
        nullptr, nullptr, instance, this);
    if (!hwnd_) return false;

    // Prefer the real DPI of the monitor the panel landed on, then build the
    // fonts for it. Every later metric goes through the view's DPI scale.
    dpi_ = windowDpi(hwnd_);
    view_.setDpi(dpi_);

    applyRegion();

    view_.setDark(systemUsesDarkMode());
    view_.onShowSettingsChanged = [this](bool show) { resizeForSettings(show); };
    return true;
}

void PanelWindow::destroy() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void PanelWindow::applyRegion() {
    if (!hwnd_) return;
    RECT rect{};
    GetClientRect(hwnd_, &rect);
    const int radius = MulDiv(kCornerRadius, dpi_, 96) * 2;
    HRGN region = CreateRoundRectRgn(0, 0, rect.right + 1, rect.bottom + 1,
                                     radius, radius);
    SetWindowRgn(hwnd_, region, TRUE);
}

void PanelWindow::resizeForSettings(bool show) {
    if (!hwnd_) return;
    const int width = show ? MulDiv(kSettingsWidth, dpi_, 96)
                           : MulDiv(kMainWidth, dpi_, 96);
    const int height = show ? MulDiv(kSettingsHeight, dpi_, 96)
                            : MulDiv(kMainHeight, dpi_, 96);
    SetWindowPos(hwnd_, nullptr, 0, 0, width, height,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    applyRegion();
    reposition();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void PanelWindow::reposition() {
    if (!hwnd_) return;

    RECT window{};
    GetWindowRect(hwnd_, &window);
    const int width = window.right - window.left;
    const int height = window.bottom - window.top;

    // Anchor to the tray icon when the shell can report it; otherwise fall back
    // to a fixed spot near the notification area. Deliberately NOT the cursor:
    // the panel must always open in the same, predictable place.
    RECT iconRect{};
    const bool haveIcon = app_.trayIconRect(iconRect);

    HMONITOR monitor = nullptr;
    if (haveIcon) {
        const POINT anchor{(iconRect.left + iconRect.right) / 2, iconRect.bottom};
        monitor = MonitorFromPoint(anchor, MONITOR_DEFAULTTONEAREST);
    } else {
        const POINT corner{GetSystemMetrics(SM_CXSCREEN) - 1,
                           GetSystemMetrics(SM_CYSCREEN) - 1};
        monitor = MonitorFromPoint(corner, MONITOR_DEFAULTTONEAREST);
    }

    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) {
        info.rcWork = RECT{0, 0, GetSystemMetrics(SM_CXSCREEN),
                           GetSystemMetrics(SM_CYSCREEN)};
    }
    const RECT work = info.rcWork;
    const int margin = 8;

    int x;
    int y;
    if (haveIcon) {
        x = (iconRect.left + iconRect.right) / 2 - width / 2;
        const bool iconBelowMiddle = iconRect.bottom > (work.top + work.bottom) / 2;
        y = iconBelowMiddle ? iconRect.top - height - 4 : iconRect.bottom + 4;
    } else {
        // Fixed default: tucked into the bottom-right corner of the work area.
        x = work.right - width - margin;
        y = work.bottom - height - margin;
    }
    clampToWorkArea(x, y, width, height, work);

    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, width, height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void PanelWindow::show() {
    if (!hwnd_) return;
    view_.setDark(systemUsesDarkMode());
    view_.syncSwitch(false);
    resizeForSettings(view_.showSettings());
    reposition();
    ShowWindow(hwnd_, SW_SHOW);
    BringWindowToTop(hwnd_);
    SetForegroundWindow(hwnd_);
    SetFocus(hwnd_);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void PanelWindow::hide() {
    if (!hwnd_) return;
    ShowWindow(hwnd_, SW_HIDE);
}

void PanelWindow::showSettings() {
    view_.setShowSettings(true);
    show();
}

void PanelWindow::toggle() {
    if (isVisible()) {
        hide();
    } else {
        show();
    }
}

bool PanelWindow::isVisible() const {
    return hwnd_ && IsWindowVisible(hwnd_);
}

void PanelWindow::setDark(bool dark) {
    view_.setDark(dark);
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
}

void PanelWindow::refresh() {
    if (!hwnd_) return;
    // A hot key can start the switch animation without a mouse click, so make
    // sure the frame timer is running whenever the switch still has to move.
    if (view_.needsTimer()) SetTimer(hwnd_, 1, 16, nullptr);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void PanelWindow::syncSwitch(bool animate) {
    view_.syncSwitch(animate);
}

LRESULT CALLBACK PanelWindow::windowProc(HWND hwnd, UINT message, WPARAM wParam,
                                         LPARAM lParam) {
    PanelWindow* self =
        reinterpret_cast<PanelWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<PanelWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    if (!self) return DefWindowProcW(hwnd, message, wParam, lParam);
    return self->handleMessage(message, wParam, lParam);
}

LRESULT PanelWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd_, &ps);
            RECT client{};
            GetClientRect(hwnd_, &client);
            const int width = client.right;
            const int height = client.bottom;

            // Double buffering: the switch animates, so paint off-screen and
            // blit once to avoid flicker.
            HDC memory = CreateCompatibleDC(dc);
            HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
            HGDIOBJ oldBitmap = SelectObject(memory, bitmap);

            view_.paint(memory, width, height);
            BitBlt(dc, 0, 0, width, height, memory, 0, 0, SRCCOPY);

            SelectObject(memory, oldBitmap);
            DeleteObject(bitmap);
            DeleteDC(memory);

            EndPaint(hwnd_, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_MOUSEMOVE: {
            if (!trackingMouse_) {
                TRACKMOUSEEVENT track{};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = hwnd_;
                TrackMouseEvent(&track);
                trackingMouse_ = true;
            }
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            if (view_.onMouseMove(x, y)) InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }
        case WM_MOUSELEAVE:
            trackingMouse_ = false;
            if (view_.onMouseMove(-1, -1)) InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        case WM_SETCURSOR:
            SetCursor(LoadCursorW(nullptr, view_.wantsHandCursor() ? IDC_HAND
                                                                   : IDC_ARROW));
            return TRUE;
        case WM_LBUTTONDOWN:
            SetCapture(hwnd_);
            view_.onMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONUP: {
            ReleaseCapture();
            const bool handled =
                view_.onMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            if (view_.needsTimer()) SetTimer(hwnd_, 1, 16, nullptr);
            if (handled) InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }
        case WM_TIMER:
            view_.tick();
            InvalidateRect(hwnd_, nullptr, FALSE);
            if (!view_.needsTimer()) KillTimer(hwnd_, 1);
            return 0;
        case WM_KEYDOWN:
            if (view_.onKeyDown(wParam)) {
                InvalidateRect(hwnd_, nullptr, FALSE);
            } else if (wParam == VK_ESCAPE) {
                hide();
            }
            return 0;
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                lastHiddenTick_ = GetTickCount();
                hide();
            }
            return 0;
        case WM_DPICHANGED: {
            // The panel moved to a monitor with a different scale factor:
            // rebuild fonts, adopt the OS-suggested size/position and redraw.
            dpi_ = HIWORD(wParam);
            view_.setDpi(dpi_);
            const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
            if (suggested) {
                SetWindowPos(hwnd_, nullptr, suggested->left, suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            applyRegion();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        }
        case WM_SETTINGCHANGE:
            // "ImmersiveColorSet" is broadcast when the light/dark app theme
            // changes. systemUsesDarkMode() only reports dark on Windows 10+.
            if (lParam && lstrcmpiW(reinterpret_cast<const wchar_t*>(lParam),
                                    L"ImmersiveColorSet") == 0) {
                view_.setDark(app_.effectiveDark());
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            return 0;
        case WM_THEMECHANGED:
            view_.setDark(app_.effectiveDark());
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

} // namespace clipclean
