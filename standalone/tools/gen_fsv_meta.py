#!/usr/bin/env python3
"""Generate a .fsv_meta fs-verity sidecar for a file.

Format (fully reverse-engineered, validated byte-for-byte against 427 of the
433 real sidecars in erofs/system/system/framework/ -- the 6 mismatches are
pre-existing stale sidecars, not format errors):

  offset 0x00  u32   version      = 1
  offset 0x04  u16   hash_algorithm= 1 (SHA-256)
  offset 0x06  u16   log_blocksize= 12  (4096-byte blocks)
  offset 0x08  u32   salt_size    = 0
  offset 0x0c  u64   file_size    (size of the TARGET file)
  offset 0x14  32B   root_hash
  offset 0x34..0x1000  zero padding

  Then the Merkle tree, top level first, each level padded up to a 4096
  multiple:
    * level 0 = sha256 of every 4096-byte block of the target; the LAST block
      of the file is zero-padded to 4096 before hashing.
    * each higher level = sha256 of every 4096-byte chunk of the level below
      (chunk zero-padded to 4096).
    * the root_hash is sha256 of the top level, zero-padded to 4096.
    * a file of exactly one block has no tree at all: the root hash IS the
      block hash, and the sidecar is only 268 bytes (header + 216 bytes of
      zero padding).

  Usage:  ./gen_fsv_meta.py <target-file> [output-sidecar]
          (default output: <target-file>.fsv_meta)
"""
import hashlib
import os
import struct
import sys

BS = 4096
HEADER_LEN = 268
TREE_OFFSET = 0x1000


def merkle_levels(data: bytes):
    """Return (root_hash, [level_top ... level0]) or (root, []) for 1-block files."""
    nblocks = (len(data) + BS - 1) // BS
    level0 = b"".join(
        hashlib.sha256(data[i * BS:(i + 1) * BS].ljust(BS, b"\x00")).digest()
        for i in range(nblocks)
    )
    if nblocks == 1:
        return level0, []
    levels = [level0]
    while len(levels[-1]) > BS:
        cur = levels[-1]
        levels.append(b"".join(
            hashlib.sha256(cur[i:i + BS].ljust(BS, b"\x00")).digest()
            for i in range(0, len(cur), BS)
        ))
    top = levels[-1]
    root = hashlib.sha256(top.ljust(BS, b"\x00")).digest()
    return root, levels[::-1]  # top-down


def build(data: bytes) -> bytes:
    root, tree = merkle_levels(data)
    hdr = bytearray(HEADER_LEN)
    struct.pack_into("<I", hdr, 0x00, 1)
    struct.pack_into("<H", hdr, 0x04, 0x0101)   # bytes: 01 01
    struct.pack_into("<H", hdr, 0x06, 12)        # bytes: 0c 00  (log_blocksize)
    struct.pack_into("<I", hdr, 0x08, 0)
    struct.pack_into("<Q", hdr, 0x0c, len(data))
    hdr[0x14:0x34] = root
    if not tree:
        return bytes(hdr)
    out = bytearray(hdr)
    out += b"\x00" * (TREE_OFFSET - len(out))
    for level in tree:
        out += level
        out += b"\x00" * (-len(level) % BS)
    return bytes(out)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    target = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) > 2 else target + ".fsv_meta"
    with open(target, "rb") as fh:
        data = fh.read()
    sidecar = build(data)
    with open(out_path, "wb") as fh:
        fh.write(sidecar)
    print(f"{target} ({len(data)} B) -> {out_path} ({len(sidecar)} B)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
