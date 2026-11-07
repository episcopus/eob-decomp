"""tools/rebuild/dataskel.py and the matcher's data mode (playbook 80, step D1): what the drafts and the
proof rest on. The proofs themselves (a unit's data matching, each defect rejected) are make match-gate's:
src/E0A72.C, src/D1AAD.ASM and the fixtures tests/match/negative/*-data-*."""
import re
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "rebuild"))
sys.path.insert(0, str(ROOT / "tools" / "match"))
import cmatch  # noqa: E402
import dataskel  # noqa: E402

needs_map = pytest.mark.skipif(not (ROOT / "build/re/eob.json").exists(), reason="needs make overlays")


def test_assembled_by_the_order_of_relocations():
    # d1aad's seven data fixups rise in the table (TASM), e0a72's twelve fall (Turbo C)
    assert dataskel.assembled("d1aad")
    assert not dataskel.assembled("e0a72")


def test_parse_type():
    assert dataskel.parse_type("gX", "extern signed char gX[];") == ("signed char", False, True, False)
    assert dataskel.parse_type("gT", "extern char far *gT[];") == ("char", True, True, False)
    assert dataskel.parse_type("gF", "extern void (far *gF)(int x);") == ("void", False, False, True)
    assert dataskel.Decl("gL", 0, "u", "extern long gL;").elem() == 4


def test_c_decl_drops_comments_and_initialisers():
    assert dataskel.c_decl("extern int gA; /*@ ds:0001: a note */") == "extern int gA;"
    assert dataskel.c_decl("signed char gB[6] = { 1, 2 }; /*@ ds:0002 */") == "signed char gB[6];"


@needs_map
def test_draft_of_e0a72_is_what_src_holds():
    # the pilot of step D1: src/E0A72.C's definitions are the draft, unedited
    src = (ROOT / "src/E0A72.C").read_text()
    block = src[src.index("/*@data */"):src.index("(word alignment) */\n") + len("(word alignment) */\n")]
    assert dataskel.draft("e0a72") == block


@needs_map
def test_draft_names_far_pointers_by_their_targets():
    text = dataskel.draft("e0a72")
    assert "char far *gSaveTables[6] = { g33BB, g33ED, g337A, g339D, g33BB, g33BB }; /*@ ds:3406 */" in text
    # an unnamed target of a table of long far pointers is typed by the table
    assert re.search(r"^long g34D0\[12\] = \{ /\*@ ds:34D0 \*/\n    0L, 2500L,", text, re.M)


def test_data_definitions_need_the_marker():
    defs = "int gA = 1; /*@ ds:3066 */\nextern int gB; /*@ ds:3068 */\n"
    cmatch.parse_annotations(defs)
    assert cmatch.DEFDATA == {} and not cmatch.OWN
    cmatch.parse_annotations("/*@data */\n" + defs)
    assert cmatch.DEFDATA == {"gA": 0x3066} and cmatch.OWN
    with pytest.raises(SystemExit):
        cmatch.parse_annotations("/*@data */\nstatic int gS = 1; /*@ ds:3066 */\n")
    cmatch.parse_annotations("")                      # (the module's state, for the other tests)


def test_stub_leaves_out_the_units_own_data():
    cmatch.parse_annotations("/*@data */\nint gA = 1; /*@ ds:3066 */\nextern int gB; /*@ ds:3068 */\n")
    stub = cmatch.stub_source({}, {"gA": 0x3066, "gB": 0x3068}, set(), False)
    assert "_gB db 256" in stub and "_gA" not in stub
    cmatch.parse_annotations("")
