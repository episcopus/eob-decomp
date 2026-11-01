#!/usr/bin/env python3
"""Shared MZ helpers for the Eye of the Beholder RE tools."""
from __future__ import annotations

import struct
from pathlib import Path


def read_mz(data: bytes) -> dict:
    """Parse an MZ header. `image` is the load module, `tail` what follows it in the file."""
    if data[:2] not in (b"MZ", b"ZM"):
        raise ValueError("not an MZ executable")
    last, pages, nreloc, hdrpara = struct.unpack_from("<HHHH", data, 2)
    size = pages * 512 if last in (0, 512) else (pages - 1) * 512 + last
    min_alloc, max_alloc, ss, sp, checksum, ip, cs, reloc_off, ovno = struct.unpack_from("<9H", data, 10)
    relocs = [struct.unpack_from("<HH", data, reloc_off + 4 * i) for i in range(nreloc)]
    return {
        "nreloc": nreloc, "hdrpara": hdrpara, "header_bytes": hdrpara * 16,
        "min_alloc": min_alloc, "max_alloc": max_alloc, "ss": ss, "sp": sp,
        "checksum": checksum, "ip": ip, "cs": cs, "reloc_off": reloc_off, "ovno": ovno,
        "size": size, "image": data[hdrpara * 16:size], "tail": data[size:],
        "relocs": relocs,  # (offset, segment) pairs, relative to the load image
        # TLINK writes FB, a version byte and "jr" at 1Eh: 30h is TLINK 3.0
        "tlink": data[0x1F] if data[0x1E] == 0xFB and data[0x20:0x22] == b"jr" else None,
    }


def read_mz_file(path: str | Path) -> dict:
    return read_mz(Path(path).read_bytes())


def reloc_targets(mz: dict) -> list[int]:
    """The segment value stored at each relocation (before load-time adjustment)."""
    img = mz["image"]
    return [struct.unpack_from("<H", img, s * 16 + o)[0] for o, s in mz["relocs"]]


def fmt_addr(para: int, offset: int) -> str:
    return "%04X:%04X" % (para & 0xFFFF, offset & 0xFFFF)
