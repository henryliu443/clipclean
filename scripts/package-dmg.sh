#!/bin/bash
#
# Builds a Release .app and packages it into a DMG.
#
# Output: build/Clipclean-<VERSION>.dmg
#
# Environment:
#   VERSION             Version string used in the DMG name (default: 1.0.0).
#   CODESIGN_IDENTITY   "Developer ID Application: ..." to sign for distribution.
#   DERIVED_DATA_PATH   Override the build directory.
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PROJECT="com.clipclean.paste.xcodeproj"
SCHEME="com.clipclean.paste"
CONFIG="Release"
APP_NAME="Clipclean"
VERSION="${VERSION:-1.0.0}"

# Build outside the project folder: this repo lives under ~/Documents, which is
# managed by a file provider that adds xattrs (com.apple.FinderInfo) and makes
# codesign fail with "resource fork, Finder information ... not allowed".
DERIVED="${DERIVED_DATA_PATH:-$HOME/Library/Developer/Xcode/DerivedData/ClipcleanRelease}"
BUILD_DIR="$ROOT/build"
APP_PATH="$DERIVED/Build/Products/$CONFIG/$APP_NAME.app"
DMG_PATH="$BUILD_DIR/$APP_NAME-$VERSION.dmg"
STAGE="$(mktemp -d)/dmg-stage"

echo "==> Clipclean $VERSION"

echo "==> Building $CONFIG"
xcodebuild \
    -project "$PROJECT" \
    -scheme "$SCHEME" \
    -configuration "$CONFIG" \
    -derivedDataPath "$DERIVED" \
    build

if [[ ! -d "$APP_PATH" ]]; then
    echo "error: expected app at $APP_PATH" >&2
    exit 1
fi

echo "==> Stripping extended attributes"
xattr -cr "$APP_PATH"

if [[ -n "${CODESIGN_IDENTITY:-}" ]]; then
    echo "==> Re-signing with: $CODESIGN_IDENTITY"
    codesign --force --options runtime --timestamp \
        --sign "$CODESIGN_IDENTITY" "$APP_PATH"
    codesign --verify --verbose=2 "$APP_PATH"
fi

echo "==> Staging DMG contents"
mkdir -p "$BUILD_DIR"
mkdir -p "$STAGE"
cp -R "$APP_PATH" "$STAGE/"
xattr -cr "$STAGE/$APP_NAME.app"
ln -s /Applications "$STAGE/Applications"

echo "==> Creating DMG"
rm -f "$DMG_PATH"
hdiutil create \
    -volname "$APP_NAME" \
    -srcfolder "$STAGE" \
    -ov -format UDZO \
    "$DMG_PATH" >/dev/null

rm -rf "$(dirname "$STAGE")"

echo ""
echo "Done: $DMG_PATH"
