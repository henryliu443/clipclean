#pragma once

#include "core/settings.h"

namespace clipclean {

/// Loads preferences from `HKEY_CURRENT_USER\Software\Clipclean`.
Settings loadSettings();

/// Persists preferences. `launchAtLogin` is owned by the autostart module and
/// is not written here.
void saveSettings(const Settings& settings);

} // namespace clipclean
