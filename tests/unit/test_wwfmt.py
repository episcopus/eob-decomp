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
