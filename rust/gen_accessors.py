#!/usr/bin/env python3
"""Generate rust/src/charger/accessors.rs from src/Charger.cpp.

The C++ file is itself generated, so the node lists and their fallback order
live there and nowhere else.  Reading them out at build time keeps the Rust
port from drifting by transcription.  Methods whose body is not a single
helper call are reported as HAND-WRITTEN and must be ported by hand.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src" / "Charger.cpp"
OUT = ROOT / "rust" / "src" / "charger" / "accessors.rs"

METHOD_RE = re.compile(
    r"::ndk::ScopedAStatus Charger::(\w+)\(([^)]*)\) \{\n(.*?)\n\}\n",
    re.S)

ARG_RE = re.compile(r"(?:const\s+)?(?:std::string\s*[&*]|int|bool)\s*\*?\s*(\w+)")

# Matches one helper call and captures the helper plus its literal args.
CALL_RE = re.compile(
    r"(readNodeInt|readNodeIntSingle|readNodeStr|readNodeStrSingle|writeNode)"
    r"\(((?:\"[^\"]*\"|nullptr|\w+)(?:\s*,\s*(?:\"[^\"]*\"|nullptr|\w+))*)"
    r"(?:\s*,\s*(\w+))?\)")


def parse_args(sig):
    """[(name, rust_type)] for the C++ parameters, out-params included."""
    out = []
    for part in sig.split(","):
        part = part.strip()
        if not part:
            continue
        m = ARG_RE.search(part)
        if not m:
            return None
        if "string" in part:
            rust_ty = "&str"
        elif part.startswith("bool"):
            rust_ty = "bool"
        else:
            rust_ty = "i32"
        out.append((m.group(1), rust_ty))
    return out


def split_top(s):
    """Split a C++ argument list on top-level commas."""
    parts, depth, cur = [], 0, ""
    for ch in s:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        parts.append(cur.strip())
    return parts


def rust_lit(s):
    """A Rust double-quoted string literal, or None for a nullptr."""
    if s is None:
        return "None"
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def opt(path):
    """The nullable secondary node: Some("path") or None."""
    if path is None:
        return "None"
    return f"Some({rust_lit(path)})"


def classify(name, args, body):
    """Return (rust_expr, is_string) or None when the body is not portable."""
    b = " ".join(body.split())

    # *_out = readNodeInt*(...);   (no clamp - the raw sentinel ships)
    m = re.match(r"\*_out = (readNodeInt|readNodeIntSingle)\(([^;]*)\);", b)
    if m:
        raw = split_top(m.group(2))
        lit = [x for x in raw if x.startswith('"') or x == "nullptr"]
        if m.group(1) == "readNodeIntSingle":
            return f"read_node_int_single({rust_lit(lit[0].strip(chr(34)))})", False
        sec = lit[1].strip('"') if len(lit) > 1 and lit[1] != "nullptr" else None
        return f"read_node_int({rust_lit(lit[0].strip(chr(34)))}, {opt(sec)})", False

    # *_out = <helper>(...);
    m = re.match(r"\*_out = (readNodeStr|readNodeStrSingle)\(([^;]*)\);", b)
    if m:
        raw = split_top(m.group(2))
        lit = [x for x in raw if x.startswith('"') or x == "nullptr"]
        if m.group(1) == "readNodeStrSingle":
            return f"read_node_str_single({rust_lit(lit[0].strip(chr(34)))})", True
        sec = lit[1].strip('"') if len(lit) > 1 and lit[1] != "nullptr" else None
        return f"read_node_str({rust_lit(lit[0].strip(chr(34)))}, {opt(sec)})", True

    # int val = <helper>(...); *_out = (clamp) ? 0 : val;
    m = re.match(
        r"int val = (readNodeInt|readNodeIntSingle)\(([^;]*)\); "
        r"\*_out = \(val == kReadError \|\| val == kIoError \|\| val == kParseError\) \? 0 : val;", b)
    if m:
        raw = split_top(m.group(2))
        lit = [x for x in raw if x.startswith('"') or x == "nullptr"]
        if m.group(1) == "readNodeIntSingle":
            return (f"clamp0(read_node_int_single({rust_lit(lit[0].strip(chr(34)))}))", False)
        sec = lit[1].strip('"') if len(lit) > 1 and lit[1] != "nullptr" else None
        return f"clamp0(read_node_int({rust_lit(lit[0].strip(chr(34)))}, {opt(sec)}))", False

    # *_out = writeNode(...);
    m = re.match(r"\*_out = writeNode\(([^;]*)\);", b)
    if m:
        raw = split_top(m.group(1))
        if len(raw) == 3:
            primary = raw[0].strip('"')
            secondary = raw[1].strip('"') if raw[1] != "nullptr" else None
            value = raw[2].strip()
            # The C++ value argument is one of the method parameters; the
            # generated signature names them a0, a1, ...
            if args and value in args:
                value = f"a{args.index(value)}"
            return (
                f"write_node({rust_lit(primary)}, {opt(secondary)}, {value})",
                False,
            )
    return None


def main():
    src = SRC.read_text()
    methods = METHOD_RE.findall(src)
    hand = []
    entries = []
    for name, sig, body in methods:
        parsed = parse_args(sig)
        # classify() works on the plain parameter names.
        names = [a for a, _ in parsed] if parsed else None
        got = classify(name, names, body) if parsed is not None else None
        if got is None:
            hand.append(name)
        else:
            expr, is_str = got
            entries.append((name, parsed, expr, is_str))

    lines = [
        "// GENERATED by rust/gen_accessors.py from src/Charger.cpp - do not edit by hand.",
        "// Node paths and their fallback order are the shipped ones.",
        "use crate::charger::Charger;",
        "use crate::nodes::{clamp0, read_node_int, read_node_int_single,",
        "    read_node_str, read_node_str_single, write_node};",
        "",
        "impl Charger {",
    ]
    for name, args, expr, is_str in entries:
        # The C++ out-param becomes the return value; drop it from the signature.
        params = ", ".join(f"a{i}: {t}" for i, (a, t) in enumerate(args)
                           if a != "_out")
        sig = "&self" if not params else f"&self, {params}"
        ret = "String" if is_str else "i32"
        lines.append(f"    pub fn {name}({sig}) -> {ret} {{")
        lines.append(f"        {expr}")
        lines.append("    }")
        lines.append("")
    lines.append("}")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines) + "\n")

    print(f"generated {OUT}: {len(entries)} accessors")
    print(f"hand-written ({len(hand)}): {' '.join(hand)}")
    (ROOT / "rust" / "hand_written.txt").write_text("\n".join(hand) + "\n")


if __name__ == "__main__":
    main()
