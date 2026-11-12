/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: /opt/android-sdk/build-tools/35.0.0/aidl --lang=ndk --structured --stability=vintf --version=6 --hash=ede5e600328119dbf1c38269f810a3fdce74029f -I /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/aidl --header_out=/mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/standalone/build/aidl_gen -o /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/standalone/build/aidl_gen /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/aidl/vendor/oplus/hardware/charger/ICharger.aidl
 */
#pragma once

#include "aidl/vendor/oplus/hardware/charger/ICharger.h"

#include <android/binder_ibinder.h>
#include <cassert>

#ifndef __BIONIC__
#ifndef __assert2
#define __assert2(a,b,c,d) ((void)0)
#endif
#endif

namespace aidl {
namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {
class BnCharger : public ::ndk::BnCInterface<ICharger> {
public:
  BnCharger();
  virtual ~BnCharger();
  ::ndk::ScopedAStatus getInterfaceVersion(int32_t* _aidl_return) final;
  ::ndk::ScopedAStatus getInterfaceHash(std::string* _aidl_return) final;
protected:
  ::ndk::SpAIBinder createBinder() override;
private:
};
class IChargerDelegator : public BnCharger {
public:
  explicit IChargerDelegator(const std::shared_ptr<ICharger> &impl) : _impl(impl) {
     int32_t _impl_ver = 0;
     if (!impl->getInterfaceVersion(&_impl_ver).isOk()) {;
        __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Delegator failed to get version of the implementation.");
     }
     if (_impl_ver != ICharger::version) {
        __assert2(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Mismatched versions of delegator and implementation is not allowed.");
     }
  }

  ::ndk::ScopedAStatus VolDividerIcWorkModeSet(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->VolDividerIcWorkModeSet(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus chgExchangeMesgInit(int32_t* _aidl_return) override {
    return _impl->chgExchangeMesgInit(_aidl_return);
  }
  ::ndk::ScopedAStatus chgExchangeSohMesgInit(int32_t* _aidl_return) override {
    return _impl->chgExchangeSohMesgInit(_aidl_return);
  }
  ::ndk::ScopedAStatus getAcType(int32_t* _aidl_return) override {
    return _impl->getAcType(_aidl_return);
  }
  ::ndk::ScopedAStatus getBattAuthenticate(int32_t* _aidl_return) override {
    return _impl->getBattAuthenticate(_aidl_return);
  }
  ::ndk::ScopedAStatus getBattPPSChgIng(int32_t* _aidl_return) override {
    return _impl->getBattPPSChgIng(_aidl_return);
  }
  ::ndk::ScopedAStatus getBattPPSChgPower(int32_t* _aidl_return) override {
    return _impl->getBattPPSChgPower(_aidl_return);
  }
  ::ndk::ScopedAStatus getBattParamNoplug(std::string* _aidl_return) override {
    return _impl->getBattParamNoplug(_aidl_return);
  }
  ::ndk::ScopedAStatus getBattShortIcOtpStatus(int32_t* _aidl_return) override {
    return _impl->getBattShortIcOtpStatus(_aidl_return);
  }
  ::ndk::ScopedAStatus getBattSubCurrent(int32_t* _aidl_return) override {
    return _impl->getBattSubCurrent(_aidl_return);
  }
  ::ndk::ScopedAStatus getBattVoocChgIng(int32_t* _aidl_return) override {
    return _impl->getBattVoocChgIng(_aidl_return);
  }
  ::ndk::ScopedAStatus getBatteryVoltageNow(int32_t* _aidl_return) override {
    return _impl->getBatteryVoltageNow(_aidl_return);
  }
  ::ndk::ScopedAStatus getBccCsvData(std::string* _aidl_return) override {
    return _impl->getBccCsvData(_aidl_return);
  }
  ::ndk::ScopedAStatus getBccExpStatus(int32_t* _aidl_return) override {
    return _impl->getBccExpStatus(_aidl_return);
  }
  ::ndk::ScopedAStatus getBmsHeatingRunningStatus(std::string* _aidl_return) override {
    return _impl->getBmsHeatingRunningStatus(_aidl_return);
  }
  ::ndk::ScopedAStatus getBmsHeatingStatus(int32_t* _aidl_return) override {
    return _impl->getBmsHeatingStatus(_aidl_return);
  }
  ::ndk::ScopedAStatus getChargerControl(std::string* _aidl_return) override {
    return _impl->getChargerControl(_aidl_return);
  }
  ::ndk::ScopedAStatus getChargerCoolDown(int32_t* _aidl_return) override {
    return _impl->getChargerCoolDown(_aidl_return);
  }
  ::ndk::ScopedAStatus getChargerCriticalLog(int32_t* _aidl_return) override {
    return _impl->getChargerCriticalLog(_aidl_return);
  }
  ::ndk::ScopedAStatus getChargerIdVolt(int32_t* _aidl_return) override {
    return _impl->getChargerIdVolt(_aidl_return);
  }
  ::ndk::ScopedAStatus getChargerLog(int32_t* _aidl_return) override {
    return _impl->getChargerLog(_aidl_return);
  }
  ::ndk::ScopedAStatus getCustomSelectChgMode(int32_t* _aidl_return) override {
    return _impl->getCustomSelectChgMode(_aidl_return);
  }
  ::ndk::ScopedAStatus getDevinfoFastchg(std::string* _aidl_return) override {
    return _impl->getDevinfoFastchg(_aidl_return);
  }
  ::ndk::ScopedAStatus getFastCharge(int32_t* _aidl_return) override {
    return _impl->getFastCharge(_aidl_return);
  }
  ::ndk::ScopedAStatus getParallelChgMosTestResult(int32_t* _aidl_return) override {
    return _impl->getParallelChgMosTestResult(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyAcOnline(int32_t* _aidl_return) override {
    return _impl->getPsyAcOnline(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryCC(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryCC(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryCurrentNow(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryCurrentNow(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryFcc(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryFcc(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryHmac(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryHmac(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryLevel(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryLevel(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryNotify(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryNotify(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryPchg(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryPchg(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryPchgResetCount(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryPchgResetCount(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryRm(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryRm(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryShortFeature(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryShortFeature(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryShortStatus(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryShortStatus(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryStatus(std::string* _aidl_return) override {
    return _impl->getPsyBatteryStatus(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatteryTemp(int32_t* _aidl_return) override {
    return _impl->getPsyBatteryTemp(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyChargeTech(int32_t* _aidl_return) override {
    return _impl->getPsyChargeTech(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyFastChgType(int32_t* _aidl_return) override {
    return _impl->getPsyFastChgType(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyInputCurrent(int32_t* _aidl_return) override {
    return _impl->getPsyInputCurrent(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyOtgOnline(int32_t* _aidl_return) override {
    return _impl->getPsyOtgOnline(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyOtgSwitch(int32_t* _aidl_return) override {
    return _impl->getPsyOtgSwitch(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyPcPortOnline(int32_t* _aidl_return) override {
    return _impl->getPsyPcPortOnline(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyQGVbatDeviation(int32_t* _aidl_return) override {
    return _impl->getPsyQGVbatDeviation(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyTypeOrientation(int32_t* _aidl_return) override {
    return _impl->getPsyTypeOrientation(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyUsbOnline(int32_t* _aidl_return) override {
    return _impl->getPsyUsbOnline(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyUsbStatus(int32_t* _aidl_return) override {
    return _impl->getPsyUsbStatus(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyWirelessRX(std::string* _aidl_return) override {
    return _impl->getPsyWirelessRX(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyWirelessRxVersion(std::string* _aidl_return) override {
    return _impl->getPsyWirelessRxVersion(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyWirelessTX(std::string* _aidl_return) override {
    return _impl->getPsyWirelessTX(_aidl_return);
  }
  ::ndk::ScopedAStatus getPsyWirelessTxVersion(std::string* _aidl_return) override {
    return _impl->getPsyWirelessTxVersion(_aidl_return);
  }
  ::ndk::ScopedAStatus getQgVbatDeviation(int32_t* _aidl_return) override {
    return _impl->getQgVbatDeviation(_aidl_return);
  }
  ::ndk::ScopedAStatus getQuickModeGain(std::string* _aidl_return) override {
    return _impl->getQuickModeGain(_aidl_return);
  }
  ::ndk::ScopedAStatus getReserveSocDebug(std::string* _aidl_return) override {
    return _impl->getReserveSocDebug(_aidl_return);
  }
  ::ndk::ScopedAStatus getSmartChgMode(int32_t* _aidl_return) override {
    return _impl->getSmartChgMode(_aidl_return);
  }
  ::ndk::ScopedAStatus getUIsohValue(int32_t* _aidl_return) override {
    return _impl->getUIsohValue(_aidl_return);
  }
  ::ndk::ScopedAStatus getUisohDebugParameterInfo(std::string* _aidl_return) override {
    return _impl->getUisohDebugParameterInfo(_aidl_return);
  }
  ::ndk::ScopedAStatus getUsbInputCurrentNow(int32_t* _aidl_return) override {
    return _impl->getUsbInputCurrentNow(_aidl_return);
  }
  ::ndk::ScopedAStatus getUsbPrimalType(int32_t* _aidl_return) override {
    return _impl->getUsbPrimalType(_aidl_return);
  }
  ::ndk::ScopedAStatus getWiredOtgOnline(int32_t* _aidl_return) override {
    return _impl->getWiredOtgOnline(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessAdapterPower(int32_t* _aidl_return) override {
    return _impl->getWirelessAdapterPower(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessCapacity(int32_t* _aidl_return) override {
    return _impl->getWirelessCapacity(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessChargePumpEn(int32_t* _aidl_return) override {
    return _impl->getWirelessChargePumpEn(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessCurrentNow(int32_t* _aidl_return) override {
    return _impl->getWirelessCurrentNow(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessDeviated(std::string* _aidl_return) override {
    return _impl->getWirelessDeviated(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessOnline(int32_t* _aidl_return) override {
    return _impl->getWirelessOnline(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessPenPresent(int32_t* _aidl_return) override {
    return _impl->getWirelessPenPresent(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessPtmcId(int32_t* _aidl_return) override {
    return _impl->getWirelessPtmcId(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessRXEnable(int32_t* _aidl_return) override {
    return _impl->getWirelessRXEnable(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessRealType(int32_t* _aidl_return) override {
    return _impl->getWirelessRealType(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessTXEnable(std::string* _aidl_return) override {
    return _impl->getWirelessTXEnable(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessUserSleepMode(int32_t* _aidl_return) override {
    return _impl->getWirelessUserSleepMode(_aidl_return);
  }
  ::ndk::ScopedAStatus getWirelessVoltageNow(int32_t* _aidl_return) override {
    return _impl->getWirelessVoltageNow(_aidl_return);
  }
  ::ndk::ScopedAStatus healthd_update_ui_soc_decimal(std::string* _aidl_return) override {
    return _impl->healthd_update_ui_soc_decimal(_aidl_return);
  }
  ::ndk::ScopedAStatus nightstandby(int32_t in_param0, int32_t* _aidl_return) override {
    return _impl->nightstandby(in_param0, _aidl_return);
  }
  ::ndk::ScopedAStatus notifyScreenStatus(int32_t in_param0, int32_t* _aidl_return) override {
    return _impl->notifyScreenStatus(in_param0, _aidl_return);
  }
  ::ndk::ScopedAStatus queryChargeInfo(std::string* _aidl_return) override {
    return _impl->queryChargeInfo(_aidl_return);
  }
  ::ndk::ScopedAStatus setChargeEMMode(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChargeEMMode(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setChargerControl(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChargerControl(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setChargerCoolDown(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChargerCoolDown(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setChargerCriticalLog(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChargerCriticalLog(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setChargerCycle(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChargerCycle(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setChargerFactoryModeTest(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChargerFactoryModeTest(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setChargerLog(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChargerLog(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setChgStatusToBcc(int32_t in_param0, int32_t* _aidl_return) override {
    return _impl->setChgStatusToBcc(in_param0, _aidl_return);
  }
  ::ndk::ScopedAStatus setCustomSelectChgMode(int32_t in_param0, bool in_enable, int32_t* _aidl_return) override {
    return _impl->setCustomSelectChgMode(in_param0, in_enable, _aidl_return);
  }
  ::ndk::ScopedAStatus setFastchgFwUpdate(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setFastchgFwUpdate(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setPsyMmiChgEn(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setPsyMmiChgEn(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setPsyOtgSwitch(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setPsyOtgSwitch(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setReserveSocDebug(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setReserveSocDebug(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setShipMode(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setShipMode(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setSmartChgMode(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setSmartChgMode(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setSmartCoolDown(int32_t in_param0, int32_t in_param1, const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setSmartCoolDown(in_param0, in_param1, in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setTbattPwrOff(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setTbattPwrOff(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setUisohDebugInfo(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setUisohDebugInfo(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setUsbPrimalType(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setUsbPrimalType(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setWirelessChargePumpEn(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setWirelessChargePumpEn(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setWirelessFtmMode(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setWirelessFtmMode(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setWirelessIconDelay(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setWirelessIconDelay(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setWirelessIdtAdcTest(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setWirelessIdtAdcTest(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setWirelessPenSoc(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setWirelessPenSoc(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setWirelessRXEnable(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setWirelessRXEnable(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setWirelessTXEnable(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setWirelessTXEnable(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setWirelessUserSleepMode(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setWirelessUserSleepMode(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setWlsThirdPartitionInfo(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setWlsThirdPartitionInfo(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus testKitFeatureTest(int32_t in_param0, ::aidl::vendor::oplus::hardware::charger::testKitFeatureTestResult* _aidl_return) override {
    return _impl->testKitFeatureTest(in_param0, _aidl_return);
  }
  ::ndk::ScopedAStatus testKitGetFeatureList(std::string* _aidl_return) override {
    return _impl->testKitGetFeatureList(_aidl_return);
  }
  ::ndk::ScopedAStatus testKitGetFeatureName(int32_t in_param0, std::string* _aidl_return) override {
    return _impl->testKitGetFeatureName(in_param0, _aidl_return);
  }
  ::ndk::ScopedAStatus testKitGetFeatureNum(int32_t* _aidl_return) override {
    return _impl->testKitGetFeatureNum(_aidl_return);
  }
  ::ndk::ScopedAStatus updateUiSohToPartion(int32_t* _aidl_return) override {
    return _impl->updateUiSohToPartion(_aidl_return);
  }
  ::ndk::ScopedAStatus queryWlsPencilInfo(std::string* _aidl_return) override {
    return _impl->queryWlsPencilInfo(_aidl_return);
  }
  ::ndk::ScopedAStatus getChgOlcConfig(std::string* _aidl_return) override {
    return _impl->getChgOlcConfig(_aidl_return);
  }
  ::ndk::ScopedAStatus setChgOlcConfig(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChgOlcConfig(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setSuperEnduranceStatus(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setSuperEnduranceStatus(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setSuperEnduranceCount(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setSuperEnduranceCount(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setBobStatus(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setBobStatus(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setPsySlowChgEn(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setPsySlowChgEn(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus getCpVbatDeviation(int32_t* _aidl_return) override {
    return _impl->getCpVbatDeviation(_aidl_return);
  }
  ::ndk::ScopedAStatus setBatteryLogPush(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setBatteryLogPush(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus getChargingModeInGsmCall(int32_t* _aidl_return) override {
    return _impl->getChargingModeInGsmCall(_aidl_return);
  }
  ::ndk::ScopedAStatus setChargingModeInGsmCall(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChargingModeInGsmCall(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus setChgRusConfig(const std::string& in_value, int32_t* _aidl_return) override {
    return _impl->setChgRusConfig(in_value, _aidl_return);
  }
  ::ndk::ScopedAStatus getPsyBatterySN(std::string* _aidl_return) override {
    return _impl->getPsyBatterySN(_aidl_return);
  }
  ::ndk::ScopedAStatus getBattGaugeInfo(std::string* _aidl_return) override {
    return _impl->getBattGaugeInfo(_aidl_return);
  }
  ::ndk::ScopedAStatus setChgConfig(int32_t in_flag, const std::string& in_extra, int32_t in_callname, int32_t* _aidl_return) override {
    return _impl->setChgConfig(in_flag, in_extra, in_callname, _aidl_return);
  }
  ::ndk::ScopedAStatus getChgConfig(int32_t in_flag, const std::string& in_extra, int32_t in_callname, std::string* _aidl_return) override {
    return _impl->getChgConfig(in_flag, in_extra, in_callname, _aidl_return);
  }
  ::ndk::ScopedAStatus setUsbEyeDiagram(int32_t in_model, const std::string& in_eyeDiagram, bool in_isDefaultEyeDiagram, int32_t* _aidl_return) override {
    return _impl->setUsbEyeDiagram(in_model, in_eyeDiagram, in_isDefaultEyeDiagram, _aidl_return);
  }
  ::ndk::ScopedAStatus getUsbCurrentEyeDiagram(int32_t in_model, std::string* _aidl_return) override {
    return _impl->getUsbCurrentEyeDiagram(in_model, _aidl_return);
  }
protected:
private:
  std::shared_ptr<ICharger> _impl;
};

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
}  // namespace aidl
