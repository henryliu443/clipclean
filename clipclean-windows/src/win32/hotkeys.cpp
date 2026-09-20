#include "win32/hotkeys.h"

namespace clipclean {

bool registerHotKey(HWND owner, int id, UINT modifiers, UINT virtualKey) {
    return RegisterHotKey(owner, id, modifiers, virtualKey) != FALSE;
}

void unregisterHotKey(HWND owner, int id) {
    UnregisterHotKey(owner, id);
}

} // namespace clipclean
