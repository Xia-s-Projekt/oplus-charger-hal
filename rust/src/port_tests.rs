// Host tests for the ported pure logic.  The cases mirror
// standalone/tests/bypass_policy_test.cpp one for one, plus the C `atoi`
// semantics and the node sentinels.
#[cfg(test)]
use crate::bypass::{self, BypassInputs};
use crate::constants;
use crate::nodes::c_atoi;

fn node_for(
    game_switch: i32,
    game_target: i32,
    up_limit_enabled: i32,
    up_limit_percent: i32,
    capacity: i32,
) -> i32 {
    let inputs = BypassInputs {
        game_switch,
        game_target_percent: game_target,
        up_limit_enabled,
        up_limit_percent,
        capacity,
    };
    let mut value = -1;
    bypass::decide(&inputs, &mut value);
    value
}

#[test]
fn parse_game_toggle() {
    let mut target = 0;
    assert_eq!(bypass::parse_game_toggle("1+switch=1", &mut target), 1);
    assert_eq!(target, 0);
    target = 0;
    assert_eq!(bypass::parse_game_toggle("1+switch=0", &mut target), 0);
    target = 0;
    assert_eq!(bypass::parse_game_toggle("20+switch=1", &mut target), 1);
    assert_eq!(target, 20);
}

#[test]
fn parse_up_limit() {
    let mut state = -1;
    let mut percent = 0;
    bypass::parse_up_limit("2++0+80", &mut state, &mut percent);
    assert_eq!(state, 0);
    assert_eq!(percent, 80);
    bypass::parse_up_limit("2++1+80", &mut state, &mut percent);
    assert_eq!(state, 1);
    assert_eq!(percent, 80);
}

#[test]
fn bypass_policy_cases() {
    // GameSpace toggle, target 20.
    assert_eq!(node_for(1, 20, -1, 0, 50), 1);
    assert_eq!(node_for(0, 20, -1, 0, 50), 0);
    assert_eq!(node_for(1, 40, -1, 0, 20), 0);
    assert_eq!(node_for(1, 40, -1, 0, 40), 1);
    assert_eq!(node_for(1, 40, -1, 0, 41), 1);
    assert_eq!(node_for(1, 40, -1, 0, 39), 0);

    // Charge limit only.
    assert_eq!(node_for(-1, 0, 1, 80, 79), 0);
    assert_eq!(node_for(-1, 0, 1, 80, 80), 1);
    assert_eq!(node_for(-1, 0, 0, 80, 90), 0);

    // Game toggle with no target of its own follows the charge limit.
    assert_eq!(node_for(1, 0, 1, 80, 79), 0);
    assert_eq!(node_for(1, 0, 1, 80, 80), 1);

    // Unknown capacity engages nothing.
    assert_eq!(node_for(1, 40, -1, 0, -1), 0);
}

#[test]
fn c_atoi_matches_libc() {
    assert_eq!(c_atoi(b"0"), 0);
    assert_eq!(c_atoi(b"42"), 42);
    assert_eq!(c_atoi(b"  7"), 7);
    assert_eq!(c_atoi(b"\t-13"), -13);
    assert_eq!(c_atoi(b"+9x"), 9);
    assert_eq!(c_atoi(b"abc"), 0);
    assert_eq!(c_atoi(b""), 0);
    // Out-of-range input is clamped, not wrapped: the C++ uses std::atoi,
    // which is UB on overflow (see RUST_DELTAS.md).  Node contents are always
    // in range, so this branch never runs on the device.
    assert_eq!(c_atoi(b"9999999999999"), i32::MAX);
}

#[test]
fn node_sentinels_match_cpp() {
    assert_eq!(constants::K_READ_ERROR, i32::MIN);
    assert_eq!(constants::K_IO_ERROR, i32::MIN + 2);
    assert_eq!(constants::K_PARSE_ERROR, i32::MIN);
    assert_eq!(constants::K_WRITE_ERROR, 0x8000_0003u32 as i32);
    assert_eq!(constants::K_FAST_CHARGE_WATTS, 33);
    assert_eq!(constants::K_SUPER_VOOC_WATTS, 45);
}
