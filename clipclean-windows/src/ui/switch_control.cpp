#include "ui/switch_control.h"

#include <cmath>

namespace clipclean {

void SwitchControl::setOn(bool on, bool animate) {
    target_ = on ? 1.0f : 0.0f;
    animating_ = animate;
    if (!animate) position_ = target_;
}

void SwitchControl::tick() {
    const float delta = target_ - position_;
    if (std::fabs(delta) < 0.01f) {
        position_ = target_;
        animating_ = false;
        return;
    }
    position_ += delta * 0.28f;
}

void SwitchControl::draw(HDC dc, const RECT& track, const Theme& theme) const {
    const int width = track.right - track.left;
    const int height = track.bottom - track.top;
    const int inset = theme.px(5);
    const int knobSize = height - inset * 2;

    const COLORREF trackColor =
        blendColor(theme.trackOff, theme.accent, position_);

    HBRUSH trackBrush = CreateSolidBrush(trackColor);
    HPEN trackPen = CreatePen(PS_NULL, 0, trackColor);
    HGDIOBJ oldBrush = SelectObject(dc, trackBrush);
    HGDIOBJ oldPen = SelectObject(dc, trackPen);
    RoundRect(dc, track.left, track.top, track.right, track.bottom,
              height, height);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(trackPen);
    DeleteObject(trackBrush);

    const int travel = width - knobSize - inset * 2;
    const int knobLeft = track.left + inset + static_cast<int>(travel * position_);

    HBRUSH knobBrush = CreateSolidBrush(theme.knob);
    HPEN knobPen = CreatePen(PS_NULL, 0, theme.knob);
    oldBrush = SelectObject(dc, knobBrush);
    oldPen = SelectObject(dc, knobPen);
    Ellipse(dc, knobLeft, track.top + inset, knobLeft + knobSize,
            track.top + inset + knobSize);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(knobPen);
    DeleteObject(knobBrush);
}

} // namespace clipclean
