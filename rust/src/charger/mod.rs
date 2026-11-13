// charger/mod.rs - the Charger object: shared state plus the hand-written
// methods.  The node-backed accessors are generated into accessors.rs.
//
// Threading: the AIDL calls arrive on the binder thread pool, the monitor
// loop runs on its own thread, and the FastChargeDetector is shared between
// them.  Every field that both touch is behind a Mutex, exactly as the C++
// FastChargeDetector is.

use std::collections::HashMap;
use std::sync::atomic::{AtomicI32, Ordering};
use std::sync::Mutex;

mod accessors;
mod bridge;
use crate::bypass::{self, BypassInputs};
use crate::constants;
use crate::decimal::DecimalState;
use crate::detection::ChgDetectionCheck;
use crate::fastcharge::FastChargeDetector;
use crate::nodes;
use crate::props;
use crate::txn::TestKitResult;
use crate::testkit::KernelTest;

/// The shared HAL state, mirroring struct ChargerState in ChargerManual.cpp.
pub struct ChargerState {
    pub custom_select_chg_mode: i32,
    pub smart_chg_mode: i32,
    pub bcc_exp_status: i32,
    pub bms_heating_status: i32,
    pub bms_heating_running_status: i32,
    pub chg_exchange_mesg_init: i32,
    pub chg_exchange_soh_mesg_init: i32,
    pub ui_soh_value: i32,
    pub screen_status: i32,
    pub battery_log_push: i32,
    pub chg_rus_config: i32,
    pub wireless_pen_soc: i32,
    pub wls_third_partition_info: i32,
    pub uisoh_debug_info: String,
    pub chg_config: HashMap<i32, String>,
    pub usb_eye_diagram: HashMap<i32, String>,
    pub bypass: BypassInputs,
    /// MMI factory-mode charging disable (setPsyMmiChgEn).  A request on the
    /// bypass node, kept out of the policy Inputs and folded into the
    /// decision by apply_bypass_policy.
    pub mmi_charging_disabled: i32,
}

impl Default for ChargerState {
    fn default() -> Self {
        Self {
            custom_select_chg_mode: 0,
            smart_chg_mode: 0,
            bcc_exp_status: 0,
            bms_heating_status: 0,
            bms_heating_running_status: 0,
            chg_exchange_mesg_init: 0,
            chg_exchange_soh_mesg_init: 0,
            ui_soh_value: 0,
            screen_status: 0,
            battery_log_push: 0,
            chg_rus_config: 0,
            wireless_pen_soc: 0,
            wls_third_partition_info: 0,
            uisoh_debug_info: String::new(),
            chg_config: HashMap::new(),
            usb_eye_diagram: HashMap::new(),
            bypass: BypassInputs::default(),
            mmi_charging_disabled: 0,
        }
    }
}

/// sOtgSwitchState in Charger.cpp: the last value written through
/// setPsyOtgSwitch, used when the node cannot be read back.
static OTG_SWITCH_STATE: AtomicI32 = AtomicI32::new(0);

pub struct Charger {
    pub(crate) state: Mutex<ChargerState>,
    fast_charge: Mutex<FastChargeDetector>,
    detection: Mutex<ChgDetectionCheck>,
    decimal: Mutex<DecimalState>,
    running: std::sync::atomic::AtomicBool,
}

impl Charger {
    pub fn new() -> Self {
        Self {
            state: Mutex::new(ChargerState::default()),
            fast_charge: Mutex::new(FastChargeDetector::new()),
            detection: Mutex::new(ChgDetectionCheck::new()),
            decimal: Mutex::new(DecimalState::new()),
            running: std::sync::atomic::AtomicBool::new(false),
        }
    }

    pub(crate) fn running(&self) -> bool {
        self.running.load(Ordering::Relaxed)
    }

    pub fn set_running(&self, v: bool) {
        self.running.store(v, Ordering::Relaxed);
    }

    pub(crate) fn lock_state(&self) -> std::sync::MutexGuard<'_, ChargerState> {
        // A poisoned lock still holds valid state; the HAL must not panic.
        self.state.lock().unwrap_or_else(|e| e.into_inner())
    }

    // ---- shared helpers -------------------------------------------------

    pub(crate) fn read_int_path(path: &str) -> i32 {
        nodes::read_int_path(path)
    }

    pub(crate) fn read_string_path(path: &str) -> String {
        nodes::read_string_path(path)
    }

    pub(crate) fn write_string_path(path: &str, value: &str) -> bool {
        nodes::write_string_path(path, value)
    }

    fn append_int(out: &mut String, key: &str, value: i32) {
        out.push_str(key);
        out.push('=');
        out.push_str(&value.to_string());
        out.push('\n');
    }

    fn append_string(out: &mut String, key: &str, value: &str) {
        out.push_str(key);
        out.push('=');
        out.push_str(value);
        out.push('\n');
    }

    fn fast_charge_refresh(&self) {
        if let Ok(mut fc) = self.fast_charge.lock() {
            fc.refresh();
        }
    }

    /// The monitor loop needs this from outside the Charger impl.
    pub(crate) fn fast_charge_refresh_pub(&self) {
        self.fast_charge_refresh();
    }

    /// resetDecimalState(): the ramp restarts on every power transition.
    pub(crate) fn decimal_reset(&self) {
        self.decimal
            .lock()
            .unwrap_or_else(|e| e.into_inner())
            .reset();
    }

    fn fast_charge_fast(&self) -> i32 {
        self.fast_charge
            .lock()
            .map(|f| f.get_fast_charge())
            .unwrap_or(0)
    }

    fn fast_charge_type(&self) -> constants::ChargeTechnology {
        self.fast_charge
            .lock()
            .map(|f| f.get_fast_chg_type())
            .unwrap_or(constants::ChargeTechnology::Unknown)
    }

    fn fast_charge_watts(&self) -> i32 {
        self.fast_charge
            .lock()
            .map(|f| f.get_power_watts())
            .unwrap_or(0)
    }

    fn fast_charge_pump(&self) -> bool {
        self.fast_charge
            .lock()
            .map(|f| f.is_pump_engaged())
            .unwrap_or(false)
    }

    fn external_charge_technology(&self) -> i32 {
        let internal = if self.fast_charge.lock().is_ok() {
            self.fast_charge_type()
        } else {
            constants::ChargeTechnology::Unknown
        };
        constants::to_external_charge_technology(internal)
    }

    // ---- the bypass node ------------------------------------------------

    /// Re-derives the bypass node from the live requests.  Returns the value
    /// it decided on, or -1 when nothing is requesting bypass and the node
    /// must be left alone.
    pub(crate) fn apply_bypass_policy(&self, force: bool) -> i32 {
        let any_request;
        let value;
        {
            let mut st = self.lock_state();
            st.bypass.capacity =
                Self::read_int_path("/sys/class/power_supply/battery/capacity");
            let mut v = 0;
            let any = bypass::decide(&st.bypass, &mut v);
            // The MMI factory-mode disable is a request on the same node.
            if st.mmi_charging_disabled != 0 {
                any_request = true;
                value = 1;
            } else {
                any_request = any;
                value = v;
            }
        }
        if !any_request && !force {
            return -1;
        }
        let node = value.to_string();
        Self::write_string_path(constants::tran::BYPASS_CHARGER, &node);
        Self::write_string_path(constants::tran::TRAN_AICHG_DISABLE_CHARGER, &node);
        // The prop is the user's standing intent and only the user's toggle
        // sets it: a transient decision (charge limit reached, GameSpace
        // asked) must not persist, or init re-applies it at the next boot and
        // the bypass engages at any capacity with the in-memory target lost.
        value
    }

    /// The bypass-charge state, /sys/devices/platform/charger/bypass_charger.
    /// The node is the source of truth.
    fn bypass_charge_status() -> i32 {
        let value = Self::read_int_path(constants::tran::BYPASS_CHARGER);
        if nodes::is_error(value) {
            return props::is_bypass_prop_enabled() as i32;
        }
        value
    }

    fn bypass_charge_supported() -> bool {
        nodes::node_exists(constants::tran::BYPASS_CHARGER)
    }

    /// The MTK charger driver only accepts a bare 0 or 1 on the bypass node,
    /// but the Oplus callers pass "1+switch=1".  Take the leading integer.
    fn bypass_node_value(value: &str) -> String {
        let end = value
            .bytes()
            .position(|b| !b.is_ascii_digit())
            .unwrap_or(value.len());
        if end == 0 {
            return String::new();
        }
        value[..end].to_string()
    }

    pub(crate) fn trigger_battery_update(&self, force: bool) {
        crate::monitor::trigger_battery_update(force);
    }
}

// ---------------------------------------------------------------------------
// The 130 AIDL methods.
// ---------------------------------------------------------------------------

impl Charger {
    // --- fast charge ------------------------------------------------------

    pub(crate) fn txn_getFastCharge(&self) -> i32 {
        self.fast_charge_refresh();
        self.fast_charge_fast()
    }

    pub(crate) fn txn_getPsyFastChgType(&self) -> i32 {
        self.fast_charge_refresh();
        if self.fast_charge.lock().is_ok() {
            self.fast_charge_type() as i32
        } else {
            constants::ChargeTechnology::Unknown as i32
        }
    }

    pub(crate) fn txn_getPsyChargeTech(&self) -> i32 {
        self.fast_charge_refresh();
        self.external_charge_technology()
    }

    // Shipped ELF 0x1cf54: reads voocchg_ing, and when the wireless driver
    // holds the status the cached value is returned instead.
    pub(crate) fn txn_getBattVoocChgIng(&self) -> i32 {
        let mut value = Self::read_int_path("/sys/devices/platform/charger/fast_chg_indicator");
        if nodes::is_error(value) {
            value = Self::read_int_path("/sys/class/oplus_chg/battery/voocchg_ing");
            if nodes::is_error(value) {
                value = Self::read_int_path("/sys/devices/platform/charger/fast_chg_indicator");
            }
        }
        if nodes::is_error(value) {
            value = if self.fast_charge_type() == constants::ChargeTechnology::Svooc {
                1
            } else {
                0
            };
        }
        value
    }

    // --- member state -----------------------------------------------------

    pub(crate) fn txn_getCustomSelectChgMode(&self) -> i32 {
        self.lock_state().custom_select_chg_mode
    }

    pub(crate) fn txn_setCustomSelectChgMode(&self, a0: i32, a1: bool) -> i32 {
        self.lock_state().custom_select_chg_mode = if a1 { a0 } else { 0 };
        // A custom charge mode selection is a charging-mode switch, not a
        // bypass request: it must not latch the bypass node on its own.
        0
    }

    pub(crate) fn txn_getSmartChgMode(&self) -> i32 {
        self.lock_state().smart_chg_mode
    }

    pub(crate) fn txn_getBccExpStatus(&self) -> i32 {
        self.lock_state().bcc_exp_status
    }

    pub(crate) fn txn_setChgStatusToBcc(&self, a0: i32) -> i32 {
        self.lock_state().bcc_exp_status = a0;
        0
    }

    pub(crate) fn txn_getBmsHeatingStatus(&self) -> i32 {
        self.lock_state().bms_heating_status
    }

    pub(crate) fn txn_getBmsHeatingRunningStatus(&self) -> String {
        let v = self.lock_state().bms_heating_running_status;
        let mut out = String::new();
        Self::append_int(&mut out, "bms_heating_running_status", v);
        out
    }

    pub(crate) fn txn_chgExchangeMesgInit(&self) -> i32 {
        self.lock_state().chg_exchange_mesg_init = 1;
        0
    }

    pub(crate) fn txn_chgExchangeSohMesgInit(&self) -> i32 {
        self.lock_state().chg_exchange_soh_mesg_init = 1;
        0
    }

    pub(crate) fn txn_notifyScreenStatus(&self, a0: i32) -> i32 {
        self.lock_state().screen_status = a0;
        if a0 == 1 {
            self.fast_charge_refresh();
            self.trigger_battery_update(false);
        }
        0
    }

    pub(crate) fn txn_setChgRusConfig(&self, a0: &str) -> i32 {
        self.lock_state().chg_rus_config = c_atoi(a0.as_bytes());
        0
    }

    pub(crate) fn txn_setWlsThirdPartitionInfo(&self, a0: &str) -> i32 {
        self.lock_state().wls_third_partition_info = c_atoi(a0.as_bytes());
        0
    }


    // --- SOH --------------------------------------------------------------

    pub(crate) fn txn_getUIsohValue(&self) -> i32 {
        let mut soh = Self::read_int_path("/sys/devices/platform/charger/battery_health_percent");
        if soh <= 0 || soh > 100 || nodes::is_error(soh) {
            let cached = self.lock_state().ui_soh_value;
            soh = if cached > 0 { cached } else { 100 };
        }
        soh
    }

    pub(crate) fn txn_updateUiSohToPartion(&self) -> i32 {
        0
    }

    pub(crate) fn txn_setUisohDebugInfo(&self, a0: &str) -> i32 {
        self.lock_state().uisoh_debug_info = a0.to_string();
        0
    }

    pub(crate) fn txn_getUisohDebugParameterInfo(&self) -> String {
        let v = self.lock_state().uisoh_debug_info.clone();
        let mut out = String::new();
        Self::append_string(&mut out, "uisoh_debug_info", &v);
        out
    }


    pub(crate) fn txn_healthd_update_ui_soc_decimal(&self) -> String {
        self.fast_charge_refresh();

        let mut capacity = Self::read_int_path("/sys/class/power_supply/battery/capacity");
        if capacity <= 0 || capacity > 100 || nodes::is_error(capacity) {
            capacity = Self::read_int_path("/sys/class/power_supply/tran-auto-test/capacity");
        }
        if capacity <= 0 || nodes::is_error(capacity) {
            capacity = 50;
        }
        if capacity >= 100 {
            capacity = 99;
        }

        let is_super_vooc = self.fast_charge_type() == constants::ChargeTechnology::Svooc;
        let mut online = Self::read_int_path("/sys/class/power_supply/charger/online");
        if nodes::is_error(online) {
            online = 1;
        }
        let mut status = Self::read_string_path("/sys/class/power_supply/battery/status");
        while status.ends_with('\n') || status.ends_with('\r') || status.ends_with(' ') {
            status.pop();
        }
        let is_discharging = online == 0 && (status == "Discharging" || status == "Not charging");

        let mut dec = self.decimal.lock().unwrap_or_else(|e| e.into_inner());
        if is_discharging || online == 0 || !is_super_vooc {
            dec.reset();
            let val = capacity * 100;
            return format!("{val},{val}");
        }

        let (start_dec, end_dec) = dec.step();
        let start_val = capacity * 100 + start_dec;
        let end_val = capacity * 100 + end_dec;
        format!("{start_val},{end_val}")
    }

    // --- /dev/oplus_chg ---------------------------------------------------

    // Shipped ELF 0x1b54c: open("/dev/oplus_chg"), read 255 bytes, close.
    pub(crate) fn txn_getChargerControl(&self) -> String {
        use std::io::Read;
        let mut file = match std::fs::File::open("/dev/oplus_chg") {
            Ok(f) => f,
            Err(_) => return String::new(),
        };
        let mut buf = [0u8; 256];
        let n = match file.read(&mut buf[..255]) {
            Ok(n) => n,
            Err(_) => return String::new(),
        };
        let end = n.min(255);
        String::from_utf8_lossy(&buf[..end]).into_owned()
    }

    pub(crate) fn txn_setChargerControl(&self, a0: &str) -> i32 {
        if a0.is_empty() || a0.len() > 16 {
            return -1;
        }
        use std::io::Write;
        let mut file = match std::fs::OpenOptions::new()
            .write(true)
            .open("/dev/oplus_chg")
        {
            Ok(f) => f,
            Err(_) => return constants::K_READ_ERROR,
        };
        match file.write(a0.as_bytes()) {
            Ok(n) => n as i32,
            Err(_) => constants::K_WRITE_ERROR,
        }
    }

    // Shipped ELF 0x1c3f0: snprintf into 32 bytes, write to soc_ajust with
    // the NUL terminator included.
    pub(crate) fn txn_nightstandby(&self, a0: i32) -> i32 {
        let buf = format!("{a0}");
        let payload = format!("{buf}\0");
        let bytes = payload.as_bytes();
        let opened = std::fs::OpenOptions::new()
            .write(true)
            .open("/sys/class/power_supply/battery/soc_ajust")
            .ok()
            .or_else(|| {
                std::fs::OpenOptions::new()
                    .write(true)
                    .open("/sys/class/oplus_chg/battery/soc_ajust")
                    .ok()
            });
        use std::io::Write;
        let mut file = match opened {
            Some(f) => f,
            None => return constants::K_READ_ERROR,
        };
        match file.write(bytes) {
            Ok(n) => n as i32,
            Err(_) => constants::K_WRITE_ERROR,
        }
    }

    // --- charger config ---------------------------------------------------

    pub(crate) fn txn_getChgConfig(&self, a0: i32, _a1: &str, a2: i32) -> String {
        self.fast_charge_refresh();
        let result = match a0 {
            0 => self.external_charge_technology(),
            1 => self.fast_charge_watts(),
            4 => self.fast_charge_watts(),
            6 => self.fast_charge_fast(),
            0x18 | 0x19 => {
                // The third argument is the Oplus caller id.  The framework
                // (OplusBatteryService) asks with 1 and wants the live
                // state, while GameSpace (through COSA) asks with 3 and
                // wants the support code, where 3 means the device supports
                // bypass charging.
                if a2 == 3 {
                    if Self::bypass_charge_supported() {
                        3
                    } else {
                        0
                    }
                } else {
                    Self::bypass_charge_status()
                }
            }
            _ => 0,
        };
        result.to_string()
    }

    pub(crate) fn txn_setChgConfig(&self, a0: i32, a1: &str, _a2: i32) -> i32 {
        self.lock_state()
            .chg_config
            .insert(a0, a1.to_string());
        match a0 {
            0x18 | 0x19 => {
                let mut target = 0;
                let sw = bypass::parse_game_toggle(a1, &mut target);
                if sw >= 0 {
                    {
                        let mut st = self.lock_state();
                        st.bypass.game_switch = sw;
                        st.bypass.game_target_percent = target;
                    }
                    self.apply_bypass_policy(false);
                }
            }
            0x5 | 0xe => {
                let mut st = -1;
                let mut percent = 0;
                bypass::parse_up_limit(a1, &mut st, &mut percent);
                {
                    let mut guard = self.lock_state();
                    if st >= 0 {
                        guard.bypass.up_limit_enabled = st;
                    }
                    if percent > 0 {
                        guard.bypass.up_limit_percent = percent;
                    }
                }
                self.apply_bypass_policy(false);
            }
            _ => {}
        }
        0
    }

    // --- USB eye diagram --------------------------------------------------

    pub(crate) fn txn_setUsbEyeDiagram(&self, a0: i32, a1: &str, _a2: bool) -> i32 {
        self.lock_state()
            .usb_eye_diagram
            .insert(a0, a1.to_string());
        0
    }

    pub(crate) fn txn_getUsbCurrentEyeDiagram(&self, a0: i32) -> String {
        self.lock_state()
            .usb_eye_diagram
            .get(&a0)
            .cloned()
            .unwrap_or_default()
    }

    // --- test kit ---------------------------------------------------------

    pub(crate) fn txn_testKitGetFeatureNum(&self) -> i32 {
        let kit = KernelTest::new();
        match kit.feature_num() {
            Some(n) => n as i32,
            None => -1,
        }
    }

    pub(crate) fn txn_testKitGetFeatureList(&self) -> String {
        let kit = KernelTest::new();
        kit.feature_list().unwrap_or_default()
    }

    pub(crate) fn txn_testKitGetFeatureName(&self, a0: i32) -> String {
        let kit = KernelTest::new();
        kit.name(a0 as u32).unwrap_or_default()
    }

    pub(crate) fn txn_testKitFeatureTest(&self, a0: i32) -> TestKitResult {
        let kit = KernelTest::new();
        let (rc, result) = kit.run_test(a0 as u32);
        TestKitResult {
            ret: rc,
            str: result.unwrap_or_default(),
        }
    }

    // --- diagnostic dumps -------------------------------------------------

    pub(crate) fn txn_queryChargeInfo(&self) -> String {
        let mut out = String::new();
        Self::append_int(&mut out, "bcc_exp_status", self.lock_state().bcc_exp_status);
        Self::append_int(
            &mut out,
            "battery_capacity",
            Self::read_int_path("/sys/class/power_supply/battery/capacity"),
        );
        Self::append_int(
            &mut out,
            "battery_voltage_now",
            Self::read_int_path("/sys/class/power_supply/battery/voltage_now"),
        );
        Self::append_int(
            &mut out,
            "battery_voltage_min",
            Self::read_int_path("/sys/class/power_supply/battery/voltage_min"),
        );
        Self::append_int(
            &mut out,
            "battery_temp",
            Self::read_int_path("/sys/class/power_supply/battery/temp"),
        );
        Self::append_int(
            &mut out,
            "battery_current_now",
            Self::read_int_path("/sys/class/power_supply/battery/current_now"),
        );
        Self::append_int(
            &mut out,
            "battery_charge_now",
            Self::read_int_path("/sys/class/power_supply/battery/charge_now"),
        );
        Self::append_int(
            &mut out,
            "battery_sub_current",
            Self::read_int_path("/sys/class/power_supply/battery/current_avg"),
        );
        Self::append_int(
            &mut out,
            "usb_fast_chg_type",
            Self::read_int_path("/sys/class/power_supply/usb/fast_chg_type"),
        );
        Self::append_int(
            &mut out,
            "battery_voocchg_ing",
            Self::read_int_path("/sys/devices/platform/charger/fast_chg_indicator"),
        );
        Self::append_int(
            &mut out,
            "battery_ppschg_ing",
            Self::read_int_path("/sys/devices/platform/charger/fast_chg_indicator"),
        );
        Self::append_int(
            &mut out,
            "battery_ppschg_power",
            Self::read_int_path("/sys/class/oplus_chg/battery/ppschg_power"),
        );
        Self::append_int(
            &mut out,
            "usb_input_current_now",
            Self::read_int_path("/sys/class/power_supply/charger/input_current_limit"),
        );
        Self::append_int(
            &mut out,
            "battery_short_ic_otp_status",
            Self::read_int_path("/sys/class/power_supply/battery/short_ic_otp_status"),
        );
        Self::append_int(
            &mut out,
            "battery_authenticate",
            Self::read_int_path("/sys/class/power_supply/battery/authenticate"),
        );
        Self::append_int(
            &mut out,
            "battery_charge_technology",
            Self::read_int_path("/sys/class/power_supply/battery/charge_technology"),
        );
        Self::append_int(
            &mut out,
            "parallel_chg_mos_status",
            Self::read_int_path("/sys/class/oplus_chg/battery/parallel_chg_mos_status"),
        );
        Self::append_int(
            &mut out,
            "wireless_current_now",
            Self::read_int_path("/sys/class/power_supply/wireless/current_now"),
        );
        Self::append_string(
            &mut out,
            "wireless_rx_version",
            &Self::read_string_path("/proc/wireless/rx_version"),
        );
        Self::append_string(
            &mut out,
            "wireless_tx_version",
            &Self::read_string_path("/proc/wireless/tx_version"),
        );
        Self::append_int(
            &mut out,
            "wireless_idt_adc_test",
            Self::read_int_path("/proc/wireless/idt_adc_test"),
        );
        Self::append_string(
            &mut out,
            "wireless_real_type",
            &Self::read_string_path("/sys/class/power_supply/wireless/real_type"),
        );
        Self::append_int(
            &mut out,
            "wireless_charge_pump_en",
            Self::read_int_path("/proc/wireless/charge_pump_en"),
        );
        Self::append_string(
            &mut out,
            "wireless_deviated",
            &Self::read_string_path("/proc/wireless/deviated"),
        );
        Self::append_string(
            &mut out,
            "battery_status",
            &Self::read_string_path("/sys/class/power_supply/battery/status"),
        );
        Self::append_string(
            &mut out,
            "wireless_voltage_now",
            &Self::read_string_path("/sys/class/power_supply/wireless/voltage_now"),
        );
        Self::append_string(
            &mut out,
            "battery_type_str",
            &Self::read_string_path("/sys/class/oplus_chg/battery/battery_type"),
        );

        // The fast-charge block is the part this port has to fill in itself.
        Self::append_int(&mut out, "fast_charge", self.fast_charge_fast());
        Self::append_int(
            &mut out,
            "charge_technology",
            self.fast_charge_type() as i32,
        );
        Self::append_int(&mut out, "chg_power_watts", self.fast_charge_watts());
        Self::append_int(
            &mut out,
            "pump_express",
            if self.fast_charge_pump() { 1 } else { 0 },
        );
        out
    }

    pub(crate) fn txn_queryWlsPencilInfo(&self) -> String {
        let mut out = String::new();
        Self::append_string(
            &mut out,
            "ble_mac_addr",
            &Self::read_string_path("/sys/class/oplus_wireless/pencil/ble_mac_addr"),
        );
        Self::append_int(
            &mut out,
            "ping_time",
            Self::read_int_path("/sys/class/oplus_wireless/pencil/ping_time"),
        );
        Self::append_int(
            &mut out,
            "tx_status",
            Self::read_int_path("/sys/class/oplus_wireless/pencil/tx_status"),
        );
        Self::append_int(
            &mut out,
            "tx_current",
            Self::read_int_path("/sys/class/oplus_wireless/pencil/tx_current"),
        );
        Self::append_int(
            &mut out,
            "tx_voltage",
            Self::read_int_path("/sys/class/oplus_wireless/pencil/tx_voltage"),
        );
        Self::append_string(
            &mut out,
            "fw_version",
            &Self::read_string_path("/sys/class/oplus_wireless/pencil/fw_version"),
        );
        Self::append_int(
            &mut out,
            "wireless_ic_id",
            Self::read_int_path("/sys/class/oplus_wireless/pencil/wireless_ic_id"),
        );
        out
    }

    pub(crate) fn txn_getBccCsvData(&self) -> String {
        let mut out = String::new();
        Self::append_int(
            &mut out,
            "cool_down",
            Self::read_int_path("/sys/devices/platform/charger/Thermal_throttle"),
        );
        Self::append_int(
            &mut out,
            "chip_soc",
            Self::read_int_path("/sys/class/power_supply/battery/chip_soc"),
        );
        Self::append_int(
            &mut out,
            "capacity",
            Self::read_int_path("/sys/class/power_supply/battery/capacity"),
        );
        Self::append_string(
            &mut out,
            "status",
            &Self::read_string_path("/sys/class/power_supply/battery/status"),
        );
        out
    }

    // --- hand-written accessors ------------------------------------------

    // The MTK ADC node is in mV, the Oplus node is in uV, so the fallback is
    // scaled before it is reported.
    pub(crate) fn txn_getChargerIdVolt(&self) -> i32 {
        let mut val = nodes::read_node_int(
            "/sys/class/power_supply/battery/chargerid_volt",
            Some("/sys/devices/platform/charger/ADC_Charger_Voltage"),
        );
        if !nodes::is_error(val) && val != 0 {
            val *= 1000;
        }
        val
    }

    pub(crate) fn txn_getBattPPSChgPower(&self) -> i32 {
        let cap_path = std::env::var("OPLUS_CHARGER_CAPACITY_NODE")
            .ok()
            .filter(|v| !v.is_empty())
            .unwrap_or_else(|| "/sys/devices/platform/charger/adapter_capacity".to_string());
        let mut val = nodes::read_node_int_single(&cap_path);
        if val <= 0 || nodes::is_error(val) {
            val = nodes::read_node_int_single("/sys/class/power_supply/tran-auto-test/ta_power");
        }
        nodes::clamp0(val)
    }

    pub(crate) fn txn_getPsyBatteryFcc(&self) -> i32 {
        let mut raw = nodes::read_node_int_single("/sys/class/power_supply/battery/charge_full");
        if !nodes::is_error(raw) {
            raw /= 1000;
        }
        raw
    }

    pub(crate) fn txn_getPsyBatteryRm(&self) -> i32 {
        let mut raw = nodes::read_node_int_single("/sys/class/power_supply/battery/charge_counter");
        if !nodes::is_error(raw) {
            raw /= 1000;
        }
        raw
    }

    pub(crate) fn txn_getPsyOtgOnline(&self) -> i32 {
        let mut val = nodes::read_node_int(
            "/sys/class/power_supply/usb/otg_online",
            Some("/sys/class/oplus_chg/usb/otg_online"),
        );
        if nodes::is_error(val) {
            let is_otg = nodes::read_node_int_single("/sys/class/udc/11201000.usb0/is_otg");
            val = if is_otg == 1 { 1 } else { 0 };
        }
        val
    }

    pub(crate) fn txn_getPsyOtgSwitch(&self) -> i32 {
        let mut val = nodes::read_node_int(
            "/sys/class/power_supply/usb/otg_switch",
            Some("/sys/class/oplus_chg/usb/otg_switch"),
        );
        if nodes::is_error(val) {
            val = OTG_SWITCH_STATE.load(Ordering::Relaxed);
        }
        val
    }

    pub(crate) fn txn_getPsyUsbOnline(&self) -> i32 {
        let mut val = nodes::read_node_int(
            "/sys/class/power_supply/usb/online",
            Some("/sys/class/oplus_chg/usb/online"),
        );
        if nodes::is_error(val) {
            val = nodes::read_node_int_single("/sys/class/power_supply/charger/online");
            if nodes::is_error(val) {
                val = 0;
            }
        }
        val
    }

    pub(crate) fn txn_getPsyUsbStatus(&self) -> i32 {
        let mut val = nodes::read_node_int(
            "/sys/class/power_supply/usb/usb_status",
            Some("/sys/class/power_supply/charger/online"),
        );
        if nodes::is_error(val) {
            val = 0;
        }
        val
    }

    // The MMI factory-mode disable is a request on the bypass node, not a
    // latch: recording it and re-deriving the node keeps the monitor able to
    // clear it again, which a direct write could never do.
    pub(crate) fn txn_setPsyMmiChgEn(&self, a0: &str) -> i32 {
        {
            let mut st = self.lock_state();
            st.mmi_charging_disabled = if a0 == "0" { 1 } else { 0 };
        }
        if a0 == "0" || a0 == "1" {
            self.apply_bypass_policy(true);
        }
        let rc = nodes::write_node(
            "/sys/class/power_supply/battery/mmi_charging_enable",
            Some("/sys/class/oplus_chg/battery/mmi_charging_enable"),
            a0,
        );
        if rc < 0 {
            0
        } else {
            rc
        }
    }

    pub(crate) fn txn_setPsyOtgSwitch(&self, a0: &str) -> i32 {
        let parsed = match a0 {
            "1" | "true" => 1,
            "0" | "false" => 0,
            _ => c_atoi(a0.as_bytes()),
        };
        let val_str = if parsed == 1 { "1" } else { "0" };
        nodes::write_node(constants::tran::OTG_CTL, None, val_str);
        nodes::write_node(
            "/sys/class/power_supply/usb/otg_switch",
            Some("/sys/class/oplus_chg/usb/otg_switch"),
            val_str,
        );
        OTG_SWITCH_STATE.store(parsed, Ordering::Relaxed);
        0
    }
}

/// atoi(), used for the string-encoded setters.
pub(crate) fn c_atoi(buf: &[u8]) -> i32 {
    let mut i = 0;
    while i < buf.len()
        && (buf[i] == b' ' || buf[i] == b'\t' || buf[i] == b'\n' || buf[i] == b'\r')
    {
        i += 1;
    }
    let mut neg = false;
    if i < buf.len() && (buf[i] == b'+' || buf[i] == b'-') {
        neg = buf[i] == b'-';
        i += 1;
    }
    let mut v: i64 = 0;
    while i < buf.len() && buf[i].is_ascii_digit() {
        v = v * 10 + (buf[i] - b'0') as i64;
        i += 1;
    }
    let v = if neg { -v } else { v };
    v.clamp(i32::MIN as i64, i32::MAX as i64) as i32
}

// Re-export so the generated accessors and the dispatcher can find the impl.
pub(crate) use accessors::*;
