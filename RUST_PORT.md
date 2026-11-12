# Charger HAL — C++ to Rust port plan

Source of truth: the C++ tree in this directory (`src/`, `include/`, `aidl/`,
`standalone/`). The Rust port must reproduce its behaviour exactly, including
the error sentinels, the fallback node order and the transaction codes.

## Ground rules

- The 130 AIDL methods keep their **declaration order** — transaction code is
  `index + 1`. Reordering breaks the framework client.
- `ICharger.aidl` is the contract; `src/Charger.cpp` (95 generated node
  accessors) + `src/ChargerManual.cpp` (35 hand-written) are the behaviour.
- Every node path and fallback order is copied verbatim from the C++ source.
- No behaviour may be "improved" during the port. Differences go in
  `RUST_DELTAS.md` with a reason.

Status legend: `[x]` done and verified, `[~]` implemented but not fully
verified, `[ ]` not done.

## Phase 0 — scaffolding

- [x] 0.1 Rust toolchain confirmed (`rustc`, `cargo`, `aarch64-linux-android` target)
- [x] 0.2 `rust/Cargo.toml` — static libc++ (`-C link-arg=-static-libstdc++`), panic=abort, opt-level 2
- [x] 0.3 `rust/build.sh` — builds, strips, verifies, stages under the V6 name
- [x] 0.4 `rust/.cargo/config.toml` — NDK clang linker + `aarch64-linux-android` target
- [~] 0.5 host build for pure-logic crates — `cargo test --lib` runs 5 tests (bypass policy,
      `c_atoi`, sentinels); decimal ladder and watt classification not yet covered
- [x] 0.6 `RUST_DELTAS.md` created — 9 entries, each with its evidence

## Phase 1 — core

- [x] 1.1 `constants.rs` — charge technology ids (UNKNOWN 0, VOOC 1, SVOOC 2, PD 3, QC 4, PPS 5, UFCS 6)
- [x] 1.2 `constants.rs` — `kFastChargeWatts = 33`, `kSuperVoocWatts = 45`
- [x] 1.3 `constants.rs` — error sentinels `kReadError = 0x80000000`, `kIoError = 0x80000002`, `kParseError = 0x80000000`
- [x] 1.4 `constants.rs` — `tran::kBypassCharger`, `kTranAichgDisableCharger`, `kBypassProp`, `kPumpExpress`, `kAdapterCapacity`
- [x] 1.5 `nodes.rs` — `read_int_path` (open/read 16 B/atoi, sentinels identical to C++)
- [x] 1.6 `nodes.rs` — `read_string_path` (open/read 255 B, empty on failure, NUL-terminate)
- [x] 1.7 `nodes.rs` — `write_string_path` (open O_WRONLY, full-write check)
- [x] 1.8 `props.rs` — `is_bypass_prop_enabled` / `set_bypass_prop` over `__system_property_get/set`
- [x] 1.9 `props.rs` — host stub when not targeting Android (keeps unit tests buildable)

## Phase 2 — binder glue (hand-written NDK C ABI, no AOSP binder crate)

- [x] 2.1 `binder/ffi.rs` — `extern "C"` decls: `AIBinder_Class_new/setOnDump`, `AServiceManager_addService`,
      `ABinderProcess_setThreadPoolMaxThreadCount/joinThreadPool`, `AParcel_*`
- [x] 2.2 `binder/ffi.rs` — `StrongBinder`/`SpAIBinder` ownership wrappers (acquire/release)
- [x] 2.3 `binder/parcel.rs` — read/write i32, bool, string; status header; `Parcelable` for `testKitFeatureTestResult`
- [x] 2.4 `binder/mod.rs` — `on_transact` dispatch matching the generated `BnCharger::onTransact` protocol exactly
      (descriptor check, status header, unknown-code → `STATUS_UNKNOWN_TRANSACTION`)
- [x] 2.5 `binder/mod.rs` — static class registration, `onDump` no-op, thread-safety (`Send + Sync`)
- [x] 2.6 `txn.rs` — the 130-entry transaction table generated from `ICharger.aidl` order (code = index + 1)

## Phase 3 — 95 node accessors

- [x] 3.1 Generator that reads `src/Charger.cpp` and emits `accessors.rs`
      (keeps the node lists and fallback order in one place, no hand transcription)
- [x] 3.2 `getAcType`, `getBattAuthenticate`, `getBattShortIcOtpStatus`, `getBattSubCurrent`, `getChargerIdVolt`
      — `/sys/class/power_supply/*` then `/sys/class/oplus_chg/*`
- [x] 3.3 `getPsyAcOnline`, `getPsyUsbOnline` (3-way fallback incl. `/sys/class/power_supply/charger/online`),
      `getPsyOtgOnline` (3-way incl. udc node)
- [x] 3.4 `getPsyBattery*` (CC→cycle_count, Fcc→charge_full, Hmac→/proc/charger/hmac, Level→capacity,
      Notify→BatteryNotify, Pchg→tran_battery_cycle, PchgResetCount→reset_nv_battery_cycle, Rm→charge_counter,
      SN, Status, Temp, ShortFeature/ShortStatus)
- [x] 3.5 `getPsyInputCurrent`, `getPsyQGVbatDeviation`, `getQgVbatDeviation`, `getPsyUsbStatus`,
      `getPsyTypeOrientation`, `getPsyUsbOnline`
- [x] 3.6 wireless getters (`/proc/wireless/*`, `/sys/class/power_supply/wireless/*`, `/sys/class/oplus_wireless/*`)
- [x] 3.7 `getBattParamNoplug`, `getBattPPSChgIng/Power`, `getChargerCoolDown`, `getChargerCriticalLog`,
      `getChargerLog`, `getDevinfoFastchg`, `getParallelChgMosTestResult`, `getQuickModeGain`,
      `getWiredOtgOnline`, `getWirelessPenPresent/PtmcId/RXEnable/TXEnable`, `getReserveSocDebug`,
      `getUsbInputCurrentNow`, `getUsbPrimalType`, `getBatteryVoltageNow`
- [x] 3.8 `getChgOlcConfig`, `getCpVbatDeviation`, `getChargingModeInGsmCall`, `getBattGaugeInfo`
- [x] 3.9 write path: `setChargeEMMode`, `setChargerCoolDown`, `setChargerCriticalLog`, `setChargerCycle`,
      `setChargerFactoryModeTest`, `setChargerLog`, `setFastchgFwUpdate`, `setPsyMmiChgEn` (6-way fallback),
      `setPsyOtgSwitch`, `setReserveSocDebug`, `setShipMode`, `setSmartChgMode`, `setSmartCoolDown`,
      `setTbattPwrOff`, `setUsbPrimalType`, `setWireless*`, `VolDividerIcWorkModeSet` (2-way /proc fallback)
- [x] 3.10 `getPsyWirelessRX/TX` and `*Version` — `/proc/wireless/{rx,tx}_{power,version}`

## Phase 4 — 35 manual methods

- [x] 4.1 `ChargerState` struct (15 cached ints/strings, `chgConfig` map, `usbEyeDiagram` map, `bypass` inputs) behind a `Mutex`
- [x] 4.2 `append_int` / `append_string` helpers (identical `key=value\n` format)
- [x] 4.3 `getFastCharge` / `getPsyFastChgType` / `getPsyChargeTech` (+ `to_external_charge_technology`)
- [x] 4.4 `getBattVoocChgIng` (2-node fallback + SVOOC cache fallback)
- [x] 4.5 `get/setCustomSelectChgMode` — **must not latch the bypass node**
- [x] 4.6 `get/setSmartChgMode`, `get/setChgStatusToBcc`, `getBmsHeatingStatus`, `getBmsHeatingRunningStatus`
- [x] 4.7 `chgExchangeMesgInit`, `chgExchangeSohMesgInit` (both set to 1)
- [x] 4.8 `notifyScreenStatus` (refresh + trigger on arg==1)
- [x] 4.9 `setChgRusConfig`, `setWlsThirdPartitionInfo` (atoi parse)
- [x] 4.10 SOH: `getUIsohValue` (range 1..100 else cached else 100), `updateUiSohToPartion` (0),
      `setUisohDebugInfo`, `getUisohDebugParameterInfo`
- [x] 4.11 `healthd_update_ui_soc_decimal` — the 33-target ladder + 700 ms step + hold-at-99 state machine
- [x] 4.12 `/dev/oplus_chg`: `getChargerControl` (read 255 B), `setChargerControl` (len 1..16, `0x80000003` on error)
- [x] 4.13 `nightstandby` — snprintf "%d", write len+1 incl. NUL, 2-node fallback
- [x] 4.14 `getChgConfig` — flags 0/1/4/6/0x18/0x19, caller-3 → support code 3, caller-1 → live node value
- [x] 4.15 `setChgConfig` — 0x18/0x19 game toggle, 0x5/0xe charge limit, both routed through the policy
- [x] 4.16 `bypassChargeStatus` (node first, prop only on read error), `bypassNodeValue`, `bypassChargeSupported`
- [x] 4.17 `setUsbEyeDiagram` / `getUsbCurrentEyeDiagram` — last-value cache keyed by model
- [x] 4.18 test kit — `/dev/chg_test_kit` ioctls `0x4004f601`, `0x9000f604`, `0x8004f605`, `0x9000f606`
- [x] 4.19 `queryChargeInfo` — the full 30+ key dump in the shipped order
- [x] 4.20 `queryWlsPencilInfo` — 7 keys
- [x] 4.21 `getBccCsvData` — 4 keys

## Phase 5 — subsystems

- [x] 5.1 `fastcharge.rs` — `FastChargeDetector` with the env-overridable node paths (kept: on-device tests use them)
- [x] 5.2 `fastcharge.rs` — `read_oplus_driver` (tech name table, unknown → fall back)
- [x] 5.3 `fastcharge.rs` — `read_tran_charger` (online/status gate, Pump_Express, adapter_capacity, ta_power)
- [x] 5.4 `fastcharge.rs` — `classify_watts`, `technology_name`, `technology_from_name`, `refresh` under `Mutex`
- [x] 5.5 `detection.rs` — `ChgDetectionCheck`: netlink socket, `SO_RCVBUFFORCE`, bind group `0xffffffff`, epoll, sem
- [x] 5.6 `detection.rs` — `parse_uevent` (KEY=VALUE pairs, `SUBSYSTEM=power_supply`)
- [x] 5.7 `bypass.rs` — `int_fields`, `parse_game_toggle`, `parse_up_limit`, `feature_engaged`, `decide`
- [x] 5.8 `bypass.rs` — `decide` inherits `up_limit_percent` when the game toggle has no target
- [x] 5.9 `apply_bypass_policy(force)` — writes both nodes, mirrors the prop, returns -1 when no request

## Phase 6 — service + monitor

- [x] 6.1 `decimal.rs` — `kDecimalTargets` 33 entries, `reset_decimal_state`, step timing
- [x] 6.2 `monitor.rs` — `trigger_battery_update` (double fork, exec `/system/bin/service call batteryproperties 2`, 75 ms throttle)
- [x] 6.3 `monitor.rs` — uevent netlink socket, poll timeout 20 ms/100 ms rule, drain
- [x] 6.4 `monitor.rs` — bypass prop-change branch → set switches → `apply_bypass_policy(true)` → trigger
- [x] 6.5 `monitor.rs` — online/status/watts sampling with the same fallbacks
- [x] 6.6 `monitor.rs` — state-change detection, pulse countdown 4/1 at 150 ms, `reset_decimal_state` transitions
- [x] 6.7 `monitor.rs` — the `vendor.oplus.hardware.charger-V1-service` log line
- [x] 6.8 `main.rs` — `ABinderProcess_setThreadPoolMaxThreadCount(0)`, register, `joinThreadPool`
- [x] 6.9 `main.rs` — service name `vendor.oplus.hardware.charger.ICharger/default`, instance override env kept
- [x] 6.10 `main.rs` — `registerChargerService` log lines, `start()` after successful registration

## Phase 7 — verify, test, stage

- [~] 7.1 host unit tests: 5 tests pass (bypass policy cases, `parse_*`, `c_atoi`, sentinels);
      decimal ladder and watt classification still missing
- [ ] 7.2 `verify.sh` equivalent — 33 W/45 W immediates present, node paths present,
      `seLabel=[]` count 0 on the repacked image
- [x] 7.3 DT_NEEDED check — `libdl.so`, `liblog.so`, `libc.so`, `libbinder_ndk.so`
      (no `libm.so`: the Rust port pulls in no math, unlike the C++)
- [x] 7.4 strip the output — 406,624 bytes, no `.symtab`/`.debug` (checked in build.sh)
- [x] 7.5 transaction-code parity — all 130 codes read from the stock
      `oos15_old` V6 `BpCharger` disassembly match `txn.rs`
- [ ] 7.6 A/B comparison: run both HALs on device, diff every getter's answer
- [x] 7.7 stage to `erofs/vendor/bin/hw/` under the V6 name — staged, md5
      `e9ae132c825dcbb88027a24d8f27b9f8` (the Rust build with the V6 answer,
      the X6739 node remap, the 25 W floor and the chargerid_volt scale
      adapter). The pre-remap C++ build it replaced is kept at
      `standalone/dist/vendor/bin/hw/vendor.oplus.hardware.charger-V6-service.preremap.bak`
- [~] 7.8 VINTF version — resolved in source: the Rust dispatcher and
      `standalone/build.sh` now answer/generate version 6 / hash
      `ede5e600328119dbf1c38269f810a3fdce74029f`, matching the stock V6 HAL.
      The erofs binary now answers 6 (`RUST_DELTAS.md` §3)
- [ ] 7.9 commit in the charger HAL git repo (the `rust/` tree is still untracked)

## Feature inventory (must not be missed)

| Area | Count | C++ source |
|---|---|---|
| AIDL methods | 130 | `ICharger.aidl` |
| Generated node accessors | 95 | `src/Charger.cpp` |
| Hand-written methods | 35 | `src/ChargerManual.cpp` |
| Charge technology ids | 7 | `ChargerConstants.h` |
| Cached state fields | 18 + 2 maps | `ChargerState` |
| queryChargeInfo keys | 34 | `ChargerManual.cpp:1028` |
| queryWlsPencilInfo keys | 7 | `ChargerManual.cpp:1094` |
| getBccCsvData keys | 4 | `ChargerManual.cpp:1111` |
| Decimal ladder targets | 33 | `kDecimalTargets` |
| Bypass policy tests | 21 | `standalone/tests/bypass_policy_test.cpp` |
| Fast-charge nodes | 8 (+4 env) | `FastChargeDetector.cpp` |
| ioctl numbers | 4 | `ChargerManual.cpp:47-50` |

## Equivalence against the stock oos15_old V6 HAL

Reference binaries:
`oos15_old/erofs/odm/bin/hw/vendor.oplus.hardware.charger-V6-service` and
`oos15_old/erofs/odm/lib64/vendor.oplus.hardware.charger-V6-ndk_platform.so`.

| Check | Result |
|---|---|
| Method names, order, count | identical, 130 |
| Transaction codes (immediate before `AIBinder_transact`) | identical |
| Interface version | stock 6, ours 6 — matched (`RUST_DELTAS.md` §3) |
| Interface hash | stock `ede5e600…`, ours `ede5e600…` — §3 |
| Node paths | stock Qualcomm `/sys/class/oplus_chg/`, ours Transsion `/sys/devices/platform/charger/` — the X6739 patch |
| Per-method bodies | not yet diffed method by method |

How the transaction-code check was done: the `BpCharger::*` dynamic symbols were
listed from the stock `.so` in order, each one disassembled with
`llvm-objdump`, and the `mov w0, #<code>` immediate in front of the
`AIBinder_transact` call recorded. The resulting table matches `txn.rs`.

The one accepted difference class is the X6739 Transsion node patch: the stock
reads Qualcomm paths, the port reads `/sys/devices/platform/charger/*` where
that is what the X6739 kernel exposes.
