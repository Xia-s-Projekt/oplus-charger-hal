// decimal.rs - the decimal-SOH ramp behind healthd_update_ui_soc_decimal.
//
// A port of the globals in ChargerManual.cpp: gDecimalStep, gHoldingAt99,
// gLastStepTime and the kDecimalTargets table.

use std::time::{Duration, Instant};

use crate::constants::{K_DECIMAL_TARGETS, K_NUM_DECIMAL_TARGETS};

const STEP_INTERVAL_MS: u128 = 700;

pub struct DecimalState {
    step: usize,
    holding_at_99: bool,
    last_step: Option<Instant>,
}

impl DecimalState {
    pub fn new() -> Self {
        Self {
            step: 0,
            holding_at_99: false,
            last_step: None,
        }
    }

    pub fn reset(&mut self) {
        self.step = 0;
        self.holding_at_99 = false;
        self.last_step = None;
    }

    /// Advance the ramp and return the (start, end) decimal pair.
    pub fn step(&mut self) -> (i32, i32) {
        if self.holding_at_99 || self.step >= K_NUM_DECIMAL_TARGETS {
            self.holding_at_99 = true;
            return (99, 99);
        }

        match self.last_step {
            None => self.last_step = Some(Instant::now()),
            Some(prev) => {
                let elapsed = prev.elapsed().as_millis();
                if elapsed >= STEP_INTERVAL_MS {
                    self.step += 1;
                    self.last_step = Some(Instant::now());
                }
            }
        }

        if self.step >= K_NUM_DECIMAL_TARGETS {
            self.holding_at_99 = true;
            return (99, 99);
        }
        let start = if self.step == 0 {
            0
        } else {
            K_DECIMAL_TARGETS[self.step - 1]
        };
        (start, K_DECIMAL_TARGETS[self.step])
    }
}

impl Default for DecimalState {
    fn default() -> Self {
        Self::new()
    }
}

/// The trigger throttle: 75 ms between non-forced battery-property pushes.
pub struct Throttle {
    last: Option<Instant>,
}

impl Throttle {
    pub fn new() -> Self {
        Self { last: None }
    }

    /// Returns true when the caller may fire.
    pub fn allow(&mut self, force: bool, min_gap: Duration) -> bool {
        let now = Instant::now();
        match self.last {
            None => {
                self.last = Some(now);
                true
            }
            Some(prev) => {
                if force || now.duration_since(prev) >= min_gap {
                    self.last = Some(now);
                    true
                } else {
                    false
                }
            }
        }
    }
}

impl Default for Throttle {
    fn default() -> Self {
        Self::new()
    }
}
