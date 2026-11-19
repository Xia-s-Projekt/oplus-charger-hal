#!/system/bin/sh
# Boundary test for the OplusCharger HAL.
# Run as root. Push probe + the HAL + this script to /data/local/tmp.
#
# For each watt value it starts the HAL with the two charger nodes pointed at
# fixture files (through the
# OPLUS_CHARGER_PUMP_NODE / OPLUS_CHARGER_CAPACITY_NODE environment
# variables), then asks the running service what it reports over binder.
#
# No mount namespaces and no bind mounts: the real sysfs nodes are never
# touched.
REMOTE_DIR=/data/local/tmp
SERVICE="$REMOTE_DIR/vendor.oplus.hardware.charger-V3-service"
PROBE="$REMOTE_DIR/probe"
FIXTURE_DIR="$REMOTE_DIR/chgfix"
INSTANCE=vendor.oplus.hardware.charger.ICharger/rebuilt

mkdir -p "$FIXTURE_DIR"
chmod 755 "$FIXTURE_DIR"

# Kill the test instance by PID.  A `pkill -f charger-V3-service` would also
# kill the SHIPPED service that init runs from /vendor/bin/hw - same binary
# name - and the shipped HAL is the one the port's framework talks to.
stop_test_service() {
    if [ -n "$TEST_PID" ]; then
        kill "$TEST_PID" 2>/dev/null
        wait "$TEST_PID" 2>/dev/null
        TEST_PID=""
    fi
}

run_case() {
    cap="$1"
    label="$2"

    stop_test_service
    sleep 1
    echo "$cap" > "$FIXTURE_DIR/adapter_capacity"
    echo 1 > "$FIXTURE_DIR/Pump_Express"

    cd "$REMOTE_DIR" || exit 1
    # The oplus driver nodes are pointed at a path that does not exist, so
    # the detector falls through to the charger-driver pair.
    OPLUS_CHARGER_INSTANCE=rebuilt \
    OPLUS_CHARGER_TECH_NODE="$FIXTURE_DIR/absent-tech" \
    OPLUS_CHARGER_FAST_NODE="$FIXTURE_DIR/absent-fast" \
    OPLUS_CHARGER_PUMP_NODE="$FIXTURE_DIR/Pump_Express" \
    OPLUS_CHARGER_CAPACITY_NODE="$FIXTURE_DIR/adapter_capacity" \
        "$SERVICE" >/dev/null 2>&1 &
    TEST_PID=$!
    sleep 3

    printf '%-22s cap=%-4s ' "$label" "$cap"
    "$PROBE" "$INSTANCE" 2>&1 | grep -E 'getFastCharge|getPsyChargeTech|getPsyFastChgType' \
        | tr -s ' ' | tr '\n' ' '
    echo
}

run_case 5   "normal"
run_case 32  "below boundary"
run_case 33  "fast charge (min)"
run_case 44  "fast charge (max)"
run_case 45  "supervooc (min)"
run_case 65  "supervooc"
run_case 120 "supervooc (high)"

stop_test_service
echo "charger node mounts (must be 0): $(grep -c 'sys/devices/platform/charger' /proc/mounts)"
echo "shipped service still up: $(service list | grep -c 'vendor.oplus.hardware.charger.ICharger/default')"
