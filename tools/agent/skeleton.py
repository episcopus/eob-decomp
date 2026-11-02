#!/usr/bin/env python3
"""skeleton.py: start the source file of a unit with every function as a stub (playbook 30).

  make skeleton UNIT=<id>        writes src/<ID>.C (src/<ID>.ASM for a unit the map calls assembly)

Each function of the unit in re/symbols.toml gets a stub line at its address, named by the registry
(or f<SEG>_<OFF>, Ovl<N>_<OFF> in an overlay). `make match` accepts the file as it is (nothing is
compared yet); decompile one function at a time by replacing its stub with the code, and turn a
stub's `void f(void)` into the real signature once the callers show it. Refuses to overwrite a file.
"""
from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402


def stub_name(f: dict) -> str:
    if f.get("name"):
        return f["name"]
    seg, off = f["addr"].split(":")
    return ("Ovl%s_%s" % (seg[3:], off.upper())) if seg.lower().startswith("ovl") else "f%s_%s" % (seg.upper(), off.upper())


def text(uid: str) -> tuple[str, str]:
    u = common.unit(uid)
    fs = sorted(common.unit_functions(uid), key=lambda f: int(f["addr"].split(":")[1], 16))
    where = "overlay %d of %s" % (u["overlay"], u["program"]) if "overlay" in u else "%s %s" % (u["program"], u["start"])
    if u["kind"] == "asm":
        lines = ["; Unit at %s (%d bytes, %d functions), kind asm [H] by the map. Skeleton by `make skeleton`:" % (
                     where, u["bytes"], len(fs)),
                 "; every function a stub. Placeholder names.", "",
                 "        .MODEL  LARGE", "        .CODE", ""]
        for f in fs:
            n = "_" + stub_name(f)
            lines += ["        PUBLIC  %-24s ; @ %s stub" % (n, f["addr"]), "%s PROC FAR" % n, "        ret", "%s ENDP" % n, ""]
        lines.append("        END")
        return ".ASM", "\n".join(lines) + "\n"
    lines = ["/* Unit at %s (%d bytes, %d functions). Skeleton by `make skeleton`: every function a" % (
                 where, u["bytes"], len(fs)),
             "   stub. Placeholder names. */", ""]
    for f in fs:
        done = f.get("level") in ("L2", "L3")
        lines.append("/*@ %s stub */ void %s(void) {}%s" % (
            f["addr"], stub_name(f), "   /* recorded L2 elsewhere: check src/ */" if done else ""))
    return ".C", "\n".join(lines) + "\n"


def write(uid: str) -> Path:
    if common.source_of(uid):
        raise ValueError("%s exists; a skeleton never overwrites a source file" % common.source_of(uid).relative_to(common.ROOT))
    ext, body = text(uid)
    p = common.ROOT / "src" / (uid.upper() + ext)
    p.write_text(body)
    return p


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", nargs="?", default=os.environ.get("UNIT", ""))
    a = ap.parse_args(argv)
    try:
        print("skeleton: wrote %s" % write(common.identifier(a.unit, "unit")).relative_to(common.ROOT))
    except (ValueError, OSError) as e:
        print("skeleton: %s" % e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
