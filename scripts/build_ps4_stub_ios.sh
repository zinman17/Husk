#!/bin/bash
# Build a stub libhuskps4.dylib for CI builds.
#
# The real libhuskps4.dylib comes from two private source trees (Bachata-S4
# on branch husk, husk-fex on branch husk-fex) that are not in this repository,
# so it cannot be built in CI. project.yml embeds the dylib unconditionally
# (embed: true), and xcodebuild's "Embed Frameworks" phase fails when the file
# is missing, so a CI build dies at the link step. This stub fills the slot:
# it exports the one symbol the host expects (husk_console_module_v1) and
# returns a module table whose every entry reports failure. PS4 games in a CI
# build surface a clear "not available" error instead of running, but the rest
# of Husk -- the translation layer, QEMU/LineageOS, the A13 Android path --
# is unaffected.
#
# Idempotent: skips itself if build/ios-arm64/lib/libhuskps4.dylib already
# exists, so local builds that ran the real build_ps4_ios.sh are not touched.
set -euo pipefail

HUSK_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$HUSK_ROOT/build/ios-arm64/lib"
DYLIB="$OUT/libhuskps4.dylib"

[ -f "$DYLIB" ] && { echo "==> libhuskps4.dylib already present, skipping stub"; exit 0; }

mkdir -p "$OUT"
SDK=$(xcrun --sdk iphoneos --show-sdk-path)

echo "==> building stub libhuskps4.dylib (real dylib not available in CI)"
xcrun -sdk iphoneos clang \
    -target arm64-apple-ios15.5 \
    -isysroot "$SDK" \
    -dynamiclib \
    -fvisibility=hidden \
    -Wno-unguarded-availability-new \
    -o "$DYLIB" \
    "$HUSK_ROOT/scripts/stubs/libhuskps4_stub.c"

# install_name has to match what project.yml's embed step expects, or dyld
# will not find it at runtime from the app's Frameworks directory.
install_name_tool -id "@rpath/libhuskps4.dylib" "$DYLIB"

# build_ps4_ios.sh's sanity check: exports exactly _husk_console_module_v1.
exports=$(nm -gU "$DYLIB" | awk '{print $3}')
[ "$exports" = "_husk_console_module_v1" ] || {
    echo "error: stub exports more than _husk_console_module_v1:" >&2
    echo "$exports" >&2
    exit 1
}

echo "stub: libhuskps4.dylib ($(du -h "$DYLIB" | cut -f1)) -> $OUT"
echo "stub: PS4 layer unavailable in this build (private sources absent)" > "$OUT/libhuskps4.revision"
