#!/usr/bin/env python3
"""Append 0xFF blocks covering [addr, addr+size) to a UF2 so flashing it also erases that flash range.

Usage: uf2_add_erased.py <in.uf2> <out.uf2> <addr> <size>
"""
import struct
import sys

MAGIC0, MAGIC1, MAGIC_END = 0x0A324655, 0x9E5D5157, 0x0AB16F30


def main():
    src, dst, addr, size = sys.argv[1], sys.argv[2], int(sys.argv[3], 0), int(sys.argv[4], 0)
    raw = open(src, "rb").read()
    blocks = [bytearray(raw[i:i + 512]) for i in range(0, len(raw), 512)]
    flags, family = struct.unpack_from("<I", blocks[0], 8)[0], struct.unpack_from("<I", blocks[0], 28)[0]
    for a in range(addr, addr + size, 256):
        b = bytearray(512)
        struct.pack_into("<IIIIIIII", b, 0, MAGIC0, MAGIC1, flags, a, 256, 0, 0, family)
        b[32:32 + 256] = b"\xff" * 256
        struct.pack_into("<I", b, 508, MAGIC_END)
        blocks.append(b)
    for n, b in enumerate(blocks):
        struct.pack_into("<II", b, 20, n, len(blocks))
    open(dst, "wb").write(b"".join(blocks))
    print("%s: %d blocks (+%d erased @ 0x%X)" % (dst, len(blocks), size // 256, addr))


if __name__ == "__main__":
    main()
