# Rust port — intentional differences

Every entry is a place where the Rust port does not reproduce the C++ byte for
byte. Each one names the evidence. Anything not listed here is meant to be
identical to `../src` and `../include`.

Equivalence evidence collected while writing this file:

- **Interface table.** The 130 AIDL methods, their names, their declaration
  order and their transaction codes (`index + 1`) match the stock
  `oos15_old/erofs/odm/lib64/vendor.oplus.hardware.charger-V6-ndk_platform.so`
  exactly. Verified by extracting the `BpCharger::*` dynamic symbols and
  disassembling each one to read the transaction-code immediate in front of
  `AIBinder_transact`.
- **Node paths.** Every `/sys/...` literal in `src/*.cpp` and
  `include/charger/*.h` is present in the built binary; the only literal that
  is not (`/sys/devices/platform/charger/chgspeed_ctl`, `kChgSpeedCtl`) is
  declared in `ChargerConstants.h:90` and never used by any C++ function.
  Verified with a string-by-string comparison of the staged C++ binary against
  the Rust binary.

## 1. `AIBinder_Class_define`, not `AIBinder_Class_new`

`AIBinder_Class_new` is not exported by the ROM's
`erofs/system/system/lib64/libbinder_ndk.so` nor by NDK r27c's copy.
`AIBinder_Class_define` is, with the identical signature
`(descriptor, onCreate, onDestroy, onTransact)`, so `main.rs` calls that.

## 2. Links the ROM's `libbinder_ndk.so`

`ABinderProcess_setThreadPoolMaxThreadCount`, `ABinderProcess_joinThreadPool`
and `ABinderProcess_startThreadPool` exist in the ROM's library
(`nm -D` shows all three) but not in
`NDK/sysroot/usr/lib/aarch64-linux-android/34/libbinder_ndk.so`, which exports
only the `AParcel_*` / `AIBinder_*` subset. `rust/.cargo/config.toml` therefore
adds `-L <rom>/system/system/lib64` before `-lbinder_ndk`, the same library the
C++ `standalone/build.sh` links.

## 3. Interface version 3 / hash `587d614e…`, not version 6

The Rust dispatcher answers `INTERFACE_VERSION = 3` and
`INTERFACE_HASH = 587d614e5ed419609cd450643820624cf151d465`, because that is
what our pre-port C++ was generated with (`standalone/build.sh` passes
`--version=3`). Keeping it is required for the port to be one-to-one with the
C++ it replaces.

The stock reference is different, and this is a **pre-existing mismatch in the
port, not something the Rust port introduced**:

- `oos15_old/erofs/odm/lib64/vendor.oplus.hardware.charger-V6-ndk_platform.so`
  answers version 6. Evidence: `BnCharger::getInterfaceVersion` at `.text`
  `0x20bc4` contains `mov w9, #0x6` / `str w9, [x1]` at `0x20bd0`.
- The same library carries the interface hash
  `ede5e600328119dbf1c38269f810a3fdce74029f`.

The cause is that `aidl/vendor/oplus/hardware/charger/ICharger.aidl` was
reconstructed from the V3 tree, so `aidl-gen` was run with `--version=3`. The
method table is identical, so the difference is textual.

**This must be settled before a repack+flash.** Our tree's
`erofs/vendor/etc/vintf/manifest/manifest_oplus_charger_aidl.xml` declares
`<version>6</version>` and `erofs/vendor/etc/init/...rc` starts
`...charger-V6-service`, while the binary answers 3. Either regenerate the AIDL
stubs with `--version=6 --hash=ede5e600…`, or drop the manifest/init to 3.

## 4. `open` is declared variadic in `monitor::libc`

`std` already defines a non-variadic `open` symbol for the runtime. Declaring
`fn open(*const c_char, c_int) -> c_int` in the local `libc` module made rustc
emit "invalid definition of the runtime `open` symbol". The declaration is
therefore `pub fn open(path: *const c_char, flags: c_int, ...) -> c_int;`, which
is what libc actually has. Same function, same ABI.

## 5. `c_atoi` clamps instead of wrapping on overflow

The C++ calls `std::atoi` (`src/Charger.cpp:47,60,79` and
`src/BypassPolicy.cpp:56,67`), whose overflow behaviour is UB. The Rust
accumulates into `i64` and clamps to `i32::MIN..=i32::MAX`. Node contents are
always small, so the difference is unreachable in practice; it exists so the
port has defined behaviour rather than UB.

## 6. `setWirelessPenSoc` and `setBatteryLogPush` are generated accessors

Both were first hand-written in `charger/mod.rs` as state-only setters. The C++
bodies (`src/Charger.cpp:600` and `:655`) are plain `writeNode(...)` calls with
no state update, so the hand-written versions dropped the node write. They are
now produced by `gen_accessors.py` and forwarded by `charger/bridge.rs`, which
restores the write. This is a port-accuracy fix, not a behaviour change.

## 7. `constants::tran::OTG_CTL` path

`ChargerConstants.h:92` defines
`/sys/devices/platform/odm/odm:tran_battery/OTG_CTL`. An earlier draft of
`constants.rs` had `/sys/class/...` instead; corrected to match the C++.

## 8. Monitor thread ownership

`service.cpp` spawns the monitor loop inside `registerChargerService()`. The
Rust `main.rs` does the same: the `Charger` singleton is a leaked `Box`
returning `&'static Charger`, the binder context is null, and the monitor
thread is started after `AServiceManager_addService` succeeds. The C++ uses a
`shared_ptr` singleton; the ownership differs, the lifetime does not (both live
for the process).

## 9. Warnings, not behaviour

230 compiler warnings, none of which change behaviour: `non_snake_case` on the
`txn_<MethodName>` names (kept so the names match the AIDL methods one to one),
`non_camel_case_types` on the hand-written `libc` shim types, and dead-code
warnings for fields the C++ also only writes.
