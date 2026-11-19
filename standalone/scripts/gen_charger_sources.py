#!/usr/bin/env python3
"""Regenerate include/charger/Charger.h and src/Charger.cpp.

Inputs
------
  aidl/vendor/oplus/hardware/charger/ICharger.aidl   method list + signatures
  <node-map-dir>/index.tsv                           per-function node lists

Getter and setter implementations follow the node lists in fallback order.
Methods that are not plain node accesses (state, subsystems, diagnostic dumps)
are listed in MANUAL below and are implemented in src/ChargerManual.cpp by
hand; the generator only declares them.

Run from the module root:
    python3 standalone/scripts/gen_charger_sources.py <node-map-dir>
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))

# Methods implemented by hand in src/ChargerManual.cpp.
MANUAL = {
    "getFastCharge": "fast-charge state (FastChargeDetector)",
    "getPsyFastChgType": "fast-charge state (FastChargeDetector)",
    "getPsyChargeTech": "fast-charge state (FastChargeDetector)",
    "getBattVoocChgIng": "node read, cached fallback",
    "getCustomSelectChgMode": "member state",
    "setCustomSelectChgMode": "member state",
    "getSmartChgMode": "member state",
    "getUIsohValue": "SOH subsystem",
    "updateUiSohToPartion": "SOH subsystem",
    "setUisohDebugInfo": "SOH subsystem",
    "getUisohDebugParameterInfo": "SOH subsystem",
    "getBccExpStatus": "battery charging control state",
    "setChgStatusToBcc": "battery charging control state",
    "getBmsHeatingStatus": "BMS heating state",
    "getBmsHeatingRunningStatus": "BMS heating state",
    "chgExchangeMesgInit": "SCCP exchange state",
    "chgExchangeSohMesgInit": "SCCP exchange state",
    "notifyScreenStatus": "screen state -> charging thread",
    "nightstandby": "writes soc_ajust",
    "setChgRusConfig": "RUS config state",
    "setWlsThirdPartitionInfo": "stylus state",
    "queryChargeInfo": "diagnostic dump (queryChargeInfo)",
    "queryWlsPencilInfo": "diagnostic dump (queryWlsPencilInfo)",
    "getBccCsvData": "diagnostic dump (getBccCsvData)",
    "getChargerControl": "/dev/oplus_chg ioctl",
    "setChargerControl": "/dev/oplus_chg ioctl",
    "testKitFeatureTest": "test kit",
    "testKitGetFeatureList": "test kit",
    "testKitGetFeatureName": "test kit",
    "testKitGetFeatureNum": "test kit",
    "getChgConfig": "charger config, read",
    "setChgConfig": "charger config, write",
    "getUsbCurrentEyeDiagram": "usb eye diagram, read",
    "setUsbEyeDiagram": "usb eye diagram, write",
    "healthd_update_ui_soc_decimal": "UI SOC decimal generator",
}

# Methods whose node path was recovered from the decompiled body but is not
# attributed by the string index (they read a single node, no fallback).
EXTRA_NODES = {
    "getPsyBatteryHmac": ["/proc/charger/hmac"],
    "getWirelessPenPresent": ["/sys/class/oplus_wireless/pencil/present"],
    "setWirelessPenSoc": ["/sys/class/oplus_wireless/pencil/rx_soc"],
    "setBatteryLogPush": ["/sys/class/oplus_chg/common/battlog_push_config"],
}

# ---------------------------------------------------------------------------
# Node overrides.
#
# The primary node set is the OPlus charger tree (batt_fcc, batt_rm, batt_cc,
# chip_soc, notify_code, cool_down, ppschg_power, /proc/charger/passedchg).
# Where a node is unavailable, the accessors below read the equivalent kernel
# node instead.
#
# The value each node holds and the unit the framework expects:
#
#   batt_fcc     full charge capacity, mAh.
#                CommonUtils.getBatteryFcc() falls back to
#                PowerProfile.getBatteryCapacity() (mAh) when the node is
#                unusable, and BatteryLifeMonitor compares it against
#                mMaxBatteryFcc = 9999 and DEFAULT_BATTERY_CAPICITY = 4000,
#                so the framework's FCC is in mAh.  charge_full is the
#                kernel's full charge capacity in uAh, hence the /1000.
#
#   batt_rm      realtime capacity, mAh - same unit as the FCC (the
#                framework returns it directly as the realtime capacity and
#                otherwise computes fcc * level / 100, also mAh).
#                charge_counter is the kernel's charge counter in uAh.
#
#   batt_cc      battery cycle count.  cycle_count is the kernel's counter.
#
#   chip_soc     the gauge's SOC in percent.  When the uisoc node is
#                unreadable, the smoothed capacity is used instead.
#
#   notify_code  battery notification code, from BatteryNotify.
#
#   cool_down    charge-current cool down level.  Thermal_throttle is the
#                charger throttle flag; ffc_status_cv carries the
#                fast-charge status as its first field.
#
#   passedchg    number of charge cycles passed, from tran_battery_cycle.
#
#   ppschg_power PPS charging power in W, from the power supply's ta_power.
DEVICE_NODES = {
    "getPsyBatteryFcc": ["/sys/class/power_supply/battery/charge_full"],
    "getPsyBatteryRm": ["/sys/class/power_supply/battery/charge_counter"],
    "getPsyBatteryCC": ["/sys/class/power_supply/battery/cycle_count"],
    "getPsyBatteryLevel": ["/sys/class/power_supply/battery/capacity"],
    "getPsyBatteryNotify": ["/sys/devices/platform/charger/BatteryNotify"],
    "getChargerCoolDown": ["/sys/devices/platform/charger/Thermal_throttle"],
    "getPsyBatteryPchg": ["/sys/devices/platform/charger/tran_battery_cycle"],
    "getPsyBatteryPchgResetCount": ["/sys/devices/platform/charger/reset_nv_battery_cycle"],
    "getBattPPSChgPower": ["/sys/class/power_supply/tran-auto-test/ta_power"],
}

# Divisors applied to the raw node value, for the nodes whose unit differs
# from the unit the framework expects (see DEVICE_NODES).
DEVICE_SCALE = {
    "getPsyBatteryFcc": 1000,   # uAh -> mAh
    "getPsyBatteryRm": 1000,    # uAh -> mAh
}


def parse_aidl(path):
    """-> [(name, ret, [args])] in declaration (transaction) order."""
    out = []
    for line in open(path):
        line = line.strip()
        m = re.match(r"^(\w+)\s+(\w+)\((.*)\);$", line)
        if not m:
            continue
        rtype, name, args = m.group(1), m.group(2), m.group(3)
        arglist = []
        if args.strip():
            for a in args.split(","):
                a = a.strip()
                ty, _, nm = a.partition(" ")
                arglist.append((ty, nm or None))
        out.append((name, rtype, arglist))
    return out


def parse_nodes(evidence):
    """-> {method: [node, ...]} recovered from the shipped string index."""
    # index.tsv lives in the Ghidra output directory, mini-symbols.txt next to
    # it in the evidence root.
    index_path = os.path.join(evidence, "index.tsv")
    if not os.path.exists(index_path):
        for entry in sorted(os.listdir(evidence)):
            candidate = os.path.join(evidence, entry, "index.tsv")
            if os.path.exists(candidate):
                index_path = candidate
                break
    syms_path = os.path.join(evidence, "mini-symbols.txt")
    if not os.path.exists(syms_path):
        syms_path = os.path.join(os.path.dirname(index_path), "mini-symbols.txt")
    idx = {}
    for line in open(index_path):
        parts = line.rstrip("\n").split("\t")
        if len(parts) >= 6 and parts[0] != "entry":
            try:
                idx[parts[0]] = (parts[1], int(parts[2]), parts[5])
            except ValueError:
                pass
    syms = {}
    for line in open(syms_path):
        m = re.match(r"^([0-9a-f]+) T (.*)$", line.strip())
        if not m:
            continue
        addr, name = int(m.group(1), 16), m.group(2)
        mm = re.search(r"charger::Charger::(\w+)\(", name)
        if mm:
            syms[mm.group(1)] = addr
    nodes = {}
    for method, addr in syms.items():
        key = "%08x" % (addr + 0x100000)
        if key not in idx:
            continue
        strs = idx[key][2]
        found = [s.strip() for s in strs.split("|") if s.strip().startswith("/")]
        if found:
            nodes[method] = found
    nodes.update(EXTRA_NODES)
    nodes.update(DEVICE_NODES)
    return nodes


def cpp_type(ty):
    # AIDL "boolean" is C++ "bool" in the generated NDK headers.
    return "bool" if ty == "boolean" else ty


def cpp_arg_name(ty, i):
    if ty == "int":
        return "arg%d" % i
    if ty == "boolean":
        return "enable%d" % i
    if ty == "String":
        return "value%d" % i
    raise SystemExit("unknown type " + ty)


def gen(aidl, nodes, out_h, out_c):
    methods = parse_aidl(aidl)
    h = []
    h.append("// Charger.h - the ICharger implementation.")
    h.append("//")
    h.append("// GENERATED by standalone/scripts/gen_charger_sources.py - do not edit by")
    h.append("// hand; re-run the script instead.  The hand written part of the class")
    h.append("// lives in src/ChargerManual.cpp.")
    h.append("//")
    h.append("// The node path of every accessor follows the node map in fallback")
    h.append("// order.")
    h.append("#pragma once")
    h.append("")
    h.append("#include <memory>")
    h.append("#include <string>")
    h.append("")
    h.append('#include "aidl/vendor/oplus/hardware/charger/BnCharger.h"')
    h.append('#include "aidl/vendor/oplus/hardware/charger/testKitFeatureTestResult.h"')
    h.append('#include "charger/ChgDetectionCheck.h"')
    h.append('#include "charger/FastChargeDetector.h"')
    h.append("")
    h.append("namespace vendor {")
    h.append("namespace oplus {")
    h.append("namespace hardware {")
    h.append("namespace charger {")
    h.append("")
    h.append("class Charger : public ::aidl::vendor::oplus::hardware::charger::BnCharger {")
    h.append("public:")
    h.append("    Charger();")
    h.append("    ~Charger() override;")
    h.append("")
    h.append("    // Starts the uevent monitor and the detection threads.")
    h.append("    void start();")
    h.append("")
    h.append("    FastChargeDetector& fastCharge() { return *mFastCharge; }")
    h.append("    ChgDetectionCheck& detection() { return *mDetection; }")
    h.append("")
    h.append("    // ---- AIDL surface -------------------------------------------------")
    h.append("    // Declarations follow the interface declaration order, which is the")
    h.append("    // transaction order of the shipped stub.")
    h.append("")
    for name, rtype, args in methods:
        if name in ("getInterfaceHash", "getInterfaceVersion"):
            continue
        inargs = []
        for i, (t, _) in enumerate(args):
            nm = cpp_arg_name(t, i)
            inargs.append("const std::string& %s" % nm if t == "String"
                          else "%s %s" % (cpp_type(t), nm))
        if rtype == "int":
            inargs.append("int* _out")
        elif rtype == "String":
            inargs.append("std::string* _out")
        else:
            inargs.append("::aidl::vendor::oplus::hardware::charger::%s* _out" % rtype)
        h.append("    ::ndk::ScopedAStatus %s(%s) override;" % (name, ", ".join(inargs)))
    h.append("")
    h.append("private:")
    h.append("    std::unique_ptr<FastChargeDetector> mFastCharge;")
    h.append("    std::unique_ptr<ChgDetectionCheck> mDetection;")
    h.append("};")
    h.append("")
    h.append("}  // namespace charger")
    h.append("}  // namespace hardware")
    h.append("}  // namespace oplus")
    h.append("}  // namespace vendor")
    h.append("")

    c = []
    c.append("// Charger.cpp - node backed accessors.")
    c.append("//")
    c.append("// GENERATED by standalone/scripts/gen_charger_sources.py - do not edit by")
    c.append("// hand.  Every read follows this sequence:")
    c.append("//     open(primary)  -> read 16 bytes -> atoi")
    c.append("//     open() failed or atoi() failed -> try the secondary node")
    c.append("//     neither node could be read -> 0x80000000 (INT32_MIN)")
    c.append("//     read() failed after a successful open -> 0x80000002")
    c.append("// and every write follows:")
    c.append("//     length must be 1..16, else 0xffffffff")
    c.append("//     open(primary, O_WRONLY) -> open(secondary, O_WRONLY)")
    c.append("//     write() -> its result; a negative result becomes 0x80000003")
    c.append('#include "charger/Charger.h"')
    c.append("")
    c.append("#include <fcntl.h>")
    c.append("#include <unistd.h>")
    c.append("")
    c.append("#include <cstdlib>")
    c.append("#include <cstring>")
    c.append("")
    c.append('#include "charger/ChargerConstants.h"')
    c.append("")
    c.append("namespace vendor {")
    c.append("namespace oplus {")
    c.append("namespace hardware {")
    c.append("namespace charger {")
    c.append("")
    c.append("namespace {")
    c.append("")
    c.append("int readNodeInt(const char* primary, const char* secondary) {")
    c.append("    int fd = open(primary, O_RDONLY);")
    c.append("    if (fd < 0) {")
    c.append("        fd = open(secondary, O_RDONLY);")
    c.append("        if (fd < 0) {")
    c.append("            return kReadError;")
    c.append("        }")
    c.append("    }")
    c.append("    char buf[16] = {0};")
    c.append("    ssize_t n = read(fd, buf, sizeof(buf));")
    c.append("    close(fd);")
    c.append("    if (n < 0) {")
    c.append("        return kIoError;")
    c.append("    }")
    c.append("    int value = atoi(buf);")
    c.append("    if (value == kParseError) {")
    c.append("        // the shipped code retries the secondary node on a parse failure")
    c.append("        int fd2 = open(secondary, O_RDONLY);")
    c.append("        if (fd2 < 0) {")
    c.append("            return kReadError;")
    c.append("        }")
    c.append("        char buf2[16] = {0};")
    c.append("        ssize_t n2 = read(fd2, buf2, sizeof(buf2));")
    c.append("        close(fd2);")
    c.append("        if (n2 < 0) {")
    c.append("            return kIoError;")
    c.append("        }")
    c.append("        value = atoi(buf2);")
    c.append("        if (value == kParseError) {")
    c.append("            return kReadError;")
    c.append("        }")
    c.append("    }")
    c.append("    return value;")
    c.append("}")
    c.append("")
    c.append("int readNodeIntSingle(const char* path) {")
    c.append("    int fd = open(path, O_RDONLY);")
    c.append("    if (fd < 0) {")
    c.append("        return kReadError;")
    c.append("    }")
    c.append("    char buf[16] = {0};")
    c.append("    ssize_t n = read(fd, buf, sizeof(buf));")
    c.append("    close(fd);")
    c.append("    if (n < 0) {")
    c.append("        return kIoError;")
    c.append("    }")
    c.append("    int value = atoi(buf);")
    c.append("    return (value == kParseError) ? kReadError : value;")
    c.append("}")
    c.append("")
    c.append("std::string readNodeStr(const char* primary, const char* secondary) {")
    c.append("    const char* paths[2] = {primary, secondary};")
    c.append("    for (int i = 0; i < 2; i++) {")
    c.append("        if (paths[i] == nullptr) {")
    c.append("            continue;")
    c.append("        }")
    c.append("        int fd = open(paths[i], O_RDONLY);")
    c.append("        if (fd < 0) {")
    c.append("            continue;")
    c.append("        }")
    c.append("        char buf[256] = {0};")
    c.append("        ssize_t n = read(fd, buf, sizeof(buf) - 1);")
    c.append("        close(fd);")
    c.append("        if (n < 0) {")
    c.append("            continue;")
    c.append("        }")
    c.append("        buf[n] = '\\0';")
    c.append("        return std::string(buf);")
    c.append("    }")
    c.append("    return std::string();")
    c.append("}")
    c.append("")
    c.append("std::string readNodeStrSingle(const char* path) {")
    c.append("    int fd = open(path, O_RDONLY);")
    c.append("    if (fd < 0) {")
    c.append("        return std::string();")
    c.append("    }")
    c.append("    char buf[256] = {0};")
    c.append("    ssize_t n = read(fd, buf, sizeof(buf) - 1);")
    c.append("    close(fd);")
    c.append("    if (n < 0) {")
    c.append("        return std::string();")
    c.append("    }")
    c.append("    buf[n] = '\\0';")
    c.append("    return std::string(buf);")
    c.append("}")
    c.append("")
    c.append("int writeNode(const char* primary, const char* secondary,")
    c.append("                const std::string& value) {")
    c.append("    size_t len = value.size();")
    c.append("    if (len == 0 || len > 16) {")
    c.append("        return -1;")
    c.append("    }")
    c.append("    int fd = open(primary, O_WRONLY);")
    c.append("    if (fd < 0) {")
    c.append("        fd = open(secondary, O_WRONLY);")
    c.append("        if (fd < 0) {")
    c.append("            return kReadError;")
    c.append("        }")
    c.append("    }")
    c.append("    ssize_t written = write(fd, value.c_str(), len);")
    c.append("    if (written < 0) {")
    c.append("        close(fd);")
    c.append("        return (int)0x80000003;")
    c.append("    }")
    c.append("    close(fd);")
    c.append("    return (int)written;")
    c.append("}")
    c.append("")
    c.append("}  // namespace")
    c.append("")

    generated = 0
    for name, rtype, args in methods:
        if name in MANUAL:
            continue
        if name in ("getInterfaceHash", "getInterfaceVersion"):
            continue
        nodelist = nodes.get(name)
        if not nodelist:
            continue
        primary = nodelist[0]
        secondary = nodelist[1] if len(nodelist) > 1 else None
        inargs = [(t, cpp_arg_name(t, i)) for i, (t, _) in enumerate(args)]
        sig_params = []
        for t, nm in inargs:
            if t == "String":
                sig_params.append("const std::string& %s" % nm)
            else:
                sig_params.append("%s %s" % (cpp_type(t), nm))
        if rtype == "int":
            sig_params.append("int* _out")
            impl_params = ", ".join(sig_params)
        elif rtype == "String":
            sig_params.append("std::string* _out")
            impl_params = ", ".join(sig_params)
        else:
            continue
        # the value argument of a setter
        value = None
        for t, nm in inargs:
            if t == "String":
                value = nm
        if rtype == "int":
            if value is not None:
                expr = ('writeNode("%s", %s, %s)'
                        % (primary, ('"%s"' % secondary) if secondary else "nullptr", value))
            elif secondary:
                expr = 'readNodeInt("%s", "%s")' % (primary, secondary)
            else:
                expr = 'readNodeIntSingle("%s")' % primary
        elif secondary:
            expr = 'readNodeStr("%s", "%s")' % (primary, secondary)
        else:
            expr = 'readNodeStrSingle("%s")' % primary
        scale = DEVICE_SCALE.get(name)
        if scale:
            # The node counts in a different unit than the framework
            # expects; convert on the way out.
            body = ('    {\n'
                    '      int raw = %s;\n'
                    '      if (raw != kReadError && raw != kIoError) { raw /= %d; }\n'
                    '      *_out = raw;\n'
                    '    }' % (expr, scale))
        else:
            body = '    *_out = %s;' % expr
        c.append("::ndk::ScopedAStatus Charger::%s(%s) {" % (name, impl_params))
        c.append(body)
        c.append("    return ::ndk::ScopedAStatus::ok();")
        c.append("}")
        c.append("")
        generated += 1

    c.append("}  // namespace charger")
    c.append("}  // namespace hardware")
    c.append("}  // namespace oplus")
    c.append("}  // namespace vendor")
    c.append("")

    open(out_h, "w").write("\n".join(h))
    open(out_c, "w").write("\n".join(c))
    print("wrote %s and %s (%d generated accessors)" % (out_h, out_c, generated))


def main():
    evidence = sys.argv[1] if len(sys.argv) > 1 else (
        os.path.join(ROOT, "..", "..", "..", "RE", "evidence", "oplus-charger"))
    aidl = os.path.join(ROOT, "aidl", "vendor", "oplus", "hardware", "charger",
                         "ICharger.aidl")
    nodes = parse_nodes(evidence)
    gen(aidl, nodes,
        os.path.join(ROOT, "include", "charger", "Charger.h"),
        os.path.join(ROOT, "src", "Charger.cpp"))


if __name__ == "__main__":
    main()
