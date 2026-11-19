// BypassPolicy.cpp - the decision logic behind the MTK bypass-charge node.
//
// See include/charger/BypassPolicy.h for the contract.
#include "charger/BypassPolicy.h"

#include <cctype>
#include <cstdlib>

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {
namespace bypass {

namespace {

// A percent is a value in 1..100.
bool isPercent(int v) {
    return v >= 1 && v <= 100;
}

}  // namespace

std::vector<int> intFields(const std::string& value) {
    std::vector<int> fields;
    size_t i = 0;
    while (i < value.size()) {
        if (std::isdigit(static_cast<unsigned char>(value[i]))) {
            long v = std::strtol(value.c_str() + i, nullptr, 10);
            fields.push_back(static_cast<int>(v));
            while (i < value.size() && std::isdigit(static_cast<unsigned char>(value[i]))) {
                ++i;
            }
        } else {
            ++i;
        }
    }
    return fields;
}

int parseGameToggle(const std::string& value, int* targetPercent) {
    // The payload is "<n>+switch=<0|1>".  The switch field is authoritative.
    const std::string key = "switch=";
    size_t pos = value.find(key);
    if (pos == std::string::npos) {
        return -1;
    }
    size_t start = pos + key.size();
    size_t end = start;
    while (end < value.size() && std::isdigit(static_cast<unsigned char>(value[end]))) {
        ++end;
    }
    if (end == start) {
        return -1;
    }
    int state = std::atoi(value.substr(start, end - start).c_str());

    // The leading integer is the charge target percent when it is a plausible
    // percent.  The stock COSA client always sends "1", which is not a target.
    if (targetPercent != nullptr) {
        *targetPercent = 0;
        size_t lead = 0;
        while (lead < value.size() && std::isdigit(static_cast<unsigned char>(value[lead]))) {
            ++lead;
        }
        if (lead > 0) {
            int n = std::atoi(value.substr(0, lead).c_str());
            if (isPercent(n) && n > 1) {
                *targetPercent = n;
            }
        }
    }
    return state;
}

void parseUpLimit(const std::string& value, int* state, int* percent) {
    if (state != nullptr) {
        *state = -1;
    }
    if (percent != nullptr) {
        *percent = 0;
    }
    std::vector<int> fields = intFields(value);
    if (fields.empty()) {
        return;
    }
    // The state is the first integer after the "++" separator, the percent the
    // last integer that is a valid percent.
    size_t start = 0;
    size_t sep = value.find("++");
    if (sep != std::string::npos) {
        // Count the integers that appear before the separator.
        for (size_t i = 0; i < sep; ++i) {
            if (std::isdigit(static_cast<unsigned char>(value[i]))) {
                ++start;
                while (i < sep && std::isdigit(static_cast<unsigned char>(value[i]))) {
                    ++i;
                }
            }
        }
    }
    if (state != nullptr && fields.size() > start) {
        *state = fields[start];
    }
    if (percent != nullptr) {
        for (size_t i = fields.size(); i > start; --i) {
            if (isPercent(fields[i - 1])) {
                *percent = fields[i - 1];
                break;
            }
        }
    }
}

bool featureEngaged(bool requested, int targetPercent, int capacity) {
    if (!requested) {
        return false;
    }
    if (targetPercent <= 0) {
        // No target: the feature is on as soon as it is requested.
        return true;
    }
    if (capacity < 0) {
        // The battery percent is unknown; do not engage.
        return false;
    }
    return capacity >= targetPercent;
}

bool decide(const Inputs& inputs, int* nodeValue) {
    // The two features are independent requests that share one node:
    //
    //   * the GameSpace toggle (setChgConfig 0x19) carries a charge target
    //     percent in its payload when the client sends one ("<pct>+switch=1").
    //     Without a target it engages as soon as it is switched on - the
    //     GameSpace app itself stops asking for bypass below its own target.
    //   * the charge limit from the battery settings (setChgConfig 0x5/0xe)
    //     carries the limit percent; the node goes to 1 at and above that
    //     percent, and back to 0 below it so the battery keeps charging.
    //
    // A GameSpace toggle that carries no target of its own follows the charge
    // limit: engaging it must not stop charging below the limit the user set
    // in the battery settings.
    // The GameSpace toggle (setChgConfig 0x19) carries a charge target
    // percent in its payload when the client sends one ("<pct>+switch=1").
    // When the toggle is switched on without a target (e.g. "1+switch=1"),
    // bypass engages immediately regardless of battery capacity.
    bool gameEngaged = false;
    if (inputs.gameSwitch == 1) {
        if (inputs.gameTargetPercent > 1) {
            gameEngaged = featureEngaged(true, inputs.gameTargetPercent, inputs.capacity);
        } else {
            gameEngaged = true;
        }
    }
    bool limitEngaged = featureEngaged(inputs.upLimitEnabled == 1, inputs.upLimitPercent, inputs.capacity);
    int value = (gameEngaged || limitEngaged) ? 1 : 0;
    if (nodeValue != nullptr) {
        *nodeValue = value;
    }
    // Nothing is requested at all: the caller keeps the node as it is.
    return inputs.gameSwitch >= 0 || inputs.upLimitEnabled >= 0;
}

}  // namespace bypass
}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
