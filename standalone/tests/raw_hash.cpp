// raw_hash.cpp - read the reserved AIDL built-ins from a running service.
//
// The framework checks a VINTF-stable service's interface version and hash
// through the reserved transaction codes 0xfffffe (getInterfaceHash) and
// 0xffffff (getInterfaceVersion).  They are answered by the Bn* stub, not by
// the interface's method list, so a client built from a plain .aidl cannot
// call them.  This probe talks to the binder directly.
//
// Usage: raw_hash <service-instance>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_parcel.h>
#include <android/binder_parcel_utils.h>
#include <android/binder_process.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "aidl/vendor/oplus/hardware/charger/ICharger.h"

namespace {

const uint32_t kGetInterfaceHash = 16777214;      // 0xfffffe
const uint32_t kGetInterfaceVersion = 16777215;   // 0xffffff

bool readBuiltIn(AIBinder* binder, uint32_t code, std::string* out, int32_t* version) {
    AParcel* in = AParcel_create();
    AParcel* outp = AParcel_create();
    if (in == nullptr || outp == nullptr) {
        printf("AParcel_create failed\n");
        return false;
    }
    // AIBinder_prepareTransaction writes the interface token, which AIDL's
    // onTransact compares - without it the call is answered with
    // STATUS_BAD_VALUE (-22).  It needs the class to be associated with the
    // binder, which the generated fromBinder() does.
    binder_status_t prepared = AIBinder_prepareTransaction(binder, &in);
    if (prepared != STATUS_OK) {
        printf("prepareTransaction failed: %d\n", prepared);
        AParcel_delete(in);
        AParcel_delete(outp);
        return false;
    }
    binder_status_t status = AIBinder_transact(binder, code, &in, &outp, 0);
    if (status != STATUS_OK) {
        printf("transact(code=%u) failed: %d\n", code, status);
        AParcel_delete(in);
        AParcel_delete(outp);
        return false;
    }
    AStatus* st = nullptr;
    if (AParcel_readStatusHeader(outp, &st) != STATUS_OK) {
        printf("readStatusHeader failed\n");
        AParcel_delete(in);
        AParcel_delete(outp);
        return false;
    }
    bool ok = AStatus_isOk(st);
    if (!ok) {
        printf("code=%u returned an exception: %d\n", code, AStatus_getExceptionCode(st));
    } else if (code == kGetInterfaceHash) {
        ok = ndk::AParcel_readString(outp, out) == STATUS_OK;
        if (!ok) {
            printf("readString failed\n");
        }
    } else {
        ok = AParcel_readInt32(outp, version) == STATUS_OK;
        if (!ok) {
            printf("readInt32 failed\n");
        }
    }
    AParcel_delete(in);
    AParcel_delete(outp);
    return ok;
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

    // fromBinder associates the AIDL class with the binder, which is what
    // lets AIBinder_prepareTransaction write the interface token.
    ::ndk::SpAIBinder sp(binder);
    std::shared_ptr<aidl::vendor::oplus::hardware::charger::ICharger> charger =
            aidl::vendor::oplus::hardware::charger::ICharger::fromBinder(sp);
    if (charger == nullptr) {
        printf("FAIL fromBinder returned null\n");
        return 1;
    }

    std::string hash;
    int32_t version = -1;
    if (readBuiltIn(binder, kGetInterfaceHash, &hash, &version)) {
        printf("hash    = %s\n", hash.c_str());
    }
    if (readBuiltIn(binder, kGetInterfaceVersion, &hash, &version)) {
        printf("version = %d\n", version);
    }
    return 0;
}
