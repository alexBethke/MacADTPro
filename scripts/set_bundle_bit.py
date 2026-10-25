#!/usr/bin/env python3
"""Copy a MacBinary file and set its classic Finder has-bundle flag."""

import pathlib
import sys


def crc16(data):
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: set_bundle_bit.py INPUT.bin OUTPUT.bin")
    data = bytearray(pathlib.Path(sys.argv[1]).read_bytes())
    if len(data) < 128 or data[0] != 0:
        raise SystemExit("input is not a MacBinary file")
    data[73] |= 0x20
    checksum = crc16(data[:124])
    data[124:126] = checksum.to_bytes(2, "big")
    pathlib.Path(sys.argv[2]).write_bytes(data)


if __name__ == "__main__":
    main()
