#!/usr/bin/env bash
# Build the device-side test binaries.
#
#   probe   - an AIDL client built from the RECONSTRUCTED stubs.  It calls the
#             SHIPPED service that is already running on the device, so a clean
#             run proves the reconstructed .aidl is wire-compatible.
#
# The generated AIDL stubs are reused from the main build so the probe links
# exactly the same glue the HAL does.
#
# Usage:  ./standalone/tests/build.sh
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
BUILD="$ROOT/standalone/build"
DIST="$ROOT/standalone/dist"
API="${API:-34}"

# reuse the toolchain discovery of the main build
# shellcheck disable=SC1091
source <(sed -n '/^find_ndk()/,/^}/p;/^find_aidl()/,/^}/p;/^find_binder_platform_headers()/,/^}/p;/^find_libbinder_ndk()/,/^}/p' \
        "$ROOT/standalone/build.sh")

TOOLCHAIN="$(find_ndk)" || { echo "ERROR: no NDK found"; exit 1; }
AIDL_BIN="$(find_aidl)" || { echo "ERROR: no aidl found"; exit 1; }
BINDER_HEADERS="$(find_binder_platform_headers)" || { echo "ERROR: no binder headers"; exit 1; }
BINDER_LIB="$(find_libbinder_ndk)" || { echo "ERROR: no libbinder_ndk.so"; exit 1; }
CXX="$TOOLCHAIN/bin/aarch64-linux-android${API}-clang++"
HOST_CXX="${HOST_CXX:-c++}"
SYSROOT="$TOOLCHAIN/sysroot"

GEN="$BUILD/aidl_gen"
if [ ! -f "$GEN/aidl/vendor/oplus/hardware/charger/ICharger.h" ]; then
    echo "ERROR: no generated AIDL stubs in $GEN - run standalone/build.sh first"
    exit 1
fi

mkdir -p "$DIST/tests" "$BUILD/obj"
echo "== compiling probe =="
"$CXX" \
    --target=aarch64-linux-android${API} \
    -std=c++20 \
    -fno-exceptions \
    -fno-rtti \
    -Wall \
    -Werror \
    -O2 \
    -I"$GEN" \
    -I"$BINDER_HEADERS" \
    --sysroot="$SYSROOT" \
    "$HERE/probe.cpp" \
    "$GEN/vendor/oplus/hardware/charger/ICharger.cpp" \
    "$GEN/vendor/oplus/hardware/charger/testKitFeatureTestResult.cpp" \
    -static-libstdc++ \
    -llog \
    "$BINDER_LIB" \
    -o "$DIST/tests/probe"

echo "== compiling classify_test =="
"$CXX" \
    --target=aarch64-linux-android${API} \
    -std=c++20 \
    -fno-exceptions \
    -fno-rtti \
    -Wall \
    -Werror \
    -O2 \
    -I"$ROOT/include" \
    -I"$GEN" \
    -I"$BINDER_HEADERS" \
    --sysroot="$SYSROOT" \
    "$HERE/classify_test.cpp" \
    "$ROOT/src/FastChargeDetector.cpp" \
    -static-libstdc++ \
    -o "$DIST/tests/classify_test"

echo "== compiling bypass_policy_test =="

# Host build: the policy is pure logic with no device access, so it runs on
# the build box as well as on the device.
"$HOST_CXX" \
    -std=c++20 \
    -Wall \
    -Werror \
    -O2 \
    -I"$ROOT/include" \
    "$HERE/bypass_policy_test.cpp" \
    "$ROOT/src/BypassPolicy.cpp" \
    -o "$DIST/tests/bypass_policy_test_host"

echo "== compiling raw_hash =="
"$CXX" \
    --target=aarch64-linux-android${API} \
    -std=c++20 \
    -fno-exceptions \
    -fno-rtti \
    -Wall \
    -Werror \
    -O2 \
    -I"$GEN" \
    -I"$BINDER_HEADERS" \
    --sysroot="$SYSROOT" \
    "$HERE/raw_hash.cpp" \
    "$GEN/vendor/oplus/hardware/charger/ICharger.cpp" \
    "$GEN/vendor/oplus/hardware/charger/testKitFeatureTestResult.cpp" \
    -static-libstdc++ \
    -llog \
    "$BINDER_LIB" \
    -o "$DIST/tests/raw_hash"

echo "== compiling feature_probe =="
"$CXX" \
    --target=aarch64-linux-android${API} \
    -std=c++20 \
    -fno-exceptions \
    -fno-rtti \
    -Wall \
    -Werror \
    -O2 \
    -I"$GEN" \
    -I"$BINDER_HEADERS" \
    --sysroot="$SYSROOT" \
    "$HERE/feature_probe.cpp" \
    "$GEN/vendor/oplus/hardware/charger/ICharger.cpp" \
    "$GEN/vendor/oplus/hardware/charger/testKitFeatureTestResult.cpp" \
    -static-libstdc++ \
    -llog \
    "$BINDER_LIB" \
    -o "$DIST/tests/feature_probe"

echo "== compiling node_probe =="
"$CXX" \
    --target=aarch64-linux-android${API} \
    -std=c++20 \
    -fno-exceptions \
    -fno-rtti \
    -Wall \
    -Werror \
    -O2 \
    --sysroot="$SYSROOT" \
    "$HERE/node_probe.cpp" \
    -static-libstdc++ \
    -o "$DIST/tests/node_probe"

echo "== built =="
ls -l "$DIST/tests/probe" "$DIST/tests/classify_test" "$DIST/tests/raw_hash" "$DIST/tests/feature_probe" "$DIST/tests/node_probe"
llvm-readelf -d "$DIST/tests/probe" | grep NEEDED
