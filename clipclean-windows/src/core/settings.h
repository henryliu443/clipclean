#pragma once

namespace clipclean {

enum class Language {
    English,
    Chinese,
};

/// Persisted user preferences. This is a plain data model: the platform layer
/// is responsible for loading and saving it.
struct Settings {
    bool plainTextModeEnabled = false;
    Language language = Language::English;
    bool launchAtLogin = false;

    /// When true (Windows 10/11 only) the panel follows the system appearance.
    bool followSystemTheme = true;

    /// Manual appearance used when not following the system.
    bool darkTheme = false;

    /// Transient: true on the very first launch (no settings key yet). Used to
    /// show the one-time tray balloon. Never persisted.
    bool firstRun = false;
};

} // namespace clipclean
