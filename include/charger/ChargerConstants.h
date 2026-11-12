// ChargerConstants.h - sysfs paths, charge-technology ids and the fast-charge
// thresholds used by the OplusCharger HAL.
//
// Every path here was read out of the shipped binary
// (erofs/vendor/bin/hw/vendor.oplus.hardware.charger-V3-service): the
// "strings" column of RE/evidence/oplus-charger/.../index.tsv is the exact
// node list of each getter, in the exact fallback order the shipped code
// uses (power_supply first, oplus_chg second).
#pragma once

#include <string>

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {

// ---------------------------------------------------------------------------
// Charge technology ids
//
// Recovered from the {char* name; int id} table at file offset 0x510c0 of the
// shipped binary (stride 0x28, six entries):
//   0x510c0 'SVOOC' id=2   0x51110 'PD'   id=3
//   0x510e8 'VOOC'  id=1   0x51138 'QC'   id=4
//                            0x51160 'PPS'  id=5
//                            0x51188 'UFCS' id=6
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
// The shipped Oplus HAL does not compare watts itself: it reports whatever
// the kernel's oplus_chg driver published in
// /sys/class/power_supply/battery/{fast_charge,charge_technology}.  This
// port runs the MTK/Transsion charger driver, which has no oplus_chg driver,
// so the watt thresholds are applied here instead - the same place the
// Transsion trancharge HAL applies them (see FastChargeDetector.h).
//
// 33 W  -> "fast charge"   (charge technology VOOC)
// 45 W  -> "SuperVOOC"     (charge technology SVOOC)
// ---------------------------------------------------------------------------
const int kFastChargeWatts = 33;
const int kSuperVoocWatts = 45;

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
// Transsion charger driver nodes - the fast-charge source on this port.
//
// From the trancharge HAL (vendor.hardware.trancharge-service), handler table
// entry 0 "PUMPSTAT" (fid 0x0), handler TransferPump_Express:
//   node  = "/sys/devices/platform/charger/Pump_Express;"
//           "/sys/devices/platform/charger/adapter_capacity"
//   the first value is the fast-charge pump state (1 = engaged),
//   the second is the adapter output in watts.
// Evidence: RE/evidence/trancharge/handler_table.txt
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
// Error sentinels - the values the shipped getters return when a node cannot
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
