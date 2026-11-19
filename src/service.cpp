// service.cpp - the HAL entry point.
//
// main() sets up the binder thread pool, registers the service and joins the
// pool. registerChargerService() builds the instance name by appending
// "/default" to the interface descriptor
// "vendor.oplus.hardware.charger.ICharger" and calls
// AServiceManager_addService() with the BnCharger binder.
//
// The service creates and starts the charge-detection check here, which is
// the part the fast-charge path needs.
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/log.h>
#include <android/binder_status.h>

#include <memory>
#include <string>

#include "aidl/vendor/oplus/hardware/charger/BnCharger.h"
#include "aidl/vendor/oplus/hardware/charger/ICharger.h"
#include "charger/Charger.h"

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {
namespace {

const char kServiceName[] = "vendor.oplus.hardware.charger.ICharger";
// `used` keeps the literal in .rodata: the optimizer would otherwise fold
// this 8-byte string into immediate stores when it builds the std::string,
// and the service instance name would stop being greppable in the binary.
__attribute__((used)) const char kServiceInstance[] = "/default";
const char kLogTag[] = "vendor.oplus.hardware.charger-V1-service";

// The instance suffix can be overridden so a test build can register next to
// the HAL that is already running on the device instead of colliding with it.
// Unset - the shipped behaviour - means "/default".
std::string serviceInstance() {
    const char* override = getenv("OPLUS_CHARGER_INSTANCE");
    if (override != nullptr && override[0] != '\0') {
        return std::string("/") + override;
    }
    return kServiceInstance;
}

}  // namespace

int registerChargerService() {
    // The shipped code builds the BnCharger object in place, installs the
    // derived Charger vtable on it and registers the resulting binder under
    // the descriptor + instance.  The generated BnCharger::createBinder()
    // binds "this", so the onTransact dispatch reaches the Charger overrides
    // directly - no delegator is needed.
    std::shared_ptr<::aidl::vendor::oplus::hardware::charger::ICharger> service =
            ndk::SharedRefBase::make<vendor::oplus::hardware::charger::Charger>();

    const std::string name = std::string(kServiceName) + serviceInstance();
    ::ndk::SpAIBinder binder = service->asBinder();
    binder_status_t status = AServiceManager_addService(binder.get(), name.c_str());
    if (status != STATUS_OK) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "addService(%s) failed: %d", name.c_str(), status);
        return -1;
    }

    auto* charger = static_cast<vendor::oplus::hardware::charger::Charger*>(service.get());
    if (charger != nullptr) {
        charger->start();
    }
    // The shipped binary logs "registerChargerService" with the registration
    // result appended, so a failure reads "registerChargerService-1".
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "registerChargerService%d", 0);
    return 0;
}

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor

int main(int /*argc*/, char** /*argv*/) {
    using namespace vendor::oplus::hardware::charger;

    ABinderProcess_setThreadPoolMaxThreadCount(0);

    int status = registerChargerService();
    if (status != 0) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "registerChargerService%d", status);
    } else {
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "oplus charger service start");
    }

    ABinderProcess_joinThreadPool();
    return 0;
}
