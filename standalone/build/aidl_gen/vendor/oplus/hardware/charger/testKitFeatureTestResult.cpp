/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: /opt/android-sdk/build-tools/35.0.0/aidl --lang=ndk --structured --stability=vintf --version=3 --hash=587d614e5ed419609cd450643820624cf151d465 -I /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/aidl --header_out=/mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/standalone/build/aidl_gen -o /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/standalone/build/aidl_gen /mnt/project/RomTools/ROM_WORKDIR/oos/ReconstructedHALSource/Oplus/OplusCharger/aidl/vendor/oplus/hardware/charger/testKitFeatureTestResult.aidl
 */
#include "aidl/vendor/oplus/hardware/charger/testKitFeatureTestResult.h"

#include <android/binder_parcel_utils.h>

namespace aidl {
namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {
const char* testKitFeatureTestResult::descriptor = "vendor.oplus.hardware.charger.testKitFeatureTestResult";

binder_status_t testKitFeatureTestResult::readFromParcel(const AParcel* _aidl_parcel) {
  binder_status_t _aidl_ret_status = STATUS_OK;
  int32_t _aidl_start_pos = AParcel_getDataPosition(_aidl_parcel);
  int32_t _aidl_parcelable_size = 0;
  _aidl_ret_status = AParcel_readInt32(_aidl_parcel, &_aidl_parcelable_size);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (_aidl_parcelable_size < 4) return STATUS_BAD_VALUE;
  if (_aidl_start_pos > INT32_MAX - _aidl_parcelable_size) return STATUS_BAD_VALUE;
  if (AParcel_getDataPosition(_aidl_parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(_aidl_parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readData(_aidl_parcel, &ret);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  if (AParcel_getDataPosition(_aidl_parcel) - _aidl_start_pos >= _aidl_parcelable_size) {
    AParcel_setDataPosition(_aidl_parcel, _aidl_start_pos + _aidl_parcelable_size);
    return _aidl_ret_status;
  }
  _aidl_ret_status = ::ndk::AParcel_readData(_aidl_parcel, &str);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  AParcel_setDataPosition(_aidl_parcel, _aidl_start_pos + _aidl_parcelable_size);
  return _aidl_ret_status;
}
binder_status_t testKitFeatureTestResult::writeToParcel(AParcel* _aidl_parcel) const {
  binder_status_t _aidl_ret_status;
  size_t _aidl_start_pos = AParcel_getDataPosition(_aidl_parcel);
  _aidl_ret_status = AParcel_writeInt32(_aidl_parcel, 0);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_parcel, ret);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  _aidl_ret_status = ::ndk::AParcel_writeData(_aidl_parcel, str);
  if (_aidl_ret_status != STATUS_OK) return _aidl_ret_status;

  size_t _aidl_end_pos = AParcel_getDataPosition(_aidl_parcel);
  AParcel_setDataPosition(_aidl_parcel, _aidl_start_pos);
  AParcel_writeInt32(_aidl_parcel, _aidl_end_pos - _aidl_start_pos);
  AParcel_setDataPosition(_aidl_parcel, _aidl_end_pos);
  return _aidl_ret_status;
}

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
}  // namespace aidl
