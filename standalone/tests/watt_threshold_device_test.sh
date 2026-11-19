#!/system/bin/sh
# On-device proof of the 25 W reporting threshold.
#
# The HAL is started as a separate binder instance with the charger nodes
# pointed at fixture files, then getChgConfig(1) - the value the
# framework turns into the "chargewattage" broadcast extra that the charging
# animation prints - is read back over binder.
#
# The shipped service is not touched: the test instance uses its own instance
# name and is killed by PID.
REMOTE_DIR=/data/local/tmp
SERVICE="$REMOTE_DIR/vendor.oplus.hardware.charger-V6-service"
PROBE="$REMOTE_DIR/probe"
FIXTURE_DIR="$REMOTE_DIR/chgfix"
INSTANCE=vendor.oplus.hardware.charger.ICharger/rebuilt

mkdir -p "$FIXTURE_DIR"
chmod 755 "$FIXTURE_DIR"

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
    OPLUS_CHARGER_INSTANCE=rebuilt \
    OPLUS_CHARGER_TECH_NODE="$FIXTURE_DIR/absent-tech" \
    OPLUS_CHARGER_FAST_NODE="$FIXTURE_DIR/absent-fast" \
    OPLUS_CHARGER_PUMP_NODE="$FIXTURE_DIR/Pump_Express" \
    OPLUS_CHARGER_CAPACITY_NODE="$FIXTURE_DIR/adapter_capacity" \
        "$SERVICE" >/dev/null 2>&1 &
    TEST_PID=$!
    sleep 3

    printf '%-24s cap=%-4s ' "$label" "$cap"
    "$PROBE" "$INSTANCE" 2>&1 | grep -E 'getChgConfig\(1\)|getChgConfig\(4\)' \
        | tr -s ' ' | tr '\n' ' '
    echo
}

run_case 5   "below threshold"
run_case 24  "just below threshold"
run_case 25  "at threshold"
run_case 33  "fast charge (min)"
run_case 45  "supervooc (min)"
run_case 120 "supervooc (high)"

stop_test_service
echo "shipped service still up: $(service list | grep -c 'vendor.oplus.hardware.charger.ICharger/default')"
