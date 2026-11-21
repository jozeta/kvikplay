#!/bin/bash
set -e

APP_PATH="${1:-build/kvikplay.app}"
CUSTOM_MACDEPLOYQT="$2"

echo "[Standalone Deploy] Packaging standalone macOS app: $APP_PATH"

if [ ! -d "$APP_PATH" ]; then
    echo "[Standalone Deploy] Error: Bundle directory not found at $APP_PATH"
    exit 1
fi

# Locate macdeployqt
MACDEPLOYQT_BIN=""
if [ -n "$CUSTOM_MACDEPLOYQT" ] && [ -x "$CUSTOM_MACDEPLOYQT" ]; then
    MACDEPLOYQT_BIN="$CUSTOM_MACDEPLOYQT"
elif command -v macdeployqt >/dev/null 2>&1; then
    MACDEPLOYQT_BIN="$(command -v macdeployqt)"
elif [ -x "/opt/homebrew/opt/qtbase/bin/macdeployqt" ]; then
    MACDEPLOYQT_BIN="/opt/homebrew/opt/qtbase/bin/macdeployqt"
elif [ -x "/opt/homebrew/bin/macdeployqt" ]; then
    MACDEPLOYQT_BIN="/opt/homebrew/bin/macdeployqt"
elif [ -x "/usr/local/opt/qtbase/bin/macdeployqt" ]; then
    MACDEPLOYQT_BIN="/usr/local/opt/qtbase/bin/macdeployqt"
elif [ -x "/usr/local/bin/macdeployqt" ]; then
    MACDEPLOYQT_BIN="/usr/local/bin/macdeployqt"
fi

if [ -z "$MACDEPLOYQT_BIN" ]; then
    echo "[Standalone Deploy] Error: macdeployqt tool not found in PATH or standard Qt locations."
    exit 1
fi

echo "[Standalone Deploy] Using macdeployqt: $MACDEPLOYQT_BIN"
"$MACDEPLOYQT_BIN" "$APP_PATH" -no-strip

# Fix any dylib install names that need manual normalization
if [ -f "$APP_PATH/Contents/Frameworks/libbrotlicommon.1.dylib" ]; then
    install_name_tool -id @rpath/libbrotlicommon.1.dylib "$APP_PATH/Contents/Frameworks/libbrotlicommon.1.dylib" 2>/dev/null || true
fi
if [ -f "$APP_PATH/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus" ]; then
    install_name_tool -id @rpath/QtDBus.framework/Versions/A/QtDBus "$APP_PATH/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus" 2>/dev/null || true
fi

# Remove library search paths that point outside the bundle (e.g. /opt/homebrew/lib):
# a standalone app must only load its own copies.
while IFS= read -r -d '' f; do
    if file -b "$f" | grep -q "Mach-O"; then
        otool -l "$f" | awk '/LC_RPATH/{getline; getline; print $2}' | while read -r rp; do
            case "$rp" in
                @*) ;; # @executable_path / @loader_path / @rpath: inside the bundle
                *) install_name_tool -delete_rpath "$rp" "$f" 2>/dev/null || true ;;
            esac
        done
    fi
done < <(find "$APP_PATH" -type f -print0)

# Ad-hoc code signing for the bundled frameworks and binary
echo "[Standalone Deploy] Signing bundle with ad-hoc signature..."
codesign --force --deep -s - "$APP_PATH"

echo "[Standalone Deploy] Standalone bundle ready at: $APP_PATH"
