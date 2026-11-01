"""cmatch.py annotation parsing and the segment table extents it relies on."""
import os, sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "match"))
sys.path.insert(0, os.path.join(ROOT, "tools", "re"))
import cmatch  # noqa: E402
import fbov  # noqa: E402

SRC = """
extern char gFlag; /*@ ds:F3B9 */
extern void far *gTable[]; /*@ ds:5102 */
extern void (far *gPrint)(char far *text); /*@ ds:80D2 */
extern struct Node far *ListAdd(struct Node far *list, void far *item); /*@ 1658:001E */
/*@ 0CF7:0007 */ void ListInit(struct Node far *head)
{
}
"""


def test_annotations():
    funcs, data, defined = cmatch.parse_annotations(SRC)
    assert data == {"gFlag": 0xF3B9, "gTable": 0x5102, "gPrint": 0x80D2}
    assert funcs == {"ListAdd": 0x1658 * 16 + 0x1E, "ListInit": 0x0CF7 * 16 + 7}
    assert defined == {"ListInit"}


def test_stub_defines_every_extern():
    stub = cmatch.stub_source(*cmatch.parse_annotations(SRC))
    assert "char gFlag[256];" in stub and "void far ListAdd(void) {}" in stub
    assert "ListInit" not in stub and "int main(void)" in stub


def test_segments_are_byte_aligned():
    p = os.path.join(ROOT, "eob", "EOB.EXE")
    if not os.path.exists(p):
        pytest.skip("eob/ not present")
    m, _ = fbov.manifest(open(p, "rb").read())
    code = {s["para"]: s for s in m["segments"] if s["kind"] == 1 and s["size"]}
    assert code[0x07C8]["start"] == 0x07C81  # main, right after the retf ending 07A9
    assert code[0x0CF7]["start"] == 0x0CF77 and code[0x0CF7]["size"] == 591
    assert sum(s["size"] for s in code.values()) == 127435


def test_classify_c_and_asm():
    import map_units
    c_fn = bytes.fromhex("558bec56" "8b7606" "5e5dcb")          # push bp; mov bp,sp; push si; ...; retf
    asm_fn = bytes.fromhex("558bec50571e" "1f5f585dcb")        # pushes ax, di, ds on entry
    isr = bytes.fromhex("5053" "5b58cf")                       # an interrupt handler
    code = c_fn * 4
    funcs = [{"linear": i * len(c_fn)} for i in range(4)]
    assert map_units.classify(code, 0, len(code), funcs) == "c"
    assert map_units.classify(asm_fn * 2, 0, 2 * len(asm_fn), [{"linear": 0}, {"linear": len(asm_fn)}]) == "asm"
    assert map_units.classify(isr, 0, len(isr), [{"linear": 0}]) == "asm"
