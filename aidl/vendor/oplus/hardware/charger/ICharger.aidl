// ICharger.aidl - reconstructed from the shipped AIDL stub
// erofs/vendor/lib64/vendor.oplus.hardware.charger-V3-ndk.so
//
// Every method's transaction code was read out of the shipped BpCharger::
// methods (AIBinder_transact + 'mov w1, #imm'), so the method order below IS
// the transaction order: code = index + 1.  Reordering these lines changes
// every transaction code and breaks the framework client.
//
// Evidence: RE/evidence/oplus-charger/txn_codes.txt (codes 1..126)
//           RE/evidence/oplus-charger/v3-ndk-symbols.txt (signatures)
//           RE/java/smali/.../ICharger.smali (Java view of the same surface)
package vendor.oplus.hardware.charger;

import vendor.oplus.hardware.charger.testKitFeatureTestResult;

@VintfStability
interface ICharger {
    int VolDividerIcWorkModeSet(String value);
    int chgExchangeMesgInit();
    int chgExchangeSohMesgInit();
    int getAcType();
    int getBattAuthenticate();
    int getBattPPSChgIng();
    int getBattPPSChgPower();
    String getBattParamNoplug();
    int getBattShortIcOtpStatus();
    int getBattSubCurrent();
    int getBattVoocChgIng();
    int getBatteryVoltageNow();
    String getBccCsvData();
    int getBccExpStatus();
    String getBmsHeatingRunningStatus();
    int getBmsHeatingStatus();
    String getChargerControl();
    int getChargerCoolDown();
    int getChargerCriticalLog();
    int getChargerIdVolt();
    int getChargerLog();
    int getCustomSelectChgMode();
    String getDevinfoFastchg();
    int getFastCharge();
    int getParallelChgMosTestResult();
    int getPsyAcOnline();
    int getPsyBatteryCC();
    int getPsyBatteryCurrentNow();
    int getPsyBatteryFcc();
    int getPsyBatteryHmac();
    int getPsyBatteryLevel();
    int getPsyBatteryNotify();
    int getPsyBatteryPchg();
    int getPsyBatteryPchgResetCount();
    int getPsyBatteryRm();
    int getPsyBatteryShortFeature();
    int getPsyBatteryShortStatus();
    String getPsyBatteryStatus();
    int getPsyBatteryTemp();
    int getPsyChargeTech();
    int getPsyFastChgType();
    int getPsyInputCurrent();
    int getPsyOtgOnline();
    int getPsyOtgSwitch();
    int getPsyPcPortOnline();
    int getPsyQGVbatDeviation();
    int getPsyTypeOrientation();
    int getPsyUsbOnline();
    int getPsyUsbStatus();
    String getPsyWirelessRX();
    String getPsyWirelessRxVersion();
    String getPsyWirelessTX();
    String getPsyWirelessTxVersion();
    int getQgVbatDeviation();
    String getQuickModeGain();
    String getReserveSocDebug();
    int getSmartChgMode();
    int getUIsohValue();
    String getUisohDebugParameterInfo();
    int getUsbInputCurrentNow();
    int getUsbPrimalType();
    int getWiredOtgOnline();
    int getWirelessAdapterPower();
    int getWirelessCapacity();
    int getWirelessChargePumpEn();
    int getWirelessCurrentNow();
    String getWirelessDeviated();
    int getWirelessOnline();
    int getWirelessPenPresent();
    int getWirelessPtmcId();
    int getWirelessRXEnable();
    int getWirelessRealType();
    String getWirelessTXEnable();
    int getWirelessUserSleepMode();
    int getWirelessVoltageNow();
    String healthd_update_ui_soc_decimal();
    int nightstandby(int param0);
    int notifyScreenStatus(int param0);
    String queryChargeInfo();
    int setChargeEMMode(String value);
    int setChargerControl(String value);
    int setChargerCoolDown(String value);
    int setChargerCriticalLog(String value);
    int setChargerCycle(String value);
    int setChargerFactoryModeTest(String value);
    int setChargerLog(String value);
    int setChgStatusToBcc(int param0);
    int setCustomSelectChgMode(int param0, boolean enable);
    int setFastchgFwUpdate(String value);
    int setPsyMmiChgEn(String value);
    int setPsyOtgSwitch(String value);
    int setReserveSocDebug(String value);
    int setShipMode(String value);
    int setSmartChgMode(String value);
    int setSmartCoolDown(int param0, int param1, String value);
    int setTbattPwrOff(String value);
    int setUisohDebugInfo(String value);
    int setUsbPrimalType(String value);
    int setWirelessChargePumpEn(String value);
    int setWirelessFtmMode(String value);
    int setWirelessIconDelay(String value);
    int setWirelessIdtAdcTest(String value);
    int setWirelessPenSoc(String value);
    int setWirelessRXEnable(String value);
    int setWirelessTXEnable(String value);
    int setWirelessUserSleepMode(String value);
    int setWlsThirdPartitionInfo(String value);
    testKitFeatureTestResult testKitFeatureTest(int param0);
    String testKitGetFeatureList();
    String testKitGetFeatureName(int param0);
    int testKitGetFeatureNum();
    int updateUiSohToPartion();
    String queryWlsPencilInfo();
    String getChgOlcConfig();
    int setChgOlcConfig(String value);
    int setSuperEnduranceStatus(String value);
    int setSuperEnduranceCount(String value);
    int setBobStatus(String value);
    int setPsySlowChgEn(String value);
    int getCpVbatDeviation();
    int setBatteryLogPush(String value);
    int getChargingModeInGsmCall();
    int setChargingModeInGsmCall(String value);
    int setChgRusConfig(String value);
    String getPsyBatterySN();
    String getBattGaugeInfo();
    int setChgConfig(int flag, String extra, int callname);
    String getChgConfig(int flag, String extra, int callname);
    int setUsbEyeDiagram(int model, String eyeDiagram, boolean isDefaultEyeDiagram);
    String getUsbCurrentEyeDiagram(int model);
}
