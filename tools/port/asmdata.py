"""asmdata.py: the initialised data of the assembly units of src/, for the port's image (step D3,
playbook 80). tc2c.py writes it into build/port/gen/units.c, beside the C units' definitions.

An assembly module's data is what its source assembles to, in order: from the line `; @data` (step
D2) to `.CODE` for its `_DATA` (placed at the module's span in re/datamap.toml), and E1948's driver
segment (`SND SEGMENT` to `SND ENDS`, at ds:88A0, whitepaper and src/E1948.ASM). It reads `db`, `dw`
and `dd` with numbers (TASM's `0FFh` and decimal), `N dup (v)`, quoted strings, and far pointers by
name (`dd _gCurFont`: an EXTRN with its `; @ ds:XXXX` or `; @ SSSS:OOOO`, or a label of the module).
Each PUBLIC's `; @ ds:XXXX` must be where the bytes put its label.
"""
from __future__ import annotations

import re
from pathlib import Path

SIZE = {"db": 1, "dw": 2, "dd": 4}


class AsmDataError(Exception):
    pass


def number(tok: str) -> int:
    t = tok.strip()
    neg = t.startswith("-")
    t = t.lstrip("-").strip()
    if re.fullmatch(r"[0-9][0-9A-Fa-f]*[hH]", t):
        v = int(t[:-1], 16)
    elif re.fullmatch(r"\d+", t):
        v = int(t)
    else:
        raise ValueError(tok)
    return -v if neg else v


def strip_comment(line: str) -> str:
    out, q = [], None
    for c in line:
        if q:
            out.append(c)
            if c == q:
                q = None
        elif c in "'\"":
            q = c
            out.append(c)
        elif c == ";":
            break
        else:
            out.append(c)
    return "".join(out).strip()


def operands(text: str) -> list[str]:
    out, depth, cur, q = [], 0, [], None
    for c in text:
        if q:
            cur.append(c)
            if c == q:
                q = None
            continue
        if c in "'\"":
            q = c
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
            continue
        cur.append(c)
    out.append("".join(cur).strip())
    return [o for o in out if o]


def module_data(path: Path, start: int, segment: str | None = None):
    """(bytes, far pointers as (offset, ('ds', off) or ('code', seg, off)), labels {name: offset})."""
    lines = path.read_text(errors="replace").split("\n")
    extern = {}
    for l in lines:
        m = re.match(r"\s*EXTRN\s+(\w+)\s*:\s*(\w+)\s*;\s*@\s*(ds:[0-9A-Fa-f]{4}|[0-9A-Fa-f]{4}:[0-9A-Fa-f]{4})", l, re.I)
        if m:
            a = m.group(3)
            extern[m.group(1)] = ("ds", int(a[3:], 16)) if a.lower().startswith("ds:") else \
                ("code", int(a[:4], 16), int(a[5:], 16))
    publics = {}
    for l in lines:
        m = re.match(r"\s*PUBLIC\s+(\w+)\s*;\s*@\s*ds:([0-9A-Fa-f]{4})", l, re.I)
        if m:
            publics[m.group(1)] = int(m.group(2), 16)
    if segment:
        i = next(k for k, l in enumerate(lines) if re.match(r"\s*%s\s+SEGMENT\b" % segment, l, re.I))
        body = []
        for l in lines[i + 1:]:
            if re.match(r"\s*%s\s+ENDS\b" % segment, l, re.I):
                break
            body.append(l)
    else:
        i = next(k for k, l in enumerate(lines) if re.match(r"\s*;\s*@data\b", l))
        body = []
        for l in lines[i + 1:]:
            if re.match(r"\s*\.CODE\b", l, re.I) or re.match(r"\s*END\b", l, re.I):
                break
            body.append(l)
    img, ptrs, labels, fixups = bytearray(), [], {}, []
    for raw in body:
        s = strip_comment(raw)
        if not s or re.match(r"(EXTRN|PUBLIC|\.DATA|ASSUME)\b", s, re.I):
            continue
        m = re.match(r"(\w+)\s+label\s+\w+$", s, re.I)
        if m:
            labels[m.group(1)] = len(img)
            continue
        m = re.match(r"(?:(\w+)\s+)?(db|dw|dd)\s+(.*)$", s, re.I)
        if not m:
            raise AsmDataError("%s: cannot read %r" % (path.name, raw.strip()))
        if m.group(1):
            labels[m.group(1)] = len(img)
        size = SIZE[m.group(2).lower()]
        for op in operands(m.group(3)):
            d = re.fullmatch(r"(\d+)\s+dup\s*\((.*)\)", op, re.I)
            reps, op = (int(d.group(1)), d.group(2).strip()) if d else (1, op)
            for _ in range(reps):
                if op.startswith(("'", '"')) and size == 1:
                    img += op[1:-1].encode("latin-1")
                elif op == "?":
                    img += b"\0" * size
                else:
                    try:
                        v = number(op)
                        img += (v & ((1 << (8 * size)) - 1)).to_bytes(size, "little")
                    except ValueError:
                        sm = re.fullmatch(r"(\w+)\s*(?:\+\s*(\w+))?", op)
                        if not sm or size != 4:
                            raise AsmDataError("%s: cannot read the operand %r" % (path.name, op))
                        fixups.append((len(img), sm.group(1), number(sm.group(2)) if sm.group(2) else 0))
                        img += b"\0" * 4
    for at, name, add in fixups:
        if name in extern:
            t = extern[name]
        elif name in labels:
            t = ("ds", start + labels[name])
        else:
            raise AsmDataError("%s: dd %s names nothing with an address" % (path.name, name))
        if t[0] == "ds":
            ptrs.append((at, ("ds", (t[1] + add) & 0xFFFF)))
        else:
            ptrs.append((at, ("code", t[1], (t[2] + add) & 0xFFFF)))
    for name, at in publics.items():
        if name in labels and start + labels[name] != at:
            raise AsmDataError("%s: %s is annotated ds:%04X, but its bytes put it at ds:%04X"
                               % (path.name, name, at, start + labels[name]))
    return bytes(img), ptrs, labels


def c_source(units: list[tuple[str, Path, int, int, str | None]]) -> list[str]:
    """The C of tc2c_data_asm: (name, file, start, end, segment) of each assembly module's data."""
    blobs, body = [], []
    for name, path, start, end, seg in units:
        img, ptrs, _ = module_data(path, start, seg)
        want = end - start
        if not (len(img) == want or (len(img) == want - 1)):
            raise AsmDataError("%s: its data is %d bytes, its span ds:%04X-%04X %d" % (path.name, len(img), start, end, want))
        sym = "tc2c_asmdata_%s" % name
        rows = [", ".join("0x%02X" % x for x in img[k:k + 16]) for k in range(0, len(img), 16)]
        blobs.append("/* %s, ds:%04X */\nstatic const u8 %s[%d] = {\n    %s\n};" % (
            path.name, start, sym, len(img), ",\n    ".join(rows)))
        body.append("    tc_put(TC_MKFP(tc_ds, 0x%04X), %s, %du);" % (start, sym, len(img)))
        for at, t in ptrs:
            e = "TC_MKFP(tc_ds, 0x%04X)" % t[1] if t[0] == "ds" else \
                "TC_MKFP(tc_load_seg + 0x%04X, 0x%04X)" % (t[1], t[2])
            body.append("    tc_st_u32(TC_MKFP(tc_ds, 0x%04X), (u32)%s);" % ((start + at) & 0xFFFF, e))
    return blobs + ["", "/* the assembly units' data (tools/port/asmdata.py) */", "void tc2c_data_asm(void)",
                    "{"] + body + ["}"]
