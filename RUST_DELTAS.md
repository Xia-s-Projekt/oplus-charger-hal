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

## 11. X6739 node remapping

The stock HAL reads `/sys/class/oplus_chg/**`, `/sys/class/power_supply/usb/**`
and `/proc/charger/**`. None of those exist on this device: 130 of the 149 node
paths in the port are missing, and the charging stack is MediaTek with
Transsion additions (`/sys/class/power_supply/mtk-*`,
`/sys/devices/platform/charger/**`, `/sys/class/power_supply/tran-auto-test/**`,
`/sys/devices/platform/odm/odm:tran_battery/**`).

Every remap keeps the Oplus path as the primary and puts the X6739 path in the
secondary slot of `readNodeInt`/`writeNode`, so behaviour on Oplus hardware is
unchanged and only the X6739 fallback is new. Node existence and a sample value
were read off the device before each mapping.

| Oplus node | X6739 node | evidence |
|---|---|---|
| `/sys/class/power_supply/ac/type` | `/sys/devices/platform/charger/Charger_Type` | exists, `4` while charging |
| `/sys/class/oplus_chg/battery/ppschg_ing` | `/sys/devices/platform/charger/fast_chg_indicator` | exists, `1` while charging |
| `/sys/class/power_supply/battery/sub_current` | `/sys/class/power_supply/battery/current_avg` | exists, `8400` |
| `/sys/class/power_supply/ac/online` | `/sys/class/power_supply/mtk-master-charger/online` | exists, `1` |
| `/sys/class/power_supply/battery/short_c_hw_status` | `/sys/devices/platform/charger/cc_smt_status` | exists, `1` |
| `/proc/charger/input_current_now` | `/sys/class/power_supply/charger/input_current_limit` | exists, `1200` |
| `/sys/class/power_supply/usb/usb_status` | `/sys/class/power_supply/charger/online` | exists, `1` |
| `/sys/class/power_supply/usb/input_current_now` | `/sys/class/power_supply/charger/input_current_limit` | same node as above |
| `/sys/class/power_supply/usb/primal_type` | `/sys/devices/platform/charger/chr_type` | exists, `5` (PD) |
| `/sys/class/oplus_chg/battery/gsm_call_ongoing` | `/sys/devices/platform/odm/odm:tran_battery/tran_call_state` | exists, `0` |
| `/sys/class/oplus_chg/battery/battery_sn` | `/sys/class/power_supply/tran-auto-test/battery_id` | exists, `0` |
| `/sys/class/oplus_chg/battery/em_mode` (write) | `/sys/devices/platform/charger/enable_meta_current_limit` | exists, `1` |
| `/sys/class/oplus_chg/battery/cool_down` (write) | `/sys/devices/platform/charger/Thermal_throttle` | exists, `0` |
| `/sys/class/oplus_chg/battery/ship_mode` (write) | `/sys/devices/platform/charger/shipmode_enable` | exists, empty |
| `/sys/class/oplus_chg/battery/chargerid_volt` | `/sys/devices/platform/charger/ADC_Charger_Voltage` | exists, `4993` (mV) |

`getChargerIdVolt` reads the MTK node through a scale adapter: `ADC_Charger_Voltage`
is in mV while the Oplus node is in uV, so the fallback value is multiplied
by 1000 before it is reported (`* 1000` in C++, `val *= 1000` in Rust). The
framework consumer (`OplusBatteryService.writeCriticalLog`, classes.dex)
appends the int verbatim under the label `chargerid_volt:`, so the unit
convention is the Oplus driver's and is not verifiable inside this tree; the
mV side is - `ADC_Charger_Voltage` reads 4993 on a 5 V adapter. Error returns
are passed through unscaled, so a missing node still reports `kReadError`.

`setPsyMmiChgEn` needed its own branch: the device has no
`mmi_charging_enable`, and `/sys/class/power_supply/battery/disable` is the
inverse of it, so the value is inverted before the write. `getPsyUsbOnline`
already had a `/sys/class/power_supply/charger/online` fallback and
`getPsyOtgOnline` already had the `/sys/class/udc/11201000.usb0/is_otg`
fallback, so neither needed a change. `setPsyOtgSwitch` already writes
`odm:tran_battery/OTG_CTL`, which exists and is `rw-rw-rw`.

Left unmapped on purpose, with no equivalent on this device:
`/proc/wireless/**`, `/sys/class/power_supply/wireless/**` and
`/sys/class/oplus_wireless/pencil/**` (no wireless charging hardware),
`/dev/oplus_chg` and `/dev/chg_test_kit` (Oplus-only ioctl devices),
`battery_authenticate`, `battery_charge_technology`, `chip_soc`,
`battery_voltage_min`, `battery_charge_now`, `parallel_chg_mos_status`,
`short_ic_otp_status`, `ppschg_power`, `usb_fast_chg_type`, `soc_ajust`,
`chg_olc_config`, `super_endurance_mode_*`, `bob_status`, `slow_chg_en`,
`battlog_push_config`, `gauge_info`, `battery_type`.

## 12. Wattage below 25 W is reported as 0

`kMinReportWatts = 25` (`ChargerConstants.h:57`). A sub-25 W reading is the
5 W / 10 W baseline a stock charger reports, not a rated adapter, so
`reportWatts()` clamps it to 0 before it reaches the detector state, the
`chg_power_watts` entry in the queryChargeInfo dump, `getPowerWatts()` and the
monitor's "Power change detected" log line. The Rust mirrors it as
`report_watts()` in `fastcharge.rs` and a clamp in `monitor.rs`.

This is the only intentional behaviour change in the port: everything else is
the shipped logic on different node paths. The classification thresholds are
untouched (33 W / 45 W), so a clamped reading can never have been a fast charge
anyway.

## 13. Bypass charging: the prop no longer latches transient decisions

An audit of the bypass path found three flaws that together explain a bypass
that engages on its own and then will not clear:

1. `applyBypassPolicy` wrote `persist.sys.rianixia.bypass_charge.state` to
   follow every decision. A transient engagement (the charge limit was
   reached, GameSpace asked) persisted the prop, `tran_optimize.rc` re-applied
   it at the next boot, and the monitor then latched `gameSwitch = 1` with
   `upLimitPercent = 0` (in-memory only, lost on reboot) — so `featureEngaged`
   returned true at *any* capacity and the phone stopped charging right after
   boot.
2. The monitor re-derived the node only when the prop *changed*. When the node
   was left at 1 with the prop at 0 (an external writer, or the charge limit
   crossed back below its target), nothing ever cleared it, and
   `bypassChargeStatus()` reads the node, so the framework reported bypass on
   with the toggle showing off. This was also a regression: d085d15's monitor
   had an `else if (bypassProp == 1)` node guard that the `applyBypassPolicy`
   refactor dropped.
3. `setPsyMmiChgEn` wrote the node directly without recording a request, so
   the monitor could never reconcile it.

The fix:

- `applyBypassPolicy` no longer writes the prop. The prop is the user's
  standing intent and only the user's toggle sets it; the rc trigger applies
  it at boot. Transient decisions live on the node and are re-derived.
- The monitor reconciles every iteration: when the live node disagrees with
  the last decision it re-runs the policy with `force`, so a node left at 1
  with nothing requesting it clears within one poll interval, and a charge
  limit releases when the battery drops below its target without waiting for
  the framework to re-send `setChgConfig`.
- `setPsyMmiChgEn` records `mmiChargingDisabled` in the charger state and
  routes through `applyBypassPolicy(true)` instead of writing the node, so
  the factory-mode disable is a request the monitor owns and can clear.

The MMI request is deliberately kept out of `bypass::Inputs`: the policy
struct stays the Oplus-feature-only unit-testable surface, and the C++ and
Rust `applyBypassPolicy` fold it into the decision (`value = 1,
anyRequest = true`) after `decide()`.

## 13. Wire-format corrections found by the on-device A/B (not deltas)

The probe-full A/B (`standalone/tests/probe_full.cpp`, RUST_PORT.md 7.6)
surfaced three port defects that are bugs, not intentional differences. All
were fixed to match the generated C++ exactly, and are recorded here so the
wire format is documented in one place.

1. **Parcelable reply missing the non-null marker.** The generated Bp reads
   a parcelable return through `ndk::AParcel_readParcelable`
   (binder_parcel_utils.h:698), which consumes an int32 marker first and
   requires it non-zero. `ndk::AParcel_writeParcelable` writes `1` before
   `writeToParcel`. `txn.rs`'s `write_parcelable` skipped the marker, so
   `testKitFeatureTest` replies made the client read the parcelable size as
   the marker and fail with `STATUS_BAD_VALUE` (-22). Fixed by writing the
   marker before the size backpatch.
2. **`read_string` kept the wire NUL.** `AParcelStringAllocator` is called
   with a length that *includes* the NUL terminator (binder_parcel.h). The
   Rust allocator sized the Vec to that length, so every AIDL `String` read
   carried an interior NUL; `CString::new` then rejected the value when it
   was written back out, which silently dropped the string from the reply
   (`getUsbCurrentEyeDiagram` after `setUsbEyeDiagram` answered a 4-byte
   reply and the client saw -2147483640). Fixed by truncating the trailing
   NUL after the read.
3. **Pre-remap node paths still live in the hand-written accessors.** The
   X6739 remap (section 11) was applied to the generated accessors but not
   to five hand-written call sites: `txn_getBattVoocChgIng`
   (`fast_chg_indicator` primary), `txn_getPsyUsbStatus`
   (`charger/online` fallback), `txn_getBccCsvData` (`cool_down` ->
   `Thermal_throttle`), and `queryChargeInfo`'s `battery_voocchg_ing`,
   `battery_ppschg_ing`, `battery_sub_current`, `usb_input_current_now`.
   All now match `Charger.cpp` / `ChargerManual.cpp` line for line.

Also corrected during the A/B: `KernelTest::run_test` in `testkit.rs`
returned `-ENODEV` when the device is missing, but the C++ caller
(`Charger::testKitFeatureTest`) propagates `KernelTest::open()`'s return,
which is `-1` (the `::open()` failure value, not `-errno` despite the class
comment). The probe showed the C++ HAL answering `ret=-1`; the Rust port now
matches.

## 14. The monitor loop never ran (fixed, not a delta)

`run_monitor_loop` took a `stop: &AtomicBool` that main.rs constructed as
`AtomicBool::new(false)` and looped `while stop.load(...)`, so the loop body
executed zero times: no bypass reconciliation, no fast-charge refresh, no
battery-update pulses. The C++ it ports (`Charger::runMonitorLoop`,
ChargerManual.cpp:339) loops `while (mRunning.load(std::memory_order_relaxed))`
with mRunning set true by `Charger::start()`. The Rust port already had the
equivalent `Charger::running` flag and called `set_running(true)` before
registration; the loop now reads `charger.running()` and the redundant `stop`
parameter is gone. Verified on device: a `bypass_charger` node latched at 1
with the prop at 0 clears within one poll interval, and toggling
`persist.sys.rianixia.bypass_charge.state` 0->1->0 drives the node 0->1->0.
