// fuzz_stress.cpp - adversarial and load harness for the charger HAL.
//
// Modes:
//   malformed <instance>  - call every one of the 130 transaction codes with a
//                           token-only parcel (no arguments) and with a single
//                           int32 where more arguments are declared.  Methods
//                           that declare arguments must answer with a non-OK
//                           status without executing their body; zero-argument
//                           methods execute their (read-only) body and are fine.
//   range <instance>      - call codes outside the 1..130 band plus the two
//                           reserved VINTF codes.  Everything except the
//                           reserved pair must answer UNKNOWN_TRANSACTION.
//   big <instance>        - 128 KiB string arguments against the in-memory
//                           setters only; nothing that writes a kernel node.
//   roundtrip <instance>  - set then read back the in-memory state.
//   hammer <instance> <seconds> <gap_us> - sustained mixed load for a fixed
//                           wall-clock duration.
//
// Output: one "FUZZ <mode> ..." summary line, one "FAIL" line per anomaly.
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_parcel.h>
#include <android/binder_parcel_utils.h>
#include <android/binder_process.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "aidl/vendor/oplus/hardware/charger/ICharger.h"

namespace {

struct Method {
    const char* name;
    int nargs;
};

// The 130 interface methods in declaration order; transaction code = index + 1.
const Method kMethods[] = {
    {"VolDividerIcWorkModeSet", 1},  // code 1
    {"chgExchangeMesgInit", 0},  // code 2
    {"chgExchangeSohMesgInit", 0},  // code 3
    {"getAcType", 0},  // code 4
    {"getBattAuthenticate", 0},  // code 5
    {"getBattPPSChgIng", 0},  // code 6
    {"getBattPPSChgPower", 0},  // code 7
    {"getBattParamNoplug", 0},  // code 8
    {"getBattShortIcOtpStatus", 0},  // code 9
    {"getBattSubCurrent", 0},  // code 10
    {"getBattVoocChgIng", 0},  // code 11
    {"getBatteryVoltageNow", 0},  // code 12
    {"getBccCsvData", 0},  // code 13
    {"getBccExpStatus", 0},  // code 14
    {"getBmsHeatingRunningStatus", 0},  // code 15
    {"getBmsHeatingStatus", 0},  // code 16
    {"getChargerControl", 0},  // code 17
    {"getChargerCoolDown", 0},  // code 18
    {"getChargerCriticalLog", 0},  // code 19
    {"getChargerIdVolt", 0},  // code 20
    {"getChargerLog", 0},  // code 21
    {"getCustomSelectChgMode", 0},  // code 22
    {"getDevinfoFastchg", 0},  // code 23
    {"getFastCharge", 0},  // code 24
    {"getParallelChgMosTestResult", 0},  // code 25
    {"getPsyAcOnline", 0},  // code 26
    {"getPsyBatteryCC", 0},  // code 27
    {"getPsyBatteryCurrentNow", 0},  // code 28
    {"getPsyBatteryFcc", 0},  // code 29
    {"getPsyBatteryHmac", 0},  // code 30
    {"getPsyBatteryLevel", 0},  // code 31
    {"getPsyBatteryNotify", 0},  // code 32
    {"getPsyBatteryPchg", 0},  // code 33
    {"getPsyBatteryPchgResetCount", 0},  // code 34
    {"getPsyBatteryRm", 0},  // code 35
    {"getPsyBatteryShortFeature", 0},  // code 36
    {"getPsyBatteryShortStatus", 0},  // code 37
    {"getPsyBatteryStatus", 0},  // code 38
    {"getPsyBatteryTemp", 0},  // code 39
    {"getPsyChargeTech", 0},  // code 40
    {"getPsyFastChgType", 0},  // code 41
    {"getPsyInputCurrent", 0},  // code 42
    {"getPsyOtgOnline", 0},  // code 43
    {"getPsyOtgSwitch", 0},  // code 44
    {"getPsyPcPortOnline", 0},  // code 45
    {"getPsyQGVbatDeviation", 0},  // code 46
    {"getPsyTypeOrientation", 0},  // code 47
    {"getPsyUsbOnline", 0},  // code 48
    {"getPsyUsbStatus", 0},  // code 49
    {"getPsyWirelessRX", 0},  // code 50
    {"getPsyWirelessRxVersion", 0},  // code 51
    {"getPsyWirelessTX", 0},  // code 52
    {"getPsyWirelessTxVersion", 0},  // code 53
    {"getQgVbatDeviation", 0},  // code 54
    {"getQuickModeGain", 0},  // code 55
    {"getReserveSocDebug", 0},  // code 56
    {"getSmartChgMode", 0},  // code 57
    {"getUIsohValue", 0},  // code 58
    {"getUisohDebugParameterInfo", 0},  // code 59
    {"getUsbInputCurrentNow", 0},  // code 60
    {"getUsbPrimalType", 0},  // code 61
    {"getWiredOtgOnline", 0},  // code 62
    {"getWirelessAdapterPower", 0},  // code 63
    {"getWirelessCapacity", 0},  // code 64
    {"getWirelessChargePumpEn", 0},  // code 65
    {"getWirelessCurrentNow", 0},  // code 66
    {"getWirelessDeviated", 0},  // code 67
    {"getWirelessOnline", 0},  // code 68
    {"getWirelessPenPresent", 0},  // code 69
    {"getWirelessPtmcId", 0},  // code 70
    {"getWirelessRXEnable", 0},  // code 71
    {"getWirelessRealType", 0},  // code 72
    {"getWirelessTXEnable", 0},  // code 73
    {"getWirelessUserSleepMode", 0},  // code 74
    {"getWirelessVoltageNow", 0},  // code 75
    {"healthd_update_ui_soc_decimal", 0},  // code 76
    {"nightstandby", 1},  // code 77
    {"notifyScreenStatus", 1},  // code 78
    {"queryChargeInfo", 0},  // code 79
    {"setChargeEMMode", 1},  // code 80
    {"setChargerControl", 1},  // code 81
    {"setChargerCoolDown", 1},  // code 82
    {"setChargerCriticalLog", 1},  // code 83
    {"setChargerCycle", 1},  // code 84
    {"setChargerFactoryModeTest", 1},  // code 85
    {"setChargerLog", 1},  // code 86
    {"setChgStatusToBcc", 1},  // code 87
    {"setCustomSelectChgMode", 2},  // code 88
    {"setFastchgFwUpdate", 1},  // code 89
    {"setPsyMmiChgEn", 1},  // code 90
    {"setPsyOtgSwitch", 1},  // code 91
    {"setReserveSocDebug", 1},  // code 92
    {"setShipMode", 1},  // code 93
    {"setSmartChgMode", 1},  // code 94
    {"setSmartCoolDown", 3},  // code 95
    {"setTbattPwrOff", 1},  // code 96
    {"setUisohDebugInfo", 1},  // code 97
    {"setUsbPrimalType", 1},  // code 98
    {"setWirelessChargePumpEn", 1},  // code 99
    {"setWirelessFtmMode", 1},  // code 100
    {"setWirelessIconDelay", 1},  // code 101
    {"setWirelessIdtAdcTest", 1},  // code 102
    {"setWirelessPenSoc", 1},  // code 103
    {"setWirelessRXEnable", 1},  // code 104
    {"setWirelessTXEnable", 1},  // code 105
    {"setWirelessUserSleepMode", 1},  // code 106
    {"setWlsThirdPartitionInfo", 1},  // code 107
    {"testKitFeatureTest", 1},  // code 108
    {"testKitGetFeatureList", 0},  // code 109
    {"testKitGetFeatureName", 1},  // code 110
    {"testKitGetFeatureNum", 0},  // code 111
    {"updateUiSohToPartion", 0},  // code 112
    {"queryWlsPencilInfo", 0},  // code 113
    {"getChgOlcConfig", 0},  // code 114
    {"setChgOlcConfig", 1},  // code 115
    {"setSuperEnduranceStatus", 1},  // code 116
    {"setSuperEnduranceCount", 1},  // code 117
    {"setBobStatus", 1},  // code 118
    {"setPsySlowChgEn", 1},  // code 119
    {"getCpVbatDeviation", 0},  // code 120
    {"setBatteryLogPush", 1},  // code 121
    {"getChargingModeInGsmCall", 0},  // code 122
    {"setChargingModeInGsmCall", 1},  // code 123
    {"setChgRusConfig", 1},  // code 124
    {"getPsyBatterySN", 0},  // code 125
    {"getBattGaugeInfo", 0},  // code 126
    {"setChgConfig", 3},  // code 127
    {"getChgConfig", 3},  // code 128
    {"setUsbEyeDiagram", 3},  // code 129
    {"getUsbCurrentEyeDiagram", 1},  // code 130
};

const size_t kMethodCount = sizeof(kMethods) / sizeof(kMethods[0]);

// Codes whose handler stays in memory (setter classification from the audit).
const int kInMemory[] = {
    2,  // chgExchangeMesgInit
    3,  // chgExchangeSohMesgInit
    78,  // notifyScreenStatus
    87,  // setChgStatusToBcc
    88,  // setCustomSelectChgMode
    97,  // setUisohDebugInfo
    107,  // setWlsThirdPartitionInfo
    112,  // updateUiSohToPartion
    124,  // setChgRusConfig
    127,  // setChgConfig
    128,  // getChgConfig
    129,  // setUsbEyeDiagram
    130,  // getUsbCurrentEyeDiagram
};
const size_t kInMemoryCount = sizeof(kInMemory) / sizeof(kInMemory[0]);

const std::string kBigString(131072, 'Z');

struct Tally {
    long total = 0, ok = 0, unknown = 0, failed = 0;
};

void bump(Tally* t, binder_status_t st) {
    ++t->total;
    if (st == STATUS_OK) {
        ++t->ok;
    } else if (st == STATUS_UNKNOWN_TRANSACTION || st == STATUS_BAD_VALUE) {
        ++t->unknown;
    } else {
        ++t->failed;
    }
}

binder_status_t oneRaw(AIBinder* b, uint32_t code, const std::string* big) {
    AParcel* in = AParcel_create();
    AParcel* out = AParcel_create();
    if (in == nullptr || out == nullptr) {
        if (in) AParcel_delete(in);
        if (out) AParcel_delete(out);
        return STATUS_NO_MEMORY;
    }
    binder_status_t st = AIBinder_prepareTransaction(b, &in);
    if (st == STATUS_OK && big != nullptr) {
        st = ndk::AParcel_writeString(in, *big);
    }
    if (st == STATUS_OK) {
        st = AIBinder_transact(b, code, &in, &out, 0);
        if (st == STATUS_OK) {
            AStatus* reply = nullptr;
            if (AParcel_readStatusHeader(out, &reply) == STATUS_OK) {
                if (!AStatus_isOk(reply)) {
                    st = static_cast<binder_status_t>(AStatus_getExceptionCode(reply));
                }
                AStatus_delete(reply);
            } else {
                st = STATUS_BAD_VALUE;
            }
        }
    }
    AParcel_delete(in);
    AParcel_delete(out);
    return st;
}

void report(const char* mode, const Tally& t, long ms) {
    printf("FUZZ %s total=%ld ok=%ld unknown=%ld failed=%ld elapsed_ms=%ld\n",
           mode, t.total, t.ok, t.unknown, t.failed, ms);
}

long msSince(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - t0)
            .count();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("usage: %s <mode> <instance> [seconds] [gap_us]\n", argv[0]);
        return 2;
    }
    const std::string mode = argv[1];
    const char* instance = argv[2];
    const auto t0 = std::chrono::steady_clock::now();

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    AIBinder* binder = AServiceManager_checkService(instance);
    if (binder == nullptr) {
        printf("FAIL service not found: %s\n", instance);
        return 1;
    }
    ::ndk::SpAIBinder sp(binder);
    auto c = aidl::vendor::oplus::hardware::charger::ICharger::fromBinder(sp);
    if (c == nullptr) {
        printf("FAIL fromBinder\n");
        return 1;
    }
    AIBinder* raw = sp.get();

    Tally t;

    if (mode == "malformed") {
        // Token-only parcel: every method that declares an argument must fail
        // to read it and answer UNKNOWN_TRANSACTION without running its body.
        for (size_t i = 0; i < kMethodCount; ++i) {
            const uint32_t code = static_cast<uint32_t>(i + 1);
            binder_status_t st = oneRaw(raw, code, nullptr);
            bump(&t, st);
            const bool expect_ok = (kMethods[i].nargs == 0);
            if (expect_ok != (st == STATUS_OK)) {
                printf("FAIL malformed empty code=%u %s nargs=%d -> %d\n", code,
                       kMethods[i].name, kMethods[i].nargs, st);
            }
        }
        // One int32 where the method declares 2+ arguments: only the first
        // argument is satisfiable, the rest must fail.
        for (size_t i = 0; i < kMethodCount; ++i) {
            if (kMethods[i].nargs < 2) continue;
            const uint32_t code = static_cast<uint32_t>(i + 1);
            AParcel* in = AParcel_create();
            AParcel* out = AParcel_create();
            binder_status_t st = AIBinder_prepareTransaction(raw, &in);
            if (st == STATUS_OK) st = AParcel_writeInt32(in, 7);
            if (st == STATUS_OK) st = AIBinder_transact(raw, code, &in, &out, 0);
            AStatus* reply = nullptr;
            if (st == STATUS_OK && AParcel_readStatusHeader(out, &reply) == STATUS_OK) {
                if (!AStatus_isOk(reply)) {
                    st = static_cast<binder_status_t>(AStatus_getExceptionCode(reply));
                }
                AStatus_delete(reply);
            }
            AParcel_delete(in);
            AParcel_delete(out);
            bump(&t, st);
            if (st == STATUS_OK) {
                printf("FAIL malformed partial code=%u %s nargs=%d ran with one of %d args\n",
                       code, kMethods[i].name, kMethods[i].nargs, kMethods[i].nargs);
            }
        }
        report("malformed", t, msSince(t0));
        return t.failed == 0 && t.total > 0 ? 0 : 1;
    }

    if (mode == "range") {
        const uint32_t codes[] = {0u, 131u, 255u, 4096u, 65535u, 0x7fffFFdu, 0x1000000u,
                                  0x7fffffffu, 0x80000000u, 0xffffFFFdu, 0xffffffffu};
        for (uint32_t code : codes) {
            binder_status_t st = oneRaw(raw, code, nullptr);
            ++t.total;
            if (st != STATUS_OK) ++t.unknown; else ++t.ok;
            if (st == STATUS_OK) {
                printf("FAIL range code=%u unexpectedly OK\n", code);
            }
        }
        // The reserved VINTF pair must answer.
        for (uint32_t code : {16777214u, 16777215u}) {
            binder_status_t st = oneRaw(raw, code, nullptr);
            ++t.total;
            if (st == STATUS_OK) ++t.ok; else { ++t.failed; printf("FAIL range reserved code=%u -> %d\n", code, st); }
        }
        report("range", t, msSince(t0));
        return t.failed == 0 ? 0 : 1;
    }

    if (mode == "big") {
        for (size_t i = 0; i < kInMemoryCount; ++i) {
            const uint32_t code = static_cast<uint32_t>(kInMemory[i]);
            binder_status_t st = oneRaw(raw, code, &kBigString);
            bump(&t, st);
            if (st != STATUS_OK) {
                printf("FAIL big code=%u -> %d\n", code, st);
            }
        }
        report("big", t, msSince(t0));
        return t.failed == 0 ? 0 : 1;
    }

    if (mode == "roundtrip") {
        // setUsbEyeDiagram / getUsbCurrentEyeDiagram is the clean in-memory pair.
        for (int k = 0; k < 3; ++k) {
            const int32_t key = 100 + k;
            const std::string val = "v" + std::to_string(k);
            int32_t rc = 0;
            auto st1 = c->setUsbEyeDiagram(key, val, false, &rc);
            std::string back;
            auto st2 = c->getUsbCurrentEyeDiagram(key, &back);
            ++t.total; ++t.total;
            if (st1.isOk() && st2.isOk() && back == val) {
                ++t.ok; ++t.ok;
            } else {
                ++t.failed; ++t.failed;
                printf("FAIL roundtrip key=%d set=%d get=%d back=%s\n", key,
                       st1.getStatus(), st2.getStatus(), back.c_str());
            }
        }
        // setUisohDebugInfo stores into state; nothing reads it back, so it only
        // has to answer OK.
        int32_t rc = 0;
        auto st = c->setUisohDebugInfo("probe", &rc);
        ++t.total;
        if (st.isOk()) ++t.ok; else { ++t.failed; printf("FAIL roundtrip setUisohDebugInfo -> %d\n", st.getStatus()); }
        report("roundtrip", t, msSince(t0));
        return t.failed == 0 ? 0 : 1;
    }

    if (mode == "one") {
        // Call a single transaction code repeatedly; used to bisect a crash.
        if (argc < 4) {
            printf("usage: one <instance> <code> [count]\n");
            return 2;
        }
        const uint32_t code = static_cast<uint32_t>(atoi(argv[3]));
        const int count = argc > 4 ? atoi(argv[4]) : 3;
        for (int i = 0; i < count; ++i) {
            binder_status_t st = oneRaw(raw, code, nullptr);
            bump(&t, st);
            if (st != STATUS_OK) printf("  one code=%u i=%d -> %d\n", code, i, st);
        }
        report("one", t, msSince(t0));
        return 0;
    }

    if (mode == "hammer") {
        const double seconds = argc > 3 ? atof(argv[3]) : 5.0;
        const long gap_us = argc > 4 ? atol(argv[4]) : 1000;
        // A spread of read-only methods, called round-robin.
        long iter = 0;
        while (true) {
            const double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            if (el >= seconds) break;
            binder_status_t st = STATUS_OK;
            switch (iter % 6) {
                case 0: { int32_t v = 0; st = c->getPsyUsbStatus(&v).getStatus(); break; }
                case 1: { int32_t v = 0; st = c->getPsyUsbOnline(&v).getStatus(); break; }
                case 2: { int32_t v = 0; st = c->getFastCharge(&v).getStatus(); break; }
                case 3: { int32_t v = 0; st = c->getChargerIdVolt(&v).getStatus(); break; }
                case 4: { int32_t v = 0; st = c->getBattVoocChgIng(&v).getStatus(); break; }
                case 5: { int32_t v = 0; st = c->getPsyChargeTech(&v).getStatus(); break; }
            }
            bump(&t, st);
            if (st != STATUS_OK) {
                printf("FAIL hammer iter=%ld case=%ld -> %d\n", iter, iter % 6, st);
            }
            ++iter;
            if (gap_us > 0) usleep(gap_us);
        }
        report("hammer", t, msSince(t0));
        return t.failed == 0 ? 0 : 1;
    }

    printf("FAIL unknown mode: %s\n", mode.c_str());
    return 2;
}
