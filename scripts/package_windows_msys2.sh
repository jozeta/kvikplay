#!/bin/bash
# Package a standalone Windows build made with MSYS2 (UCRT64): kvikplay.exe, Qt (via
# windeployqt6), every MSYS2 DLL the app and its Qt plugins load, the licence texts of the
# MSYS2 packages they come from, and a THIRD_PARTY_NOTICES.txt listing them.
#
# Usage (in an MSYS2 UCRT64 shell): scripts/package_windows_msys2.sh <build-dir> <version>
# Output: <build-dir>/Kvikplay-<version>-Windows-x64.zip containing a Kvikplay/ folder.
set -euo pipefail

BUILD="$1"
VERSION="$2"
SRC="$(cd "$(dirname "$0")/.." && pwd)"
DIST="$BUILD/dist/Kvikplay"
ZIP="$BUILD/Kvikplay-$VERSION-Windows-x64.zip"

rm -rf "$BUILD/dist" "$ZIP"
mkdir -p "$DIST"
cp "$BUILD/kvikplay.exe" "$DIST/"

# Qt libraries and plugins (platform, styles, image formats incl. SVG, TLS for updates).
windeployqt6 --no-translations --no-system-d3d-compiler --no-opengl-sw "$DIST/kvikplay.exe"

# Every DLL from the MSYS2 prefix that the exe or any copied DLL/plugin needs (transitively).
copy_deps() {
    ldd "$1" | awk '{print $3}' | grep -i '^/ucrt64/' || true
}
changed=1
while [ "$changed" = 1 ]; do
    changed=0
    while IFS= read -r -d '' bin; do
        for dep in $(copy_deps "$bin"); do
            name="$(basename "$dep")"
            if [ ! -f "$DIST/$name" ]; then
                cp "$dep" "$DIST/"
                changed=1
            fi
        done
    done < <(find "$DIST" \( -name '*.exe' -o -name '*.dll' \) -print0)
done

# Licences: the MSYS2 package of each bundled DLL, its version and licence files.
mkdir -p "$DIST/licenses"
notices="$DIST/THIRD_PARTY_NOTICES.txt"
{
    echo "Third-party software in the Kvikplay Windows build"
    echo
    echo "Kvikplay's own source code is MIT licensed (LICENSE.txt). This build bundles the"
    echo "MSYS2 packages below; some are GPL licensed (FFmpeg, mpv, x264, x265), so the"
    echo "distributed app as a whole is covered by the GNU GPL version 3. Licence texts are in"
    echo "licenses/<package>/; sources: https://github.com/msys2/MINGW-packages and each"
    echo "project's upstream."
    echo
} > "$notices"
pkgs=$(find "$DIST" -name '*.dll' -print0 | xargs -0 -I{} basename {} | sort -u | while read -r dll; do
    if [ -f "/ucrt64/bin/$dll" ]; then pacman -Qoq "/ucrt64/bin/$dll" 2>/dev/null || true; fi
done | sort -u)
for pkg in $pkgs; do
    pacman -Q "$pkg" >> "$notices"
    lic="/ucrt64/share/licenses/${pkg#mingw-w64-ucrt-x86_64-}"
    if [ -d "$lic" ]; then cp -r "$lic" "$DIST/licenses/"; fi
done
cp "$SRC/LICENSE" "$DIST/LICENSE.txt"

(cd "$BUILD/dist" && zip -qr "../$(basename "$ZIP")" Kvikplay)
echo "Packaged $ZIP ($(du -h "$ZIP" | cut -f1))"
