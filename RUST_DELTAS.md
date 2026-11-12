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

## 3. Interface version 6 / hash `ede5e600…` (resolved)

The dispatcher answers `INTERFACE_VERSION = 6` and
`INTERFACE_HASH = ede5e600328119dbf1c38269f810a3fdce74029f`, matching the stock
reference and our VINTF manifest. Verified in the built binary: the dispatch
compares `w1` against `0xfffffe` (hash) and `0xffffff` (version), the version
arm writes `mov w1, #0x6`, and the hash literal is in `.rodata`.

Evidence for the two constants:
- Version: `BnCharger::getInterfaceVersion` in
  `oos15_old/erofs/odm/lib64/vendor.oplus.hardware.charger-V6-ndk_platform.so`
  (`.text` 0x20bc4) contains `mov w9, #0x6` / `str w9, [x1]` at 0x20bd0.
  `IChargerDefault::getInterfaceVersion` writes 0, which is the unstubbed
  default; the interface itself is 6.
- Hash: `ede5e600…` is the only 40-hex string in that library, and
  AIDL-generated code embeds exactly one interface-hash string per interface.

This was previously a mismatch: `aidl/vendor/oplus/hardware/charger/ICharger.aidl`
was reconstructed from the V3 tree, so `aidl-gen` ran with `--version=3
--hash=587d614e…`. `standalone/build.sh` now passes `--version=6
--hash=ede5e600…` so a C++ rebuild does not regress.

**Not yet done:** the binary staged at
`erofs/vendor/bin/hw/vendor.oplus.hardware.charger-V6-service` is still the
earlier C++ build, which answers 3. It has to be replaced (with the Rust build
or a V6 C++ rebuild) before a repack+flash.

## 4. `dispatch` answers the version/hash transactions

`gen_txn.py` emits `dispatch_interface_query` as a separate function, and the
first draft never called it, so the version/hash transactions fell through to
`STATUS_UNKNOWN_TRANSACTION` and both constants were dead-code-eliminated out
of the binary. `dispatch` now calls it before the method-table match, the way
the generated `_onTransact` does.

## 5. `open` is declared variadic in `monitor::libc`

`std` already defines a non-variadic `open` symbol for the runtime. Declaring
`fn open(*const c_char, c_int) -> c_int` in the local `libc` module made rustc
emit "invalid definition of the runtime `open` symbol". The declaration is
therefore `pub fn open(path: *const c_char, flags: c_int, ...) -> c_int;`, which
is what libc actually has. Same function, same ABI.

## 6. `c_atoi` clamps instead of wrapping on overflow

The C++ calls `std::atoi` (`src/Charger.cpp:47,60,79` and
`src/BypassPolicy.cpp:56,67`), whose overflow behaviour is UB. The Rust
accumulates into `i64` and clamps to `i32::MIN..=i32::MAX`. Node contents are
always small, so the difference is unreachable in practice; it exists so the
port has defined behaviour rather than UB.

## 7. `setWirelessPenSoc` and `setBatteryLogPush` are generated accessors

Both were first hand-written in `charger/mod.rs` as state-only setters. The C++
bodies (`src/Charger.cpp:600` and `:655`) are plain `writeNode(...)` calls with
no state update, so the hand-written versions dropped the node write. They are
now produced by `gen_accessors.py` and forwarded by `charger/bridge.rs`, which
restores the write. This is a port-accuracy fix, not a behaviour change.

## 8. `constants::tran::OTG_CTL` path

`ChargerConstants.h:92` defines
`/sys/devices/platform/odm/odm:tran_battery/OTG_CTL`. An earlier draft of
`constants.rs` had `/sys/class/...` instead; corrected to match the C++.

## 9. Monitor thread ownership

`service.cpp` spawns the monitor loop inside `registerChargerService()`. The
Rust `main.rs` does the same: the `Charger` singleton is a leaked `Box`
returning `&'static Charger`, the binder context is null, and the monitor
thread is started after `AServiceManager_addService` succeeds. The C++ uses a
`shared_ptr` singleton; the ownership differs, the lifetime does not (both live
for the process).

## 10. Warnings, not behaviour

230 compiler warnings, none of which change behaviour: `non_snake_case` on the
`txn_<MethodName>` names (kept so the names match the AIDL methods one to one),
`non_camel_case_types` on the hand-written `libc` shim types, and dead-code
warnings for fields the C++ also only writes.
