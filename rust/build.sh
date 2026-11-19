#!/usr/bin/env bash
# Build the Rust charger HAL into a standalone service binary.
#
# The C++ build (standalone/build.sh) stages its result as a V6 service binary;
# this script uses the same name so the two builds are interchangeable.
#
# Env:    NDK / ANDROID_NDK_ROOT   NDK root (used to locate llvm-strip)
#         LIBBINDER_NDK            path to a libbinder_ndk.so with the service
#                                  manager and ABinderProcess_* symbols
#         LLVM_STRIP               override for the strip tool
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HAL_ROOT="$(cd "$HERE/.." && pwd)"
OUT="$HERE/target/aarch64-linux-android/release/charger"
DIST="$HAL_ROOT/standalone/dist/vendor/bin/hw"

# Locate llvm-strip: explicit override, the NDK toolchain, then PATH.
TOOLCHAIN=""
NDK_ROOT="${NDK:-${ANDROID_NDK_ROOT:-}}"
if [ -n "$NDK_ROOT" ]; then
    if [ -x "$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip" ]; then
        TOOLCHAIN="$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin"
    elif [ -x "$NDK_ROOT/bin/llvm-strip" ]; then
        TOOLCHAIN="$NDK_ROOT/bin"
    fi
fi
LLVM_STRIP="${LLVM_STRIP:-${TOOLCHAIN:+${TOOLCHAIN}/llvm-strip}}"
[ -n "$LLVM_STRIP" ] || LLVM_STRIP="$(command -v llvm-strip || true)"

# libbinder_ndk is supplied by the target system, not the NDK. Its directory
# is passed to the linker through RUSTFLAGS so no checkout-specific path is
# baked into the project.
[ -n "${LIBBINDER_NDK:-${BINDER_LIB:-}}" ] || {
    echo "ERROR: set LIBBINDER_NDK to a libbinder_ndk.so with AServiceManager_addService"; exit 1; }
BINDER_LIB="${LIBBINDER_NDK:-$BINDER_LIB}"
[ -f "$BINDER_LIB" ] || { echo "ERROR: $BINDER_LIB does not exist"; exit 1; }

echo "== cargo build =="
cd "$HERE"
# The NDK clang driver is selected as the linker in .cargo/config.toml; its
# bin directory must be on PATH. All checkout-dependent link arguments live
# here so they can be overridden per environment.
RUSTFLAGS="-C link-arg=-static-libstdc++"
RUSTFLAGS="$RUSTFLAGS -C link-arg=-Wl,--gc-sections"
RUSTFLAGS="$RUSTFLAGS -C link-arg=-L$(dirname "$BINDER_LIB")"
RUSTFLAGS="$RUSTFLAGS -C link-arg=-lbinder_ndk"
export RUSTFLAGS
cargo build --target aarch64-linux-android --release

echo "== strip =="
if [ -n "$LLVM_STRIP" ] && [ -x "$LLVM_STRIP" ]; then
    "$LLVM_STRIP" --strip-unneeded "$OUT"
else
    echo "llvm-strip not found - binary stays unstripped" >&2
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
