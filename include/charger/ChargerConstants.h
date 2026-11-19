// ChargerConstants.h - sysfs paths, charge-technology ids and the fast-charge
// thresholds used by the OplusCharger HAL.
//
// Each getter lists its nodes in fallback order: power_supply first, then
// oplus_chg.
#pragma once

#include <string>

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {

// ---------------------------------------------------------------------------
// Charge technology ids
//
// The charge technologies reported by the vendor charger driver:
//   VOOC = 1, SVOOC = 2, PD = 3, QC = 4, PPS = 5, UFCS = 6,
// with UNKNOWN = 0.
// ---------------------------------------------------------------------------
enum ChargeTechnology {
    CHG_TECH_UNKNOWN = 0,
    CHG_TECH_VOOC = 1,
    CHG_TECH_SVOOC = 2,
    CHG_TECH_PD = 3,
    CHG_TECH_QC = 4,
    CHG_TECH_PPS = 5,
    CHG_TECH_UFCS = 6,
};

// Name -> id, as spelled by the vendor charger driver.
const char* chargeTechnologyName(int id);

// ---------------------------------------------------------------------------
// Fast-charge thresholds (watts).
//
// When the kernel oplus_chg driver is present it publishes the charge
// technology in /sys/class/power_supply/battery/{fast_charge,
// charge_technology} and its verdict wins. Otherwise the thresholds below
// are applied here (see FastChargeDetector.h).
//
// 33 W  -> "fast charge"   (charge technology VOOC)
// 45 W  -> "SuperVOOC"     (charge technology SVOOC)
// ---------------------------------------------------------------------------
const int kFastChargeWatts = 33;
const int kSuperVoocWatts = 45;

// Anything below this is not a real adapter rating: it is the 5 W / 10 W
// baseline a stock charger reports, and the framework would show it as a
// charge wattage.  Report 0 instead.
const int kMinReportWatts = 25;

// ---------------------------------------------------------------------------
// Oplus native nodes (present only when the oplus_chg driver is loaded).
// ---------------------------------------------------------------------------
namespace node {
const char kPowerSupplyBattery[] = "/sys/class/power_supply/battery/";
const char kOplusChgBattery[] = "/sys/class/oplus_chg/battery/";
const char kOplusChgUsb[] = "/sys/class/oplus_chg/usb/";
const char kOplusChgWireless[] = "/sys/class/oplus_chg/wireless/";
const char kOplusChgCommon[] = "/sys/class/oplus_chg/common/";
const char kOplusChgAc[] = "/sys/class/oplus_chg/ac/";
const char kProcCharger[] = "/proc/charger/";
const char kProcWireless[] = "/proc/wireless/";
const char kPowerSupplyUsb[] = "/sys/class/power_supply/usb/";
const char kPowerSupplyAc[] = "/sys/class/power_supply/ac/";
const char kPowerSupplyCharger[] = "/sys/class/power_supply/charger/";
const char kPowerSupplyWireless[] = "/sys/class/power_supply/wireless/";
const char kPowerSupplyBatteryRoot[] = "/sys/class/power_supply/battery/";
}  // namespace node

// ---------------------------------------------------------------------------
// Charger driver nodes used as the fast-charge source when oplus_chg is not
// available:
//   "/sys/devices/platform/charger/Pump_Express"
//       fast-charge pump state (1 = engaged)
//   "/sys/devices/platform/charger/adapter_capacity"
//       adapter output in watts
// ---------------------------------------------------------------------------
namespace tran {
const char kPumpExpress[] = "/sys/devices/platform/charger/Pump_Express";
const char kAdapterCapacity[] = "/sys/devices/platform/charger/adapter_capacity";
const char kChgSpeedCtl[] = "/sys/devices/platform/charger/chgspeed_ctl";
const char kChargerRoot[] = "/sys/devices/platform/charger/";
const char kOtgCtl[] = "/sys/devices/platform/odm/odm:tran_battery/OTG_CTL";
const char kBypassCharger[] = "/sys/devices/platform/charger/bypass_charger";
const char kTranAichgDisableCharger[] = "/sys/devices/platform/charger/tran_aichg_disable_charger";
const char kBypassProp[] = "persist.sys.rianixia.bypass_charge.state";
}  // namespace tran

#if defined(__ANDROID__)
#include <sys/system_properties.h>

inline bool isBypassPropEnabled() {
    char val[PROP_VALUE_MAX] = {0};
    __system_property_get("persist.sys.rianixia.bypass_charge.state", val);
    return (val[0] == '1');
}

// Keeps the prop in step with the node.  init applies the prop to the node on
// every boot (tran_optimize.rc), so a HAL that only writes the node loses the
// setting on reboot; writing both keeps them in sync.
inline void setBypassProp(int value) {
    __system_property_set("persist.sys.rianixia.bypass_charge.state",
                          value == 1 ? "1" : "0");
}
#else
inline bool isBypassPropEnabled() {
    return false;
}
#endif

// ---------------------------------------------------------------------------
// Error sentinels - the values the getters return when a node cannot
// be read.  Verified in the decompiled bodies of getAcType/getFastCharge:
//   open() failed            -> INT32_MIN  (0x80000000)
//   read() failed            -> INT32_MIN + 1 (0x80000002)
//   atoi() could not parse   -> INT32_MIN
// ---------------------------------------------------------------------------
const int kReadError = (int)0x80000000;      // -2147483648
const int kIoError = (int)0x80000002;        // -2147483646
const int kParseError = (int)0x80000000;

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
