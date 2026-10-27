#!/usr/bin/env bash
# Build the Linux x86_64 player and package it as a Steam Deck AppImage.
#
# Prerequisites: gcc, g++, cmake 3.21+, make, libsdl2-dev, wget.
# The user's ROMs are never bundled: the AppImage contains the recompiled
# player and the build-time ROM decrypt tool only.
#
# Usage: ./packaging/appimage/build-appimage.sh [ROM_DIR]
# Default ROM_DIR=$ROOT/roms-src, output dist/steamdeck/...
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ROMS="${1:-$ROOT/roms-src}"
BUILD="$ROOT/build-linux"
APPDIR="$BUILD/AppDir"
OUT="$ROOT/dist/steamdeck/KOF 99-SteamDeck-x86_64.AppImage"

echo "== configure/build (Release) =="
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release "-DKOF99_ROM_DIR=$ROMS"
cmake --build "$BUILD" --parallel "$(nproc)"

echo "== assemble AppDir =="
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" \
         "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps" \
         "$APPDIR/usr/share/doc/kof99"
cp "$BUILD/kof99" "$APPDIR/usr/bin/kof99"
cp "$BUILD/kof99_decrypt" "$APPDIR/usr/bin/kof99_decrypt"
strip --strip-unneeded "$APPDIR/usr/bin/kof99" 2>/dev/null || true
strip --strip-unneeded "$APPDIR/usr/bin/kof99_decrypt" 2>/dev/null || true

# Bundle the SDL2 the build linked against; SteamOS's system copy may differ.
SDL=$(ldd "$BUILD/kof99" | awk '/libSDL2-2\.0\.so\.0/{print $3; exit}')
if [ -n "$SDL" ]; then cp "$SDL" "$APPDIR/usr/lib/"; fi

cp "$ROOT/packaging/appimage/kof99.png" "$APPDIR/kof99.png"
cp "$ROOT/packaging/appimage/kof99.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/kof99.png"
cp "$ROOT/packaging/appimage/kof99.desktop" "$APPDIR/kof99.desktop"
cp "$ROOT/packaging/appimage/kof99.desktop" "$APPDIR/usr/share/applications/kof99.desktop"
cp "$ROOT/packaging/appimage/README.txt" "$APPDIR/usr/share/doc/kof99/README.txt"
cp "$ROOT/LICENSE" "$APPDIR/usr/share/doc/kof99/LICENSE.txt"
cp "$ROOT/LICENSE-STATUS.md" "$APPDIR/usr/share/doc/kof99/LICENSE-STATUS.txt" 2>/dev/null || true
cp "$ROOT/THIRD_PARTY_NOTICES.md" "$APPDIR/usr/share/doc/kof99/THIRD_PARTY_NOTICES.txt"
mkdir -p "$APPDIR/usr/share/doc/kof99/LICENSES"
cp "$ROOT/neogeorecomp/LICENSE" "$APPDIR/usr/share/doc/kof99/LICENSES/neogeorecomp.txt"
cp "$ROOT/neogeorecomp/NOTICE" "$APPDIR/usr/share/doc/kof99/LICENSES/neogeorecomp-NOTICE.txt"
cp "$ROOT/neogeorecomp/LICENSES/"*.txt "$APPDIR/usr/share/doc/kof99/LICENSES/"
cp "$ROOT/packaging/windows/SDL2.txt" "$APPDIR/usr/share/doc/kof99/LICENSES/SDL2.txt"
cp "$ROOT/LICENSES/MAME-BSD-3-Clause.txt" "$APPDIR/usr/share/doc/kof99/LICENSES/MAME-BSD-3-Clause.txt"

cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
HERE=$(dirname "$(readlink -f "$0")")
export LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Explicit --rom-path always wins.  --select-rom asks for the picker, so do
# not pre-empt it with an automatically found folder.
has_rom=0
select_rom=0
for a in "$@"; do
    [ "$a" = "--rom-path" ] && has_rom=1
    [ "$a" = "--select-rom" ] && select_rom=1
done

if [ "$has_rom" = 0 ] && [ "$select_rom" = 0 ]; then
    cand=""
    if [ -n "${KOF99_ROM_DIR:-}" ]; then
        cand="$KOF99_ROM_DIR"
    elif [ -n "${APPIMAGE:-}" ]; then
        for d in "$(dirname "$APPIMAGE")/roms" \
                 "$(dirname "$APPIMAGE")/roms/kof99" \
                 "$HOME/.local/share/kof99/roms" \
                 "$HOME/.local/share/kof99" \
                 "$HOME/Emulation/roms/kof99" \
                 "$HOME/roms/kof99" "$HOME/roms"; do
            if [ -d "$d" ]; then cand="$d"; break; fi
        done
    fi
    if [ -n "$cand" ]; then
        if [ -f "$cand/251-pd.bin" ]; then
            set -- --rom-path "$cand" "$@"
        elif [ -f "$cand/251-p1.p1" ]; then
            # The user pointed us at the original encrypted set: decrypt into
            # the per-user data directory once, then launch from there.
            CACHE="$HOME/.local/share/kof99/roms"
            if [ ! -f "$CACHE/251-pd.bin" ]; then
                mkdir -p "$CACHE"
                "$HERE/usr/bin/kof99_decrypt" --rom-path "$cand" --out "$CACHE" || exit 1
            fi
            set -- --rom-path "$CACHE" "$@"
        else
            set -- --rom-path "$cand" "$@"
        fi
    fi
fi

# Steam Deck / Gaming Mode wants a fullscreen game.  Keep --headless and an
# explicit --scale or --fullscreen from being overridden.
add_fs=1
for a in "$@"; do
    case "$a" in --headless|--fullscreen|--scale) add_fs=0;; esac
done
[ "$add_fs" = 1 ] && set -- --fullscreen "$@"

exec "$HERE/usr/bin/kof99" "$@"
EOF
chmod +x "$APPDIR/AppRun"

echo "== appimagetool =="
mkdir -p "$ROOT/dist/steamdeck"
TOOL="${APPIMAGETOOL:-$HOME/appimagetool-x86_64.AppImage}"
if [ ! -x "$TOOL" ]; then
    TOOL="$BUILD/appimagetool.AppImage"
    wget -q -O "$TOOL" \
      https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x "$TOOL"
fi
ARCH=x86_64 "$TOOL" --appimage-extract-and-run --no-appstream "$APPDIR" "$OUT"
chmod +x "$OUT"
echo "wrote $OUT"
