#!/usr/bin/env python3
"""packet.py: the work packet of a unit, everything to start decompiling it (playbook 30).

  make packet UNIT=<id>          writes build/packets/<id>.md and prints its path

The packet has the unit's facts from re/units.toml; every function with its level, name and notes
from re/symbols.toml; who calls each function and what it calls (a call graph of the whole program:
far calls, near calls, and overlay calls through their stub entries); the globals and callees that
src/ already names (reuse those names and annotations); and the disassembly of every function not
at L2, with call targets and known globals named. Read it, then write src/<ID>.C (`make skeleton`
starts one) and prove it with `make match FILE=...`.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "re"))
import common  # noqa: E402
from mzutil import read_mz  # noqa: E402

MD = Cs(CS_ARCH_X86, CS_MODE_16)
DGROUP = 0x212E
ANNOT = re.compile(r"(?:/\*|;)\s*@\s*(ds:[0-9A-Fa-f]{1,4}|ovl\d+:[0-9A-Fa-f]{1,4}|[0-9A-Fa-f]{4}:[0-9A-Fa-f]{4})")


class Program:
    """EOB.EXE's code: resident functions by seg:off, overlay functions by ovlN:off."""

    def __init__(self):
        root = common.main_checkout()
        self.image = read_mz((common.ROOT / "eob/EOB.EXE").read_bytes())["image"]
        man = json.loads((root / "build/re/eob.json").read_text())
        self.segments = man["segments"]
        self.stubs = {o["stub_para"]: o for o in man["overlays"]}
        self.ovl = {o["overlay"]: ((root / "build/re/eob-overlays" / ("ovl%02d.bin" % o["overlay"])).read_bytes(),
                                   set(o["fixups"])) for o in man["overlays"]}
        self.funcs = common.functions()
        self.by_addr = {f["addr"].upper(): f for f in self.funcs}     # keys upper case: OVL2:09CC

    def code(self, f: dict) -> tuple[bytes, int, str]:
        """(bytes, address of the first byte in the listing, segment label)."""
        seg, off = f["addr"].split(":")
        off = int(off, 16)
        if seg.lower().startswith("ovl"):
            data, _ = self.ovl[int(seg[3:])]
            return data[off:off + f["size"]], off, seg.lower()
        lin = int(seg, 16) * 16 + off
        return self.image[lin:lin + f["size"]], off, seg.upper()

    def target(self, f: dict, insn) -> str | None:
        """The address a call reaches, in the registry's spelling."""
        seg = f["addr"].split(":")[0]
        if insn.mnemonic == "call" and insn.op_str.startswith("0x"):
            return "%s:%04X" % (seg.lower() if seg.lower().startswith("ovl") else seg.upper(), int(insn.op_str, 16) & 0xFFFF)
        if insn.mnemonic == "lcall" and insn.bytes[0] == 0x9A:
            off, para = struct.unpack_from("<HH", insn.bytes, 1)
            if seg.lower().startswith("ovl"):
                _, fixups = self.ovl[int(seg[3:])]
                if insn.address + 3 in fixups and para // 8 < len(self.segments):
                    para = self.segments[para // 8]["para"]
            o = self.stubs.get(para)
            if o is not None and off >= 0x20 and (off - 0x20) % 5 == 0:
                k = (off - 0x20) // 5
                if k < len(o["entries"]):
                    return "ovl%d:%04X" % (o["overlay"], o["entries"][k]["code_offset"])
            return "%04X:%04X" % (para, off)
        return None

    def calls(self) -> dict[str, set[str]]:
        out = defaultdict(set)
        for f in self.funcs:
            if not f["unit"].startswith("e") or f["unit"] in ("e_lib", "e_c0"):
                continue
            data, base, _ = self.code(f)
            for i in MD.disasm(data, base):
                t = self.target(f, i)
                if t:
                    out[f["addr"].upper()].add(t.upper())
        return out


def known_names() -> tuple[dict[int, str], dict[str, str]]:
    """Globals (ds offset) and functions (address) that some file of src/ names, with the file."""
    data, funcs = {}, {}
    for p in sorted((common.ROOT / "src").glob("*")):
        for line in p.read_text().splitlines():
            m = ANNOT.search(line)
            if not m:
                continue
            decl = ANNOT.sub("", line)
            names = re.findall(r"([A-Za-z_]\w*)\s*(?:\(|\[|;|=|,)", decl) or re.findall(r"PUBLIC\s+(\w+)", decl)
            names = [n for n in names if n not in ("extern", "far", "void", "int", "char", "unsigned", "struct")]
            if not names:
                continue
            a = m.group(1)
            if a.lower().startswith("ds:"):
                data.setdefault(int(a[3:], 16), "%s (%s)" % (names[0], p.name))
            else:
                funcs.setdefault(a.upper(), "%s (%s)" % (names[0], p.name))
    return data, funcs


def spell(addr: str) -> str:
    return "ovl" + addr[3:] if addr.upper().startswith("OVL") else addr


def label(addr: str, prog: Program, fnames: dict[str, str]) -> str:
    f = prog.by_addr.get(addr.upper())
    addr = spell(addr)
    name = fnames.get(addr.upper()) or (f.get("name") if f else None)
    unit = f["unit"] if f else "?"
    return "%s %s[%s]" % (addr, (name + " ") if name else "", unit)


def write(uid: str) -> Path:
    u = common.unit(uid)
    prog = Program()
    fs = [f for f in prog.funcs if f["unit"] == uid]
    graph = prog.calls()
    callers = defaultdict(set)
    for a, ts in graph.items():
        for t in ts:
            callers[t].add(a)
    dnames, fnames = known_names()
    src = common.source_of(uid)
    lines = ["# Packet: unit %s" % uid, "",
             "| | |", "|---|---|"]
    for k in ("program", "segment", "overlay", "start", "bytes", "functions", "kind", "same_as"):
        if k in u:
            lines.append("| %s | %s |" % (k, u[k]))
    lines += ["| source | %s |" % (src.relative_to(common.ROOT) if src else "none yet: `make skeleton UNIT=%s`" % uid),
              "| complete | %s |" % common.unit_complete(uid, prog.funcs), "",
              "Playbook 30. Names are placeholders until evidence names them; reuse the names src/ already gives.", "",
              "## Functions", "", "| addr | size | level | name | called by | calls |", "|---|---:|---|---|---|---|"]
    for f in fs:
        a = f["addr"].upper()
        lines.append("| %s | %d | %s | %s | %s | %s |" % (
            f["addr"], f["size"], f.get("level", ""), f.get("name", ""),
            ", ".join(label(c, prog, fnames) for c in sorted(callers.get(a, ()))) or "(no direct call found)",
            ", ".join(label(t, prog, fnames) for t in sorted(graph.get(a, ()))) or ""))
        if f.get("notes"):
            lines.append("| | | | notes: %s | | |" % f["notes"])
    used = set()
    for f in fs:
        data, base, _ = prog.code(f)
        for i in MD.disasm(data, base):
            for m in re.finditer(r"\[(?:[a-z]{2} \+ )?0x([0-9a-f]+)\]", i.op_str):
                used.add(int(m.group(1), 16))
    named = sorted(o for o in used if o in dnames)
    if named:
        lines += ["", "## Globals src/ already names", ""] + ["- ds:%04X %s" % (o, dnames[o]) for o in named]
    for f in fs:
        if f.get("level") in ("L2", "L3"):
            continue
        data, base, seg = prog.code(f)
        lines += ["", "## %s (%d bytes)" % (f["addr"], f["size"]), "", "```"]
        for i in MD.disasm(data, base):
            note = ""
            t = prog.target(f, i)
            if t:
                note = label(t, prog, fnames)
            else:
                for m in re.finditer(r"0x([0-9a-f]+)", i.op_str):
                    v = int(m.group(1), 16)
                    if v in dnames and ("[" in i.op_str or i.mnemonic in ("mov", "push", "add")):
                        note = "ds:%04X %s" % (v, dnames[v])
            lines.append("%s:%04X  %-14s %-6s %-30s %s" % (seg, i.address, i.bytes.hex(), i.mnemonic, i.op_str, note))
        lines.append("```")
    out = common.ROOT / "build/packets" / ("%s.md" % uid)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines) + "\n")
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", nargs="?", default=os.environ.get("UNIT", ""))
    a = ap.parse_args(argv)
    try:
        print("packet: %s" % write(common.identifier(a.unit, "unit")).relative_to(common.ROOT))
    except (ValueError, OSError) as e:
        print("packet: %s" % e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
