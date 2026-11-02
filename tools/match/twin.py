#!/usr/bin/env python3
"""twin.py: prove a unit of INTRO.EXE by its twin in EOB.EXE, the same object module linked elsewhere.

Most of INTRO.EXE is Westwood's library code that EOB.EXE links too (whitepaper 2.6): the same source
file, compiled once, placed at other addresses. A unit of INTRO.EXE whose entry in re/units.toml says
`twin = "<EOB unit>"` is proven when, function by function, its bytes are its twin's with the
addresses relabelled one to one:

  - every instruction has the twin's opcode bytes and length (decoded from the twin's functions);
  - a byte that differs lies in an address field: a displacement or immediate, a relative branch, a
    far pointer (offset and segment taken together), or a word both programs relocate;
  - one EOB value maps to one INTRO value of the same kind, and no two EOB values to the same one;
  - an entry that is not code (registry notes `not a function`), the bytes between functions and a
    table inside a function (where its instructions stop decoding) are equal, except words both
    programs relocate.

Then each function of the INTRO unit takes its twin's name and level (L2 only where the twin is L2,
proven from src/), with match_flags = "twin <EOB unit>". `make match-gate` proves every twin again.

  twin.py [--unit ID] [--write]     check (and record) the INTRO units that have a twin
  twin.py --find [--write]          the INTRO units without a twin that pass against an EOB unit of the
                                    same size; --write sets their `twin` in re/units.toml
Exit code 0 when every unit checked is proven.
"""
from __future__ import annotations

import argparse
import re
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "re"))
from capstone import CS_ARCH_X86, CS_MODE_16, Cs  # noqa: E402
from mzutil import read_mz  # noqa: E402

MD = Cs(CS_ARCH_X86, CS_MODE_16)
MD.detail = True
REL = ("call", "loop", "loope", "loopne", "jcxz")


class Program:
    def __init__(self, name: str):
        mz = read_mz((ROOT / "eob" / name).read_bytes())
        self.image = mz["image"]
        self.relocs = {s * 16 + o for o, s in mz["relocs"]}


def linear(addr: str) -> int:
    s, o = (int(x, 16) for x in addr.split(":"))
    return s * 16 + o


def registry() -> tuple[dict, list]:
    units = {u["id"]: u for u in tomllib.loads((ROOT / "re/units.toml").read_text())["unit"]}
    return units, tomllib.loads((ROOT / "re/symbols.toml").read_text())["function"]


def entries(funcs: list, uid: str, start: int) -> list[dict]:
    return sorted(({**f, "at": linear(f["addr"]) - start} for f in funcs if f["unit"] == uid), key=lambda f: f["at"])


def is_code(f: dict) -> bool:
    return not (f.get("notes") or "").startswith("not a function")


def fields(ins) -> list[tuple[int, int, str]]:
    """(offset, size, kind) of the address-like fields of an instruction."""
    if ins.bytes[0] in (0x9A, 0xEA):                 # call/jmp far ptr16:16: offset and segment together
        return [(1, 4, "far")]
    out = []
    if ins.disp_size:
        out.append((ins.disp_offset, ins.disp_size, "cs" if "cs:" in ins.op_str else "disp"))
    if 0 < ins.imm_size <= 2:
        rel = ins.mnemonic.startswith("j") or ins.mnemonic in REL
        out.append((ins.imm_offset, ins.imm_size, "rel" if rel else "imm"))
    return out


def check(e_id: str, i_id: str, E: Program | None = None, I: Program | None = None,
          units: dict | None = None, funcs: list | None = None) -> tuple[str | None, dict]:
    """(None or the first reason the INTRO unit is not its twin, the EOB -> INTRO value map)."""
    if units is None:
        units, funcs = registry()
    E, I = E or Program("EOB.EXE"), I or Program("INTRO.EXE")
    eu, iu = units[e_id], units[i_id]
    a, b, n = linear(eu["start"]), linear(iu["start"]), eu["bytes"]
    if iu["bytes"] != n:
        return "%s is %d bytes, %s %d" % (i_id, iu["bytes"], e_id, n), {}
    eb, ib = E.image[a:a + n], I.image[b:b + n]
    relocated = {k for k in range(n - 1) if a + k in E.relocs}
    if relocated != {k for k in range(n - 1) if b + k in I.relocs}:
        return "the relocated words differ", {}
    seen: dict = {}

    def relabel(kind, x, y, where):
        if seen.setdefault((kind, x), y) != y:
            return "+%04X: %s %s maps to two values" % (where, kind, x)
        return None

    explained, decoded = set(), set()
    for k in relocated:                               # a segment word either program relocates
        x, y = int.from_bytes(eb[k:k + 2], "little"), int.from_bytes(ib[k:k + 2], "little")
        if x != y and (bad := relabel("seg", x, y, k)):
            return bad, {}
        explained.update((k, k + 1))
    for f in entries(funcs, e_id, a):
        if not is_code(f):
            continue
        st, size = f["at"], f.get("size", 0)
        end = st
        for ins in MD.disasm(eb[st:st + size], st):
            p, q = ins.address, ins.address + ins.size
            end = q
            if eb[p:q] == ib[p:q]:
                continue
            for off, sz, kind in fields(ins):
                r = p + off
                if r + 1 in relocated or r in relocated:
                    explained.update(range(r, r + sz))
                    if kind != "far":
                        continue
                x, y = bytes(eb[r:r + sz]), bytes(ib[r:r + sz])
                if x != y and (bad := relabel(kind, x.hex(), y.hex(), r)):
                    return bad, {}
                explained.update(range(r, r + sz))
            for k in range(p, q):
                if eb[k] != ib[k] and k not in explained:
                    return "+%04X: %s %s, the byte at +%04X is not an address" % (p, ins.mnemonic, ins.op_str, k), {}
        decoded.update(range(st, end))              # the rest (a table in the code) is compared as data
    for k in range(n):
        if eb[k] != ib[k] and k not in explained and k not in decoded:
            return "+%04X: a byte outside the decoded instructions differs" % k, {}
    back: dict = {}
    for (kind, x), y in seen.items():
        if back.setdefault((kind, y), x) != x:
            return "two %s values map to %s" % (kind, y), {}
    return None, seen


def record(e_id: str, i_id: str, units: dict, funcs: list) -> int:
    """Give each function of the INTRO unit its twin's name and level. An entry at the twin's offset
    with the twin's size is updated in place; where the INTRO map split the unit differently, its
    entries are replaced by the twin's split. The number of entries written."""
    a = linear(units[e_id]["start"])
    seg = units[i_id]["start"].split(":")[0]
    base = linear(units[i_id]["start"]) - int(seg, 16) * 16
    path = ROOT / "re/symbols.toml"
    blocks = path.read_text().split("[[function]]\n")
    head, body = blocks[0], blocks[1:]
    mine = {re.search(r'^addr = "([^"]+)"', blk, re.M).group(1): i for i, blk in enumerate(body)
            if re.search(r'^unit = "%s"$' % re.escape(i_id), blk, re.M)}
    sizes = {addr: int(re.search(r"^size = (\d+)", body[i], re.M).group(1)) for addr, i in mine.items()}
    twins = entries(funcs, e_id, a)
    wanted = {"%s:%04X" % (seg, base + f["at"]): f for f in twins}
    if all(addr in sizes and sizes[addr] == f.get("size", 0) for addr, f in wanted.items()) and len(wanted) == len(mine):
        for addr, f in wanted.items():
            blk = re.sub(r"^(name|level|match_flags) = .*\n", "", body[mine[addr]], flags=re.M)
            extra = ['name = "%s"' % f["name"]] if f.get("name") else []
            if f.get("level") in ("L2", "L3"):
                extra += ['level = "%s"' % f["level"], 'match_flags = "twin %s"' % e_id]
            lines = blk.rstrip("\n").split("\n")
            at = next(k for k, l in enumerate(lines) if l.startswith("size = ")) + 1
            body[mine[addr]] = "\n".join(lines[:at] + extra + lines[at:]) + "\n\n"
    else:
        new_blocks = []
        for addr, f in wanted.items():
            lines = ['id = "%s_%s"' % (i_id, addr.split(":")[1].lower()), 'unit = "%s"' % i_id,
                     'addr = "%s"' % addr, "size = %d" % f.get("size", 0)]
            if f.get("name"):
                lines.append('name = "%s"' % f["name"])
            if f.get("level") in ("L2", "L3"):
                lines += ['level = "%s"' % f["level"], 'match_flags = "twin %s"' % e_id]
            lines.append('notes = "%s"' % ("not a function: bytes of the unit outside its functions, proven with its twin %s" % e_id
                                          if not is_code(f) else "split as its twin %s (the INTRO map split this unit differently)" % e_id))
            new_blocks.append("\n".join(lines) + "\n\n")
        first = min(mine.values()) if mine else len(body)
        kept = [blk for i, blk in enumerate(body) if i not in mine.values()]
        at = sum(1 for i in range(first) if i not in mine.values())
        body = kept[:at] + new_blocks + kept[at:]
    path.write_text(head + "".join("[[function]]\n" + blk for blk in body))
    return len(wanted)


def set_twin(i_id: str, e_id: str) -> None:
    path = ROOT / "re/units.toml"
    text = path.read_text()
    m = re.search(r'(^id = "%s"\n(?:[^\[\n][^\n]*\n)*)' % re.escape(i_id), text, re.M)
    block = m.group(1)
    block2 = re.sub(r'^twin = .*\n', "", block, flags=re.M) + 'twin = "%s"\n' % e_id
    path.write_text(text.replace(block, block2, 1))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--unit")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--find", action="store_true")
    a = ap.parse_args(argv)
    units, funcs = registry()
    E, I = Program("EOB.EXE"), Program("INTRO.EXE")
    failed = 0
    if a.find:
        for i_id, iu in units.items():
            if iu["program"] != "INTRO.EXE" or iu.get("twin") or iu["start"].startswith("ovl") \
                    or iu["kind"] in ("startup", "library"):           # linked, never decompiled
                continue
            for e_id, eu in units.items():
                if eu["program"] == "EOB.EXE" and eu["bytes"] == iu["bytes"] and not eu["start"].startswith("ovl"):
                    why, _ = check(e_id, i_id, E, I, units, funcs)
                    if why is None:
                        print("twin: %s is %s" % (i_id, e_id))
                        if a.write:
                            set_twin(i_id, e_id)
                        break
        return 0
    for i_id, iu in sorted(units.items()):
        if not iu.get("twin") or (a.unit and i_id != a.unit):
            continue
        e_id = iu["twin"]
        why, seen = check(e_id, i_id, E, I, units, funcs)
        if why:
            print("DIFFER  %s (twin %s): %s" % (i_id, e_id, why))
            failed += 1
            continue
        l2 = sum(1 for f in funcs if f["unit"] == e_id and f.get("level") in ("L2", "L3"))
        total = sum(1 for f in funcs if f["unit"] == e_id)
        print("ok      %s is %s with %d addresses relabelled (%d of %d entries at L2)" % (i_id, e_id, len(seen), l2, total))
        if a.write:
            record(e_id, i_id, units, funcs)
            units, funcs = registry()
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
