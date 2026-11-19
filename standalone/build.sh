#!/usr/bin/env bash
# Build the OplusCharger vendor HAL without an AOSP tree.
#
# Stages:
#   1. generate the AIDL NDK stubs from aidl/vendor/oplus/hardware/charger/*.aidl
#   2. compile every source with the NDK
#   3. link the service binary
#   4. stage it the way Soong would: dist/vendor/bin/hw/<name>
#
# Usage:  ./standalone/build.sh
# Env:    NDK / ANDROID_NDK_ROOT   NDK root or toolchain dir
#         ANDROID_HOME / ANDROID_SDK_ROOT  SDK root containing ndk/ and build-tools/
#         AIDL                    path to the aidl binary
#         BINDER_HEADERS          dir containing android/binder_manager.h
#         LIBBINDER_NDK           path to a libbinder_ndk.so with service-manager symbols
#         API                     android API level to compile against (default 34)
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
BUILD="$ROOT/standalone/build"
DIST="$ROOT/standalone/dist"
API="${API:-34}"

# ---------------------------------------------------------------------------
# Toolchain discovery.  NDK may point at the root or at the toolchain dir.
# ---------------------------------------------------------------------------
find_ndk() {
    local candidates=()
    [ -n "${NDK:-}" ] && candidates+=("$NDK")
    [ -n "${ANDROID_NDK_ROOT:-}" ] && candidates+=("$ANDROID_NDK_ROOT")
    [ -n "${ANDROID_HOME:-}" ] && candidates+=(
        "$ANDROID_HOME/ndk/27.3.13750724"
        "$ANDROID_HOME/ndk/28.2.13676358")
    [ -n "${ANDROID_SDK_ROOT:-}" ] && candidates+=(
        "$ANDROID_SDK_ROOT/ndk/27.3.13750724"
        "$ANDROID_SDK_ROOT/ndk/28.2.13676358")
    candidates+=(/opt/android-sdk/ndk/27.3.13750724
                 /opt/android-sdk/ndk/28.2.13676358)
    for c in "${candidates[@]}"; do
        [ -d "$c" ] || continue
        if [ -x "$c/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++" ]; then
            echo "$c/toolchains/llvm/prebuilt/linux-x86_64"
            return 0
        fi
        if [ -x "$c/bin/clang++" ]; then
            echo "$c"
            return 0
        fi
    done
    return 1
}

find_aidl() {
    local candidates=()
    [ -n "${AIDL:-}" ] && candidates+=("$AIDL")
    [ -n "${ANDROID_HOME:-}" ] && candidates+=(
        "$ANDROID_HOME/build-tools/35.0.0/aidl"
        "$ANDROID_HOME/build-tools/34.0.0/aidl")
    [ -n "${ANDROID_SDK_ROOT:-}" ] && candidates+=(
        "$ANDROID_SDK_ROOT/build-tools/35.0.0/aidl"
        "$ANDROID_SDK_ROOT/build-tools/34.0.0/aidl")
    candidates+=(/opt/android-sdk/build-tools/35.0.0/aidl
                 /opt/android-sdk/build-tools/34.0.0/aidl)
    for c in "${candidates[@]}"; do
        [ -x "$c" ] && { echo "$c"; return 0; }
    done
    return 1
}

# The NDK sysroot has every android/binder_*.h except binder_manager.h; take
# it from the AOSP headers snapshot instead of shimming it.
find_binder_platform_headers() {
    local candidates=()
    [ -n "${BINDER_HEADERS:-}" ] && candidates+=("$BINDER_HEADERS")
    [ -n "${AOSP_HEADERS:-}" ] && candidates+=(
        "$AOSP_HEADERS/frameworks/native/libs/binder/ndk/include_platform")
    for c in "${candidates[@]}"; do
        [ -f "$c/android/binder_manager.h" ] && { echo "$c"; return 0; }
    done
    return 1
}

# The NDK's libbinder_ndk stub does not export AServiceManager_addService or
# the ABinderProcess_* entry points, so link against the real library.  The
# ROM's copy is the right one: /system/lib64/libbinder_ndk.so is listed in
# llndk.libraries.txt, so a vendor process can load it.
find_libbinder_ndk() {
    local candidates=()
    [ -n "${LIBBINDER_NDK:-}" ] && candidates+=("$LIBBINDER_NDK")
    [ -n "${BINDER_LIB:-}" ] && candidates+=("$BINDER_LIB")
    for c in "${candidates[@]}"; do
        if [ -f "$c" ]; then
            if llvm-nm -D --defined-only "$c" 2>/dev/null |
                    grep -q 'AServiceManager_addService'; then
                echo "$c"
                return 0
            fi
        fi
    done
    return 1
}

TOOLCHAIN="$(find_ndk)" || { echo "ERROR: no NDK found (set NDK=/path/to/ndk)"; exit 1; }
AIDL_BIN="$(find_aidl)" || { echo "ERROR: no aidl found (set AIDL=/path/to/aidl)"; exit 1; }
BINDER_HEADERS="$(find_binder_platform_headers)" || {
    echo "ERROR: no binder include_platform headers found (set BINDER_HEADERS)"; exit 1; }
BINDER_LIB="$(find_libbinder_ndk)" || {
    echo "ERROR: no usable libbinder_ndk.so found (set LIBBINDER_NDK; need AServiceManager_addService)"; exit 1; }
CXX="$TOOLCHAIN/bin/aarch64-linux-android${API}-clang++"
LLVM_STRIP="$TOOLCHAIN/bin/llvm-strip"
[ -x "$CXX" ] || { echo "ERROR: $CXX is missing"; exit 1; }
[ -x "$LLVM_STRIP" ] || { echo "ERROR: $LLVM_STRIP is missing"; exit 1; }
SYSROOT="$TOOLCHAIN/sysroot"

echo "toolchain : $TOOLCHAIN"
echo "aidl      : $AIDL_BIN"
echo "binder hdr: $BINDER_HEADERS"
echo "binder lib: $BINDER_LIB"
echo "api       : $API"

rm -rf "$BUILD"
mkdir -p "$BUILD/obj" "$DIST/vendor/bin/hw"

# ---------------------------------------------------------------------------
# 1. AIDL stubs.  The interface is @VintfStability, so the compiler needs
#    --structured --stability=vintf.  The output lands under <out>/aidl/ so
#    that the "#include \"aidl/vendor/...\"" spelling used by the sources
#    resolves, exactly as it does in an AOSP tree.
# ---------------------------------------------------------------------------
echo "== generating AIDL stubs =="
GEN="$BUILD/aidl_gen"
mkdir -p "$GEN"
for f in "$ROOT"/aidl/vendor/oplus/hardware/charger/*.aidl; do
    "$AIDL_BIN" \
        --lang=ndk \
        --structured \
        --stability=vintf \
        --version=6 \
        --hash=ede5e600328119dbf1c38269f810a3fdce74029f \
        -I "$ROOT/aidl" \
        --header_out="$GEN" \
        -o "$GEN" \
        "$f"
done
if [ ! -f "$GEN/aidl/vendor/oplus/hardware/charger/ICharger.h" ]; then
    echo "ERROR: aidl did not produce ICharger.h"
    exit 1
fi

# ---------------------------------------------------------------------------
# 2. compile
# ---------------------------------------------------------------------------
echo "== compiling =="
CXXFLAGS=(
    --target=aarch64-linux-android${API}
    -std=c++20
    -fno-exceptions
    -fno-rtti
    -fvisibility=hidden
    -Wall
    -Werror
    -O2
    -I"$ROOT/include"
    -I"$GEN"
    -I"$BINDER_HEADERS"
    --sysroot="$SYSROOT"
)
SOURCES=(
    "$ROOT/src/BypassPolicy.cpp"
    "$ROOT/src/Charger.cpp"
    "$ROOT/src/ChargerManual.cpp"
    "$ROOT/src/ChgDetectionCheck.cpp"
    "$ROOT/src/FastChargeDetector.cpp"
    "$ROOT/src/NodeControl.cpp"
    "$ROOT/src/service.cpp"
)
# the generated AIDL sources (the interface glue and the parcelable)
while IFS= read -r gen; do
    SOURCES+=("$gen")
done < <(find "$GEN/vendor" -name '*.cpp' | sort)

OBJECTS=()
for src in "${SOURCES[@]}"; do
    obj="$BUILD/obj/$(echo "${src#$ROOT/}" | tr '/' '_').o"
    "$CXX" "${CXXFLAGS[@]}" -c "$src" -o "$obj"
    OBJECTS+=("$obj")
done

# ---------------------------------------------------------------------------
# 3. link.  libc++ is static: a device has no libc++_shared.so and a vendor
#    binary must not carry a DT_NEEDED on the runtime apex's copy.
# ---------------------------------------------------------------------------
echo "== linking =="
LIBCXX_STATIC="$SYSROOT/usr/lib/aarch64-linux-android/libc++_static.a"
[ -f "$LIBCXX_STATIC" ] || { echo "ERROR: $LIBCXX_STATIC missing"; exit 1; }
LDFLAGS=(
    --target=aarch64-linux-android${API}
    -fno-exceptions
    -fno-rtti
    --sysroot="$SYSROOT"
    -Wl,--no-undefined
)
OUT="$DIST/vendor/bin/hw/vendor.oplus.hardware.charger-V3-service"
# The libraries come AFTER the objects: with the default link order the
# linker would drop a library that is listed before its first user.
#
# -static-libstdc++ is what actually removes libc++_shared.so from DT_NEEDED:
# the clang driver adds -lc++_shared by default, so naming libc++_static.a on
# the command line alone still leaves the shared library as a dependency.
# A device has no libc++_shared.so and a vendor binary must not depend on the
# runtime apex's copy.
"$CXX" "${LDFLAGS[@]}" "${OBJECTS[@]}" \
    -static-libstdc++ \
    -llog \
    "$BINDER_LIB" \
    -o "$OUT"

echo "== built =="
ls -l "$OUT"

# Ship a stripped binary: the .symtab is 8k+ symbols of local names and source
# paths.  It is dead weight in /vendor and the only thing that ever needed it
# was host-side symbolization of a logcat backtrace.
"$LLVM_STRIP" --strip-unneeded "$OUT"

"$HERE/verify.sh" "$OUT"
