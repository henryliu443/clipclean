# Clipclean

**English** | [中文](README.zh-CN.md)

A tiny native macOS menu bar utility that strips rich text from the clipboard.

When **Plain Text Mode** is on, every copy is reduced to its plain-text
representation: fonts, colors and other rich flavours are dropped, while
Unicode, emoji, tabs and line breaks are preserved. When it is off, the
clipboard behaves exactly as macOS normally does.

- Menu bar only — no Dock icon, no main window
- Liquid Glass panel with a large slide switch
- Right-click the menu bar icon for Mode ▸ On / Off, Settings and Quit
- Global hot keys: `F6` turns Plain Text Mode on, `F5` turns it off
- Launch at Login
- English / 中文 switch inside Settings
- No network access, no permissions, no clipboard history

## Screenshots

| Main panel | Settings | Right-click menu |
| --- | --- | --- |
| ![Main panel](Screenshots/01-main.png) | ![Settings](Screenshots/02-settings.png) | ![Right-click menu](Screenshots/03-menu.png) |

## Requirements

- macOS 14.0 or later (Apple Silicon or Intel)
- Xcode 26+ to build

On macOS 26 and later the panel uses SwiftUI Liquid Glass (following the
system *Appearance → Liquid Glass* setting); on macOS 14–15 it falls back to a
translucent `.ultraThinMaterial`.

## Install

Download `Clipclean-1.1.1.dmg` from the
[latest release](../../releases/latest), open it, and drag **Clipclean** into
**Applications**.

The build in Releases is signed locally, not notarized, so on first launch
macOS may warn. Right-click the app and choose **Open**, or allow it under
**System Settings → Privacy & Security**.

## Behaviour

**Plain Text Mode: On**

- The pasteboard is watched with a run-loop timer polling `changeCount`
  (macOS has no public pasteboard-changed notification).
- The best available text is taken from `public.utf8-plain-text`, then RTF,
  then HTML.
- The pasteboard is rewritten with that text only.
- Unicode, emoji, tabs and line breaks are preserved.
- The app ignores the change count produced by its own write, so it can never
  loop.
- Image-only pasteboards and file (URL) copies are left untouched.
- A pasteboard that is already plain text is not rewritten.

**Plain Text Mode: Off**

- The timer is stopped entirely. Nothing reads or writes the pasteboard.

## Shortcuts

| Key | Action |
| --- | --- |
| `F6` | Turn Plain Text Mode on |
| `F5` | Turn Plain Text Mode off |
| `Esc` | Leave Settings |

Implemented with the Carbon Hot Key API (`RegisterEventHotKey`), which needs
**no** Accessibility or Input Monitoring permission.

> **Magic Keyboard / laptops:** the top row defaults to media keys, so F5/F6
> alone may not reach the app. Either hold `Fn` (`Fn+F6` / `Fn+F5`), or enable
> *System Settings → Keyboard → Keyboard Shortcuts… → Function Keys → “Use
> F1, F2, etc. keys as standard function keys”.* The 2015 Magic Keyboard
> exposes these two keys directly.

## Building

```sh
open com.clipclean.paste.xcodeproj   # then ⌘R
```

Release build and DMG:

```sh
./scripts/package-dmg.sh             # build/Clipclean-1.1.1.dmg
VERSION=1.2.0 ./scripts/package-dmg.sh
```

## Signing and notarization (public distribution)

The packaged DMG is signed with whatever identity Xcode resolves (or ad-hoc).
For public distribution it must be signed with a **Developer ID Application**
certificate and notarized:

```sh
# 1. Archive + export with a Developer ID identity from Xcode, or sign the
#    built app directly:
codesign --force --options runtime --timestamp \
    --sign "Developer ID Application: Your Name (TEAMID)" "Clipclean.app"

# 2. Store notarization credentials once
xcrun notarytool store-credentials "clipclean-notary" \
    --apple-id "you@example.com" --team-id "TEAMID" \
    --password "app-specific-password"

# 3. Notarize and staple
xcrun notarytool submit build/Clipclean-1.1.1.dmg \
    --keychain-profile "clipclean-notary" --wait
xcrun stapler staple build/Clipclean-1.1.1.dmg
```

## Project layout

```
com.clipclean.paste.xcodeproj
com.clipclean.paste/
  com_clipclean_pasteApp.swift   app entry (menu bar accessory)
  AppDelegate.swift              status item, panel window, hot keys, menu
  ClipboardController.swift      mode state + pasteboard wiring
  PanelModel.swift               SwiftUI bridge (state, language, login item)
  ClipboardPanelView.swift       Liquid Glass panel
  Localization.swift             English / 中文 strings
  HotKeyCenter.swift             Carbon global hot keys
  PlainTextTransformer.swift     pasteboard → plain text
  PasteboardMonitor.swift        change-count observer
Screenshots/                     README screenshots
scripts/package-dmg.sh
```

## Deliberately not included

No clipboard history, search, cloud sync, AI, OCR, or extra settings.
