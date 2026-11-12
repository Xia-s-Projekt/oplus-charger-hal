// watt_threshold_test.cpp - host-side proof of the 25 W reporting threshold.
//
// The charging animation prints the watt number only when the value the
// framework received parses to a positive number, so an adapter rated below
// the threshold has to be reported as 0 W.  This drives the same inline
// function the HAL uses, on the host, over the interesting values.
//
// Usage:  ./watt_threshold_test
#include <cstdio>

#include "charger/ChargerConstants.h"

using vendor::oplus::hardware::charger::kMinReportedWatts;
using vendor::oplus::hardware::charger::reportedWatts;

namespace {

int g_failures = 0;

void expect(const char* label, int watts, int want) {
    int got = reportedWatts(watts);
    bool ok = (got == want);
    printf("%s  watts=%-4d reported=%-4d want=%-4d (threshold %d)\n",
           ok ? "OK  " : "FAIL", watts, got, want, kMinReportedWatts);
    if (!ok) {
        g_failures++;
    }
}

}  // namespace

int main() {
    expect("not charging", 0, 0);
    expect("5 W adapter", 5, 0);
    expect("just below threshold", kMinReportedWatts - 1, 0);
    expect("at threshold", kMinReportedWatts, kMinReportedWatts);
    expect("above threshold", 33, 33);
    expect("super vooc", 45, 45);
    expect("pps", 100, 100);

    if (g_failures == 0) {
        printf("ALL CHECKS PASSED\n");
        return 0;
    }
    printf("%d CHECK(S) FAILED\n", g_failures);
    return 1;
}
