#!/usr/bin/env python3
"""Regenerate aidl/vendor/oplus/hardware/charger/ICharger.aidl.

The AIDL compiler assigns FIRST_CALL_TRANSACTION + N in declaration order, so
the method order in the interface is also its transaction-code order. Reorder
the declarations only with the understanding that it changes the wire codes.

Run from the module root:
    python3 standalone/scripts/gen_aidl.py <path-to-txn_codes.txt>
"""
import re
import sys

STR = ("std::__1::basic_string<char, std::__1::char_traits<char>, "
       "std::__1::allocator<char>>")

HEADER = """// ICharger.aidl - vendor.oplus.hardware.charger AIDL interface
//
// The AIDL compiler assigns transaction codes in declaration order, so the
// method order below is also the transaction order: code = index + 1.
// Reordering these lines changes every transaction code and breaks framework
// clients.
package vendor.oplus.hardware.charger;

import vendor.oplus.hardware.charger.testKitFeatureTestResult;

@VintfStability
interface ICharger {
"""

FOOTER = "}\n"


def parse(txn_file):
    rows = []
    for line in open(txn_file):
        code, rest = line.rstrip("\n").split("\t", 1)
        code = int(code)
        if code > 0xFFFF:
            # getInterfaceHash / getInterfaceVersion - AIDL reserved codes
            continue
        m = re.match(r"^(\w+)\((.*)\)$", rest)
        if not m:
            raise SystemExit("unparsed: " + rest)
        name, args = m.group(1), m.group(2)
        parts = [p.strip() for p in args.replace(STR, "STR").split(",")]
        ret, ins = parts[-1], parts[:-1]
        if ret == "int*":
            rtype = "int"
        elif ret == "STR*":
            rtype = "String"
        elif "testKitFeatureTestResult*" in ret:
            rtype = "testKitFeatureTestResult"
        else:
            raise SystemExit("unknown return type: " + ret)
        decl = []
        for p in ins:
            if p == "int":
                decl.append(("int", "param%d" % len(decl)))
            elif p == "bool":
                decl.append(("boolean", "enable"))
            elif p == "STR const&":
                decl.append(("String", "value"))
            else:
                raise SystemExit("unknown argument: " + p)
        # AIDL requires every parameter to be named.
        sig = "%s %s(%s);" % (rtype, name,
                              ", ".join("%s %s" % (t, n) for t, n in decl))
        rows.append((code, "    " + sig))
    rows.sort()
    return rows


def main():
    txn = sys.argv[1] if len(sys.argv) > 1 else "txn_codes.txt"
    out = sys.argv[2] if len(sys.argv) > 2 else (
        "aidl/vendor/oplus/hardware/charger/ICharger.aidl")
    rows = parse(txn)
    with open(out, "w") as f:
        f.write(HEADER)
        for _, sig in rows:
            f.write(sig + "\n")
        f.write(FOOTER)
    print("wrote %s (%d methods)" % (out, len(rows)))


if __name__ == "__main__":
    main()
