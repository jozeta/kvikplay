#!/bin/bash
set -e

APP_PATH="${1:-build/kvikplay.app}"
echo "Packaging standalone macOS app: $APP_PATH"

macdeployqt "$APP_PATH" -no-strip
if [ -f "$APP_PATH/Contents/Frameworks/libbrotlicommon.1.dylib" ]; then
    install_name_tool -id @rpath/libbrotlicommon.1.dylib "$APP_PATH/Contents/Frameworks/libbrotlicommon.1.dylib" 2>/dev/null || true
fi
if [ -f "$APP_PATH/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus" ]; then
    install_name_tool -id @rpath/QtDBus.framework/Versions/A/QtDBus "$APP_PATH/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus" 2>/dev/null || true
fi
codesign --force --deep -s - "$APP_PATH"
echo "Done! Application bundle is 100% standalone and portable."
