#!/bin/bash
# Sign Kvikplay with a Developer ID (Hardened Runtime), notarize it with Apple, staple the
# ticket and package a zip for distribution.
#
# Usage: scripts/sign_notarize_macos.sh [path/to/kvikplay.app]
#   CODESIGN_IDENTITY        signing identity (default: first "Developer ID Application")
#   KVIKPLAY_NOTARY_PROFILE  notarytool keychain profile (default: LadaNotary, the shared one)
#
# Build the standalone app first:
#   cmake -B build-release -DCMAKE_BUILD_TYPE=Release && cmake --build build-release --target kvikplay
set -euo pipefail

APP="${1:-build-release/kvikplay.app}"
PROFILE="${KVIKPLAY_NOTARY_PROFILE:-LadaNotary}"
IDENTITY="${CODESIGN_IDENTITY:-$(security find-identity -v -p codesigning | grep -m1 'Developer ID Application:' | sed -E 's/.*"([^"]+)".*/\1/' || true)}"

if [[ ! -d "$APP" ]]; then
    echo "error: $APP not found; build it first" >&2
    exit 1
fi
if [[ -z "$IDENTITY" ]]; then
    echo "error: no 'Developer ID Application' signing identity in the keychain" >&2
    exit 1
fi

VERSION="$(/usr/libexec/PlistBuddy -c 'Print CFBundleShortVersionString' "$APP/Contents/Info.plist")"
OUT_DIR="$(dirname "$APP")"
ZIP="$OUT_DIR/Kvikplay-$VERSION-macOS-arm64.zip"
NOTARIZE_ZIP="$OUT_DIR/Kvikplay-notarize.zip"

sign() {
    codesign --force --timestamp --options runtime --sign "$IDENTITY" "$@"
}

echo "==> Signing with: $IDENTITY"
# Inside out: every loose Mach-O (dylibs, plugins), then each framework bundle, then the app.
while IFS= read -r -d '' f; do
    if file -b "$f" | grep -q "Mach-O"; then
        sign "$f"
    fi
done < <(find "$APP/Contents/Frameworks" "$APP/Contents/PlugIns" -type f ! -path "*.framework/*" -print0)
for fw in "$APP"/Contents/Frameworks/*.framework; do
    [[ -d "$fw" ]] && sign "$fw"
done
sign "$APP"

echo "==> Verifying signature"
codesign --verify --deep --strict --verbose=2 "$APP"

echo "==> Notarizing (profile: $PROFILE)"
rm -f "$NOTARIZE_ZIP"
ditto -c -k --keepParent "$APP" "$NOTARIZE_ZIP"
xcrun notarytool submit "$NOTARIZE_ZIP" --keychain-profile "$PROFILE" --wait
rm -f "$NOTARIZE_ZIP"

echo "==> Stapling"
xcrun stapler staple "$APP"
xcrun stapler validate "$APP"

echo "==> Gatekeeper check"
spctl --assess --type execute --verbose=4 "$APP"

rm -f "$ZIP"
ditto -c -k --keepParent "$APP" "$ZIP"
echo "==> Done: $APP (stapled) and $ZIP"
