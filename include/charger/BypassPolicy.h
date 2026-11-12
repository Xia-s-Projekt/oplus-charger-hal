// BypassPolicy.h - the decision logic behind the MTK bypass-charge node.
//
// The MTK/PE50 charger driver exposes one 0/1 flag,
// /sys/devices/platform/charger/bypass_charger: 1 means the system runs on
// the adapter (bypass) instead of charging the battery.
//
// Two independent Oplus features map onto that flag:
//
//   * the GameSpace bypass toggle - setChgConfig(0x19, "<n>+switch=1").  The
//     leading integer is the charge target percent when it is a valid
//     percent; the stock COSA client always sends "1", which means "no
//     target given".
//   * the charge limit from the battery settings - setChgConfig(0x5/0xe,
//     "<n>++<state>+<percent>..."), which carries the limit percent.
//
// Both are gated on the actual battery percent: the flag is only engaged
// once the battery has reached the target, and it is cleared again when the
// feature is switched off.  That is the whole policy - the header keeps it
// free of device access so it can be unit tested on the host.
#pragma once

#include <string>
#include <vector>

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {
namespace bypass {

// The state the HAL keeps while the two features are in play.
struct Inputs {
    // GameSpace toggle: 1 = on, 0 = off, -1 = never set.
    int gameSwitch = -1;
    // GameSpace charge target percent, 0 = no target given.
    int gameTargetPercent = 0;
    // Charge limit percent from the battery settings, 0 = unknown.
    int upLimitPercent = 0;
    // Charge limit enabled state from the battery settings, 0 = off.
    int upLimitEnabled = 0;
    // Actual battery percent, negative when it cannot be read.
    int capacity = -1;
};

// All the integer groups of a payload, in order.
std::vector<int> intFields(const std::string& value);

// Parses "<n>+switch=<0|1>".  Returns the switch state (-1 when the payload
// carries no switch field) and reports the leading integer as the charge
// target percent when it is a plausible percent.
int parseGameToggle(const std::string& value, int* targetPercent);

// Parses "<n>++<state>+<percent>...".  Reports the state (the first integer
// after the "++") and the percent (the last integer that is a valid percent).
void parseUpLimit(const std::string& value, int* state, int* percent);

// True when the feature is engaged.  A feature without a target percent
// engages immediately; with a target it engages once the battery has reached
// it.  An unreadable capacity counts as "not reached".
bool featureEngaged(bool requested, int targetPercent, int capacity);

// The value the bypass node must carry for the given state.  False when
// nothing has to be written because no feature is active.
bool decide(const Inputs& inputs, int* nodeValue);

}  // namespace bypass
}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
