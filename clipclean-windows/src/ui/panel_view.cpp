#include "ui/panel_view.h"

#include <cmath>

#include "app.h"
#include "core/strings.h"
#include "win32/win_util.h"

namespace clipclean {
namespace {

constexpr int kPad = 16;

void fillRoundRect(HDC dc, const RECT& rect, int radius, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_NULL, 0, color);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius * 2,
              radius * 2);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void strokeRoundRect(HDC dc, const RECT& rect, int radius, COLORREF color,
                     int thickness) {
    HPEN pen = CreatePen(PS_SOLID, thickness, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius * 2,
              radius * 2);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void fillEllipse(HDC dc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_NULL, 0, color);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    Ellipse(dc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

} // namespace

PanelView::PanelView(App& app) : app_(app) {
    theme_ = makeTheme(false, systemDpi());
}

PanelView::~PanelView() {
    destroyTheme(theme_);
}

void PanelView::setDark(bool dark) {
    if (theme_.dark == dark) return;
    destroyTheme(theme_);
    theme_ = makeTheme(dark, theme_.dpi);
}

void PanelView::setDpi(int dpi) {
    if (dpi <= 0 || dpi == theme_.dpi) return;
    const bool dark = theme_.dark;
    destroyTheme(theme_);
    theme_ = makeTheme(dark, dpi);
}

void PanelView::syncSwitch(bool animate) {
    switch_.setOn(app_.plainTextModeEnabled(), animate);
}

void PanelView::setShowSettings(bool show) {
    if (showSettings_ == show) return;
    showSettings_ = show;
    switch_.setOn(app_.plainTextModeEnabled(), false);
    if (onShowSettingsChanged) onShowSettingsChanged(show);
}

void PanelView::paint(HDC dc, int width, int height) {
    width_ = width;
    height_ = height;
    layout(width, height);

    SetBkMode(dc, TRANSPARENT);

    const RECT client{0, 0, width, height};
    HBRUSH background = CreateSolidBrush(theme_.panelTint);
    FillRect(dc, &client, background);
    DeleteObject(background);

    if (showSettings_) {
        drawSettings(dc);
    } else {
        drawMain(dc);
    }
}

void PanelView::layout(int width, int height) {
    const int centerX = width / 2;
    const auto S = [this](int value) { return theme_.px(value); };
    const int pad = S(kPad);

    if (!showSettings_) {
        gearRect_ = RECT{width - pad - S(28), S(14), width - pad, S(42)};
        iconRect_ = RECT{centerX - S(22), S(54), centerX + S(22), S(98)};
        titleRect_ = RECT{pad, S(108), width - pad, S(132)};
        switchRect_ = RECT{centerX - S(SwitchControl::width()) / 2, S(138),
                           centerX + S(SwitchControl::width()) / 2, S(202)};
        stateRect_ = RECT{pad, S(210), width - pad, S(232)};
        hintRect_ = RECT{pad, S(238), width - pad, S(258)};
    } else {
        backRect_ = RECT{pad, S(14), pad + S(28), S(42)};
        titleRect_ = RECT{pad + S(38), S(14), width - pad, S(42)};

        const int toggleLeft = width - pad - S(46);
        int y = S(58);

        launchRowRect_ = RECT{pad, y, width - pad, y + S(26)};
        launchToggleRect_ = RECT{toggleLeft, y + S(1), width - pad, y + S(25)};
        y += S(40);

        appearanceLabelRect_ = RECT{pad, y, width - pad, y + S(18)};
        y += S(20);

        darkRowRect_ = RECT{pad, y, width - pad, y + S(26)};
        darkToggleRect_ = RECT{toggleLeft, y + S(1), width - pad, y + S(25)};
        y += S(32);

        if (supportsDarkMode()) {
            followRowRect_ = RECT{pad, y, width - pad, y + S(26)};
            followToggleRect_ = RECT{toggleLeft, y + S(1), width - pad, y + S(25)};
            y += S(32);
        } else {
            followRowRect_ = RECT{};
            followToggleRect_ = RECT{};
        }
        y += S(4);

        shortcutsLabelRect_ = RECT{pad, y, width - pad, y + S(18)};
        y += S(20);
        shortcutOnRect_ = RECT{pad, y, width - pad, y + S(22)};
        y += S(22);
        shortcutOffRect_ = RECT{pad, y, width - pad, y + S(22)};
        y += S(26);

        languageLabelRect_ = RECT{pad, y, width - pad, y + S(18)};
        y += S(20);
        languageControlRect_ = RECT{pad, y, width - pad, y + S(32)};
        y += S(14);

        separatorRect_ = RECT{pad, y, width - pad, y + S(1)};
        quitRect_ = RECT{pad, height - pad - S(34), width - pad, height - pad};
    }
}

void PanelView::drawMain(HDC dc) {
    const Strings& strings = app_.strings();
    const bool enabled = app_.plainTextModeEnabled();

    fillEllipse(dc, gearRect_, theme_.controlTint);
    drawGearIcon(dc, gearRect_, theme_.primaryText);

    drawDocumentIcon(dc, iconRect_,
                     enabled ? theme_.accent : theme_.secondaryText);

    drawText(dc, strings.plainTextMode, titleRect_, theme_.titleFont,
             theme_.primaryText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    switch_.draw(dc, switchRect_, theme_);

    drawText(dc, enabled ? strings.on : strings.off, stateRect_, theme_.bodyFont,
             enabled ? theme_.accent : theme_.secondaryText,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    drawText(dc, strings.hotKeyHint, hintRect_, theme_.captionFont,
             theme_.secondaryText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void PanelView::drawSettings(HDC dc) {
    const Strings& strings = app_.strings();
    const auto S = [this](int value) { return theme_.px(value); };

    fillEllipse(dc, backRect_, theme_.controlTint);
    drawChevronIcon(dc, backRect_, theme_.primaryText);

    drawText(dc, strings.settings, titleRect_, theme_.titleFont,
             theme_.primaryText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    drawText(dc, strings.launchAtLogin, launchRowRect_, theme_.bodyFont,
             theme_.primaryText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    drawSmallToggle(dc, launchToggleRect_, app_.launchAtLogin());

    drawText(dc, strings.appearance, appearanceLabelRect_, theme_.bodyFont,
             theme_.secondaryText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    drawText(dc, strings.darkMode, darkRowRect_, theme_.bodyFont,
             theme_.primaryText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    drawSmallToggle(dc, darkToggleRect_, app_.effectiveDark());

    if (supportsDarkMode()) {
        drawText(dc, strings.followSystem, followRowRect_, theme_.bodyFont,
                 theme_.primaryText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        drawCheckbox(dc, followToggleRect_, app_.followSystemTheme());
    }

    drawText(dc, strings.shortcuts, shortcutsLabelRect_, theme_.bodyFont,
             theme_.secondaryText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    auto drawShortcut = [&](const RECT& row, const wchar_t* key,
                            const wchar_t* description) {
        RECT badge{row.left, row.top + S(1), row.left + S(34), row.bottom - S(1)};
        fillRoundRect(dc, badge, S(6), theme_.controlTint);
        drawText(dc, key, badge, theme_.captionFont, theme_.primaryText,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT label = row;
        label.left = badge.right + S(8);
        drawText(dc, description, label, theme_.captionFont,
                 theme_.secondaryText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    };
    drawShortcut(shortcutOnRect_, L"F6", strings.turnOn);
    drawShortcut(shortcutOffRect_, L"F5", strings.turnOff);

    drawText(dc, strings.languageLabel, languageLabelRect_, theme_.bodyFont,
             theme_.secondaryText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    const bool chinese = app_.language() == Language::Chinese;
    fillRoundRect(dc, languageControlRect_, S(8), theme_.controlTint);
    RECT englishRect = languageControlRect_;
    RECT chineseRect = languageControlRect_;
    const int midX = languageControlRect_.left +
                     (languageControlRect_.right - languageControlRect_.left) / 2;
    englishRect.right = midX;
    chineseRect.left = midX;

    // iOS-style segmented control: the selected side is an inset rounded pill
    // floating on the track. Because the pill stops short of the middle, the
    // two halves are never joined by a straight seam (which read as a line).
    RECT pill = chinese ? chineseRect : englishRect;
    const int inset = S(2);
    pill.left += inset;
    pill.top += inset;
    pill.right -= inset;
    pill.bottom -= inset;
    fillRoundRect(dc, pill, S(6), theme_.accent);
    drawText(dc, languageDisplayName(Language::English), englishRect,
             theme_.bodyFont, chinese ? theme_.primaryText : RGB(255, 255, 255),
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    drawText(dc, languageDisplayName(Language::Chinese), chineseRect,
             theme_.bodyFont, chinese ? RGB(255, 255, 255) : theme_.primaryText,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HBRUSH separator = CreateSolidBrush(theme_.separator);
    FillRect(dc, &separatorRect_, separator);
    DeleteObject(separator);

    fillRoundRect(dc, quitRect_, S(8), theme_.controlTint);
    drawText(dc, strings.quit, quitRect_, theme_.bodyFont, theme_.primaryText,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void PanelView::drawDocumentIcon(HDC dc, const RECT& rect, COLORREF color) {
    const auto S = [this](int value) { return theme_.px(value); };
    RECT frame{rect.left + S(6), rect.top, rect.right - S(6), rect.bottom};
    strokeRoundRect(dc, frame, S(4), color, S(2));

    HPEN pen = CreatePen(PS_SOLID, S(2), color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    for (int i = 0; i < 3; ++i) {
        const int y = frame.top + S(10) + i * S(8);
        MoveToEx(dc, frame.left + S(7), y, nullptr);
        LineTo(dc, frame.right - S(7), y);
    }
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void PanelView::drawGearIcon(HDC dc, const RECT& rect, COLORREF color) {
    const auto S = [this](int value) { return theme_.px(value); };
    const int centerX = (rect.left + rect.right) / 2;
    const int centerY = (rect.top + rect.bottom) / 2;
    const int radius = (rect.right - rect.left) / 2 - S(7);

    HPEN pen = CreatePen(PS_SOLID, S(2), color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));

    Ellipse(dc, centerX - radius, centerY - radius, centerX + radius,
            centerY + radius);

    const int inner = radius - S(5);
    for (int i = 0; i < 8; ++i) {
        const double angle = i * 3.14159265 / 4.0;
        const int x1 = centerX + static_cast<int>(std::cos(angle) * inner);
        const int y1 = centerY + static_cast<int>(std::sin(angle) * inner);
        const int x2 = centerX + static_cast<int>(std::cos(angle) * (radius + S(4)));
        const int y2 = centerY + static_cast<int>(std::sin(angle) * (radius + S(4)));
        MoveToEx(dc, x1, y1, nullptr);
        LineTo(dc, x2, y2);
    }

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void PanelView::drawChevronIcon(HDC dc, const RECT& rect, COLORREF color) {
    const auto S = [this](int value) { return theme_.px(value); };
    const int centerX = (rect.left + rect.right) / 2;
    const int centerY = (rect.top + rect.bottom) / 2;
    HPEN pen = CreatePen(PS_SOLID, S(2), color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    MoveToEx(dc, centerX + S(3), centerY - S(6), nullptr);
    LineTo(dc, centerX - S(3), centerY);
    LineTo(dc, centerX + S(3), centerY + S(6));
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void PanelView::drawSmallToggle(HDC dc, const RECT& rect, bool on) {
    const auto S = [this](int value) { return theme_.px(value); };
    const int height = rect.bottom - rect.top;
    fillRoundRect(dc, rect, height / 2, on ? theme_.accent : theme_.trackOff);

    const int knob = height - S(4);
    RECT knobRect{};
    knobRect.top = rect.top + S(2);
    knobRect.bottom = rect.top + S(2) + knob;
    if (on) {
        knobRect.right = rect.right - S(2);
        knobRect.left = knobRect.right - knob;
    } else {
        knobRect.left = rect.left + S(2);
        knobRect.right = knobRect.left + knob;
    }
    fillEllipse(dc, knobRect, theme_.knob);
}

void PanelView::drawCheckbox(HDC dc, const RECT& rect, bool checked) {
    const auto S = [this](int value) { return theme_.px(value); };
    const int size = S(20);
    RECT box{};
    box.right = rect.right;
    box.left = box.right - size;
    box.top = (rect.top + rect.bottom) / 2 - size / 2;
    box.bottom = box.top + size;

    if (checked) {
        fillRoundRect(dc, box, S(5), theme_.accent);
        HPEN pen = CreatePen(PS_SOLID, S(2), RGB(255, 255, 255));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        MoveToEx(dc, box.left + S(5), box.top + S(10), nullptr);
        LineTo(dc, box.left + S(8), box.top + S(14));
        LineTo(dc, box.left + S(15), box.top + S(6));
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    } else {
        strokeRoundRect(dc, box, S(5), theme_.secondaryText, S(2));
    }
}

void PanelView::drawText(HDC dc, const wchar_t* text, RECT rect, HFONT font,
                         COLORREF color, UINT format) {
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetTextColor(dc, color);
    DrawTextW(dc, text, -1, &rect, format);
    SelectObject(dc, oldFont);
}

PanelView::HotTarget PanelView::hitTest(int x, int y) const {
    auto inside = [x, y](const RECT& rect) {
        return x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom;
    };

    if (!showSettings_) {
        if (inside(gearRect_)) return kHotGear;
        if (inside(switchRect_)) return kHotSwitch;
        return kHotNone;
    }

    if (inside(backRect_)) return kHotBack;
    if (inside(launchToggleRect_) || inside(launchRowRect_)) return kHotLaunch;
    if (inside(darkToggleRect_) || inside(darkRowRect_)) return kHotDarkMode;
    if (supportsDarkMode() &&
        (inside(followToggleRect_) || inside(followRowRect_))) {
        return kHotFollowSystem;
    }
    if (inside(languageControlRect_)) return kHotLanguage;
    if (inside(quitRect_)) return kHotQuit;
    return kHotNone;
}

bool PanelView::onMouseMove(int x, int y) {
    const HotTarget target = (x < 0 || y < 0) ? kHotNone : hitTest(x, y);
    if (target == hot_) return false;
    hot_ = target;
    SetCursor(LoadCursorW(nullptr, target == kHotNone ? IDC_ARROW : IDC_HAND));
    return true;
}

bool PanelView::onMouseDown(int x, int y) {
    pressedTarget_ = hitTest(x, y);
    return false;
}

bool PanelView::onMouseUp(int x, int y) {
    const HotTarget released = hitTest(x, y);
    const HotTarget target = pressedTarget_;
    pressedTarget_ = kHotNone;
    if (target == kHotNone || target != released) return false;

    switch (target) {
        case kHotGear:
            setShowSettings(true);
            break;
        case kHotBack:
            setShowSettings(false);
            break;
        case kHotSwitch: {
            const bool enabled = !app_.plainTextModeEnabled();
            app_.setPlainTextModeEnabled(enabled);
            switch_.setOn(enabled, true);
            break;
        }
        case kHotLaunch:
            app_.setLaunchAtLogin(!app_.launchAtLogin());
            break;
        case kHotDarkMode:
            app_.setDarkTheme(!app_.effectiveDark());
            break;
        case kHotFollowSystem:
            app_.setFollowSystemTheme(!app_.followSystemTheme());
            break;
        case kHotLanguage: {
            const int midX = languageControlRect_.left +
                             (languageControlRect_.right - languageControlRect_.left) / 2;
            app_.setLanguage(x < midX ? Language::English : Language::Chinese);
            break;
        }
        case kHotQuit:
            app_.quit();
            break;
        case kHotNone:
            break;
    }
    return true;
}

bool PanelView::onKeyDown(WPARAM key) {
    if (key != VK_ESCAPE) return false;
    if (showSettings_) {
        setShowSettings(false);
    } else {
        return false;
    }
    return true;
}

void PanelView::tick() {
    switch_.tick();
}

} // namespace clipclean
