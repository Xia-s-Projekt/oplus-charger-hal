# OplusCharger

A standalone implementation of the `vendor.oplus.hardware.charger` AIDL HAL.
The project ships two implementations of the same interface: a C++ service and
an equivalent Rust service. The interface is `@VintfStability` and is compiled
with the AIDL NDK backend, so the result runs as a vendor process without a
full AOSP source tree.

## What it does

Functionally, the HAL is the bridge between the platform's charging nodes and
Android's battery framework. It reports battery and adapter state, available
charge technology and charging limits to the framework, and applies
charging-control requests such as bypass charging.

The service implements `ICharger` by reading charge state from sysfs nodes and
translating it into the values the framework expects. It:

- exposes charger, battery, USB, AC and wireless sysfs nodes through the AIDL
  getters and setters;
- classifies adapter power into charge technologies (unknown below 33 W, VOOC
  from 33 W, SuperVOOC from 45 W) and reports it through the charge-info and
  charge-technology queries;
- runs the charge-detection check and a bypass-charge policy backed by a system
  property;
- answers the reserved `getInterfaceVersion` and `getInterfaceHash`
  transactions required by the VINTF stability check.

Unreadable nodes are reported with the interface error sentinel rather than
raising an error.

## Layout

```
aidl/vendor/oplus/hardware/charger/  AIDL interface definitions
include/charger/                     C++ headers
src/                                 C++ implementation and service entry point
rust/                                Rust implementation (library and binary)
rust/gen_*.py                        generators for Rust accessor/txn code
standalone/build.sh                  C++ NDK build
standalone/verify.sh                 offline verification of the built artifact
standalone/tests/                    host and integration test sources
standalone/scripts/                  AIDL and C++ source generators
standalone/tools/                    auxiliary tooling
```

`src/Charger.cpp` and `include/charger/Charger.h` are produced by
`standalone/scripts/gen_charger_sources.py`; the handwritten methods live in
`src/ChargerManual.cpp`. Regenerate those files instead of editing them.

## Requirements

- Android NDK with the `aarch64-linux-android` toolchain
- AIDL compiler from the Android build tools
- A `libbinder_ndk.so` exporting `AServiceManager_addService` and the
  `ABinderProcess_*` entry points (the NDK stub does not export them)
- Binder `include_platform` headers providing `android/binder_manager.h`
- For the Rust build: `cargo`, the `aarch64-linux-android` Rust target and the
  NDK linker

All dependency locations come from the environment or the build scripts; none
is tied to a particular checkout path.

## Prebuilt

[Get Prebuilt](https://github.com/Xia-s-Projekt/oplus-charger-hal/releases/latest) — two AArch64 ODM archives (C++ and Rust). Each contains the service under `odm/bin/hw/`, its init rc under `odm/etc/init/`, and the VINTF manifest under `odm/etc/vintf/manifest/`.

## Build

C++ service:

```sh
./standalone/build.sh
```

Rust service:

```sh
./rust/build.sh
```

Both link `libc++` statically and produce an AArch64 PIE limited to libraries a
vendor process can load. Each script prints the toolchain and library paths it
resolved.

## Staging

Build output is placed under `standalone/dist/vendor/bin/hw/`. Copy the
service to the matching `vendor/bin/hw/` location of a vendor image and
register it with the AIDL service manager under the `ICharger` instance name.
The VINTF manifest declares the `vendor.oplus.hardware.charger` interface.

## Tests

```sh
./standalone/tests/build.sh
./standalone/verify.sh [path-to-service]
```

The test sources cover an AIDL client, charge classification, node and
permission probes, and a stress test. Integration tests start the service as a
separate binder instance and override node paths through environment variables
so the real sysfs nodes are left untouched.
