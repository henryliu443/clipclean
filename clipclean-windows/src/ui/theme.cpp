#include "ui/theme.h"

namespace clipclean {
namespace {

HFONT createFont(int height, int weight, const wchar_t* face) {
    LOGFONTW logFont{};
    logFont.lfHeight = height;
    logFont.lfWeight = weight;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfQuality = CLEARTYPE_QUALITY;
    lstrcpynW(logFont.lfFaceName, face, LF_FACESIZE);
    return CreateFontIndirectW(&logFont);
}

} // namespace

Theme makeTheme(bool dark, int dpi) {
    Theme theme;
    theme.dark = dark;
    theme.dpi = dpi > 0 ? dpi : 96;
    theme.panelTint = dark ? RGB(30, 30, 30) : RGB(250, 250, 250);
    theme.primaryText = dark ? RGB(245, 245, 245) : RGB(20, 20, 20);
    theme.secondaryText = dark ? RGB(155, 155, 155) : RGB(115, 115, 115);
    theme.accent = RGB(10, 132, 255);
    theme.trackOff = dark ? RGB(72, 72, 72) : RGB(208, 208, 208);
    theme.knob = RGB(255, 255, 255);
    theme.separator = dark ? RGB(58, 58, 58) : RGB(222, 222, 222);
    theme.controlTint = dark ? RGB(48, 48, 48) : RGB(235, 235, 235);

    theme.cornerRadius = theme.px(14);

    // Font heights are negative to mean "character height" and are physical
    // pixels, so scale them too.
    theme.titleFont = createFont(-theme.px(17), 600, L"Segoe UI");
    theme.bodyFont = createFont(-theme.px(14), 400, L"Segoe UI");
    theme.captionFont = createFont(-theme.px(12), 400, L"Segoe UI");
    theme.largeFont = createFont(-theme.px(30), 400, L"Segoe UI Symbol");
    return theme;
}

COLORREF blendColor(COLORREF from, COLORREF to, float t) {
    if (t <= 0.0f) return from;
    if (t >= 1.0f) return to;
    const auto lerp = [t](int a, int b) {
        return static_cast<int>(a + (b - a) * t + 0.5f);
    };
    return RGB(lerp(GetRValue(from), GetRValue(to)),
               lerp(GetGValue(from), GetGValue(to)),
               lerp(GetBValue(from), GetBValue(to)));
}

void destroyTheme(Theme& theme) {
    HFONT* fonts[] = {&theme.titleFont, &theme.bodyFont, &theme.captionFont,
                      &theme.largeFont};
    for (HFONT* font : fonts) {
        if (*font) {
            DeleteObject(*font);
            *font = nullptr;
        }
    }
}

} // namespace clipclean
