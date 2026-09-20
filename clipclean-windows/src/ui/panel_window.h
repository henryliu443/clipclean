#pragma once

#include <windows.h>

#include "ui/panel_view.h"

namespace clipclean {

class App;

/// The borderless popup that hosts the panel. Owns its `PanelView`, follows the
/// system light / dark appearance and dismisses itself when it loses activation.
class PanelWindow {
public:
    explicit PanelWindow(App& app);
    ~PanelWindow();

    bool create(HINSTANCE instance);
    void destroy();

    void show();
    void hide();
    void toggle();
    bool isVisible() const;

    /// Shows the panel with the settings page selected.
    void showSettings();

    void setDark(bool dark);
    void refresh();

    /// Points the panel's slide switch at the app's current mode without
    /// waiting for the panel to be opened (used by hot keys and the tray menu).
    void syncSwitch(bool animate);

    HWND hwnd() const { return hwnd_; }

    /// Tick count of the last time the panel auto-hid because it lost focus.
    /// Lets the tray click distinguish "just closed" from "open it".
    DWORD lastHiddenTick() const { return lastHiddenTick_; }

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam,
                                       LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void resizeForSettings(bool show);
    void reposition();
    void applyRegion();

    App& app_;
    HWND hwnd_ = nullptr;
    PanelView view_;
    int dpi_ = 96;
    DWORD lastHiddenTick_ = 0;
    bool trackingMouse_ = false;
};

} // namespace clipclean
