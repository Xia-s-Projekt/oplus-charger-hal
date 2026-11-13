// probe_full.cpp - full-coverage device probe, GENERATED from
// aidl/vendor/oplus/hardware/charger/ICharger.aidl by gen_probe_full.py.
// Calls every one of the 130 interface methods plus the two reserved
// VINTF transactions, printing "METHOD <kind> <name> <answer>" in
// declaration order so two runs against different builds can be diffed.
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

void line(const char* kind, const char* name, const ndk::ScopedAStatus& st, const std::string& value) {
    if (!st.isOk()) {
        printf("METHOD %s %s FAIL %d\n", kind, name, st.getStatus());
        ++g_failures;
        return;
    }
    printf("METHOD %s %s OK %s\n", kind, name, value.c_str());
}

std::string i2s(int32_t v) { return std::to_string(v); }

}  // namespace

int main(int argc, char** argv) {
    const char* instance = (argc > 1) ? argv[1] : "vendor.oplus.hardware.charger.ICharger/default";
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    AIBinder* binder = AServiceManager_checkService(instance);
    if (binder == nullptr) { printf("FAIL service not found: %s\n", instance); return 1; }
    ::ndk::SpAIBinder sp(binder);
    std::shared_ptr<aidl::vendor::oplus::hardware::charger::ICharger> c =
            aidl::vendor::oplus::hardware::charger::ICharger::fromBinder(sp);
    if (c == nullptr) { printf("FAIL fromBinder\n"); return 1; }

    { int32_t v = 0; line("int", "VolDividerIcWorkModeSet", c->VolDividerIcWorkModeSet("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "chgExchangeMesgInit", c->chgExchangeMesgInit(&v), i2s(v)); }
    { int32_t v = 0; line("int", "chgExchangeSohMesgInit", c->chgExchangeSohMesgInit(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getAcType", c->getAcType(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getBattAuthenticate", c->getBattAuthenticate(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getBattPPSChgIng", c->getBattPPSChgIng(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getBattPPSChgPower", c->getBattPPSChgPower(&v), i2s(v)); }
    { std::string v; line("str", "getBattParamNoplug", c->getBattParamNoplug(&v), v); }
    { int32_t v = 0; line("int", "getBattShortIcOtpStatus", c->getBattShortIcOtpStatus(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getBattSubCurrent", c->getBattSubCurrent(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getBattVoocChgIng", c->getBattVoocChgIng(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getBatteryVoltageNow", c->getBatteryVoltageNow(&v), i2s(v)); }
    { std::string v; line("str", "getBccCsvData", c->getBccCsvData(&v), v); }
    { int32_t v = 0; line("int", "getBccExpStatus", c->getBccExpStatus(&v), i2s(v)); }
    { std::string v; line("str", "getBmsHeatingRunningStatus", c->getBmsHeatingRunningStatus(&v), v); }
    { int32_t v = 0; line("int", "getBmsHeatingStatus", c->getBmsHeatingStatus(&v), i2s(v)); }
    { std::string v; line("str", "getChargerControl", c->getChargerControl(&v), v); }
    { int32_t v = 0; line("int", "getChargerCoolDown", c->getChargerCoolDown(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getChargerCriticalLog", c->getChargerCriticalLog(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getChargerIdVolt", c->getChargerIdVolt(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getChargerLog", c->getChargerLog(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getCustomSelectChgMode", c->getCustomSelectChgMode(&v), i2s(v)); }
    { std::string v; line("str", "getDevinfoFastchg", c->getDevinfoFastchg(&v), v); }
    { int32_t v = 0; line("int", "getFastCharge", c->getFastCharge(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getParallelChgMosTestResult", c->getParallelChgMosTestResult(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyAcOnline", c->getPsyAcOnline(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryCC", c->getPsyBatteryCC(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryCurrentNow", c->getPsyBatteryCurrentNow(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryFcc", c->getPsyBatteryFcc(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryHmac", c->getPsyBatteryHmac(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryLevel", c->getPsyBatteryLevel(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryNotify", c->getPsyBatteryNotify(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryPchg", c->getPsyBatteryPchg(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryPchgResetCount", c->getPsyBatteryPchgResetCount(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryRm", c->getPsyBatteryRm(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryShortFeature", c->getPsyBatteryShortFeature(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyBatteryShortStatus", c->getPsyBatteryShortStatus(&v), i2s(v)); }
    { std::string v; line("str", "getPsyBatteryStatus", c->getPsyBatteryStatus(&v), v); }
    { int32_t v = 0; line("int", "getPsyBatteryTemp", c->getPsyBatteryTemp(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyChargeTech", c->getPsyChargeTech(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyFastChgType", c->getPsyFastChgType(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyInputCurrent", c->getPsyInputCurrent(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyOtgOnline", c->getPsyOtgOnline(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyOtgSwitch", c->getPsyOtgSwitch(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyPcPortOnline", c->getPsyPcPortOnline(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyQGVbatDeviation", c->getPsyQGVbatDeviation(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyTypeOrientation", c->getPsyTypeOrientation(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyUsbOnline", c->getPsyUsbOnline(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getPsyUsbStatus", c->getPsyUsbStatus(&v), i2s(v)); }
    { std::string v; line("str", "getPsyWirelessRX", c->getPsyWirelessRX(&v), v); }
    { std::string v; line("str", "getPsyWirelessRxVersion", c->getPsyWirelessRxVersion(&v), v); }
    { std::string v; line("str", "getPsyWirelessTX", c->getPsyWirelessTX(&v), v); }
    { std::string v; line("str", "getPsyWirelessTxVersion", c->getPsyWirelessTxVersion(&v), v); }
    { int32_t v = 0; line("int", "getQgVbatDeviation", c->getQgVbatDeviation(&v), i2s(v)); }
    { std::string v; line("str", "getQuickModeGain", c->getQuickModeGain(&v), v); }
    { std::string v; line("str", "getReserveSocDebug", c->getReserveSocDebug(&v), v); }
    { int32_t v = 0; line("int", "getSmartChgMode", c->getSmartChgMode(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getUIsohValue", c->getUIsohValue(&v), i2s(v)); }
    { std::string v; line("str", "getUisohDebugParameterInfo", c->getUisohDebugParameterInfo(&v), v); }
    { int32_t v = 0; line("int", "getUsbInputCurrentNow", c->getUsbInputCurrentNow(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getUsbPrimalType", c->getUsbPrimalType(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWiredOtgOnline", c->getWiredOtgOnline(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWirelessAdapterPower", c->getWirelessAdapterPower(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWirelessCapacity", c->getWirelessCapacity(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWirelessChargePumpEn", c->getWirelessChargePumpEn(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWirelessCurrentNow", c->getWirelessCurrentNow(&v), i2s(v)); }
    { std::string v; line("str", "getWirelessDeviated", c->getWirelessDeviated(&v), v); }
    { int32_t v = 0; line("int", "getWirelessOnline", c->getWirelessOnline(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWirelessPenPresent", c->getWirelessPenPresent(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWirelessPtmcId", c->getWirelessPtmcId(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWirelessRXEnable", c->getWirelessRXEnable(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWirelessRealType", c->getWirelessRealType(&v), i2s(v)); }
    { std::string v; line("str", "getWirelessTXEnable", c->getWirelessTXEnable(&v), v); }
    { int32_t v = 0; line("int", "getWirelessUserSleepMode", c->getWirelessUserSleepMode(&v), i2s(v)); }
    { int32_t v = 0; line("int", "getWirelessVoltageNow", c->getWirelessVoltageNow(&v), i2s(v)); }
    { std::string v; line("str", "healthd_update_ui_soc_decimal", c->healthd_update_ui_soc_decimal(&v), v); }
    { int32_t v = 0; line("int", "nightstandby", c->nightstandby(0, &v), i2s(v)); }
    { int32_t v = 0; line("int", "notifyScreenStatus", c->notifyScreenStatus(0, &v), i2s(v)); }
    { std::string v; line("str", "queryChargeInfo", c->queryChargeInfo(&v), v); }
    { int32_t v = 0; line("int", "setChargeEMMode", c->setChargeEMMode("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setChargerControl", c->setChargerControl("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setChargerCoolDown", c->setChargerCoolDown("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setChargerCriticalLog", c->setChargerCriticalLog("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setChargerCycle", c->setChargerCycle("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setChargerFactoryModeTest", c->setChargerFactoryModeTest("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setChargerLog", c->setChargerLog("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setChgStatusToBcc", c->setChgStatusToBcc(0, &v), i2s(v)); }
    { int32_t v = 0; line("int", "setCustomSelectChgMode", c->setCustomSelectChgMode(0, false, &v), i2s(v)); }
    { int32_t v = 0; line("int", "setFastchgFwUpdate", c->setFastchgFwUpdate("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setPsyMmiChgEn", c->setPsyMmiChgEn("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setPsyOtgSwitch", c->setPsyOtgSwitch("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setReserveSocDebug", c->setReserveSocDebug("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setShipMode", c->setShipMode("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setSmartChgMode", c->setSmartChgMode("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setSmartCoolDown", c->setSmartCoolDown(0, 0, "0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setTbattPwrOff", c->setTbattPwrOff("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setUisohDebugInfo", c->setUisohDebugInfo("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setUsbPrimalType", c->setUsbPrimalType("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setWirelessChargePumpEn", c->setWirelessChargePumpEn("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setWirelessFtmMode", c->setWirelessFtmMode("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setWirelessIconDelay", c->setWirelessIconDelay("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setWirelessIdtAdcTest", c->setWirelessIdtAdcTest("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setWirelessPenSoc", c->setWirelessPenSoc("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setWirelessRXEnable", c->setWirelessRXEnable("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setWirelessTXEnable", c->setWirelessTXEnable("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setWirelessUserSleepMode", c->setWirelessUserSleepMode("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setWlsThirdPartitionInfo", c->setWlsThirdPartitionInfo("0", &v), i2s(v)); }
    { aidl::vendor::oplus::hardware::charger::testKitFeatureTestResult v;
      line("par", "testKitFeatureTest", c->testKitFeatureTest(0, &v),
            std::string("ret=") + i2s(v.ret) + " str=" + v.str); }
    { std::string v; line("str", "testKitGetFeatureList", c->testKitGetFeatureList(&v), v); }
    { std::string v; line("str", "testKitGetFeatureName", c->testKitGetFeatureName(0, &v), v); }
    { int32_t v = 0; line("int", "testKitGetFeatureNum", c->testKitGetFeatureNum(&v), i2s(v)); }
    { int32_t v = 0; line("int", "updateUiSohToPartion", c->updateUiSohToPartion(&v), i2s(v)); }
    { std::string v; line("str", "queryWlsPencilInfo", c->queryWlsPencilInfo(&v), v); }
    { std::string v; line("str", "getChgOlcConfig", c->getChgOlcConfig(&v), v); }
    { int32_t v = 0; line("int", "setChgOlcConfig", c->setChgOlcConfig("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setSuperEnduranceStatus", c->setSuperEnduranceStatus("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setSuperEnduranceCount", c->setSuperEnduranceCount("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setBobStatus", c->setBobStatus("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setPsySlowChgEn", c->setPsySlowChgEn("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "getCpVbatDeviation", c->getCpVbatDeviation(&v), i2s(v)); }
    { int32_t v = 0; line("int", "setBatteryLogPush", c->setBatteryLogPush("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "getChargingModeInGsmCall", c->getChargingModeInGsmCall(&v), i2s(v)); }
    { int32_t v = 0; line("int", "setChargingModeInGsmCall", c->setChargingModeInGsmCall("0", &v), i2s(v)); }
    { int32_t v = 0; line("int", "setChgRusConfig", c->setChgRusConfig("0", &v), i2s(v)); }
    { std::string v; line("str", "getPsyBatterySN", c->getPsyBatterySN(&v), v); }
    { std::string v; line("str", "getBattGaugeInfo", c->getBattGaugeInfo(&v), v); }
    { int32_t v = 0; line("int", "setChgConfig", c->setChgConfig(0, "0", 0, &v), i2s(v)); }
    { std::string v; line("str", "getChgConfig", c->getChgConfig(0, "0", 0, &v), v); }
    { int32_t v = 0; line("int", "setUsbEyeDiagram", c->setUsbEyeDiagram(0, "0", false, &v), i2s(v)); }
    { std::string v; line("str", "getUsbCurrentEyeDiagram", c->getUsbCurrentEyeDiagram(0, &v), v); }

    // The two reserved VINTF transactions.
    { int32_t v = 0; line("int", "getInterfaceVersion", c->getInterfaceVersion(&v), i2s(v)); }
    { std::string v; line("str", "getInterfaceHash", c->getInterfaceHash(&v), v); }

    printf("\n%s: %d failure(s)\n", g_failures == 0 ? "PASS" : "FAIL", g_failures);
    return g_failures == 0 ? 0 : 1;
}
