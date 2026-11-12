// Which SELinux domain can write /sys/devices/platform/charger/bypass_charger?
//
// setChgConfig(0x5, "0") writes the literal "0" to the bypass node, bypassing
// the compound-payload parser entirely.  If the node changes, the calling
// domain has write access; if it does not, the write was denied.
//
// Usage:  perm_probe <service-instance> <flag> <value>

#include <android/binder_manager.h>
#include <android/binder_process.h>

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
        printf("  %s: node unreadable\n", label);
    }
}

}  // namespace

int main(int argc, char** argv) {
    const char* instance = (argc > 1)
            ? argv[1]
            : "vendor.oplus.hardware.charger.ICharger/default";
    const int flag = (argc > 2) ? atoi(argv[2]) : 0x5;
    const char* value = (argc > 3) ? argv[3] : "0";

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    AIBinder* binder = AServiceManager_checkService(instance);
    if (binder == nullptr) {
        printf("FAIL  service not found: %s\n", instance);
        return 1;
    }

    ::ndk::SpAIBinder sp(binder);
    auto charger = aidl::vendor::oplus::hardware::charger::ICharger::fromBinder(sp);
    if (charger == nullptr) {
        printf("FAIL  fromBinder returned null\n");
        return 1;
    }

    dumpNode("before");
    int rc = 0;
    auto status = charger->setChgConfig(flag, value, 3, &rc);
    printf("setChgConfig(0x%x, \"%s\", 3) status=%d rc=%d\n",
           flag, value, static_cast<int>(status.getExceptionCode()), rc);
    dumpNode("after");
    return 0;
}
