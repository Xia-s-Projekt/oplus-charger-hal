// feature_probe.cpp - call the charger getters that matter for the features
// the Oplus HAL exposes but this port's driver implements under other names:
//   bypass charging  - getPsyBatteryPchg / getPsyBatteryPchgResetCount
//   smart charging   - getSmartChgMode / getCustomSelectChgMode
//   battery health   - getPsyBatteryStatus / getPsyBatteryFcc / getPsyBatteryCC
//                      / getPsyBatteryRm / getBattGaugeInfo
//   fast charge      - getFastCharge / getPsyChargeTech / getPsyFastChgType
//
// Usage: feature_probe <service-instance>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <cstdio>
#include <string>

#include "aidl/vendor/oplus/hardware/charger/ICharger.h"

namespace {

using aidl::vendor::oplus::hardware::charger::ICharger;

void showInt(const char* name, ICharger* c,
             ::ndk::ScopedAStatus (ICharger::*method)(int32_t*)) {
    int32_t v = 0;
    auto st = (c->*method)(&v);
    if (!st.isOk()) {
        printf("%-26s FAIL status=%d\n", name, st.getExceptionCode());
        return;
    }
    printf("%-26s %d\n", name, v);
}

void showStr(const char* name, ICharger* c, ::ndk::ScopedAStatus (ICharger::*method)(std::string*)) {
    std::string s;
    auto st = (c->*method)(&s);
    if (!st.isOk()) {
        printf("%-26s FAIL status=%d\n", name, st.getExceptionCode());
        return;
    }
    printf("%-26s \"%s\"\n", name, s.c_str());
}

}  // namespace

int main(int argc, char** argv) {
    const char* instance = (argc > 1)
            ? argv[1]
            : "vendor.oplus.hardware.charger.ICharger/default";

    ABinderProcess_setThreadPoolMaxThreadCount(0);

    AIBinder* binder = AServiceManager_checkService(instance);
    if (binder == nullptr) {
        printf("FAIL service not found: %s\n", instance);
        return 1;
    }
    ::ndk::SpAIBinder sp(binder);
    std::shared_ptr<ICharger> charger = ICharger::fromBinder(sp);
    if (charger == nullptr) {
        printf("FAIL fromBinder returned null\n");
        return 1;
    }

    printf("== bypass charging ==\n");
    showInt("getPsyBatteryPchg", charger.get(), &ICharger::getPsyBatteryPchg);
    showInt("getPsyBatteryPchgResetCount", charger.get(), &ICharger::getPsyBatteryPchgResetCount);

    printf("\n== smart charging ==\n");
    showInt("getSmartChgMode", charger.get(), &ICharger::getSmartChgMode);
    showInt("getCustomSelectChgMode", charger.get(), &ICharger::getCustomSelectChgMode);
    showInt("getChargerCoolDown", charger.get(), &ICharger::getChargerCoolDown);

    printf("\n== battery health ==\n");
    showStr("getPsyBatteryStatus", charger.get(), &ICharger::getPsyBatteryStatus);
    showInt("getPsyBatteryFcc", charger.get(), &ICharger::getPsyBatteryFcc);
    showInt("getPsyBatteryCC", charger.get(), &ICharger::getPsyBatteryCC);
    showInt("getPsyBatteryRm", charger.get(), &ICharger::getPsyBatteryRm);
    showInt("getPsyBatteryLevel", charger.get(), &ICharger::getPsyBatteryLevel);
    showInt("getPsyBatteryNotify", charger.get(), &ICharger::getPsyBatteryNotify);

    printf("\n== fast charge ==\n");
    showInt("getFastCharge", charger.get(), &ICharger::getFastCharge);
    showInt("getPsyChargeTech", charger.get(), &ICharger::getPsyChargeTech);
    showInt("getPsyFastChgType", charger.get(), &ICharger::getPsyFastChgType);
    showInt("getBattPPSChgPower", charger.get(), &ICharger::getBattPPSChgPower);
    showInt("getBattVoocChgIng", charger.get(), &ICharger::getBattVoocChgIng);
    return 0;
}
