// What setChgConfig(0x19, "1+switch=0") does to the bypass node.
//
// The COSA app sends flag 0x19 with the payload "1+switch=1" (on) or
// "1+switch=0" (off).  The HAL's bypassNodeValue() takes the leading integer
// of that payload, so both payloads start with "1" and the node always ends
// up 1.  This probe calls the shipped service and reads the node back.

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <cstdio>
#include <string>

#include "aidl/vendor/oplus/hardware/charger/BpCharger.h"
#include "aidl/vendor/oplus/hardware/charger/ICharger.h"

namespace {

void dumpNode(const char* label) {
    char buf[16] = {0};
    FILE* f = fopen("/sys/devices/platform/charger/bypass_charger", "r");
    if (f) {
        if (fgets(buf, sizeof(buf), f)) {
            printf("  %s: bypass_charger = %s", label, buf);
        }
        fclose(f);
    } else {
        printf("  %s: bypass_charger node missing\n", label);
    }

    buf[0] = '\0';
    f = fopen("/sys/devices/platform/charger/tran_aichg_disable_charger", "r");
    if (f) {
        if (fgets(buf, sizeof(buf), f)) {
            printf("  %s: tran_aichg_disable_charger = %s", label, buf);
        }
        fclose(f);
    }
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

    ::ndk::SpAIBinder sp(binder);
    std::shared_ptr<aidl::vendor::oplus::hardware::charger::ICharger> charger =
            aidl::vendor::oplus::hardware::charger::ICharger::fromBinder(sp);
    if (charger == nullptr) {
        printf("FAIL  fromBinder returned null\n");
        return 1;
    }
    auto svc = charger;

    dumpNode("before");

    int rc = 0;
    std::string out;

    // The exact COSA "off" payload: flag 0x19, "1+switch=0".
    auto status = svc->setChgConfig(0x19, "1+switch=0", 3, &rc);
    printf("setChgConfig(0x19, \"1+switch=0\", 3) status=%d rc=%d\n",
           static_cast<int>(status.getExceptionCode()), rc);
    dumpNode("after-switch=0");

    // The exact COSA "on" payload: flag 0x19, "1+switch=1".
    status = svc->setChgConfig(0x19, "1+switch=1", 3, &rc);
    printf("setChgConfig(0x19, \"1+switch=1\", 3) status=%d rc=%d\n",
           static_cast<int>(status.getExceptionCode()), rc);
    dumpNode("after-switch=1");

    // What the HAL reports back for each caller id.
    for (int caller : {1, 2, 3}) {
        status = svc->getChgConfig(0x18, "", caller, &out);
        printf("getChgConfig(0x18, caller=%d) -> \"%s\"\n", caller, out.c_str());
    }

    return 0;
}
