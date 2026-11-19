#include "charger/FastChargeDetector.h"

#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>

#include "charger/ChargerConstants.h"

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {

namespace {

// The shipped getters use open()/read()/atoi() on a 16-byte buffer and map
// the failure cases to INT32_MIN / INT32_MIN+1.  Same here, so the values
// this detector reports have the same shape as the shipped ones.
bool readIntNode(const char* path, int* out) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return false;
    }
    char buf[16] = {0};
    ssize_t n = read(fd, buf, sizeof(buf));
    close(fd);
    if (n < 0) {
        return false;
    }
    *out = atoi(buf);
    return true;
}

bool readStringNode(const char* path, std::string* out) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return false;
    }
    char buf[64] = {0};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n < 0) {
        return false;
    }
    buf[n] = '\0';
    // Trim the trailing newline the driver appends.
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    out->assign(buf);
    return true;
}

const char* const kTechNames[] = {
    "UNKNOWN",  // 0
    "VOOC",     // 1
    "SVOOC",    // 2
    "PD",       // 3
    "QC",       // 4
    "PPS",      // 5
    "UFCS",     // 6
};

}  // namespace

namespace {

// A node path can be overridden through the environment.  This is how the
// on-device test drives the detector with fixture values: a bind mount over
// a sysfs node would need a mount namespace, and toybox's mount has no
// --make-rprivate, so the mount propagates back to init and shadows the
// real charger node.
const char* nodePathOrDefault(const char* envName, const char* fallback) {
    const char* value = getenv(envName);
    if (value != nullptr && value[0] != '\0') {
        return value;
    }
    return fallback;
}

const char kEnvPumpExpress[] = "OPLUS_CHARGER_PUMP_NODE";
const char kEnvAdapterCapacity[] = "OPLUS_CHARGER_CAPACITY_NODE";
const char kEnvOplusChargeTechnology[] = "OPLUS_CHARGER_TECH_NODE";
const char kEnvOplusFastCharge[] = "OPLUS_CHARGER_FAST_NODE";

}  // namespace

FastChargeDetector::FastChargeDetector() : FastChargeDetector(FastChargeNodes()) {
}

FastChargeDetector::FastChargeDetector(const FastChargeNodes& nodes) {
    mOplusChargeTechnology = nodes.oplusChargeTechnology
            ? nodes.oplusChargeTechnology
            : nodePathOrDefault(kEnvOplusChargeTechnology,
                                "/sys/class/oplus_chg/battery/charge_technology");
    mOplusFastCharge = nodes.oplusFastCharge
            ? nodes.oplusFastCharge
            : nodePathOrDefault(kEnvOplusFastCharge,
                                "/sys/devices/platform/charger/fast_chg_indicator");
    mOplusPpschgPower = nodes.oplusPpschgPower
            ? nodes.oplusPpschgPower
            : "/sys/class/oplus_chg/battery/ppschg_power";
    mPowerSupplyChargeTechnology = nodes.powerSupplyChargeTechnology
            ? nodes.powerSupplyChargeTechnology
            : "/sys/class/power_supply/battery/charge_technology";
    mPowerSupplyFastCharge = nodes.powerSupplyFastCharge
            ? nodes.powerSupplyFastCharge
            : "/sys/devices/platform/charger/fast_chg_indicator";
    mPumpExpress = nodes.pumpExpress
            ? nodes.pumpExpress
            : nodePathOrDefault(kEnvPumpExpress, tran::kPumpExpress);
    mAdapterCapacity = nodes.adapterCapacity
            ? nodes.adapterCapacity
            : nodePathOrDefault(kEnvAdapterCapacity, tran::kAdapterCapacity);
    mFastChgIndicator = nodes.fastChgIndicator
            ? nodes.fastChgIndicator
            : nodePathOrDefault("OPLUS_CHARGER_FAST_INDICATOR_NODE",
                                "/sys/devices/platform/charger/fast_chg_indicator");
    refresh();
}

// A sub-25 W reading is a stock charger baseline, not a rated adapter, so it
// is reported as 0 and never surfaces as a charge wattage.
static int reportWatts(int watts) {
    if (watts <= 0 || watts == kReadError || watts == kIoError) {
        return 0;
    }
    return (watts < kMinReportWatts) ? 0 : watts;
}

int FastChargeDetector::classifyWatts(int watts) {
    if (watts >= kSuperVoocWatts) {
        return CHG_TECH_SVOOC;
    }
    if (watts >= kFastChargeWatts) {
        return CHG_TECH_VOOC;
    }
    return CHG_TECH_UNKNOWN;
}

const char* FastChargeDetector::technologyName(int id) {
    if (id < 0 || id >= static_cast<int>(sizeof(kTechNames) / sizeof(kTechNames[0]))) {
        return kTechNames[0];
    }
    return kTechNames[id];
}

int FastChargeDetector::technologyFromName(const std::string& name) {
    for (int i = 1; i < static_cast<int>(sizeof(kTechNames) / sizeof(kTechNames[0])); i++) {
        if (name == kTechNames[i]) {
            return i;
        }
    }
    return CHG_TECH_UNKNOWN;
}

// Oplus charger driver present: trust it, the kernel already made the call.
bool FastChargeDetector::readOplusDriver(FastChargeState* state) const {
    std::string tech;
    bool haveTech = readStringNode(mOplusChargeTechnology.c_str(), &tech);
    if (!haveTech) {
        haveTech = readStringNode(mPowerSupplyChargeTechnology.c_str(), &tech);
    }
    if (!haveTech) {
        return false;
    }

    int fastCharge = 0;
    if (!readIntNode(mOplusFastCharge.c_str(), &fastCharge)) {
        if (!readIntNode(mPowerSupplyFastCharge.c_str(), &fastCharge)) {
            return false;
        }
    }

    int watts = 0;
    readIntNode(mOplusPpschgPower.c_str(), &watts);

    int type = technologyFromName(tech);
    if (type == CHG_TECH_UNKNOWN) {
        // The oplus driver answered, but with a name this table does not
        // know (or an empty node). Reporting UNKNOWN here would hide the
        // charger-driver reading, so let the caller fall back to it.
        return false;
    }

    state->fastChgType = type;
    state->fastCharge = fastCharge;
    state->watts = reportWatts(watts);
    state->pumpEngaged = (fastCharge != 0);
    state->fromOplusDriver = true;
    state->valid = true;
    return true;
}

// Fallback source: classify the adapter power from the charger-driver nodes.
bool FastChargeDetector::readTranCharger(FastChargeState* state) const {
    int online = 1;
    if (readIntNode("/sys/class/power_supply/charger/online", &online)) {
        if (online == 0) {
            state->pumpEngaged = false;
            state->watts = 0;
            state->fastChgType = CHG_TECH_UNKNOWN;
            state->fastCharge = 0;
            state->fromOplusDriver = false;
            state->valid = true;
            return true;
        }
    } else {
        std::string status;
        if (readStringNode("/sys/class/power_supply/battery/status", &status)) {
            if (status == "Discharging" || status == "Not charging") {
                state->pumpEngaged = false;
                state->watts = 0;
                state->fastChgType = CHG_TECH_UNKNOWN;
                state->fastCharge = 0;
                state->fromOplusDriver = false;
                state->valid = true;
                return true;
            }
        }
    }

    int pump = 0;
    readIntNode(mPumpExpress.c_str(), &pump);

    int watts = 0;
    readIntNode(mAdapterCapacity.c_str(), &watts);
    if (watts <= 0) {
        readIntNode("/sys/class/power_supply/tran-auto-test/ta_power", &watts);
    }

    state->pumpEngaged = (pump != 0);
    state->watts = reportWatts(watts);
    state->fastChgType = classifyWatts(watts);
    // The watts decide: a 33 W..44 W adapter is a fast charge, 45 W and up
    // is SuperVOOC.  The pump is a separate signal (it is reported on its
    // own in the queryChargeInfo dump); gating on it would report a fast
    // charge as "not charging" whenever the adapter negotiates without the
    // VOOC pump.
    state->fastCharge = (state->fastChgType == CHG_TECH_VOOC ||
                         state->fastChgType == CHG_TECH_SVOOC)
            ? 1
            : 0;
    state->fromOplusDriver = false;
    state->valid = true;
    return true;
}

void FastChargeDetector::refresh() {
    FastChargeState next;
    if (!readOplusDriver(&next)) {
        next = FastChargeState();
        readTranCharger(&next);
    }
    std::lock_guard<std::mutex> guard(mLock);
    mState = next;
}

FastChargeState FastChargeDetector::state() const {
    std::lock_guard<std::mutex> guard(mLock);
    return mState;
}

int FastChargeDetector::getFastCharge() const {
    std::lock_guard<std::mutex> guard(mLock);
    return mState.fastCharge;
}

int FastChargeDetector::getFastChgType() const {
    std::lock_guard<std::mutex> guard(mLock);
    return mState.fastChgType;
}

int FastChargeDetector::getChargeTechnology() const {
    std::lock_guard<std::mutex> guard(mLock);
    return mState.fastChgType;
}

int FastChargeDetector::getPowerWatts() const {
    std::lock_guard<std::mutex> guard(mLock);
    return mState.watts;
}

bool FastChargeDetector::isPumpEngaged() const {
    std::lock_guard<std::mutex> guard(mLock);
    return mState.pumpEngaged;
}

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
