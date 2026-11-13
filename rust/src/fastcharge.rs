// fastcharge.rs - fast-charge / SuperVOOC classification.
//
// A direct port of src/FastChargeDetector.cpp.  The shipped Oplus HAL does
// not classify charging power itself: it reports what the kernel's oplus_chg
// driver published.  This port runs the MTK/Transsion charger driver, which
// has no oplus_chg driver, so the watt thresholds are applied here instead -
// the same place the Transsion trancharge HAL applies them.
//
//   >= 45 W -> SuperVOOC (charge technology SVOOC, id 2)
//   >= 33 W -> fast charge (charge technology VOOC, id 1)
//   <  33 W -> normal
//
// The watts decide the charge technology.  The Transsion pump state is a
// separate signal, reported on its own; it does not gate the classification.

use crate::constants::{self, ChargeTechnology, K_READ_ERROR};

// A sub-25 W reading is a stock charger baseline, not a rated adapter, so
// it is reported as 0 and never surfaces as a charge wattage.
fn report_watts(watts: i32) -> i32 {
    if watts <= 0 || watts == constants::K_READ_ERROR || watts == constants::K_IO_ERROR {
        return 0;
    }
    if watts < constants::K_MIN_REPORT_WATTS {
        return 0;
    }
    watts
}

use crate::nodes;

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct FastChargeState {
    pub fast_chg_type: i32,
    pub fast_charge: i32,
    pub watts: i32,
    pub pump_engaged: bool,
    pub valid: bool,
    pub from_oplus_driver: bool,
}

pub struct FastChargeDetector {
    oplus_charge_technology: String,
    oplus_fast_charge: String,
    oplus_ppschg_power: String,
    power_supply_charge_technology: String,
    power_supply_fast_charge: String,
    pump_express: String,
    adapter_capacity: String,
    // mFastChgIndicator in the shipped C++: assigned in the ctor, never read.
    #[allow(dead_code)]
    fast_chg_indicator: String,
    state: FastChargeState,
}

fn env_or(env: &str, fallback: &str) -> String {
    std::env::var(env)
        .ok()
        .filter(|v| !v.is_empty())
        .unwrap_or_else(|| fallback.to_string())
}

fn read_int_node(path: &str) -> Option<i32> {
    let v = nodes::read_int_path(path);
    if v == K_READ_ERROR {
        None
    } else {
        Some(v)
    }
}

fn read_string_node(path: &str) -> Option<String> {
    use std::io::Read;
    let mut file = std::fs::File::open(path).ok()?;
    let mut buf = [0u8; 64];
    let n = file.read(&mut buf).ok()?;
    let end = n.min(63);
    let mut s = String::from_utf8_lossy(&buf[..end]).into_owned();
    while s.ends_with('\n') || s.ends_with('\r') {
        s.pop();
    }
    Some(s)
}

impl Default for FastChargeDetector {
    fn default() -> Self {
        Self::new()
    }
}

impl FastChargeDetector {
    pub fn new() -> Self {
        Self {
            oplus_charge_technology: env_or(
                "OPLUS_CHARGER_TECH_NODE",
                "/sys/class/oplus_chg/battery/charge_technology",
            ),
            oplus_fast_charge: env_or(
                "OPLUS_CHARGER_FAST_NODE",
                "/sys/class/oplus_chg/battery/fast_charge",
            ),
            oplus_ppschg_power: "/sys/class/oplus_chg/battery/ppschg_power".to_string(),
            power_supply_charge_technology:
                "/sys/class/power_supply/battery/charge_technology".to_string(),
            power_supply_fast_charge: "/sys/class/power_supply/battery/fast_charge".to_string(),
            pump_express: env_or("OPLUS_CHARGER_PUMP_NODE", constants::tran::PUMP_EXPRESS),
            adapter_capacity: env_or(
                "OPLUS_CHARGER_CAPACITY_NODE",
                constants::tran::ADAPTER_CAPACITY,
            ),
            fast_chg_indicator: env_or(
                "OPLUS_CHARGER_FAST_INDICATOR_NODE",
                "/sys/devices/platform/charger/fast_chg_indicator",
            ),
            state: FastChargeState::default(),
        }
        .tap_refresh()
    }

    fn tap_refresh(mut self) -> Self {
        self.refresh();
        self
    }

    pub fn classify_watts(watts: i32) -> ChargeTechnology {
        if watts >= constants::K_SUPER_VOOC_WATTS {
            ChargeTechnology::Svooc
        } else if watts >= constants::K_FAST_CHARGE_WATTS {
            ChargeTechnology::Vooc
        } else {
            ChargeTechnology::Unknown
        }
    }

    /// Oplus charger driver present: trust it, the kernel already decided.
    fn read_oplus_driver(&self) -> Option<FastChargeState> {
        let tech = read_string_node(&self.oplus_charge_technology)
            .or_else(|| read_string_node(&self.power_supply_charge_technology))?;

        let fast_charge = read_int_node(&self.oplus_fast_charge)
            .or_else(|| read_int_node(&self.power_supply_fast_charge))?;

        let watts = read_int_node(&self.oplus_ppschg_power).unwrap_or(0);

        let ty = constants::technology_from_name(&tech);
        if ty == ChargeTechnology::Unknown {
            // The oplus driver answered with a name this table does not know
            // (or an empty node).  Reporting UNKNOWN here would hide the
            // Transsion reading, so let the caller fall back to it.
            return None;
        }

        Some(FastChargeState {
            fast_chg_type: ty as i32,
            fast_charge,
            watts: report_watts(watts),
            pump_engaged: fast_charge != 0,
            valid: true,
            from_oplus_driver: true,
        })
    }

    /// Transsion charger driver: classify the adapter power ourselves.
    fn read_tran_charger(&self) -> FastChargeState {
        let mut state = FastChargeState::default();
        let online = read_int_node("/sys/class/power_supply/charger/online");
        if let Some(online) = online {
            if online == 0 {
                state.valid = true;
                return state;
            }
        } else if let Some(status) = read_string_node("/sys/class/power_supply/battery/status") {
            if status == "Discharging" || status == "Not charging" {
                state.valid = true;
                return state;
            }
        }

        let pump = read_int_node(&self.pump_express).unwrap_or(0);

        let mut watts = read_int_node(&self.adapter_capacity).unwrap_or(0);
        if watts <= 0 {
            watts = read_int_node("/sys/class/power_supply/tran-auto-test/ta_power").unwrap_or(0);
        }

        state.pump_engaged = pump != 0;
        state.watts = report_watts(watts);
        state.fast_chg_type = Self::classify_watts(watts) as i32;
        state.fast_charge = if state.fast_chg_type == ChargeTechnology::Vooc as i32
            || state.fast_chg_type == ChargeTechnology::Svooc as i32
        {
            1
        } else {
            0
        };
        state.from_oplus_driver = false;
        state.valid = true;
        state
    }

    pub fn refresh(&mut self) {
        let next = match self.read_oplus_driver() {
            Some(s) => s,
            None => self.read_tran_charger(),
        };
        self.state = next;
    }

    pub fn get_fast_charge(&self) -> i32 {
        self.state.fast_charge
    }

    pub fn get_fast_chg_type(&self) -> ChargeTechnology {
        ChargeTechnology::from_id(self.state.fast_chg_type)
    }

    pub fn get_charge_technology(&self) -> ChargeTechnology {
        self.get_fast_chg_type()
    }

    pub fn get_power_watts(&self) -> i32 {
        self.state.watts
    }

    pub fn is_pump_engaged(&self) -> bool {
        self.state.pump_engaged
    }

    pub fn state(&self) -> FastChargeState {
        self.state
    }
}
