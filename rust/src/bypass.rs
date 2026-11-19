// bypass.rs - the decision logic behind the MTK bypass-charge node.
//
// A direct port of src/BypassPolicy.cpp.  The two features are independent
// requests that share one node:
//
//   * the GameSpace toggle (setChgConfig 0x19) carries a charge target
//     percent in its payload when the client sends one ("<pct>+switch=1").
//     Without a target it engages as soon as it is switched on.
//   * the charge limit from the battery settings (setChgConfig 0x5/0xe)
//     carries the limit percent; the node goes to 1 at and above that
//     percent, and back to 0 below it so the battery keeps charging.
//
// A GameSpace toggle that carries no target of its own follows the charge
// limit.

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct BypassInputs {
    /// -1 = never asked, 0/1 = the requested state.
    pub game_switch: i32,
    pub game_target_percent: i32,
    pub up_limit_enabled: i32,
    pub up_limit_percent: i32,
    /// The live battery percent, or a negative sentinel when unknown.
    pub capacity: i32,
}

fn is_percent(v: i32) -> bool {
    (1..=100).contains(&v)
}

/// intFields: every run of digits in the value, in order.
pub fn int_fields(value: &str) -> Vec<i32> {
    let b = value.as_bytes();
    let mut out = Vec::new();
    let mut i = 0;
    while i < b.len() {
        if b[i].is_ascii_digit() {
            let start = i;
            while i < b.len() && b[i].is_ascii_digit() {
                i += 1;
            }
            out.push(c_atoi(&b[start..i]));
        } else {
            i += 1;
        }
    }
    out
}

/// The payload is "<n>+switch=<0|1>".  The switch field is authoritative;
/// the leading integer is the charge target percent when it is a plausible
/// percent.  The stock COSA client always sends "1", which is not a target.
pub fn parse_game_toggle(value: &str, target_percent: &mut i32) -> i32 {
    let key = "switch=";
    let Some(pos) = value.find(key) else {
        return -1;
    };
    let start = pos + key.len();
    let mut end = start;
    let b = value.as_bytes();
    while end < b.len() && b[end].is_ascii_digit() {
        end += 1;
    }
    if end == start {
        return -1;
    }
    let state = c_atoi(&b[start..end]);

    *target_percent = 0;
    let mut lead = 0;
    while lead < b.len() && b[lead].is_ascii_digit() {
        lead += 1;
    }
    if lead > 0 {
        let n = c_atoi(&b[..lead]);
        if is_percent(n) && n > 1 {
            *target_percent = n;
        }
    }
    state
}

/// The state is the first integer after the "++" separator, the percent the
/// last integer that is a valid percent.
pub fn parse_up_limit(value: &str, state: &mut i32, percent: &mut i32) {
    *state = -1;
    *percent = 0;
    let fields = int_fields(value);
    if fields.is_empty() {
        return;
    }
    let mut start = 0usize;
    if let Some(sep) = value.find("++") {
        let b = value.as_bytes();
        let mut i = 0;
        while i < sep {
            if b[i].is_ascii_digit() {
                start += 1;
                while i < sep && b[i].is_ascii_digit() {
                    i += 1;
                }
            } else {
                i += 1;
            }
        }
    }
    if fields.len() > start {
        *state = fields[start];
    }
    let mut i = fields.len();
    while i > start {
        if is_percent(fields[i - 1]) {
            *percent = fields[i - 1];
            break;
        }
        i -= 1;
    }
}

const MIN_BYPASS_CAPACITY: i32 = 15;

fn feature_engaged(requested: bool, target_percent: i32, capacity: i32) -> bool {
    if !requested {
        return false;
    }
    if target_percent <= 0 {
        // Guard against starving a critically low battery when no explicit target is set.
        return capacity >= MIN_BYPASS_CAPACITY;
    }
    if capacity < 0 {
        // The battery percent is unknown; do not engage.
        return false;
    }
    capacity >= target_percent
}

/// Returns the value to write to the node, and whether anything at all is
/// requesting bypass.  When nothing is, the caller keeps the node as it is.
pub fn decide(inputs: &BypassInputs, node_value: &mut i32) -> bool {
    let game_engaged = if inputs.game_switch == 1 {
        if inputs.game_target_percent > 1 {
            feature_engaged(true, inputs.game_target_percent, inputs.capacity)
        } else {
            feature_engaged(true, 0, inputs.capacity)
        }
    } else {
        false
    };
    let limit_engaged = feature_engaged(
        inputs.up_limit_enabled == 1,
        inputs.up_limit_percent,
        inputs.capacity,
    );
    *node_value = if game_engaged || limit_engaged { 1 } else { 0 };
    inputs.game_switch >= 0 || inputs.up_limit_enabled >= 0
}

fn c_atoi(buf: &[u8]) -> i32 {
    let mut i = 0;
    while i < buf.len() && (buf[i] == b' ' || buf[i] == b'\t') {
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
