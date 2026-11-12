/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: /opt/android-sdk/build-tools/35.0.0/aidl --lang=ndk --structured --stability=vintf --version=6 --hash=ede5e600328119dbf1c38269f810a3fdce74029f -I /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/aidl --header_out=/mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/standalone/build/aidl_gen -o /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/standalone/build/aidl_gen /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/aidl/vendor/oplus/hardware/charger/ICharger.aidl
 */
#pragma once

#include "aidl/vendor/oplus/hardware/charger/ICharger.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {
class BpCharger : public ::ndk::BpCInterface<ICharger> {
public:
  explicit BpCharger(const ::ndk::SpAIBinder& binder);
  virtual ~BpCharger();

  ::ndk::ScopedAStatus VolDividerIcWorkModeSet(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus chgExchangeMesgInit(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus chgExchangeSohMesgInit(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getAcType(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBattAuthenticate(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBattPPSChgIng(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBattPPSChgPower(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBattParamNoplug(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getBattShortIcOtpStatus(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBattSubCurrent(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBattVoocChgIng(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBatteryVoltageNow(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBccCsvData(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getBccExpStatus(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getBmsHeatingRunningStatus(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getBmsHeatingStatus(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getChargerControl(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getChargerCoolDown(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getChargerCriticalLog(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getChargerIdVolt(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getChargerLog(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getCustomSelectChgMode(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getDevinfoFastchg(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getFastCharge(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getParallelChgMosTestResult(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyAcOnline(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryCC(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryCurrentNow(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryFcc(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryHmac(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryLevel(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryNotify(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryPchg(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryPchgResetCount(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryRm(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryShortFeature(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryShortStatus(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryStatus(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatteryTemp(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyChargeTech(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyFastChgType(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyInputCurrent(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyOtgOnline(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyOtgSwitch(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyPcPortOnline(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyQGVbatDeviation(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyTypeOrientation(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyUsbOnline(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyUsbStatus(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyWirelessRX(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyWirelessRxVersion(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyWirelessTX(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyWirelessTxVersion(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getQgVbatDeviation(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getQuickModeGain(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getReserveSocDebug(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getSmartChgMode(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getUIsohValue(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getUisohDebugParameterInfo(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getUsbInputCurrentNow(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getUsbPrimalType(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWiredOtgOnline(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessAdapterPower(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessCapacity(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessChargePumpEn(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessCurrentNow(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessDeviated(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessOnline(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessPenPresent(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessPtmcId(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessRXEnable(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessRealType(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessTXEnable(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessUserSleepMode(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getWirelessVoltageNow(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus healthd_update_ui_soc_decimal(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus nightstandby(int32_t in_param0, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus notifyScreenStatus(int32_t in_param0, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus queryChargeInfo(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus setChargeEMMode(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setChargerControl(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setChargerCoolDown(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setChargerCriticalLog(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setChargerCycle(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setChargerFactoryModeTest(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setChargerLog(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setChgStatusToBcc(int32_t in_param0, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setCustomSelectChgMode(int32_t in_param0, bool in_enable, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setFastchgFwUpdate(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setPsyMmiChgEn(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setPsyOtgSwitch(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setReserveSocDebug(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setShipMode(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setSmartChgMode(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setSmartCoolDown(int32_t in_param0, int32_t in_param1, const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setTbattPwrOff(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setUisohDebugInfo(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setUsbPrimalType(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setWirelessChargePumpEn(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setWirelessFtmMode(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setWirelessIconDelay(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setWirelessIdtAdcTest(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setWirelessPenSoc(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setWirelessRXEnable(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setWirelessTXEnable(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setWirelessUserSleepMode(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setWlsThirdPartitionInfo(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus testKitFeatureTest(int32_t in_param0, ::aidl::vendor::oplus::hardware::charger::testKitFeatureTestResult* _aidl_return) override;
  ::ndk::ScopedAStatus testKitGetFeatureList(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus testKitGetFeatureName(int32_t in_param0, std::string* _aidl_return) override;
  ::ndk::ScopedAStatus testKitGetFeatureNum(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus updateUiSohToPartion(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus queryWlsPencilInfo(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getChgOlcConfig(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus setChgOlcConfig(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setSuperEnduranceStatus(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setSuperEnduranceCount(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setBobStatus(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setPsySlowChgEn(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getCpVbatDeviation(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setBatteryLogPush(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getChargingModeInGsmCall(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setChargingModeInGsmCall(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus setChgRusConfig(const std::string& in_value, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getPsyBatterySN(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getBattGaugeInfo(std::string* _aidl_return) override;
  ::ndk::ScopedAStatus setChgConfig(int32_t in_flag, const std::string& in_extra, int32_t in_callname, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getChgConfig(int32_t in_flag, const std::string& in_extra, int32_t in_callname, std::string* _aidl_return) override;
  ::ndk::ScopedAStatus setUsbEyeDiagram(int32_t in_model, const std::string& in_eyeDiagram, bool in_isDefaultEyeDiagram, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getUsbCurrentEyeDiagram(int32_t in_model, std::string* _aidl_return) override;
  ::ndk::ScopedAStatus getInterfaceVersion(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus getInterfaceHash(std::string* _aidl_return) override;
  int32_t _aidl_cached_version = -1;
  std::string _aidl_cached_hash = "-1";
  std::mutex _aidl_cached_hash_mutex;
};
}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
}  // namespace aidl
