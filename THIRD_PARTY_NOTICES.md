# Third-party software in the Kvikplay macOS app

Kvikplay's own source code is MIT licensed (see LICENSE). The macOS release bundles
the libraries below, built by Homebrew. Some are GPL licensed (FFmpeg, mpv, x264,
x265), so the distributed app as a whole is covered by the GNU GPL version 3.
The licence texts are inside the app in `Contents/Resources/licenses/`; the source
code of each library is at the address listed (the Homebrew build recipes are at
https://github.com/Homebrew/homebrew-core).

| Library (Homebrew formula) | Version | Licence | Source |
|---|---|---|---|
| brotli | 1.2.0 | MIT | https://github.com/google/brotli/archive/refs/tags/v1.2.1.tar.gz |
| dav1d | 1.5.5 | BSD-2-Clause | https://code.videolan.org/videolan/dav1d/-/archive/1.5.5/dav1d-1.5.5.tar.bz2 |
| dbus | 1.16.4 | AFL-2.1 OR GPL-2.0-or-later | https://dbus.freedesktop.org/releases/dbus/dbus-1.16.4.tar.xz |
| double-conversion | 3.4.0 | BSD-3-Clause | https://github.com/google/double-conversion/archive/refs/tags/v3.4.0.tar.gz |
| ffmpeg | 9.0.2_1 | GPL-3.0-or-later | https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz |
| fontconfig | 2.18.4 | HPND-sell-variant AND Unicode-3.0 AND MIT-Modern-Variant AND MIT AND LicenseRef-Homebrew-public-domain | https://gitlab.freedesktop.org/fontconfig/fontconfig/-/archive/2.18.4/fontconfig-2.18.4.tar.gz |
| freetype | 2.15.0 | FTL | https://downloads.sourceforge.net/project/freetype/freetype2/2.15.0/freetype-2.15.0.tar.xz |
| fribidi | 1.0.18 | GPL-2.0-or-later AND LGPL-2.1-or-later | https://github.com/fribidi/fribidi/releases/download/v1.0.18/fribidi-1.0.18.tar.xz |
| gettext | 1.0 | GPL-3.0-or-later AND LGPL-2.1-or-later | https://ftpmirror.gnu.org/gettext/gettext-1.0.tar.gz |
| glib | 2.90.1 | LGPL-2.1-or-later | https://download.gnome.org/sources/glib/2.90/glib-2.90.1.tar.xz |
| graphite2 | 1.3.15 | MIT OR MPL-2.0 OR LGPL-2.1-or-later OR GPL-2.0-or-later | https://github.com/silnrsi/graphite/releases/download/1.3.15/graphite2-1.3.15.tgz |
| harfbuzz | 15.1.0 | MIT | https://github.com/harfbuzz/harfbuzz/releases/download/15.1.0/harfbuzz-15.1.0.tar.xz |
| icu4c@79 | 79.1 | Unicode-3.0 | https://github.com/unicode-org/icu/releases/download/release-79.1/icu4c-79.1-sources.tgz |
| jpeg-turbo | 3.2.1 | IJG AND Zlib AND BSD-3-Clause | https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.2.1/libjpeg-turbo-3.2.1.tar.gz |
| lame | 4.0 | LGPL-2.0-or-later | https://downloads.sourceforge.net/project/lame/lame/4.0/lame-4.0.tar.gz |
| libarchive | 3.8.11 | BSD-2-Clause | https://www.libarchive.org/downloads/libarchive-3.8.11.tar.xz |
| libass | 0.17.5_1 | ISC | https://github.com/libass/libass/releases/download/0.17.5/libass-0.17.5.tar.xz |
| libb2 | 0.98.1 | CC0-1.0 | https://github.com/BLAKE2/libb2/releases/download/v0.98.1/libb2-0.98.1.tar.gz |
| libbluray | 1.5.0 | LGPL-2.1-or-later | https://download.videolan.org/videolan/libbluray/1.5.0/libbluray-1.5.0.tar.xz |
| libplacebo | 7.387.0 | LGPL-2.1-or-later | https://code.videolan.org/videolan/libplacebo/-/archive/v7.387.0/libplacebo-v7.387.0.tar.bz2 |
| libpng | 1.6.59 | libpng-2.0 | https://downloads.sourceforge.net/project/libpng/libpng16/1.6.59/libpng-1.6.59.tar.xz |
| libsamplerate | 0.2.2 | BSD-2-Clause | https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz |
| libudfread | 1.2.0 | LGPL-2.1-or-later | https://download.videolan.org/videolan/libudfread/libudfread-1.2.0.tar.xz |
| libunibreak | 8.0.1 | Zlib | https://github.com/adah1972/libunibreak/releases/download/libunibreak_8_0_1/libunibreak-8.0.1.tar.gz |
| libvmaf | 3.2.1 | BSD-2-Clause-Patent | https://github.com/Netflix/vmaf/archive/refs/tags/v3.2.1.tar.gz |
| libvpx | 1.17.0 | BSD-3-Clause | https://github.com/webmproject/libvpx/archive/refs/tags/v1.17.0.tar.gz |
| little-cms2 | 2.20 | MIT | https://downloads.sourceforge.net/project/lcms/lcms/2.20/lcms2-2.20.tar.gz |
| luajit | 2.1.1794386141 | MIT | https://github.com/LuaJIT/LuaJIT/archive/63c8f1a9b479ba5518cf6a170ff7729178bc3cfd.tar.gz |
| lz4 | 1.10.0 | BSD-2-Clause | https://github.com/lz4/lz4/archive/refs/tags/v1.11.1.tar.gz |
| md4c | 0.6.0 | MIT | https://github.com/mity/md4c/archive/refs/tags/v0.6.0.tar.gz |
| mpg123 | 1.33.7 | LGPL-2.1-only | https://www.mpg123.de/download/mpg123-1.33.7.tar.bz2 |
| mpv | 0.42.0_1 | GPL-2.0-or-later AND LGPL-2.1-or-later | https://github.com/mpv-player/mpv/archive/refs/tags/v0.42.0.tar.gz |
| mujs | 1.3.11 | ISC | https://mujs.com/downloads/mujs-1.3.11.tar.gz |
| openssl@4 | 4.1.0 | Apache-2.0 | https://github.com/openssl/openssl/releases/download/openssl-4.1.0/openssl-4.1.0.tar.gz |
| opus | 1.6.1 | BSD-3-Clause | https://ftp.osuosl.org/pub/xiph/releases/opus/opus-1.6.1.tar.gz |
| pcre2 | 10.47_1 | BSD-3-Clause | https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.49/pcre2-10.49.tar.bz2 |
| qtbase | 6.12.0_2 | (LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only) AND (GPL-3.0-only WITH Qt-GPL-exception-1.0) AND BSD-3-Clause AND GFDL-1.3-no-invariants-only | https://download.qt.io/official_releases/qt/6.12/6.12.0/submodules/qtbase-everywhere-src-6.12.0.tar.xz |
| qtsvg | 6.12.0_1 | (LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only) AND BSD-3-Clause | https://download.qt.io/official_releases/qt/6.12/6.12.0/submodules/qtsvg-everywhere-src-6.12.0.tar.xz |
| rubberband | 4.0.0 | GPL-2.0-or-later | https://breakfastquay.com/files/releases/rubberband-4.0.0.tar.bz2 |
| shaderc | 2026.4 | Apache-2.0 | https://github.com/google/shaderc/archive/refs/tags/v2026.4.tar.gz |
| svt-av1 | 4.3.0 | BSD-3-Clause | https://gitlab.com/AOMediaCodec/SVT-AV1/-/archive/v4.3.0/SVT-AV1-v4.3.0.tar.bz2 |
| uchardet | 0.0.8 | MPL-1.1 OR GPL-2.0-or-later OR LGPL-2.1-or-later | https://www.freedesktop.org/software/uchardet/releases/uchardet-0.0.8.tar.xz |
| vulkan-loader | 1.4.363.0 | Apache-2.0 | https://github.com/KhronosGroup/Vulkan-Loader/archive/refs/tags/vulkan-sdk-1.4.363.0.tar.gz |
| x264 | r3222 | GPL-2.0-or-later | https://code.videolan.org/videolan/x264.git |
| x265 | 4.3 | GPL-2.0-or-later | https://github.com/Multicorewareinc/x265/releases/download/4.3/x265_4.3.tar.gz |
| xz | 5.8.4 | 0BSD AND GPL-2.0-or-later | https://github.com/tukaani-project/xz/releases/download/v5.8.4/xz-5.8.4.tar.gz |
| zimg | 3.0.6 | WTFPL | https://github.com/sekrit-twc/zimg/archive/refs/tags/release-3.0.6.tar.gz |
| zstd | 1.5.7_1 | (BSD-3-Clause OR GPL-2.0-only) AND BSD-2-Clause AND MIT | https://github.com/facebook/zstd/archive/refs/tags/v1.5.7.tar.gz |
