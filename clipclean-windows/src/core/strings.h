#pragma once

#include "core/settings.h"

namespace clipclean {

/// All user-facing strings. Kept in one place so the language switch can swap
/// the whole interface at once. Mirrors Localization.swift.
struct Strings {
    Language language;
    const wchar_t* appName;
    const wchar_t* mode;
    const wchar_t* on;
    const wchar_t* off;
    const wchar_t* plainTextMode;
    const wchar_t* settings;
    const wchar_t* settingsEllipsis;
    const wchar_t* launchAtLogin;
    const wchar_t* appearance;
    const wchar_t* darkMode;
    const wchar_t* followSystem;
    const wchar_t* shortcuts;
    const wchar_t* languageLabel;
    const wchar_t* turnOn;
    const wchar_t* turnOff;
    const wchar_t* quit;
    const wchar_t* hotKeyHint;
    const wchar_t* firstRunTitle;
    const wchar_t* firstRunMessage;
};

const Strings& stringsFor(Language language);

const wchar_t* languageDisplayName(Language language);

} // namespace clipclean
