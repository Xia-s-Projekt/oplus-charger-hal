// Unit test for the bypass / charge-limit policy.
//
// Covers the cases from the feature request:
//   * GameSpace toggle on, target 20, battery 50 -> node 1
//   * GameSpace toggle off                        -> node 0
//   * GameSpace toggle on, target 40, battery 20 -> node 0 (still charging)
//   * GameSpace toggle on, target 40, battery 40 -> node 1
//   * charge limit 80, battery 79                 -> node 0
//   * charge limit 80, battery 80                 -> node 1
//
// Build and run with standalone/tests/build.sh.
#include <cstdio>
#include <string>

#include "charger/BypassPolicy.h"

namespace {

using vendor::oplus::hardware::charger::bypass::Inputs;
using vendor::oplus::hardware::charger::bypass::decide;
using vendor::oplus::hardware::charger::bypass::parseGameToggle;
using vendor::oplus::hardware::charger::bypass::parseUpLimit;

int gFailures = 0;

void check(const char* name, int expected, int actual) {
    if (expected == actual) {
        printf("PASS  %s -> %d\n", name, actual);
    } else {
        printf("FAIL  %s -> %d (expected %d)\n", name, actual, expected);
        ++gFailures;
    }
}

// Runs the policy for the given state and returns the node value.
int nodeFor(int gameSwitch, int gameTarget, int upLimitEnabled, int upLimitPercent, int capacity) {
    Inputs inputs;
    inputs.gameSwitch = gameSwitch;
    inputs.gameTargetPercent = gameTarget;
    inputs.upLimitEnabled = upLimitEnabled;
    inputs.upLimitPercent = upLimitPercent;
    inputs.capacity = capacity;
    int value = -1;
    decide(inputs, &value);
    return value;
}

}  // namespace

int main() {
    // The COSA payloads.
    int target = 0;
    check("parse 1+switch=1", 1, parseGameToggle("1+switch=1", &target));
    check("parse 1+switch=0", 0, parseGameToggle("1+switch=0", &target));
    check("parse 1+switch=1 target", 0, target);
    check("parse 20+switch=1", 1, parseGameToggle("20+switch=1", &target));
    check("parse 20+switch=1 target", 20, target);

    int state = -1;
    int percent = 0;
    parseUpLimit("2++0+80", &state, &percent);
    check("upLimit state", 0, state);
    check("upLimit percent", 80, percent);
    parseUpLimit("2++1+80", &state, &percent);
    check("upLimit state on", 1, state);
    check("upLimit percent on", 80, percent);

    // GameSpace toggle, target 20.
    check("game on, target 20, battery 50", 1, nodeFor(1, 20, -1, 0, 50));
    check("game off", 0, nodeFor(0, 20, -1, 0, 50));
    check("game on, target 40, battery 20", 0, nodeFor(1, 40, -1, 0, 20));
    check("game on, target 40, battery 40", 1, nodeFor(1, 40, -1, 0, 40));
    check("game on, target 40, battery 41", 1, nodeFor(1, 40, -1, 0, 41));
    check("game on, target 40, battery 39", 0, nodeFor(1, 40, -1, 0, 39));

    // Charge limit only.
    check("limit 80, battery 79", 0, nodeFor(-1, 0, 1, 80, 79));
    check("limit 80, battery 80", 1, nodeFor(-1, 0, 1, 80, 80));
    check("limit off, battery 90", 0, nodeFor(-1, 0, 0, 80, 90));

    // The GameSpace toggle with no target of its own follows the charge limit.
    check("game on, limit 80, battery 79", 0, nodeFor(1, 0, 1, 80, 79));
    check("game on, limit 80, battery 80", 1, nodeFor(1, 0, 1, 80, 80));

    // Unknown capacity: nothing engages.
    check("game on, target 40, capacity unknown", 0, nodeFor(1, 40, -1, 0, -1));

    if (gFailures == 0) {
        printf("all policy tests passed\n");
        return 0;
    }
    printf("%d policy test(s) failed\n", gFailures);
    return 1;
}
