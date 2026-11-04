"""names.py (struct offsets) and field.py (one field name across files), Phase 6, playbook 65."""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "match"))
import field  # noqa: E402
import names  # noqa: E402

SRC = """
union Mask { unsigned char b[5]; int w; };
struct CharRec {
    char pad00;
    unsigned char m01;          /* +01 */
    char name[0x1A - 0x02];
    char *text;                 /* +1A: a far pointer under -ml */
    void (far *fn)(int x);      /* +1E */
    union Mask mask;            /* +22 */
    int m27;
};
struct Other {
    int m01;
};
extern struct CharRec far gChars[]; /*@ ds:EC1A */

void A(int who)
{
    struct CharRec far *rec;
    rec = &gChars[who];
    rec->m01 = 0;               /* rec->m01 is cleared */
    gChars[who + 1].m01 |= 2;
}

void B(struct Other *rec)
{
    rec->m01 = 1;
}
"""


def test_struct_offsets():
    fields = dict(names.struct_fields(SRC))
    assert fields["CharRec"] == [(0, "pad00"), (1, "m01"), (2, "name"), (0x1A, "text"), (0x1E, "fn"),
                                 (0x22, "mask"), (0x27, "m27")]
    assert fields["Mask"] == [(0, "b"), (0, "w")]


def test_field_by_type():
    """Another struct also has m01: only the CharRec accesses (by the nearest declaration of their
    base) and the comment that writes one are renamed."""
    text, what = field.rename_in(SRC, "CharRec", 1, "status")
    assert what == "m01 -> status, 2 uses (by type)"
    assert "unsigned char status;" in text and "    int m01;" in text
    assert "rec->status = 0;" in text and "/* rec->status is cleared */" in text
    assert "gChars[who + 1].status |= 2;" in text
    assert "rec->m01 = 1;" in text


def test_field_refusals():
    assert field.rename_in(SRC, "CharRec", 0, "first")[1].startswith("no field")       # padding
    assert field.rename_in(SRC, "CharRec", 1, "m27")[1].startswith("REFUSED")          # NEW exists
    unknown = SRC.replace("gChars[who + 1].m01", "Lookup(who)->m01")
    assert field.rename_in(unknown, "CharRec", 1, "status") == (
        None, "REFUSED: cannot tell the struct of the access at line 22 (an expression)")


def test_address_suffix_placeholders():
    assert names.is_placeholder("gDCAA", "ds:DCAA") and names.is_placeholder("g159", "ds:0159")
    assert names.is_placeholder("gIconX5721", "ds:5721") and names.is_placeholder("f19EC_0016", "19EC:0016")
    assert not names.is_placeholder("gPartyCell", "ds:FBCA") and not names.is_placeholder("gF1Key", "ds:00F1")
