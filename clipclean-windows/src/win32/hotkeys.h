#pragma once

#include <windows.h>

namespace clipclean {

/// Registers a system-wide hot key. `modifiers` uses the `MOD_*` flags.
bool registerHotKey(HWND owner, int id, UINT modifiers, UINT virtualKey);

void unregisterHotKey(HWND owner, int id);

} // namespace clipclean
