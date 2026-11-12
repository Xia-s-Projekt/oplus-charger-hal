// constants.rs - the values in include/charger/ChargerConstants.h, plus the
// node paths the accessors use.  Nothing here is invented: every path and
// threshold was read out of the shipped binary or the C++ sources.
//
// 130 AIDL methods.  The transaction code is `index + 1`, because
// ICharger.cpp emits `FIRST_CALL_TRANSACTION + N` and FIRST_CALL_TRANSACTION
// is android.os.IBinder.FIRST_CALL_TRANSACTION == 1.

pub const K_READ_ERROR: i32 = i32::MIN; // 0x80000000, open() failed / atoi() failed
pub const K_IO_ERROR: i32 = i32::MIN + 2; // 0x80000002, read() failed
pub const K_PARSE_ERROR: i32 = i32::MIN; // 0x80000000, atoi() could not parse
pub const K_WRITE_ERROR: i32 = 0x8000_0003u32 as i32; // write() returned < 0

pub const K_FAST_CHARGE_WATTS: i32 = 33;
pub const K_SUPER_VOOC_WATTS: i32 = 45;
pub const K_MIN_REPORT_WATTS: i32 = 25;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ChargeTechnology {
    Unknown = 0,
    Vooc = 1,
    Svooc = 2,
    Pd = 3,
    Qc = 4,
    Pps = 5,
    Ufcs = 6,
}

impl ChargeTechnology {
    pub fn from_id(id: i32) -> Self {
        match id {
            1 => Self::Vooc,
            2 => Self::Svooc,
            3 => Self::Pd,
            4 => Self::Qc,
            5 => Self::Pps,
            6 => Self::Ufcs,
            _ => Self::Unknown,
        }
    }
}

pub const TECH_NAMES: [&str; 7] = ["UNKNOWN", "VOOC", "SVOOC", "PD", "QC", "PPS", "UFCS"];

pub fn technology_name(id: i32) -> &'static str {
    let idx = if (0..7).contains(&id) { id as usize } else { 0 };
    TECH_NAMES[idx]
}

pub fn technology_from_name(name: &str) -> ChargeTechnology {
    for (i, n) in TECH_NAMES.iter().enumerate().skip(1) {
        if name == *n {
            return ChargeTechnology::from_id(i as i32);
        }
    }
    ChargeTechnology::Unknown
}

// The internal id the detector reports is not what the AIDL caller sees:
// SVOOC->3, VOOC->2, everything else that charges->1, unknown->0.
pub fn to_external_charge_technology(internal: ChargeTechnology) -> i32 {
    match internal {
        ChargeTechnology::Svooc => 3,
        ChargeTechnology::Vooc => 2,
        ChargeTechnology::Pd | ChargeTechnology::Qc | ChargeTechnology::Pps
        | ChargeTechnology::Ufcs => 1,
        ChargeTechnology::Unknown => 0,
    }
}

// Node paths.
pub mod tran {
    pub const PUMP_EXPRESS: &str = "/sys/devices/platform/charger/Pump_Express";
    pub const ADAPTER_CAPACITY: &str = "/sys/devices/platform/charger/adapter_capacity";
    pub const OTG_CTL: &str = "/sys/devices/platform/odm/odm:tran_battery/OTG_CTL";
    pub const BYPASS_CHARGER: &str = "/sys/devices/platform/charger/bypass_charger";
    pub const TRAN_AICHG_DISABLE_CHARGER: &str =
        "/sys/devices/platform/charger/tran_aichg_disable_charger";
    pub const BYPASS_PROP: &str = "persist.sys.rianixia.bypass_charge.state";
}

pub const TEST_KIT_DEVICE: &str = "/dev/chg_test_kit";
pub const IOCTL_GET_NAME: u32 = 0x4004f601;
pub const IOCTL_GET_FEATURE_LIST: u32 = 0x9000f604;
pub const IOCTL_GET_FEATURE_NUM: u32 = 0x8004f605;
pub const IOCTL_RUN_TEST: u32 = 0x9000f606;

// The decimal-SOH ramp the shipped healthd_update_ui_soc_decimal walks.
pub const K_DECIMAL_TARGETS: [i32; 32] = [
    5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 63, 66, 69, 72, 75, 78, 80, 82, 84, 86, 88, 90,
    91, 92, 93, 94, 95, 96, 97, 98,
];
pub const K_NUM_DECIMAL_TARGETS: usize = 32;

pub const LOG_TAG: &str = "vendor.oplus.hardware.charger-V1-service";
pub const SERVICE_NAME: &str = "vendor.oplus.hardware.charger.ICharger";
