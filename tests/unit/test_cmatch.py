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
/*@ 0CF7:0003 stub */ void Sib(void) {}
/*@ 0CF7:0007 */ void ListInit(struct Node far *head)
{
}
"""


def test_annotations():
    funcs, data, defined, stubs = cmatch.parse_annotations(SRC)
    assert data == {"gFlag": 0xF3B9, "gTable": 0x5102, "gPrint": 0x80D2}
    assert funcs == {"ListAdd": (0x1658, 0x1E), "ListInit": (0x0CF7, 7), "Sib": (0x0CF7, 3)}
    assert defined == {"ListInit", "Sib"} and stubs == {"Sib"}


def test_stub_defines_every_extern():
    funcs, data, defined, _ = cmatch.parse_annotations(SRC)
    stub = cmatch.stub_source(funcs, data, defined, pad=True)
    assert "_gFlag db 256 dup (0)" in stub and "_ListAdd PROC FAR" in stub and "padd db 273" in stub
    assert "ListInit" not in stub and "Sib" not in stub and "_main PROC FAR" in stub


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


def test_template_far_pointers_by_name():
    """An initialiser template (copied by F_SCOPY@) is compared byte by byte; a far pointer in it must
    name the same function, an overlay function as its stub entry."""
    import struct as st
    sys.path.insert(0, os.path.join(ROOT, "tools", "match"))
    import cmatch

    class B:
        def __init__(self, dgroup, image, relocs, by_linear):
            self.dgroup, self.image, self.relocs, self.by_linear = dgroup, image, relocs, by_linear

        def word(self, p):
            return st.unpack_from("<H", self.image, p)[0]

    img = bytearray(0x2000)
    st.pack_into("<HH", img, 0x1000, 0x0624, 0x0050)          # ours: far pointer to our function at 0050:0624
    img[0x1004] = 7                                           # a plain byte
    a = B(0x100, img, {0x1002}, {0x0050 * 16 + 0x0624: "_Icons"})
    b = B(0x100, img, {0x1002}, {})

    class O:
        dgroup = 0x100
        load_module = bytearray(0x2000)
        lm_relocs = {0x1802}
        stub_entry = {(3, 0x624): (0x210C, 0x25)}
    st.pack_into("<HH", O.load_module, 0x1800, 0x0025, 0x210C)
    O.load_module[0x1804] = 7
    funcs, func_of = {"Icons": ("ovl3", 0x624)}, {"_Icons": "Icons"}
    assert cmatch.compare_template(a, b, O, 0, 0, 0x800, 5, funcs, func_of) is None
    O.stub_entry = {(3, 0x624): (0x210C, 0x2A)}                # another entry: wrong
    assert "far pointer to Icons" in cmatch.compare_template(a, b, O, 0, 0, 0x800, 5, funcs, func_of)
    O.stub_entry = {(3, 0x624): (0x210C, 0x25)}
    O.load_module[0x1804] = 8                                  # a plain byte differs
    assert "byte 07, original 08" in cmatch.compare_template(a, b, O, 0, 0, 0x800, 5, funcs, func_of)


def test_twin_proof_rejects_what_relinking_cannot_explain():
    """A twin is the same code with addresses relabelled one to one: another opcode, one address
    mapping two ways, or two addresses mapping to one are rejected."""
    sys.path.insert(0, os.path.join(ROOT, "tools", "match"))
    import twin

    class P:
        def __init__(self, image):
            self.image, self.relocs = bytearray(image), set()
    # mov ax,[0010]; mov [0010],ax; mov bx,[0012]; ret
    code = bytes.fromhex("a11000" "a31000" "8b1e1200" "c3")
    units = {"e1": {"start": "0001:0000", "bytes": len(code)}, "i1": {"start": "0001:0000", "bytes": len(code)}}
    funcs = [{"id": "e1_0000", "unit": "e1", "addr": "0001:0000", "size": len(code)}]
    E = P(b"\0" * 16 + code)
    moved = bytes.fromhex("a11400" "a31400" "8b1e1600" "c3")        # the data block 4 bytes later
    assert twin.check("e1", "i1", E, P(b"\0" * 16 + moved), units, funcs)[0] is None
    two_ways = bytes.fromhex("a11400" "a31800" "8b1e1600" "c3")     # [0010] once 14, once 18
    assert "two values" in twin.check("e1", "i1", E, P(b"\0" * 16 + two_ways), units, funcs)[0]
    onto = bytes.fromhex("a11400" "a31400" "8b1e1400" "c3")         # [0010] and [0012] both 14
    assert "map to" in twin.check("e1", "i1", E, P(b"\0" * 16 + onto), units, funcs)[0]
    opcode = bytes.fromhex("a11400" "a31400" "8b0e1600" "c3")       # mov cx instead of mov bx
    assert "not an address" in twin.check("e1", "i1", E, P(b"\0" * 16 + opcode), units, funcs)[0]


def test_whole_unit_match_adds_the_functions_the_map_missed():
    """e17a6, 2026-11-02: the map listed one entry where the source has three functions; a whole-unit
    match proves their bounds, so they get entries and the covering entry ends where the next starts."""
    sys.path.insert(0, os.path.join(ROOT, "tools", "match"))
    import cmatch
    blocks = ["# head\n\n",
              'id = "u1_0000"\nunit = "u1"\naddr = "17A6:0000"\nsize = 300\nlevel = "L2"\n\n',
              'id = "x_0000"\nunit = "x"\naddr = "1800:0000"\nsize = 4\n\n']
    names = {(0x17A6, 0): "_A", (0x17A6, 100): "_B", (0x17A6, 250): "_C"}
    sizes = {(0x17A6, 0): 100, (0x17A6, 100): 150, (0x17A6, 250): 50}
    assert cmatch.split_entries(blocks, "u1", names, sizes, "tasm /mx") == 2
    text = "[[function]]\n".join(blocks)
    assert 'addr = "17A6:0000"\nsize = 100' in text
    assert 'addr = "17A6:0064"\nsize = 150\nname = "_B"' in text and 'addr = "17A6:00FA"\nsize = 50' in text
    assert 'addr = "1800:0000"\nsize = 4' in text                 # another unit is left alone


def test_small_model_stubs_are_near():
    """START1.EXE is small model: its externs are near procedures, the large model's far."""
    sys.path.insert(0, os.path.join(ROOT, "tools", "match"))
    import cmatch
    funcs = {"PutChar": (0, 0x5F6), "Mine": (0, 0x474)}
    small = cmatch.stub_source(funcs, {}, {"Mine"}, False, small=True)
    assert ".MODEL SMALL" in small and "_PutChar PROC NEAR" in small and "_Mine" not in small.replace("_main", "")
    large = cmatch.stub_source(funcs, {}, {"Mine"}, False)
    assert ".MODEL LARGE" in large and "_PutChar PROC FAR" in large
    assert cmatch.DGROUP["START1.EXE"] == 0x01AA and cmatch.RTL["START1.EXE"] == "rtl_start1.json"

