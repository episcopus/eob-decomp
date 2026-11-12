#!/usr/bin/env python3
"""build.py: EOB.EXE and TGA.OVL for the Tandy 1000 EX, from src/ plus this port's patches (patches/series).

  build.py [--name game] [--only a.patch,b.patch] [--none]

src/ is never edited: a staged copy of the tree (ports/dos/build/<name>/stage) has the patches applied (patch --fuzz=0,
so a change in src/ under a patch fails the build and says which), and the project's own tools/rebuild link it, with the
pinned Turbo C++ 1.00, TASM and TLINK. Outputs in ports/dos/build/<name>/: EOB.EXE, TGA.OVL, eob.map, d.lst, build.json.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PORT = HERE.parent
ROOT = PORT.parents[1]


def series() -> list[str]:
    out = []
    for line in (PORT / "patches/series").read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            out.append(line)
    return out


def stage_tree(w: Path) -> Path:
    s = w / "stage"
    shutil.rmtree(s, ignore_errors=True)
    s.mkdir(parents=True)
    shutil.copytree(ROOT / "src", s / "src")
    shutil.copytree(ROOT / "tools", s / "tools", ignore=shutil.ignore_patterns("__pycache__", "vendor", "handbook"))
    for link in ("re", "eob", "tests"):
        (s / link).symlink_to(ROOT / link)
    (s / "build").mkdir()
    (s / "build/toolchain").symlink_to(ROOT / "build/toolchain")
    if (ROOT / "tools/vendor").exists():
        (s / "tools/vendor").symlink_to(ROOT / "tools/vendor")
    if (ROOT / "build/re").exists():
        (s / "build/re").symlink_to(ROOT / "build/re")
    return s


# what the stage's copy of the project's linker tool gets, for this port only (src/ and tools/ are not touched):
#  - a C module with `#pragma inline` is compiled in two steps (TCC -S, then TASM), as TCC itself would through TASM;
#  - a module whose code got shorter is padded to its original length with a module of NOPs in the same segment, so that
#    every routine and everything after it stays at the original's address (`EOB_PADS` names them).
REBUILD_PATCH = [("""            args = ["-c", "-ID:\\\\INCLUDE"] + [f for f in flags if f != "-B"] + [c.path.name]
            r = tc.run("tc100", "TCC", args, cwd=w)""",
"""            args = ["-c", "-ID:\\\\INCLUDE"] + [f for f in flags if f != "-B"] + [c.path.name]
            if "#pragma inline" in c.text_out:
                sargs = [a for a in args if a != "-c"]
                sargs.insert(0, "-S")
                r = tc.run("tc100", "TCC", sargs, cwd=w)
                r = tc.run(tc.manifest()["pinned_asm"], "TASM", ["/ml", c.path.stem + ".ASM"], cwd=w)
                print(r.stdout.decode("latin-1"))
            else:
                r = tc.run("tc100", "TCC", args, cwd=w)"""),
("""    w.mkdir(parents=True)
    space = Space(program)""",
"""    w.mkdir(parents=True)
    import os as _os, json as _json
    pads = _json.loads(_os.environ.get("EOB_PADS" if program == "EOB.EXE" else "INTRO_PADS", "{}")) if program in ("EOB.EXE", "INTRO.EXE") else {}
    for stem, (seg, n) in pads.items():
        text = "%s SEGMENT BYTE PUBLIC 'CODE'\\r\\n db %d dup (90h)\\r\\n%s ENDS\\r\\nEND\\r\\n" % (seg, n, seg)
        assemble(w, "PD_" + stem[:5], text)
    global PADS
    PADS = {k.upper() for k in pads}
    space = Space(program)"""),
("""        objs.append(c.path.stem)
        if i in after:""",
"""        objs.append(c.path.stem)
        if c.path.stem.upper() in PADS:
            objs.append("PD_" + c.path.stem[:5])
        if i in after:"""),
("""DATA_DEF = re.compile(""", """PADS = set()
DATA_DEF = re.compile("""),
]


def apply(stage: Path, patch: str) -> None:
    p = PORT / "patches" / patch
    r = subprocess.run(["patch", "-p1", "--fuzz=0", "--no-backup-if-mismatch", "-i", str(p)], cwd=stage / "src",
                       capture_output=True, text=True)
    if r.returncode:
        print(r.stdout + r.stderr)
        sys.exit("build: patch %s does not apply: refresh it against src/" % patch)


def segments(map_path: Path) -> dict:
    out = {}
    for line in map_path.read_text(errors="replace").splitlines():
        m = re.match(r"\s*([0-9A-F]+)H\s+([0-9A-F]+)H\s+([0-9A-F]+)H\s+(\S+)\s+(\S+)", line)
        if m:
            # the overlay segments appear twice (the stub, and the code as `:OVY`): the code's is the one that matters
            out.setdefault(m[4], {})[m[5]] = int(m[3], 16)
    return out


def compute_pads(map_path: Path, stem: str = "eob") -> dict:
    """{module stem: (segment name, bytes)} for each code module shorter than the reference; a module longer than its
    original cannot be built here, a patch must move the extra code."""
    ours = segments(map_path)
    ref = segments(PORT / "build/ref" / (stem + ".map"))
    pads = {}
    bad = []
    for name, classes in ref.items():
        if not name.endswith("_TEXT") or name == "_TEXT":
            continue
        for cls, n in classes.items():
            if cls == ":OVY" or cls == "STUBSEG":   # an overlay may change size: its code is relocated by the overlay manager
                continue
            have = ours.get(name, {}).get(cls)
            if have is None or have == n:
                continue
            if have > n:
                bad.append("%s %d bytes over" % (name, have - n))
            elif cls != "STUBSEG":
                pads[name[:-5]] = (name, n - have)
    if bad:
        sys.exit("build: code longer than the original: " + "; ".join(bad))
    return pads


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--name", default="game")
    ap.add_argument("--only", help="comma-separated patch names, in series order")
    ap.add_argument("--none", action="store_true", help="no patches: the reference")
    a = ap.parse_args(argv)
    w = PORT / "build" / a.name
    w.mkdir(parents=True, exist_ok=True)
    for f in ("EOB.EXE", "INTRO.EXE", "TGA.OVL", "eob.map", "intro.map", "d.lst", "build.json"):
        (w / f).unlink(missing_ok=True)      # a failed build must not leave the last one looking current
    patches = [] if a.none else series()
    if patches and not (PORT / "build/ref/eob.map").exists():
        main(["--name", "ref", "--none"])          # the reference: every unit's original length comes from its map
    if a.only:
        want = {x.strip() for x in a.only.split(",")}
        patches = [p for p in patches if p in want or p.rsplit("/", 1)[-1] in want]
    stage = stage_tree(w)
    for p in patches:
        apply(stage, p)
    rb = stage / "tools/rebuild/rebuild.py"
    text = rb.read_text()
    for old, new in REBUILD_PATCH:
        if old not in text:
            sys.exit("build: tools/rebuild/rebuild.py changed: the port's patch to it does not apply (%r)" % old[:50])
        text = text.replace(old, new, 1)
    rb.write_text(text)
    py = str(ROOT / ".venv/bin/python")
    r1 = subprocess.run([py, str(rb), "EOB.EXE"], cwd=stage, capture_output=True, text=True)
    if "Error:" in r1.stdout or "does not build" in r1.stdout:
        print(r1.stdout[-3000:])
        return 1
    if not (stage / "build/rebuild/eob/eob.map").exists():
        print(r1.stdout[-4000:], r1.stderr[-1000:])
        return 1
    pads = compute_pads(stage / "build/rebuild/eob/eob.map") if patches else {}
    if pads:
        env = dict(os.environ, EOB_PADS=json.dumps(pads))
        r1 = subprocess.run([py, str(rb), "EOB.EXE"], cwd=stage, capture_output=True, text=True, env=env)
        if "Error:" in r1.stdout or "does not build" in r1.stdout:
            print(r1.stdout[-3000:])
            return 1
        left = compute_pads(stage / "build/rebuild/eob/eob.map")
        if left:
            print(r1.stdout[-2000:], r1.stderr[-2000:])
            sys.exit("build: units not back at the original length: %s" % left)
    r2 = subprocess.run([py, str(stage / "tools/rebuild/rebuild.py"), "TGA.OVL"], cwd=stage, capture_output=True, text=True)
    r3 = subprocess.run([py, str(rb), "INTRO.EXE"], cwd=stage, capture_output=True, text=True)
    exe = stage / "build/rebuild/eob/eob.exe"
    drv = stage / "build/rebuild/tga/d.exe"
    intro = stage / "build/rebuild/intro/intro.exe"
    if patches and (stage / "build/rebuild/intro/intro.map").exists():      # INTRO.EXE shares units with EOB.EXE (the file layer, the unpackers)
        pads = compute_pads(stage / "build/rebuild/intro/intro.map", "intro")
        if pads:
            r3 = subprocess.run([py, str(rb), "INTRO.EXE"], cwd=stage, capture_output=True, text=True,
                                env=dict(os.environ, INTRO_PADS=json.dumps(pads)))
            left = compute_pads(stage / "build/rebuild/intro/intro.map", "intro")
            if left:
                sys.exit("build: INTRO.EXE units not back at the original length: %s" % left)
    if not intro.exists() or "Error:" in r3.stdout or "does not build" in r3.stdout:
        print(r3.stdout[-3000:], r3.stderr[-1000:])
        return 1
    # the link succeeded when the files exist; `rebuild` exits non-zero whenever the result differs from the original
    if not exe.exists() or not drv.exists() or "does not assemble" in r2.stdout or "does not build" in r1.stdout:
        print(r1.stdout[-3000:], r1.stderr[-2000:], r2.stdout[-2000:], r2.stderr[-500:])
        return 1
    shutil.copy(exe, w / "EOB.EXE")
    shutil.copy(drv, w / "TGA.OVL")
    shutil.copy(intro, w / "INTRO.EXE")
    shutil.copy(stage / "build/rebuild/intro/intro.map", w / "intro.map")
    shutil.copy(stage / "build/rebuild/eob/eob.map", w / "eob.map")
    shutil.copy(stage / "build/rebuild/tga/d.lst", w / "d.lst")
    info = {"patches": patches, "EOB.EXE": sha(w / "EOB.EXE"), "INTRO.EXE": sha(w / "INTRO.EXE"), "TGA.OVL": sha(w / "TGA.OVL"),
            "sizes": {f: (w / f).stat().st_size for f in ("EOB.EXE", "INTRO.EXE", "TGA.OVL")}}
    (w / "build.json").write_text(json.dumps(info, indent=1))
    print("%s: %d patches, EOB.EXE %d, INTRO.EXE %d, TGA.OVL %d bytes" % (a.name, len(patches), info["sizes"]["EOB.EXE"], info["sizes"]["INTRO.EXE"], info["sizes"]["TGA.OVL"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
