#!/usr/bin/env bash
# Builds the Rust charger HAL into a flashable service binary.
#
# The C++ build (standalone/build.sh) stages the result under the V6 filename
# because erofs/vendor/etc/init and the VINTF manifest both reference V6; this
# script does the same so the two builds stay interchangeable.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HAL_ROOT="$(cd "$HERE/.." && pwd)"
OUT="$HERE/target/aarch64-linux-android/release/charger"
DIST="$HERE/../standalone/dist/vendor/bin/hw"
LLVM_STRIP="/home/ryanistr/Projects/android-ndk-r27c/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"

cd "$HERE"

echo "== cargo build =="
cargo build --target aarch64-linux-android --release

echo "== strip =="
if [ -x "$LLVM_STRIP" ]; then
    "$LLVM_STRIP" --strip-unneeded "$OUT"
else
    echo "llvm-strip not found at $LLVM_STRIP - binary stays unstripped" >&2
    exit 1
fi

echo "== verify =="
# Same gate the C++ build uses: no .symtab/.debug, and the dynamic symbol set
# must still resolve everything it calls.
readelf -S "$OUT" | grep -qE '\.symtab|\.debug' && { echo "FAIL: debug sections present" >&2; exit 1; }
readelf -d "$OUT" | grep -q 'libbinder_ndk.so' || { echo "FAIL: libbinder_ndk not linked" >&2; exit 1; }

echo "== stage =="
mkdir -p "$DIST"
cp "$OUT" "$DIST/vendor.oplus.hardware.charger-V6-service"
ls -l "$DIST/vendor.oplus.hardware.charger-V6-service"
md5sum "$DIST/vendor.oplus.hardware.charger-V6-service"
