/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: /opt/android-sdk/build-tools/35.0.0/aidl --lang=ndk --structured --stability=vintf --version=6 --hash=ede5e600328119dbf1c38269f810a3fdce74029f -I /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/aidl --header_out=/mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/standalone/build/aidl_gen -o /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/standalone/build/aidl_gen /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/aidl/vendor/oplus/hardware/charger/testKitFeatureTestResult.aidl
 */
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <android/binder_interface_utils.h>
#include <android/binder_parcelable_utils.h>
#include <android/binder_to_string.h>
#ifdef BINDER_STABILITY_SUPPORT
#include <android/binder_stability.h>
#endif  // BINDER_STABILITY_SUPPORT

namespace aidl {
namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {
class testKitFeatureTestResult {
public:
  typedef std::false_type fixed_size;
  static const char* descriptor;

  int32_t ret = 0;
  std::string str;

  binder_status_t readFromParcel(const AParcel* parcel);
  binder_status_t writeToParcel(AParcel* parcel) const;

  inline bool operator==(const testKitFeatureTestResult& _rhs) const {
    return std::tie(ret, str) == std::tie(_rhs.ret, _rhs.str);
  }
  inline bool operator<(const testKitFeatureTestResult& _rhs) const {
    return std::tie(ret, str) < std::tie(_rhs.ret, _rhs.str);
  }
  inline bool operator!=(const testKitFeatureTestResult& _rhs) const {
    return !(*this == _rhs);
  }
  inline bool operator>(const testKitFeatureTestResult& _rhs) const {
    return _rhs < *this;
  }
  inline bool operator>=(const testKitFeatureTestResult& _rhs) const {
    return !(*this < _rhs);
  }
  inline bool operator<=(const testKitFeatureTestResult& _rhs) const {
    return !(_rhs < *this);
  }

  static const ::ndk::parcelable_stability_t _aidl_stability = ::ndk::STABILITY_VINTF;
  inline std::string toString() const {
    std::ostringstream _aidl_os;
    _aidl_os << "testKitFeatureTestResult{";
    _aidl_os << "ret: " << ::android::internal::ToString(ret);
    _aidl_os << ", str: " << ::android::internal::ToString(str);
    _aidl_os << "}";
    return _aidl_os.str();
  }
};
}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
}  // namespace aidl
