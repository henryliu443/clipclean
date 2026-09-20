#pragma once

#include <functional>

#include <windows.h>

#include "ui/switch_control.h"
#include "ui/theme.h"

namespace clipclean {

class App;

/// Draws the panel contents (main view and settings) and performs hit testing.
/// It owns no window; `PanelWindow` forwards paint and input events here.
class PanelView {
public:
    explicit PanelView(App& app);
    ~PanelView();

    void setShowSettings(bool show);
    bool showSettings() const { return showSettings_; }

    void setDark(bool dark);
    bool dark() const { return theme_.dark; }

    /// Rebuilds the fonts for a new display DPI.
    void setDpi(int dpi);
    int dpi() const { return theme_.dpi; }

    /// Points the slide switch at the app's current mode. Pass `animate` when
    /// the change comes from a hot key so the knob slides instead of jumping.
    void syncSwitch(bool animate);

    void paint(HDC dc, int width, int height);
    bool onMouseMove(int x, int y);
    bool onMouseDown(int x, int y);
    bool onMouseUp(int x, int y);
    bool onKeyDown(WPARAM key);

    void tick();
    bool needsTimer() const { return switch_.isAnimating(); }

    bool wantsHandCursor() const { return hot_ != kHotNone; }

    /// Invoked when the settings page is entered or left so the window can
    /// resize itself.
    std::function<void(bool)> onShowSettingsChanged;

private:
    enum HotTarget {
        kHotNone = -1,
        kHotGear,
        kHotBack,
        kHotSwitch,
        kHotLaunch,
        kHotDarkMode,
        kHotFollowSystem,
        kHotLanguage,
        kHotQuit,
    };

    void layout(int width, int height);
    void drawMain(HDC dc);
    void drawSettings(HDC dc);
    void drawDocumentIcon(HDC dc, const RECT& rect, COLORREF color);
    void drawGearIcon(HDC dc, const RECT& rect, COLORREF color);
    void drawChevronIcon(HDC dc, const RECT& rect, COLORREF color);
    void drawSmallToggle(HDC dc, const RECT& rect, bool on);
    void drawCheckbox(HDC dc, const RECT& rect, bool checked);
    void drawText(HDC dc, const wchar_t* text, RECT rect, HFONT font,
                  COLORREF color, UINT format);

    HotTarget hitTest(int x, int y) const;

    App& app_;
    Theme theme_;
    SwitchControl switch_;
    bool showSettings_ = false;
    int width_ = 240;
    int height_ = 300;

    RECT gearRect_{};
    RECT backRect_{};
    RECT iconRect_{};
    RECT titleRect_{};
    RECT switchRect_{};
    RECT stateRect_{};
    RECT hintRect_{};
    RECT launchRowRect_{};
    RECT launchToggleRect_{};
    RECT appearanceLabelRect_{};
    RECT darkRowRect_{};
    RECT darkToggleRect_{};
    RECT followRowRect_{};
    RECT followToggleRect_{};
    RECT languageRowRect_{};
    RECT languageControlRect_{};
    RECT quitRect_{};
    RECT separatorRect_{};
    RECT shortcutOnRect_{};
    RECT shortcutOffRect_{};
    RECT languageLabelRect_{};
    RECT shortcutsLabelRect_{};

    HotTarget hot_ = kHotNone;
    HotTarget pressedTarget_ = kHotNone;
};

} // namespace clipclean
