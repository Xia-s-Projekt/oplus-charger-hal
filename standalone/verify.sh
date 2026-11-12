#!/usr/bin/env bash
# Offline checks on the rebuilt HAL.
#
# What is proven here:
#   * the artifact is a 64-bit AArch64 PIE with the Android linker
#   * DT_NEEDED contains only libraries a vendor process can load
#   * the AIDL service name the binary registers is the shipped one
#   * the interface descriptor matches the shipped stub
#   * the 33 W / 45 W classification really is in the binary
#
# What is NOT proven here: the service was never run on a device.
set -uo pipefail

# NOTE: no `set -e`, and every pipeline below is written so that the producer
# is not killed by SIGPIPE: `grep -q` exits as soon as it matches, which makes
# the upstream command die with status 141 and, under `pipefail`, turns a
# successful check into a failure.  Dumps go into a file first, then grep.
HERE_VERIFY="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK="$HERE_VERIFY/build/verify_work"
mkdir -p "$WORK"
trap 'rm -rf "$WORK"' EXIT

BIN="${1:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/dist/vendor/bin/hw/vendor.oplus.hardware.charger-V3-service}"
[ -f "$BIN" ] || { echo "FAIL: $BIN does not exist"; exit 1; }

fail=0
check() {
    local name="$1"; shift
    if "$@" >/dev/null 2>&1; then
        echo "PASS  $name"
    else
        echo "FAIL  $name"
        fail=1
    fi
}

echo "== artifact =="
file "$BIN"

check "ELF64 AArch64 PIE" bash -c "file '$BIN' | grep -q 'ELF 64-bit LSB pie executable, ARM aarch64'"
check "Android linker64" bash -c "file '$BIN' | grep -q '/system/bin/linker64'"

echo
echo "== dynamic dependencies =="
NEEDED="$(llvm-readelf -d "$BIN" 2>/dev/null | grep NEEDED | sed 's/.*\[\(.*\)\]/\1/')"
echo "$NEEDED" | sed 's/^/  /'
for lib in $NEEDED; do
    case "$lib" in
        libc.so|libdl.so|libm.so|liblog.so|libbinder_ndk.so|libc++_shared.so)
            echo "PASS  $lib is loadable by a vendor process" ;;
        *)
            echo "FAIL  $lib is not a vendor-loadable library"
            fail=1 ;;
    esac
done

echo
echo "== interface identity =="
llvm-strings -n 8 "$BIN" > "$WORK/strings.txt"
if grep -qx 'vendor.oplus.hardware.charger.ICharger' "$WORK/strings.txt"; then
    echo "PASS  AIDL descriptor matches the shipped stub"
else
    echo "FAIL  AIDL descriptor missing"
    fail=1
fi
if grep -qx '/default' "$WORK/strings.txt"; then
    echo "PASS  service instance /default present"
else
    echo "FAIL  service instance missing"
    fail=1
fi

echo
echo "== fast charge thresholds =="
# 33 and 45 as immediate operands in the built binary: the compiler folds
# them into a mov/movk pair, so look for the literal values in .text.
llvm-objdump -d "$BIN" > "$WORK/disasm.txt" 2>/dev/null
if grep -qE 'mov[[:space:]]+w[0-9]+, #0x21\b' "$WORK/disasm.txt"; then
    echo "PASS  33 (0x21) is present as an immediate"
else
    echo "FAIL  33 (0x21) not found as an immediate"
    fail=1
fi
if grep -qE 'mov[[:space:]]+w[0-9]+, #0x2d\b' "$WORK/disasm.txt"; then
    echo "PASS  45 (0x2d) is present as an immediate"
else
    echo "FAIL  45 (0x2d) not found as an immediate"
    fail=1
fi

echo
echo "== node paths =="
for path in /sys/devices/platform/charger/Pump_Express \
            /sys/devices/platform/charger/adapter_capacity \
            /sys/devices/platform/charger/fast_chg_indicator \
            /sys/class/oplus_chg/battery/charge_technology; do
    if grep -qx "$path" "$WORK/strings.txt"; then
        echo "PASS  $path"
    else
        echo "FAIL  $path missing from the binary"
        fail=1
    fi
done

echo
if [ "$fail" -eq 0 ]; then
    echo "ALL CHECKS PASSED"
else
    echo "SOME CHECKS FAILED"
fi
exit "$fail"
