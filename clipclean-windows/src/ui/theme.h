#pragma once

#include <windows.h>

namespace clipclean {

/// Colours and fonts for one light/dark appearance.
struct Theme {
    bool dark = false;
    /// Dots per inch the metrics below were scaled for (96 = 100%).
    int dpi = 96;
    COLORREF panelTint = RGB(250, 250, 250);
    COLORREF primaryText = RGB(20, 20, 20);
    COLORREF secondaryText = RGB(110, 110, 110);
    COLORREF accent = RGB(10, 132, 255);
    COLORREF trackOff = RGB(210, 210, 210);
    COLORREF knob = RGB(255, 255, 255);
    COLORREF separator = RGB(220, 220, 220);
    COLORREF controlTint = RGB(235, 235, 235);
    int cornerRadius = 14;
    HFONT titleFont = nullptr;
    HFONT bodyFont = nullptr;
    HFONT captionFont = nullptr;
    HFONT largeFont = nullptr;

    /// Scales a 96-DPI ("logical") metric to the current display DPI. Every
    /// fixed size in the UI goes through this so the layout keeps the same
    /// physical size on high-DPI displays.
    int px(int value) const { return MulDiv(value, dpi, 96); }
};

Theme makeTheme(bool dark, int dpi);

void destroyTheme(Theme& theme);

/// Linear interpolation between two colours, `t` in [0, 1].
COLORREF blendColor(COLORREF from, COLORREF to, float t);

} // namespace clipclean
