#!/usr/bin/env python3
"""Third-party notices for a standalone Kvikplay.app built from Homebrew libraries.

For every library macdeployqt copied into Contents/Frameworks, find the Homebrew formula
it came from, then:
  * write THIRD_PARTY_NOTICES.md (formula, version, licence, upstream source), and
  * copy each formula's licence files into Contents/Resources/licenses/<formula>/.

The released app bundles GPL components (FFmpeg, mpv, x264, x265), so the licence texts
and the source locations must ship with it.

Usage: scripts/third_party_licenses.py <path/to/kvikplay.app> [notices.md]
"""
import glob
import json
import os
import shutil
import subprocess
import sys

CELLAR = "/opt/homebrew/Cellar"
LICENSE_PATTERNS = ("COPYING*", "LICENSE*", "LICENCE*", "NOTICE*", "COPYRIGHT*", "*.LICENSE")


def formula_of(lib_name):
    for sub in ("lib", "Frameworks"):
        hits = sorted(glob.glob(os.path.join(CELLAR, "*", "*", sub, lib_name)))
        if hits:
            parts = hits[0].split(os.sep)
            return parts[4], parts[5]  # /opt/homebrew/Cellar/<formula>/<version>/...
    return None, None


# Formulae whose Homebrew install has no licence file: the standard licence texts they
# use, taken from FFmpeg's install (which ships the GNU texts).
FALLBACK_TEXTS = {
    "glib": ("COPYING.LGPLv2.1",),
    "qtbase": ("COPYING.LGPLv3", "COPYING.GPLv3", "COPYING.GPLv2"),
    "qtsvg": ("COPYING.LGPLv3", "COPYING.GPLv3", "COPYING.GPLv2"),
}


def fallback_files(formula):
    names = FALLBACK_TEXTS.get(formula, ())
    ffmpeg = sorted(glob.glob(os.path.join(CELLAR, "ffmpeg", "*")))
    if not names or not ffmpeg:
        return []
    return [os.path.join(ffmpeg[-1], n) for n in names if os.path.isfile(os.path.join(ffmpeg[-1], n))]


def license_files(formula, version):
    root = os.path.join(CELLAR, formula, version)
    found = []
    for base in (root, os.path.join(root, "share", "doc", formula), os.path.join(root, "share", "licenses", formula)):
        for pattern in LICENSE_PATTERNS:
            found += [p for p in glob.glob(os.path.join(base, pattern)) if os.path.isfile(p)]
    return sorted(set(found)) or fallback_files(formula)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    app = sys.argv[1]
    notices_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(app, "Contents", "Resources", "THIRD_PARTY_NOTICES.md")

    formulas = {}
    for name in sorted(os.listdir(os.path.join(app, "Contents", "Frameworks"))):
        formula, version = formula_of(name)
        if formula:
            formulas.setdefault((formula, version), []).append(name)

    names = sorted({f for f, _ in formulas})
    info = json.loads(subprocess.run(["brew", "info", "--json=v2", *names], check=True,
                                     capture_output=True, text=True).stdout)
    meta = {f["name"]: f for f in info["formulae"]}

    licenses_dir = os.path.join(app, "Contents", "Resources", "licenses")
    shutil.rmtree(licenses_dir, ignore_errors=True)
    rows = []
    for (formula, version), libs in sorted(formulas.items()):
        m = meta.get(formula, {})
        source = (m.get("urls", {}).get("stable") or {}).get("url", "")
        rows.append(f"| {formula} | {version} | {m.get('license') or 'see licence files'} | {source} |")
        for path in license_files(formula, version):
            dest = os.path.join(licenses_dir, formula)
            os.makedirs(dest, exist_ok=True)
            shutil.copy2(path, dest)

    text = "\n".join([
        "# Third-party software in the Kvikplay macOS app",
        "",
        "Kvikplay's own source code is MIT licensed (see LICENSE). The macOS release bundles",
        "the libraries below, built by Homebrew. Some are GPL licensed (FFmpeg, mpv, x264,",
        "x265), so the distributed app as a whole is covered by the GNU GPL version 3.",
        "The licence texts are inside the app in `Contents/Resources/licenses/`; the source",
        "code of each library is at the address listed (the Homebrew build recipes are at",
        "https://github.com/Homebrew/homebrew-core).",
        "",
        "| Library (Homebrew formula) | Version | Licence | Source |",
        "|---|---|---|---|",
        *rows,
        "",
    ])
    with open(notices_path, "w", encoding="utf-8") as fh:
        fh.write(text)
    print(f"{len(rows)} third-party packages documented in {notices_path}")


if __name__ == "__main__":
    main()
