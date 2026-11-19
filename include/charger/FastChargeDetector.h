// FastChargeDetector.h - fast-charge / SuperVOOC classification.
//
// WHY THIS EXISTS
// ---------------
// When the kernel oplus_chg driver is present it publishes the charge
// technology and power in sysfs:
//     /sys/class/power_supply/battery/fast_charge        0/1
//     /sys/class/power_supply/battery/charge_technology  "VOOC"/"SVOOC"/...
//     /sys/class/oplus_chg/battery/ppschg_power          watts
//
// When those nodes are absent, the detector falls back to the charger-driver
// nodes:
//     /sys/devices/platform/charger/Pump_Express      1 = fast-charge pump on
//     /sys/devices/platform/charger/adapter_capacity  adapter output in watts
//
//     >= 45 W  -> SuperVOOC   (charge technology SVOOC, id 2)
//     >= 33 W  -> fast charge (charge technology VOOC,  id 1)
//     <  33 W  -> normal
//
// The watts decide the charge technology. The pump state is a separate signal,
// reported on its own; it does not gate the classification, because an adapter
// can negotiate 33 W or more without the VOOC pump.
#pragma once

#include <mutex>
#include <string>

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {

struct FastChargeState {
    int fastChgType = 0;   // charge technology id, see ChargeTechnology
    int fastCharge = 0;    // 1 when a fast charge is in progress
    int watts = 0;         // measured charging power in watts
    bool pumpEngaged = false;  // Pump_Express state
    bool valid = false;    // false when no source could be read
    bool fromOplusDriver = false;  // true when the oplus_chg driver answered
};

// The sysfs nodes the detector reads.  Defaults are the real device paths;
// the struct exists so a test can point the detector at fixture files.
struct FastChargeNodes {
    const char* oplusChargeTechnology = nullptr;  // nullptr -> use the default
    const char* oplusFastCharge = nullptr;
    const char* oplusPpschgPower = nullptr;
    const char* powerSupplyChargeTechnology = nullptr;
    const char* powerSupplyFastCharge = nullptr;
    const char* pumpExpress = nullptr;
    const char* adapterCapacity = nullptr;
    const char* fastChgIndicator = nullptr;
};

class FastChargeDetector {
public:
    FastChargeDetector();
    explicit FastChargeDetector(const FastChargeNodes& nodes);

    // Re-read every source and update the cached state.
    void refresh();

    // Cached accessors - these are what the AIDL getters return.
    int getFastCharge() const;
    int getFastChgType() const;
    int getChargeTechnology() const;
    int getPowerWatts() const;
    bool isPumpEngaged() const;

    FastChargeState state() const;

    // "VOOC" -> 1, "SVOOC" -> 2, "PD" -> 3, "QC" -> 4, "PPS" -> 5, "UFCS" -> 6.
    // Names and ids match the vendor charger driver's technology table.
    static int technologyFromName(const std::string& name);
    static const char* technologyName(int id);

    // Exposed for the host-side test harness.
    static int classifyWatts(int watts);

private:
    bool readOplusDriver(FastChargeState* state) const;
    bool readTranCharger(FastChargeState* state) const;

    // The resolved node paths: the defaults, or the test's overrides.
    std::string mOplusChargeTechnology;
    std::string mOplusFastCharge;
    std::string mOplusPpschgPower;
    std::string mPowerSupplyChargeTechnology;
    std::string mPowerSupplyFastCharge;
    std::string mPumpExpress;
    std::string mAdapterCapacity;
    std::string mFastChgIndicator;

    mutable std::mutex mLock;
    FastChargeState mState;
};

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
