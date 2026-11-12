// ChargerManual.cpp - the part of the Charger class that is not a plain node
// access: the fast-charge state, the subsystem state and the diagnostic dumps.
//
// Everything here is written from the decompiled body of the matching shipped
// function; the file name and the shipped address are given per function.
#include "charger/Charger.h"

#include <android/log.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "charger/ChargerConstants.h"
#include "charger/BypassPolicy.h"
#include "charger/NodeControl.h"

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {

namespace {

// ---------------------------------------------------------------------------
// TestKit::KernelTest - the shipped test-kit client.
//
// Recovered from the shipped TestKit::KernelTest methods:
//   open()            ELF 0x3f0f8  open("/dev/chg_test_kit", O_RDWR)
//   close()           ELF 0x3f0bc
//   getName(uint)     ELF 0x3f2d4  ioctl(fd, 0x4004f601, &index)
//   getFeatureList()  ELF 0x3f370  ioctl(fd, 0x9000f604, buf)
//   getFeatureNum()   ELF 0x3f4e4  ioctl(fd, 0x8004f605, &num)
//   runTest(uint,str) ELF 0x3f7f4  ioctl(fd, 0x9000f606, buf)
// The ioctl numbers are _IOR('f', n, ...) / _IOWR('f', n, ...) with type 'f'.
// ---------------------------------------------------------------------------
const char kTestKitDevice[] = "/dev/chg_test_kit";
const unsigned int kIoctlGetName = 0x4004f601;      // _IOR('f', 1, unsigned int)
const unsigned int kIoctlGetFeatureList = 0x9000f604;  // _IOWR('f', 4, char[])
const unsigned int kIoctlGetFeatureNum = 0x8004f605;   // _IOR('f', 5, unsigned int)
const unsigned int kIoctlRunTest = 0x9000f606;         // _IOWR('f', 6, char[])

class KernelTest {
public:
    ~KernelTest() { close(); }

    // Returns 0 on success, the negative errno otherwise.
    int open() {
        if (mFd >= 0) {
            return 0;
        }
        mFd = ::open(kTestKitDevice, O_RDWR);
        return (mFd < 0) ? mFd : 0;
    }
    void close() {
        if (mFd >= 0) {
            ::close(mFd);
            mFd = -1;
        }
    }
    int getFeatureNum(unsigned int* num) {
        if (mFd < 0) {
            return -ENODEV;
        }
        if (ioctl(mFd, kIoctlGetFeatureNum, num) < 0) {
            return errno;
        }
        return 0;
    }
    int getFeatureList(std::string* out) {
        if (mFd < 0) {
            return -ENODEV;
        }
        char buf[512] = {0};
        if (ioctl(mFd, kIoctlGetFeatureList, buf) < 0) {
            return errno;
        }
        buf[sizeof(buf) - 1] = '\0';
        out->assign(buf);
        return 0;
    }
    int getName(unsigned int index, std::string* out) {
        if (mFd < 0) {
            return -ENODEV;
        }
        unsigned int idx = index;
        if (ioctl(mFd, kIoctlGetName, &idx) < 0) {
            return errno;
        }
        *out = std::to_string(idx);
        return 0;
    }
    int runTest(unsigned int index, std::string* out) {
        if (mFd < 0) {
            return -ENODEV;
        }
        unsigned int idx = index;
        struct {
            unsigned int index;
            char text[504];
        } payload;
        memset(&payload, 0, sizeof(payload));
        payload.index = idx;
        if (ioctl(mFd, kIoctlRunTest, &payload) < 0) {
            return errno;
        }
        out->assign(payload.text);
        return 0;
    }

private:
    int mFd = -1;
};

// Shared state of the HAL.  The shipped binary keeps these in globals
// (DAT_00153bb0 = custom chg mode, DAT_001521b0 = smart chg mode, ...).
struct ChargerState {
    int customSelectChgMode = 0;
    int smartChgMode = 0;
    int bccExpStatus = 0;
    int bmsHeatingStatus = 0;
    int bmsHeatingRunningStatus = 0;
    int chgExchangeMesgInit = 0;
    int chgExchangeSohMesgInit = 0;
    int uiSohValue = 0;
    int screenStatus = 0;
    int batteryLogPush = 0;
    int chgRusConfig = 0;
    int wirelessPenSoc = 0;
    int wlsThirdPartitionInfo = 0;
    std::string uisohDebugInfo;
    // The charger config the caller set last, keyed by the config flag.
    std::map<int, std::string> chgConfig;
    // The USB eye diagram set last, keyed by the model number.
    std::map<int, std::string> usbEyeDiagram;
    // The bypass-charge requests, kept so the node can be re-derived from
    // them instead of being latched at whatever value a caller last sent.
    bypass::Inputs bypass;
};

ChargerState& state() {
    static ChargerState s;
    return s;
}

int readIntPath(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return kReadError;
    }
    char buf[16] = {0};
    ssize_t n = read(fd, buf, sizeof(buf));
    close(fd);
    if (n < 0) {
        return kIoError;
    }
    int v = atoi(buf);
    return (v == kParseError) ? kReadError : v;
}

std::string readStringPath(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return std::string();
    }
    char buf[256] = {0};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n < 0) {
        return std::string();
    }
    buf[n] = '\0';
    return std::string(buf);
}

bool writeStringPath(const char* path, const std::string& value) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        return false;
    }
    ssize_t written = write(fd, value.c_str(), value.size());
    close(fd);
    return written == static_cast<ssize_t>(value.size());
}

void appendInt(std::string* out, const char* key, int value) {
    char line[256];
    snprintf(line, sizeof(line), "%s=%d\n", key, value);
    out->append(line);
}

void appendString(std::string* out, const char* key, const std::string& value) {
    out->append(key);
    out->append("=");
    out->append(value);
    out->append("\n");
}

std::mutex gDecimalMutex;
std::chrono::steady_clock::time_point gLastStepTime;
int gDecimalStep = 0;
bool gHoldingAt99 = false;

const int kDecimalTargets[] = {
    5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60,
    63, 66, 69, 72, 75, 78,
    80, 82, 84, 86, 88, 90,
    91, 92, 93, 94, 95, 96, 97, 98, 99
};
const int kNumDecimalTargets = sizeof(kDecimalTargets) / sizeof(kDecimalTargets[0]);

void resetDecimalState() {
    std::lock_guard<std::mutex> lock(gDecimalMutex);
    gDecimalStep = 0;
    gHoldingAt99 = false;
    gLastStepTime = {};
}

void triggerBatteryUpdate(bool force = false) {
    static std::mutex triggerMutex;
    static std::chrono::steady_clock::time_point lastTriggerTime;
    {
        std::lock_guard<std::mutex> lock(triggerMutex);
        auto now = std::chrono::steady_clock::now();
        auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTriggerTime).count();
        if (!force && diffMs < 75) {
            return;
        }
        lastTriggerTime = now;
    }

    pid_t pid = fork();
    if (pid == 0) {
        pid_t p2 = fork();
        if (p2 == 0) {
            int devnull = open("/dev/null", O_RDWR);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
            char* const argv[] = {
                (char*)"/system/bin/service",
                (char*)"call",
                (char*)"batteryproperties",
                (char*)"2",
                nullptr
            };
            execv("/system/bin/service", argv);
            _exit(127);
        }
        _exit(0);
    }
    if (pid > 0) {
        waitpid(pid, nullptr, 0);
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Charger life cycle
// ---------------------------------------------------------------------------

Charger::Charger() {
    mFastCharge.reset(new FastChargeDetector());
    mDetection.reset(new ChgDetectionCheck());
    mRunning.store(false);
    mMonitorThread = 0;
}

Charger::~Charger() {
    if (mRunning.load()) {
        mRunning.store(false);
        if (mMonitorThread != 0) {
            pthread_join(mMonitorThread, nullptr);
            mMonitorThread = 0;
        }
    }
    mDetection.reset();
    mFastCharge.reset();
}

void* Charger::monitorThreadFunc(void* arg) {
    auto* self = static_cast<Charger*>(arg);
    self->runMonitorLoop();
    return nullptr;
}

// Re-derives the bypass node from the live requests.  Returns the value it
// decided on, or -1 when nothing is requesting bypass and the node must be
// left alone.
int applyBypassPolicy(bool force);

void Charger::runMonitorLoop() {
    int lastWatts = -1;
    int lastOnline = -1;
    std::string lastStatus = "";
    int pulseCountdown = 0;
    auto onlineSinceTime = std::chrono::steady_clock::now();
    auto lastPulseTime = std::chrono::steady_clock::now();
    int lastBypassProp = -1;

    const char* envCapNode = getenv("OPLUS_CHARGER_CAPACITY_NODE");
    const char* adapterCapacityPath = (envCapNode != nullptr && envCapNode[0] != '\0')
            ? envCapNode
            : "/sys/devices/platform/charger/adapter_capacity";

    int ueventFd = socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, NETLINK_KOBJECT_UEVENT);
    if (ueventFd >= 0) {
        struct sockaddr_nl sa;
        memset(&sa, 0, sizeof(sa));
        sa.nl_family = AF_NETLINK;
        sa.nl_groups = 1;
        if (bind(ueventFd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
            close(ueventFd);
            ueventFd = -1;
        }
    }

    while (mRunning.load(std::memory_order_relaxed)) {
        auto now = std::chrono::steady_clock::now();
        auto onlineElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - onlineSinceTime).count();
        int pollTimeoutMs = (lastOnline == 1 && lastWatts < kSuperVoocWatts && onlineElapsedMs < 15000) ? 20 : 100;
        if (ueventFd >= 0) {
            struct pollfd pfd;
            pfd.fd = ueventFd;
            pfd.events = POLLIN;
            int ret = poll(&pfd, 1, pollTimeoutMs);
            if (ret > 0 && (pfd.revents & POLLIN)) {
                char buf[2048];
                while (recv(ueventFd, buf, sizeof(buf), MSG_DONTWAIT) > 0) {
                    // Drain socket
                }
            }
        } else {
            usleep(pollTimeoutMs * 1000);
        }

        int bypassProp = isBypassPropEnabled() ? 1 : 0;
        if (bypassProp != lastBypassProp) {
            lastBypassProp = bypassProp;
            // The prop is the user's intent; when it is off the kernel owns
            // the node, so the policy only runs when something is actually
            // requesting bypass.
            if (bypassProp == 1) {
                state().bypass.gameSwitch = 1;
                state().bypass.upLimitEnabled = 1;
            } else {
                state().bypass.gameSwitch = 0;
                state().bypass.upLimitEnabled = 0;
            }
            int applied = applyBypassPolicy(true);
            if (applied >= 0) {
                triggerBatteryUpdate(true);
            }
        }

        int online = readIntPath("/sys/class/power_supply/charger/online");
        if (online <= 0 || online == kReadError || online == kIoError) {
            int usbOnline = readIntPath("/sys/class/power_supply/usb/online");
            if (usbOnline == 1) {
                online = 1;
            } else if (online == kReadError || online == kIoError) {
                online = 0;
            }
        }

        std::string status = readStringPath("/sys/class/power_supply/battery/status");
        while (!status.empty() && (status.back() == '\n' || status.back() == '\r' || status.back() == ' ')) {
            status.pop_back();
        }

        int watts = 0;
        if (online == 1 || status == "Charging") {
            watts = readIntPath(adapterCapacityPath);
            if (watts <= 0 || watts == kReadError || watts == kIoError) {
                watts = readIntPath("/sys/class/power_supply/tran-auto-test/ta_power");
                if (watts == kReadError || watts == kIoError) {
                    watts = 0;
                }
            }
        }
        if (online == 0 && (status == "Discharging" || status == "Not charging")) {
            watts = 0;
        }
        if (watts < kMinReportWatts) {
            watts = 0;
        }

        now = std::chrono::steady_clock::now();
        bool stateChanged = (watts != lastWatts || online != lastOnline || status != lastStatus);
        if (stateChanged) {
            if (online == 1 && lastOnline != 1) {
                onlineSinceTime = now;
                resetDecimalState();
            } else if (online == 0 || (watts >= kSuperVoocWatts && lastWatts < kSuperVoocWatts)) {
                resetDecimalState();
            }

            __android_log_print(ANDROID_LOG_INFO, "vendor.oplus.hardware.charger-V1-service",
                                "Power change detected: watts=%d->%d online=%d->%d status=%s->%s",
                                lastWatts, watts, lastOnline, online,
                                lastStatus.c_str(), status.c_str());
            lastWatts = watts;
            lastOnline = online;
            lastStatus = status;

            if (mFastCharge) {
                mFastCharge->refresh();
            }
            triggerBatteryUpdate(true);
            pulseCountdown = (online == 1) ? 4 : 1;
            lastPulseTime = now;
        } else if (pulseCountdown > 0) {
            auto pulseElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPulseTime).count();
            if (pulseElapsedMs >= 150) {
                pulseCountdown--;
                lastPulseTime = now;
                if (mFastCharge) {
                    mFastCharge->refresh();
                }
                triggerBatteryUpdate(true);
            }
        }
    }

    if (ueventFd >= 0) {
        close(ueventFd);
    }
}

void Charger::start() {
    if (!mRunning.exchange(true)) {
        pthread_create(&mMonitorThread, nullptr, &Charger::monitorThreadFunc, this);
    }
}

// ---------------------------------------------------------------------------
// Fast charge - the 33 W / 45 W classification.
//
// getFastCharge / getPsyFastChgType / getPsyChargeTech all read the same
// state in the shipped binary; here they read the FastChargeDetector, which
// classifies the charging power:
//
//     >= 45 W  -> SuperVOOC   (charge technology SVOOC, id 2)
//     >= 33 W  -> fast charge (charge technology VOOC,  id 1)
//     <  33 W  -> normal
//
// The detector prefers the Oplus charger driver when it is present and falls
// back to the Transsion charger driver's Pump_Express / adapter_capacity
// pair, which is where the watts come from on this port.
// ---------------------------------------------------------------------------


namespace {

int toExternalChargeTechnology(int internalTech) {
    switch (internalTech) {
        case CHG_TECH_SVOOC:
            return 3;
        case CHG_TECH_VOOC:
            return 2;
        case CHG_TECH_PD:
        case CHG_TECH_QC:
        case CHG_TECH_PPS:
        case CHG_TECH_UFCS:
            return 1;
        case CHG_TECH_UNKNOWN:
        default:
            return 0;
    }
}

}  // namespace

::ndk::ScopedAStatus Charger::getFastCharge(int* _out) {
    if (mFastCharge) {
        mFastCharge->refresh();
    }
    *_out = mFastCharge ? mFastCharge->getFastCharge() : 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::getPsyFastChgType(int* _out) {
    if (mFastCharge) {
        mFastCharge->refresh();
    }
    *_out = mFastCharge ? mFastCharge->getFastChgType() : CHG_TECH_UNKNOWN;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::getPsyChargeTech(int* _out) {
    if (mFastCharge) {
        mFastCharge->refresh();
    }
    *_out = mFastCharge ? toExternalChargeTechnology(mFastCharge->getChargeTechnology()) : 0;
    return ::ndk::ScopedAStatus::ok();
}

// Shipped ELF 0x1cf54: reads voocchg_ing, and when the wireless driver
// holds the status the cached value is returned instead.
::ndk::ScopedAStatus Charger::getBattVoocChgIng(int* _out) {
    int value = readIntPath("/sys/devices/platform/charger/fast_chg_indicator");
    if (value == kReadError || value == kIoError) {
        value = readIntPath("/sys/class/oplus_chg/battery/voocchg_ing");
        if (value == kReadError || value == kIoError) {
            value = readIntPath("/sys/devices/platform/charger/fast_chg_indicator");
        }
    }
    if (value == kReadError || value == kIoError) {
        value = mFastCharge ? (mFastCharge->getChargeTechnology() ==
                                CHG_TECH_SVOOC ? 1 : 0)
                             : 0;
    }
    *_out = value;
    return ::ndk::ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// Member state
// ---------------------------------------------------------------------------

::ndk::ScopedAStatus Charger::getCustomSelectChgMode(int* _out) {
    *_out = state().customSelectChgMode;
    return ::ndk::ScopedAStatus::ok();
}

// Re-derives the bypass node from the live requests.  Returns the value it
// decided on so callers can tell whether anything actually changed.
int applyBypassPolicy(bool force) {
    bypass::Inputs& in = state().bypass;
    in.capacity = readIntPath("/sys/class/power_supply/battery/capacity");
    int value = 0;
    bool anyRequest = bypass::decide(in, &value);
    // No request at all: the caller keeps the node as it is, so a feature
    // that has never been asked for must not disturb the node.
    if (!anyRequest && !force) {
        return -1;
    }
    std::string node = std::to_string(value);
    writeStringPath("/sys/devices/platform/charger/bypass_charger", node);
    writeStringPath("/sys/devices/platform/charger/tran_aichg_disable_charger", node);
    // init re-applies the prop to the node on every boot
    // (tran_optimize.rc), so the prop has to follow the node or the setting
    // is lost across a reboot.  Only write on a change: writing the prop
    // fires the init trigger, which writes the node back.
    if ((isBypassPropEnabled() ? 1 : 0) != value) {
        setBypassProp(value);
    }
    return value;
}

::ndk::ScopedAStatus Charger::setCustomSelectChgMode(int arg0, bool arg1, int* _out) {
    state().customSelectChgMode = arg1 ? arg0 : 0;
    // A custom charge mode selection is a charging-mode switch, not a bypass
    // request: it must not latch the bypass node on its own.  The prop is the
    // only thing that owns that node, and it is applied by the monitor loop.
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::getSmartChgMode(int* _out) {
    *_out = state().smartChgMode;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::getBccExpStatus(int* _out) {
    *_out = state().bccExpStatus;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::setChgStatusToBcc(int arg0, int* _out) {
    state().bccExpStatus = arg0;
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::getBmsHeatingStatus(int* _out) {
    *_out = state().bmsHeatingStatus;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::getBmsHeatingRunningStatus(std::string* _out) {
    appendInt(_out, "bms_heating_running_status", state().bmsHeatingRunningStatus);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::chgExchangeMesgInit(int* _out) {
    state().chgExchangeMesgInit = 1;
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::chgExchangeSohMesgInit(int* _out) {
    state().chgExchangeSohMesgInit = 1;
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::notifyScreenStatus(int arg0, int* _out) {
    state().screenStatus = arg0;
    if (arg0 == 1) {
        if (mFastCharge) {
            mFastCharge->refresh();
        }
        triggerBatteryUpdate();
    }
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::setChgRusConfig(const std::string& arg0, int* _out) {
    state().chgRusConfig = atoi(arg0.c_str());
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::setWlsThirdPartitionInfo(const std::string& arg0, int* _out) {
    state().wlsThirdPartitionInfo = atoi(arg0.c_str());
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// SOH (state of health)
// ---------------------------------------------------------------------------

::ndk::ScopedAStatus Charger::getUIsohValue(int* _out) {
    int soh = readIntPath("/sys/devices/platform/charger/battery_health_percent");
    if (soh <= 0 || soh > 100 || soh == kReadError || soh == kIoError) {
        soh = (state().uiSohValue > 0) ? state().uiSohValue : 100;
    }
    *_out = soh;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::updateUiSohToPartion(int* _out) {
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::setUisohDebugInfo(const std::string& arg0, int* _out) {
    state().uisohDebugInfo = arg0;
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::getUisohDebugParameterInfo(std::string* _out) {
    appendString(_out, "uisoh_debug_info", state().uisohDebugInfo);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::healthd_update_ui_soc_decimal(std::string* _out) {
    if (mFastCharge) {
        mFastCharge->refresh();
    }
    int capacity = readIntPath("/sys/class/power_supply/battery/capacity");
    if (capacity <= 0 || capacity > 100 || capacity == kReadError || capacity == kIoError) {
        capacity = readIntPath("/sys/class/power_supply/tran-auto-test/capacity");
    }
    if (capacity <= 0 || capacity == kReadError || capacity == kIoError) {
        capacity = 50;
    }
    if (capacity >= 100) {
        capacity = 99;
    }

    bool isSuperVooc = (mFastCharge && mFastCharge->getChargeTechnology() == CHG_TECH_SVOOC);
    int online = readIntPath("/sys/class/power_supply/charger/online");
    if (online == kReadError || online == kIoError) {
        online = 1;
    }
    std::string status = readStringPath("/sys/class/power_supply/battery/status");
    while (!status.empty() && (status.back() == '\n' || status.back() == '\r' || status.back() == ' ')) {
        status.pop_back();
    }
    bool isDischarging = (online == 0 && (status == "Discharging" || status == "Not charging"));

    std::lock_guard<std::mutex> lock(gDecimalMutex);
    if (isDischarging || online == 0 || !isSuperVooc) {
        gDecimalStep = 0;
        gHoldingAt99 = false;
        gLastStepTime = {};
        int val = capacity * 100;
        *_out = std::to_string(val) + "," + std::to_string(val);
        return ::ndk::ScopedAStatus::ok();
    }

    auto now = std::chrono::steady_clock::now();
    int startDec = 0;
    int endDec = 0;

    if (gHoldingAt99 || gDecimalStep >= kNumDecimalTargets) {
        gHoldingAt99 = true;
        startDec = 99;
        endDec = 99;
    } else {
        if (gLastStepTime.time_since_epoch().count() == 0) {
            gLastStepTime = now;
        } else {
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - gLastStepTime).count();
            if (elapsedMs >= 700) {
                gDecimalStep++;
                gLastStepTime = now;
            }
        }

        if (gDecimalStep >= kNumDecimalTargets) {
            gHoldingAt99 = true;
            startDec = 99;
            endDec = 99;
        } else {
            startDec = (gDecimalStep == 0) ? 0 : kDecimalTargets[gDecimalStep - 1];
            endDec = kDecimalTargets[gDecimalStep];
        }
    }

    int startVal = capacity * 100 + startDec;
    int endVal = capacity * 100 + endDec;
    *_out = std::to_string(startVal) + "," + std::to_string(endVal);
    return ::ndk::ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// /dev/oplus_chg
//
// Shipped ELF 0x1b54c (getChargerControl): open("/dev/oplus_chg"), read 255
// bytes, close, the buffer becomes the result string.
// ---------------------------------------------------------------------------

::ndk::ScopedAStatus Charger::getChargerControl(std::string* _out) {
    int fd = open("/dev/oplus_chg", O_RDONLY);
    if (fd < 0) {
        return ::ndk::ScopedAStatus::ok();
    }
    char buf[256] = {0};
    ssize_t n = read(fd, buf, 0xff);
    close(fd);
    if (n < 0) {
        return ::ndk::ScopedAStatus::ok();
    }
    buf[0xff] = '\0';
    _out->assign(buf);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::setChargerControl(const std::string& arg0, int* _out) {
    if (arg0.empty() || arg0.size() > 16) {
        *_out = -1;
        return ::ndk::ScopedAStatus::ok();
    }
    int fd = open("/dev/oplus_chg", O_WRONLY);
    if (fd < 0) {
        *_out = kReadError;
        return ::ndk::ScopedAStatus::ok();
    }
    ssize_t written = write(fd, arg0.c_str(), arg0.size());
    close(fd);
    *_out = (written < 0) ? (int)0x80000003 : (int)written;
    return ::ndk::ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// nightstandby
//
// Shipped ELF 0x1c3f0: the argument is formatted with snprintf into a 32 byte
// buffer and written to soc_ajust, NUL terminator included, with
// /sys/class/power_supply/battery/soc_ajust tried first.
// ---------------------------------------------------------------------------
::ndk::ScopedAStatus Charger::nightstandby(int arg0, int* _out) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", arg0);
    size_t len = strlen(buf);
    int fd = open("/sys/class/power_supply/battery/soc_ajust", O_WRONLY);
    if (fd < 0) {
        fd = open("/sys/class/oplus_chg/battery/soc_ajust", O_WRONLY);
        if (fd < 0) {
            *_out = kReadError;
            return ::ndk::ScopedAStatus::ok();
        }
    }
    ssize_t written = write(fd, buf, len + 1);
    close(fd);
    *_out = (written < 0) ? (int)0x80000003 : (int)written;
    return ::ndk::ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// Charger config (getChgConfig / setChgConfig)
//
// The framework reads the charger configuration through these two calls:
//
//   OplusBatteryService.getBroadcastDataFromHal():
//       mCPAChargeWattage   = parseInt(getChgConfig(4,  "", 1))
//       mIsSupSpeedCharge   = parseInt(getChgConfig(6,  "", 1))
//       mByPassChargeStatus = parseInt(getChgConfig(0x18,"",1))
//
// so flag 4 is the CPA charge wattage, flag 6 the "is super speed charge"
// state and flag 0x18 the bypass-charge state.  The result is parsed with
// Integer.parseInt(), so the answer must always be a decimal integer - the
// framework catches a NumberFormatException and leaves the field at its
// previous value otherwise.
//
// The values come from the MTK charger driver:
//   * the CPA wattage is the adapter's output in watts
//     (/sys/devices/platform/charger/adapter_capacity),
//   * the super-speed-charge state is a flag, not the charge technology:
//     the framework puts it into the "is_support_speed_charge" broadcast
//     extra and SystemUI compares it against 1, so it must be 0 or 1,
//   * the bypass-charge state is the charger's bypass node.
//
// The extra argument and the caller name are the Oplus pass-through
// parameters; neither selects a different source here.
// ---------------------------------------------------------------------------

// The bypass-charge state, /sys/devices/platform/charger/bypass_charger.
// The MTK charger driver exposes it as a 0/1 flag.
//
// The node is the source of truth: the policy derives it from the live
// requests, so reporting the prop instead would tell the framework bypass is
// on while the battery is still charging.
int bypassChargeStatus() {
    int value = readIntPath("/sys/devices/platform/charger/bypass_charger");
    if (value == kReadError || value == kIoError) {
        return isBypassPropEnabled() ? 1 : 0;
    }
    return value;
}

// The MTK charger driver only accepts a bare 0 or 1 on the bypass node, but the
// Oplus callers pass a compound payload: "1+switch=1" to enable and
// "1+switch=0" to disable.  Take the leading integer and drop the rest.
std::string bypassNodeValue(const std::string& value) {
    size_t end = 0;
    while (end < value.size() && value[end] >= '0' && value[end] <= '9') {
        ++end;
    }
    if (end == 0) {
        return std::string();
    }
    return value.substr(0, end);
}

// The bypass node exists whenever the MTK charger driver exposes it, which is
// what the Oplus support query asks about.
bool bypassChargeSupported() {
    return access("/sys/devices/platform/charger/bypass_charger", F_OK) == 0;
}


::ndk::ScopedAStatus Charger::getChgConfig(
    int arg0,
    const std::string& value1,
    int arg2,
    std::string* _out) {
    if (mFastCharge) {
        mFastCharge->refresh();
    }
    int result = 0;
    switch (arg0) {
        case 0:
            result = mFastCharge ? toExternalChargeTechnology(mFastCharge->getChargeTechnology()) : 0;
            break;
        case 1:
            result = mFastCharge ? mFastCharge->getPowerWatts() : 0;
            break;
        case 4:
            result = mFastCharge ? mFastCharge->getPowerWatts() : 0;
            break;
        case 6:
            result = mFastCharge ? mFastCharge->getFastCharge() : 0;
            break;
        case 0x18:
        case 0x19:
            // The third argument is the Oplus caller id.  The framework
            // (OplusBatteryService) asks with 1 and wants the live state,
            // while GameSpace (through COSA) asks with 3 and wants the
            // support code, where 3 means the device supports bypass
            // charging.  Returning the raw node value to the second caller
            // makes it treat the device as unsupported.
            if (arg2 == 3) {
                result = bypassChargeSupported() ? 3 : 0;
            } else {
                result = bypassChargeStatus();
            }
            break;
        default:
            result = 0;
            break;
    }
    *_out = std::to_string(result);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::setChgConfig(
    int arg0,
    const std::string& value1,
    int arg2,
    int* _out) {
    state().chgConfig[arg0] = value1;
    switch (arg0) {
        case 0x18:
        case 0x19: {
            int targetPercent = 0;
            int sw = bypass::parseGameToggle(value1, &targetPercent);
            if (sw < 0) {
                break;
            }
            state().bypass.gameSwitch = sw;
            state().bypass.gameTargetPercent = targetPercent;
            applyBypassPolicy(false);
            break;
        }
        case 0x5:
        case 0xe: {
            int st = -1;
            int percent = 0;
            bypass::parseUpLimit(value1, &st, &percent);
            if (st >= 0) {
                state().bypass.upLimitEnabled = st;
            }
            if (percent > 0) {
                state().bypass.upLimitPercent = percent;
            }
            applyBypassPolicy(false);
            break;
        }
        default:
            break;
    }
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// USB eye diagram (getUsbCurrentEyeDiagram / setUsbEyeDiagram)
//
// The framework only forwards the value: OplusUsbDeviceFeature validates
// the eye-diagram string against "^[0-9a-zA-Z#,:_-]+$" and passes it on,
// and the result of the read is handed to the caller as-is.  There is no
// USB eye-diagram node on this device, so the last value set is kept and
// returned again.
// ---------------------------------------------------------------------------

::ndk::ScopedAStatus Charger::setUsbEyeDiagram(
    int arg0,
    const std::string& value1,
    bool enable2,
    int* _out) {
    state().usbEyeDiagram[arg0] = value1;
    *_out = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::getUsbCurrentEyeDiagram(int arg0, std::string* _out) {
    auto it = state().usbEyeDiagram.find(arg0);
    *_out = (it == state().usbEyeDiagram.end()) ? std::string() : it->second;
    return ::ndk::ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// Test kit
// ---------------------------------------------------------------------------

::ndk::ScopedAStatus Charger::testKitGetFeatureNum(int* _out) {
    KernelTest kit;
    unsigned int num = 0;
    if (kit.open() == 0 && kit.getFeatureNum(&num) == 0) {
        *_out = (int)num;
    } else {
        *_out = -1;
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::testKitGetFeatureList(std::string* _out) {
    KernelTest kit;
    std::string list;
    if (kit.open() == 0 && kit.getFeatureList(&list) == 0) {
        *_out = list;
    } else {
        *_out = std::string();
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::testKitGetFeatureName(int arg0, std::string* _out) {
    KernelTest kit;
    std::string name;
    if (kit.open() == 0 && kit.getName((unsigned int)arg0, &name) == 0) {
        *_out = name;
    } else {
        *_out = std::string();
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::testKitFeatureTest(
        int arg0,
        ::aidl::vendor::oplus::hardware::charger::testKitFeatureTestResult* _out) {
    KernelTest kit;
    std::string result;
    int rc = kit.open();
    if (rc == 0) {
        rc = kit.runTest((unsigned int)arg0, &result);
    }
    _out->ret = rc;
    _out->str = result;
    return ::ndk::ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// Diagnostic dumps
//
// Shipped ELF 0x123238 (queryChargeInfo, 8452 bytes): reads every charger
// node in turn and appends "key=value\n" to the result.  The key names and
// the node list are the shipped ones.
// ---------------------------------------------------------------------------

::ndk::ScopedAStatus Charger::queryChargeInfo(std::string* _out) {
    _out->clear();

    appendInt(_out, "bcc_exp_status", state().bccExpStatus);
    appendInt(_out, "battery_capacity",
              readIntPath("/sys/class/power_supply/battery/capacity"));
    appendInt(_out, "battery_voltage_now",
              readIntPath("/sys/class/power_supply/battery/voltage_now"));
    appendInt(_out, "battery_voltage_min",
              readIntPath("/sys/class/power_supply/battery/voltage_min"));
    appendInt(_out, "battery_temp",
              readIntPath("/sys/class/power_supply/battery/temp"));
    appendInt(_out, "battery_current_now",
              readIntPath("/sys/class/power_supply/battery/current_now"));
    appendInt(_out, "battery_charge_now",
              readIntPath("/sys/class/power_supply/battery/charge_now"));
    appendInt(_out, "battery_sub_current",
              readIntPath("/sys/class/power_supply/battery/current_avg"));
    appendInt(_out, "usb_fast_chg_type",
              readIntPath("/sys/class/power_supply/usb/fast_chg_type"));
    appendInt(_out, "battery_voocchg_ing",
              readIntPath("/sys/devices/platform/charger/fast_chg_indicator"));
    appendInt(_out, "battery_ppschg_ing",
              readIntPath("/sys/devices/platform/charger/fast_chg_indicator"));
    appendInt(_out, "battery_ppschg_power",
              readIntPath("/sys/class/oplus_chg/battery/ppschg_power"));
    appendInt(_out, "usb_input_current_now",
              readIntPath("/sys/class/power_supply/charger/input_current_limit"));
    appendInt(_out, "battery_short_ic_otp_status",
              readIntPath("/sys/class/power_supply/battery/short_ic_otp_status"));
    appendInt(_out, "battery_authenticate",
              readIntPath("/sys/class/power_supply/battery/authenticate"));
    appendInt(_out, "battery_charge_technology",
              readIntPath("/sys/class/power_supply/battery/charge_technology"));
    appendInt(_out, "parallel_chg_mos_status",
              readIntPath("/sys/class/oplus_chg/battery/parallel_chg_mos_status"));
    appendInt(_out, "wireless_current_now",
              readIntPath("/sys/class/power_supply/wireless/current_now"));
    appendString(_out, "wireless_rx_version",
                 readStringPath("/proc/wireless/rx_version"));
    appendString(_out, "wireless_tx_version",
                 readStringPath("/proc/wireless/tx_version"));
    appendInt(_out, "wireless_idt_adc_test",
              readIntPath("/proc/wireless/idt_adc_test"));
    appendString(_out, "wireless_real_type",
                 readStringPath("/sys/class/power_supply/wireless/real_type"));
    appendInt(_out, "wireless_charge_pump_en",
              readIntPath("/proc/wireless/charge_pump_en"));
    appendString(_out, "wireless_deviated", readStringPath("/proc/wireless/deviated"));
    appendString(_out, "battery_status",
                 readStringPath("/sys/class/power_supply/battery/status"));
    appendString(_out, "wireless_voltage_now",
                 readStringPath("/sys/class/power_supply/wireless/voltage_now"));
    appendString(_out, "battery_type_str",
                 readStringPath("/sys/class/oplus_chg/battery/battery_type"));

    // The fast-charge block is the part this port has to fill in itself.
    appendInt(_out, "fast_charge", mFastCharge ? mFastCharge->getFastCharge() : 0);
    appendInt(_out, "charge_technology",
              mFastCharge ? mFastCharge->getChargeTechnology() : CHG_TECH_UNKNOWN);
    appendInt(_out, "chg_power_watts", mFastCharge ? mFastCharge->getPowerWatts() : 0);
    appendInt(_out, "pump_express", mFastCharge && mFastCharge->isPumpEngaged() ? 1 : 0);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::queryWlsPencilInfo(std::string* _out) {
    _out->clear();
    appendString(_out, "ble_mac_addr",
                 readStringPath("/sys/class/oplus_wireless/pencil/ble_mac_addr"));
    appendInt(_out, "ping_time", readIntPath("/sys/class/oplus_wireless/pencil/ping_time"));
    appendInt(_out, "tx_status", readIntPath("/sys/class/oplus_wireless/pencil/tx_status"));
    appendInt(_out, "tx_current",
              readIntPath("/sys/class/oplus_wireless/pencil/tx_current"));
    appendInt(_out, "tx_voltage",
              readIntPath("/sys/class/oplus_wireless/pencil/tx_voltage"));
    appendString(_out, "fw_version",
                 readStringPath("/sys/class/oplus_wireless/pencil/fw_version"));
    appendInt(_out, "wireless_ic_id",
              readIntPath("/sys/class/oplus_wireless/pencil/wireless_ic_id"));
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Charger::getBccCsvData(std::string* _out) {
    _out->clear();
    appendInt(_out, "cool_down", readIntPath("/sys/devices/platform/charger/Thermal_throttle"));
    appendInt(_out, "chip_soc", readIntPath("/sys/class/power_supply/battery/chip_soc"));
    appendInt(_out, "capacity", readIntPath("/sys/class/power_supply/battery/capacity"));
    appendString(_out, "status",
                 readStringPath("/sys/class/power_supply/battery/status"));
    return ::ndk::ScopedAStatus::ok();
}

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
