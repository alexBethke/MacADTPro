#!/usr/bin/env python3
"""Set the HFS software-lock bit in a raw disk image's two MDB copies."""

import pathlib
import sys


SECTOR_SIZE = 512
MDB_ATTRIBUTES_OFFSET = 10
SOFTWARE_LOCK = 0x8000


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: lock_hfs_volume.py IMAGE.dsk")

    path = pathlib.Path(sys.argv[1])
    data = bytearray(path.read_bytes())
    if len(data) < 5 * SECTOR_SIZE or len(data) % SECTOR_SIZE:
        raise SystemExit("image is not a whole-sector HFS disk")

    sector_count = len(data) // SECTOR_SIZE
    for sector in (2, sector_count - 2):
        offset = sector * SECTOR_SIZE
        if data[offset:offset + 2] != b"BD":
            raise SystemExit(f"HFS MDB signature missing in sector {sector}")
        attributes_at = offset + MDB_ATTRIBUTES_OFFSET
        attributes = int.from_bytes(data[attributes_at:attributes_at + 2], "big")
        data[attributes_at:attributes_at + 2] = (attributes | SOFTWARE_LOCK).to_bytes(2, "big")

    path.write_bytes(data)


if __name__ == "__main__":
    main()
