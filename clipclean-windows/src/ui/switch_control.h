#pragma once

#include <windows.h>

#include "ui/theme.h"

namespace clipclean {

/// The large slide switch from the macOS panel. Owns only its animation state;
/// the caller provides the track rectangle and drives `tick` from a timer.
class SwitchControl {
public:
    void setOn(bool on, bool animate);
    bool isAnimating() const { return animating_; }
    void tick();

    void draw(HDC dc, const RECT& track, const Theme& theme) const;

    static int height() { return 64; }
    static int width() { return 132; }

private:
    float position_ = 0.0f; ///< 0 = off, 1 = on
    float target_ = 0.0f;
    bool animating_ = false;
};

} // namespace clipclean
