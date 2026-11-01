"""wwfmt.py: LCW commands on hand-made streams, the header, and every pinned file."""
import os, struct, sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "re"))
import wwfmt  # noqa: E402


def test_lcw_commands():
    stream = (b"\x83abc"                 # 3 literal bytes
              + b"\x00\x03"              # copy 3 from 3 back: abc
              + b"\xC0\x01\x00"          # copy 3 from absolute 1: bca
              + b"\xFE\x04\x00z"         # fill 4 with z
              + b"\xFF\x05\x00\x00\x00"  # copy 5 from absolute 0: abcab
              + b"\x80")
    assert wwfmt.lcw(stream, 100) == b"abcabcbcazzzzabcab"


def test_header_and_unpack():
    body = b"\x84data\x80"
    d = struct.pack("<HBBIH", 8 + len(body), 4, 0, 4, 0) + body
    assert wwfmt.header(d)["format"] == 4 and wwfmt.unpack(d) == (b"data", b"")
    assert wwfmt.header(b"\x00" * 12) is None


def test_every_pinned_file():
    if not os.path.isdir(os.path.join(ROOT, "eob")):
        pytest.skip("eob/ not present")
    assert wwfmt.verify(os.path.join(ROOT, "eob")) == 0


def codes12(codes):
    """Pack 12-bit codes big-endian, two codes in three bytes, as 1D9A:0299 reads them."""
    out = bytearray()
    for k in range(0, len(codes), 2):
        a, b = codes[k], codes[k + 1] if k + 1 < len(codes) else 0
        out += bytes([a >> 4, ((a & 0xF) << 4) | (b >> 8), b & 0xFF])
    return bytes(out)


def test_lzw12_codes():
    # a, b (entry 100h = "ab"), 100h ("ab"; 101h = "ba"), 102h: the entry being made, the
    # previous string plus its first byte ("aba"), then a literal
    assert wwfmt.lzw12(codes12([0x61, 0x62, 0x100, 0x102, 0x63]), 8) == b"abababac"


def game_files(fmt):
    import pak
    for f in sorted(os.listdir(os.path.join(ROOT, "eob"))):
        if f.upper().endswith(".PAK"):
            for e in pak.read_pak(open(os.path.join(ROOT, "eob", f), "rb").read())["entries"]:
                h = wwfmt.header(e["data"])
                if h and h["format"] == fmt:
                    yield e["name"], e["data"], h


@pytest.mark.parametrize("fmt", [1, 4])
def test_the_games_own_routine_agrees(fmt):
    """Unpack1 (1D9A:018B) and Unpack4 (1D9A:0013) of EOB.EXE, run in the oracle on every pinned file
    of their format, write exactly what the decoder returns (EOBTITLE.CMP included)."""
    if not os.path.isdir(os.path.join(ROOT, "eob")):
        pytest.skip("eob/ not present")
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    from oracle.run import build_machine
    from oracle.call import call_far
    SRC, DST, WORK = 0x7000, 0x8000, 0x9000       # above the program, its data and its stack
    n = 0
    for name, d, h in game_files(fmt):
        m = build_machine()
        if fmt == 1:
            m.write(SRC, 0, d[2:])                 # LoadPacked reads the first word, then the rest
            call_far(m, 0x1D9A, 0x018B, [SRC, DST, WORK])
            want = wwfmt.unpack(d)[0]
        else:
            m.write(SRC, 0, d[10 + h["palette"]:])
            call_far(m, 0x1D9A, 0x0013, [0, SRC, 0, DST, h["unpacked"]])
            want = wwfmt.lcw(d[10 + h["palette"]:], h["unpacked"])[:h["unpacked"]]
        assert bytes(m.read(DST, 0, h["unpacked"])) == want, name
        n += 1
    assert n == {1: 5, 4: 131}[fmt]
