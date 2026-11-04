"""layout.py: a rebuild's memory read in the original's form (playbook 50), on made-up memory."""
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))
from oracle import layout  # noqa: E402
from oracle.machine import LOAD_SEG  # noqa: E402


class FakeMachine:
    def __init__(self, exe, loads=()):
        self.exe = exe
        self.image_loads = list(loads)


def program(image_len=0x1000):
    """A stub at load-relative paragraph 0080 with three entries; the rebuild has entries 0 and 2
    swapped, and a word of its file that the original has as 1234h."""
    return layout.Program("EOB.EXE", layout.GAME / "EOB.EXE", entry_map={0x80: {0x20: 0x2A, 0x2A: 0x20}},
                          entry_count={0x80: 3}, image_len=image_len,
                          file_values=[(0x10, b"\x99\x99", b"\x34\x12", "test")])


def memory(base=LOAD_SEG):
    mem = bytearray(0xA0000 - layout.MEM_BASE)

    def put(lin, b):
        mem[lin - layout.MEM_BASE:lin - layout.MEM_BASE + len(b)] = b
    stub = (base + 0x80) * 16
    put(stub, b"\xcd\x3f")
    put(stub + 0x20, b"\xcd\x3f\x00\x02\x00" b"\xcd\x3f\x00\x01\x00" b"\xcd\x3f\x00\x00\x00")
    seg = struct.pack("<H", base + 0x80)
    put(0x30000, b"\x20\x00" + seg)            # a far pointer to the rebuild's entry 0
    put(0x30010, b"\x25\x00" + seg)            # to entry 1, which did not move
    put(0x30020, b"\x9a\x2a\x00" + seg)        # a far call to entry 2
    put(base * 16 + 0x10, b"\x99\x99")
    return mem, put


def lin(mem, a, n):
    return bytes(mem[a - layout.MEM_BASE:a - layout.MEM_BASE + n])


def test_entries_and_far_pointers_take_the_original_order():
    lay = layout.Layout(name="rebuilt", programs={"EOB.EXE": program()})
    mem, _ = memory()
    lay.canonical(FakeMachine("x/EOB.EXE"), mem)
    stub = (LOAD_SEG + 0x80) * 16
    assert lin(mem, stub + 0x20, 15) == b"\xcd\x3f\x00\x00\x00" b"\xcd\x3f\x00\x01\x00" b"\xcd\x3f\x00\x02\x00"
    assert lin(mem, 0x30000, 2) == b"\x2a\x00"
    assert lin(mem, 0x30010, 2) == b"\x25\x00"
    assert lin(mem, 0x30021, 2) == b"\x20\x00"
    assert lin(mem, LOAD_SEG * 16 + 0x10, 2) == b"\x34\x12"


def test_file_values_only_where_memory_still_holds_ours():
    lay = layout.Layout(name="rebuilt", programs={"EOB.EXE": program()})
    mem, put = memory()
    put(LOAD_SEG * 16 + 0x10, b"\x55\x55")        # the program changed it: left alone
    lay.canonical(FakeMachine("x/EOB.EXE"), mem)
    assert lin(mem, LOAD_SEG * 16 + 0x10, 2) == b"\x55\x55"


def test_an_image_written_over_is_left_alone():
    """INTRO.EXE loads EOB.EXE at its own segment: INTRO.EXE's rewrites no longer apply, EOB.EXE's do."""
    intro = program()
    intro.name = "INTRO.EXE"
    intro.entry_map = {0x80: {0x20: 0x25, 0x25: 0x20}}
    lay = layout.Layout(name="rebuilt", programs={"EOB.EXE": program(), "INTRO.EXE": intro})
    mem, _ = memory()
    lay.canonical(FakeMachine("x/INTRO.EXE", [("EOB.EXE", LOAD_SEG, LOAD_SEG)]), mem)
    assert lin(mem, 0x30000, 2) == b"\x2a\x00"     # EOB.EXE's map, not INTRO.EXE's
    assert lin(mem, 0x30010, 2) == b"\x25\x00"


def test_the_original_is_not_rewritten():
    mem, _ = memory()
    before = bytes(mem)
    layout.original().canonical(FakeMachine("x/EOB.EXE"), mem)
    assert bytes(mem) == before
