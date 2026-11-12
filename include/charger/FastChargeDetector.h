// FastChargeDetector.h - fast-charge / SuperVOOC classification.
//
// WHY THIS EXISTS
// ---------------
// The shipped Oplus HAL does not classify charging power itself.  It reports
// what the kernel's oplus_chg driver published:
//     /sys/class/power_supply/battery/fast_charge        0/1
//     /sys/class/power_supply/battery/charge_technology  "VOOC"/"SVOOC"/...
//     /sys/class/oplus_chg/battery/ppschg_power          watts
// (evidence: RE/evidence/oplus-charger/.../decompiled/getFastCharge__00122050.c,
//  getPsyChargeTech__0011c2c4.c, getBattPPSChgPower__0011d160.c)
//
// This port runs the MTK/Transsion charger driver, which has no oplus_chg
// driver, so those nodes are absent.  The Transsion charger HAL
// (vendor.hardware.trancharge-service) solves the same problem: its handler
// table entry 0 "PUMPSTAT" (fid 0x0) runs TransferPump_Express, which reads
//
//     /sys/devices/platform/charger/Pump_Express      1 = fast-charge pump on
//     /sys/devices/platform/charger/adapter_capacity  adapter output in watts
//
// and stores the two values in TranChargeInfo at +0x18 and +0x1c
// (evidence: RE/evidence/trancharge/handler_table.txt,
//  RE/evidence/trancharge/.../decompiled/TransferPump_Express__00108240.c).
//
// This detector is that Transsion reading, applied to the Oplus HAL's
// interface: the watts are classified here so the Oplus contract holds on a
// port without the Oplus charger driver.
//
//     >= 45 W  -> SuperVOOC   (charge technology SVOOC, id 2)
//     >= 33 W  -> fast charge (charge technology VOOC,  id 1)
//     <  33 W  -> normal
//
// The watts decide the charge technology.  The Transsion pump state is a
// separate signal, reported on its own; it does not gate the classification,
// because an adapter can negotiate 33 W or more without the VOOC pump.
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
    bool pumpEngaged = false;  // Transsion Pump_Express state
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
    // The table is the shipped binary's own {char* name; int id} table at file
    // offset 0x510c0 (stride 0x28).
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
