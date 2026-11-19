// probe.cpp - wire-compatibility probe.
//
// It links the generated AIDL stubs (the same BpCharger the HAL uses) and
// calls the charger service already running as
// "vendor.oplus.hardware.charger.ICharger/default".
//
// If the interface transaction codes or parcel layouts were wrong, every
// call would either land on the wrong method or fail to unmarshal. A clean
// set of replies with plausible values is the proof that the interface is
// wire-compatible.
//
// Usage:  probe <service-instance>
//         (default: vendor.oplus.hardware.charger.ICharger/default)
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>

#include <cstdio>
#include <string>

#include "aidl/vendor/oplus/hardware/charger/BpCharger.h"
#include "aidl/vendor/oplus/hardware/charger/ICharger.h"

namespace {

int g_failures = 0;

void report(const char* name, const ndk::ScopedAStatus& status, int32_t value) {
    if (!status.isOk()) {
        printf("FAIL  %-22s status=%d\n", name, status.getExceptionCode());
        ++g_failures;
        return;
    }
    printf("OK    %-22s -> %d\n", name, value);
}

void reportStr(const char* name, const ndk::ScopedAStatus& status, const std::string& value) {
    if (!status.isOk()) {
        printf("FAIL  %-22s status=%d\n", name, status.getExceptionCode());
        ++g_failures;
        return;
    }
    printf("OK    %-22s -> \"%s\"\n", name, value.c_str());
}

}  // namespace

int main(int argc, char** argv) {
    const char* instance = (argc > 1)
            ? argv[1]
            : "vendor.oplus.hardware.charger.ICharger/default";

    // The probe itself must not register a thread pool service.
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    AIBinder* binder = AServiceManager_checkService(instance);
    if (binder == nullptr) {
        printf("FAIL  service not found: %s\n", instance);
        return 1;
    }
    printf("found service: %s\n", instance);

    ::ndk::SpAIBinder sp(binder);
    // ICharger::fromBinder is the generated entry point for a proxy: it calls
    // AIBinder_associateClass, which a remote transaction requires
    // ("Class must be defined for a remote binder transaction").
    std::shared_ptr<aidl::vendor::oplus::hardware::charger::ICharger> charger =
            aidl::vendor::oplus::hardware::charger::ICharger::fromBinder(sp);
    if (charger == nullptr) {
        printf("FAIL  fromBinder returned null\n");
        return 1;
    }

    int32_t value = 0;
    report("getAcType", charger->getAcType(&value), value);
    report("getFastCharge", charger->getFastCharge(&value), value);
    report("getPsyChargeTech", charger->getPsyChargeTech(&value), value);
    report("getPsyFastChgType", charger->getPsyFastChgType(&value), value);
    report("getBattPPSChgPower", charger->getBattPPSChgPower(&value), value);
    report("getPsyBatteryLevel", charger->getPsyBatteryLevel(&value), value);
    report("getBatteryVoltageNow", charger->getBatteryVoltageNow(&value), value);
    report("getPsyUsbStatus", charger->getPsyUsbStatus(&value), value);
    report("getPsyInputCurrent", charger->getPsyInputCurrent(&value), value);
    report("getPsyUsbOnline", charger->getPsyUsbOnline(&value), value);
    report("getPsyAcOnline", charger->getPsyAcOnline(&value), value);
    report("getWiredOtgOnline", charger->getWiredOtgOnline(&value), value);
    report("getPsyOtgOnline", charger->getPsyOtgOnline(&value), value);
    report("getPsyOtgSwitch", charger->getPsyOtgSwitch(&value), value);
    report("setPsyOtgSwitch(1)", charger->setPsyOtgSwitch("1", &value), value);
    report("getPsyOtgSwitch[after 1]", charger->getPsyOtgSwitch(&value), value);
    report("setPsyOtgSwitch(0)", charger->setPsyOtgSwitch("0", &value), value);
    report("getPsyOtgSwitch[after 0]", charger->getPsyOtgSwitch(&value), value);

    std::string text;
    reportStr("getPsyBatteryStatus", charger->getPsyBatteryStatus(&text), text);
    reportStr("getChargerControl", charger->getChargerControl(&text), text);

    // The nine getters that used to read the Oplus charger nodes, which do
    // not exist on this device.  They must answer with the MTK driver's
    // values now.
    report("getPsyBatteryFcc", charger->getPsyBatteryFcc(&value), value);
    report("getPsyBatteryRm", charger->getPsyBatteryRm(&value), value);
    report("getPsyBatteryCC", charger->getPsyBatteryCC(&value), value);
    report("getPsyBatteryNotify", charger->getPsyBatteryNotify(&value), value);
    report("getChargerCoolDown", charger->getChargerCoolDown(&value), value);
    report("getPsyBatteryPchg", charger->getPsyBatteryPchg(&value), value);
    report("getPsyBatteryPchgResetCount",
           charger->getPsyBatteryPchgResetCount(&value), value);
    report("getUIsohValue", charger->getUIsohValue(&value), value);
    reportStr("healthd_update_ui_soc_decimal",
              charger->healthd_update_ui_soc_decimal(&text), text);

    // The four methods the framework's V6 interface adds at transaction
    // codes 127..130.
    report("setChgConfig",
           charger->setChgConfig(4, "", 1, &value), value);
    reportStr("getChgConfig(0)", charger->getChgConfig(0, "", 1, &text), text);
    reportStr("getChgConfig(1)", charger->getChgConfig(1, "", 1, &text), text);
    reportStr("getChgConfig(4)", charger->getChgConfig(4, "", 1, &text), text);
    reportStr("getChgConfig(6)", charger->getChgConfig(6, "", 1, &text), text);
    reportStr("getChgConfig(0x18)", charger->getChgConfig(0x18, "", 1, &text), text);

    // Bypass charger / stop charging controls
    report("setCustomSelectChgMode(1, true)", charger->setCustomSelectChgMode(1, true, &value), value);
    reportStr("getChgConfig(0x18) [limit on]", charger->getChgConfig(0x18, "", 1, &text), text);
    report("setCustomSelectChgMode(1, false)", charger->setCustomSelectChgMode(1, false, &value), value);
    reportStr("getChgConfig(0x18) [limit off]", charger->getChgConfig(0x18, "", 1, &text), text);
    report("setPsyMmiChgEn(0)", charger->setPsyMmiChgEn("0", &value), value);
    reportStr("getChgConfig(0x18) [mmi stop]", charger->getChgConfig(0x18, "", 1, &text), text);
    report("setPsyMmiChgEn(1)", charger->setPsyMmiChgEn("1", &value), value);
    reportStr("getChgConfig(0x18) [mmi resume]", charger->getChgConfig(0x18, "", 1, &text), text);
    report("setUsbEyeDiagram",
           charger->setUsbEyeDiagram(1, "0,1,2", false, &value), value);
    reportStr("getUsbCurrentEyeDiagram",
              charger->getUsbCurrentEyeDiagram(1, &text), text);

    printf("\n%s: %d failure(s)\n", g_failures == 0 ? "PASS" : "FAIL", g_failures);
    return g_failures == 0 ? 0 : 1;
}
