#pragma once

namespace clipclean {

/// Whether the app is registered to start with the current user's session.
bool isAutostartEnabled();

/// Adds or removes the `HKCU\...\CurrentVersion\Run` entry for this executable.
void setAutostartEnabled(bool enabled);

} // namespace clipclean
