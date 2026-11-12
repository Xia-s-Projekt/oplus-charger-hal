// caller_id_probe.cpp - device-side proof of the bypass-charge caller-id split.
//
// The framework and GameSpace ask the charger HAL for flag 0x18 with
// DIFFERENT third arguments, and the HAL has to answer differently:
//
//   caller id 1 - OplusBatteryService.getBroadcastDataFromHal().  The answer
//       becomes mByPassChargeStatus and is published as the
//       "bypass_charge_status" battery broadcast extra, so it must be the
//       LIVE state of /sys/devices/platform/charger/bypass_charger (0 or 1).
//
//   any other id - the app-facing query.  GameSpace asks COSA for the device
//       SUPPORT code, which must be 3 when the device supports bypass
//       charging (the toolbox enables its switch only for a value > 1).
//
// This probe calls both and prints the answers, so the two behaviours can be
// compared on a running service.
//
// Usage:  caller_id_probe <service-instance>
//         (default: vendor.oplus.hardware.charger.ICharger/default)
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <cstdio>
#include <string>

#include "aidl/vendor/oplus/hardware/charger/BpCharger.h"
#include "aidl/vendor/oplus/hardware/charger/ICharger.h"

namespace {

int g_failures = 0;

void show(const char* name, const ndk::ScopedAStatus& status, const std::string& value) {
    if (!status.isOk()) {
        printf("FAIL  %-28s status=%d\n", name, status.getExceptionCode());
        ++g_failures;
        return;
    }
    printf("OK    %-28s -> \"%s\"\n", name, value.c_str());
}

}  // namespace

int main(int argc, char** argv) {
    const char* instance = (argc > 1)
            ? argv[1]
            : "vendor.oplus.hardware.charger.ICharger/default";

    ABinderProcess_setThreadPoolMaxThreadCount(0);

    AIBinder* binder = AServiceManager_checkService(instance);
    if (binder == nullptr) {
        printf("FAIL  service not found: %s\n", instance);
        return 1;
    }
    printf("found service: %s\n", instance);

    ::ndk::SpAIBinder sp(binder);
    std::shared_ptr<aidl::vendor::oplus::hardware::charger::ICharger> charger =
            aidl::vendor::oplus::hardware::charger::ICharger::fromBinder(sp);
    if (charger == nullptr) {
        printf("FAIL  fromBinder returned null\n");
        return 1;
    }

    std::string text;

    // The framework's own query: live state.
    show("getChgConfig(0x18, caller 1)", charger->getChgConfig(0x18, "", 1, &text), text);

    // The app-facing query: support code.
    show("getChgConfig(0x18, caller 2)", charger->getChgConfig(0x18, "", 2, &text), text);
    show("getChgConfig(0x18, caller 3)", charger->getChgConfig(0x18, "", 3, &text), text);

    // 0x19 shares the branch with 0x18.
    show("getChgConfig(0x19, caller 1)", charger->getChgConfig(0x19, "", 1, &text), text);
    show("getChgConfig(0x19, caller 2)", charger->getChgConfig(0x19, "", 2, &text), text);

    // The neighbouring flags must be unaffected by the split.
    show("getChgConfig(4, caller 1)", charger->getChgConfig(4, "", 1, &text), text);
    show("getChgConfig(6, caller 1)", charger->getChgConfig(6, "", 1, &text), text);

    printf("\n%s: %d failure(s)\n", g_failures == 0 ? "PASS" : "FAIL", g_failures);
    return g_failures == 0 ? 0 : 1;
}
