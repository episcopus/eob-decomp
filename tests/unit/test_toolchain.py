"""The toolchain helpers: TLIB listing parser, FAT12 reader, and the installed pins when present."""
import os, struct, sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "re"))
import fat12  # noqa: E402
import rtl_compare  # noqa: E402
import tc_toolchain  # noqa: E402

LST = """Publics by module

ABS     \tsize = 14
\t_abs

BCD2    \tsize = 201
\toperator <<(ostream far&,bcd far&)  operator >>(istream far&,bcd far&)

H_LDIV  \tsize = 155
\tLDIV@                             LMOD@
\tN_LDIV@                           N_LMOD@
"""


def test_publics_by_module_skips_cpp_names():
    mods = rtl_compare.publics_by_module(LST)
    assert mods == {"ABS": ["_abs"], "BCD2": [], "H_LDIV": ["LDIV@", "LMOD@", "N_LDIV@", "N_LMOD@"]}


def floppy(files):
    """A minimal 360 KB FAT12 image with the given root files (one cluster each)."""
    img = bytearray(368640)
    struct.pack_into("<HBHBHHBH", img, 11, 512, 2, 1, 2, 112, 720, 0xFD, 2)
    fat, root, data = 512, 512 * 5, 512 * 5 + 112 * 32
    for i, (name, body) in enumerate(files):
        c = 2 + i
        v = struct.unpack_from("<H", img, fat + c * 3 // 2)[0]
        v = (v & 0x000F) | (0xFFF << 4) if c & 1 else (v & 0xF000) | 0xFFF
        struct.pack_into("<H", img, fat + c * 3 // 2, v)
        n, _, e = name.partition(".")
        ent = n.ljust(8).encode() + e.ljust(3).encode() + b"\x20" + bytes(10)
        ent += struct.pack("<HHHI", 0, (10 << 9) | (5 << 5) | 4, c, len(body))
        img[root + 32 * i:root + 32 * i + 32] = ent
        img[data + (c - 2) * 1024:data + (c - 2) * 1024 + len(body)] = body
    return bytes(img)


def test_fat12_reads_files():
    fs = fat12.Fat12(floppy([("TCC.ZIP", b"PK\3\4abc"), ("README", b"hello")]))
    assert [(f["path"], f["data"], f["date"]) for f in fs.files()] == [
        ("TCC.ZIP", b"PK\3\4abc", "1990-05-04"), ("README", b"hello", "1990-05-04")]


def test_installed_toolchain_matches_pins():
    if not (tc_toolchain.TOOLCHAIN / tc_toolchain.manifest()["pinned"]).exists():
        pytest.skip("toolchain not installed (make tc-install)")
    assert tc_toolchain.check() == 0
