// classify_test.cpp - device-side proof of the 33 W / 45 W classification.
//
// It drives the REAL FastChargeDetector (the same class the HAL uses) against
// fixture files, so the numbers printed here are produced by the shipped
// logic, not by a re-implementation of the thresholds.
//
// The fixtures stand in for the two device nodes:
//   <dir>/pump   - /sys/devices/platform/charger/Pump_Express
//   <dir>/cap    - /sys/devices/platform/charger/adapter_capacity
// and for the oplus driver nodes:
//   <dir>/tech   - charge_technology  ("VOOC"/"SVOOC"/...)
//   <dir>/fast   - fast_charge        (0/1)
//
// Usage:  classify_test <fixture-dir>
//
// Expected classification:
//   < 33 W   -> UNKNOWN (0)  normal
//   33..44 W -> VOOC    (1)  fast charge
//   >= 45 W  -> SVOOC   (2)  SuperVOOC
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include "charger/FastChargeDetector.h"

namespace {

using vendor::oplus::hardware::charger::FastChargeDetector;
using vendor::oplus::hardware::charger::FastChargeNodes;
using vendor::oplus::hardware::charger::FastChargeState;

int g_failures = 0;

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    out << content;
}

std::string fixture(const std::string& dir, const char* name) {
    return dir + "/" + name;
}

// Drives one case through the real detector.
FastChargeState runCase(const std::string& dir, const std::string& tech, int fast,
                         const std::string& pump, const std::string& cap) {
    const std::string techPath = fixture(dir, "tech");
    const std::string fastPath = fixture(dir, "fast");
    const std::string pumpPath = fixture(dir, "pump");
    const std::string capPath = fixture(dir, "cap");
    const std::string indicatorPath = fixture(dir, "fast_indicator");

    writeFile(techPath, tech);
    writeFile(fastPath, std::to_string(fast));
    writeFile(pumpPath, pump);
    writeFile(capPath, cap);
    writeFile(indicatorPath, "0");

    FastChargeNodes nodes;
    nodes.oplusChargeTechnology = techPath.c_str();
    nodes.oplusFastCharge = fastPath.c_str();
    nodes.oplusPpschgPower = nullptr;  // not present -> not read
    nodes.powerSupplyChargeTechnology = nullptr;
    nodes.powerSupplyFastCharge = nullptr;
    nodes.pumpExpress = pumpPath.c_str();
    nodes.adapterCapacity = capPath.c_str();
    nodes.fastChgIndicator = indicatorPath.c_str();

    FastChargeDetector detector(nodes);
    return detector.state();
}

void expect(const char* label, const std::string& dir, const std::string& tech, int fast,
            const std::string& pump, const std::string& cap, int wantType, const char* wantName) {
    FastChargeState state = runCase(dir, tech, fast, pump, cap);
    bool ok = (state.fastChgType == wantType);
    printf("%s  %-28s type=%d (%s)  fastCharge=%d  watts=%d  pump=%d  valid=%d\n",
           ok ? "OK  " : "FAIL",
           label, state.fastChgType, FastChargeDetector::technologyName(state.fastChgType),
           state.fastCharge, state.watts, state.pumpEngaged ? 1 : 0, state.valid ? 1 : 0);
    if (!ok) {
        printf("      expected type=%d (%s)\n", wantType, wantName);
        ++g_failures;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: %s <fixture-dir>\n", argv[0]);
        return 2;
    }
    const std::string dir = argv[1];

    // The pure threshold table, straight through the shipped classifier.
    printf("== classifyWatts threshold table ==\n");
    struct Case { int watts; int want; const char* name; };
    const Case cases[] = {
            {0, 0, "UNKNOWN"},  {20, 0, "UNKNOWN"}, {32, 0, "UNKNOWN"},
            {33, 1, "VOOC"},    {34, 1, "VOOC"},   {44, 1, "VOOC"},
            {45, 2, "SVOOC"},   {65, 2, "SVOOC"},  {100, 2, "SVOOC"},
    };
    for (const Case& c : cases) {
        int got = FastChargeDetector::classifyWatts(c.watts);
        bool ok = (got == c.want);
        printf("%s  %3d W -> type=%d (%-7s) expected %d (%s)\n",
               ok ? "OK  " : "FAIL", c.watts, got,
               FastChargeDetector::technologyName(got), c.want, c.name);
        if (!ok) {
            ++g_failures;
        }
    }

    printf("\n== end-to-end through the detector (Transsion nodes) ==\n");
    // No oplus driver on this port: the detector must fall back to the
    // Transsion pair and classify the watts itself.
    expect("normal 5 W", dir, "", 0, "0", "5", 0, "UNKNOWN");
    expect("fast charge 33 W", dir, "", 0, "1", "33", 1, "VOOC");
    expect("fast charge 40 W", dir, "", 0, "1", "40", 1, "VOOC");
    expect("supervooc 45 W", dir, "", 0, "1", "45", 2, "SVOOC");
    expect("supervooc 65 W", dir, "", 0, "1", "65", 2, "SVOOC");
    expect("supervooc 120 W", dir, "", 0, "1", "120", 2, "SVOOC");

    printf("\n== oplus driver present: the kernel's own verdict wins ==\n");
    expect("oplus says VOOC", dir, "VOOC\n", 1, "0", "0", 1, "VOOC");
    expect("oplus says SVOOC", dir, "SVOOC\n", 1, "0", "0", 2, "SVOOC");
    expect("oplus says PD", dir, "PD\n", 0, "0", "0", 3, "PD");

    printf("\n%s: %d failure(s)\n", g_failures == 0 ? "PASS" : "FAIL", g_failures);
    return g_failures == 0 ? 0 : 1;
}
