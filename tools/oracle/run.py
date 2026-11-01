#!/usr/bin/env python3
"""run.py: run the original Eye of the Beholder in the oracle (Phase 2; the machine is poolrad-decomp's).

  .venv/bin/python tools/oracle/run.py [--program EOB.EXE] [--args "3 4 0 homie 3"] [--max-insns N]
        [--out build/oracle/run] [--shot-every N]

EOB.EXE takes its configuration on the command line (whitepaper 2.6), as START1.EXE passes it through
INTRO.EXE: graphics digit ('3': VGA), sound digit ('4': none), mouse digit ('0': none), the key "homie",
and the video mode to restore on exit (3: text). Writes console.txt, shots/*.png and summary.json into --out. Exit code 0 when the run
stopped for an expected reason (instruction budget, program exit), 3 on an unimplemented service,
4 on a guest fault.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from oracle.machine import Machine, Unimplemented, GuestCrash, OracleError, ROOT  # noqa: E402
from oracle.ega import write_png  # noqa: E402

GAME = ROOT / "eob"
DEFAULT_ARGS = "3 4 0 homie 3"


def install_mouse(m: Machine) -> None:
    """INT 33h with no mouse driver loaded: reset answers AX=0, everything else is ignored."""
    def int33(mach):
        if mach.get("ax") == 0:
            mach.set("ax", 0)
            mach.set("bx", 0)
    m.register_service(0x33, int33)


def build_machine(program: str = "EOB.EXE", args: str = DEFAULT_ARGS, insn_per_sec: int = 1_000_000) -> Machine:
    tail = b" " + args.encode("latin-1").decode("unicode_escape").encode("latin-1")
    m = Machine(GAME / program, GAME, cmdline=tail, insn_per_sec=insn_per_sec)
    install_mouse(m)
    return m


def shot(m: Machine, path: Path) -> None:
    w, h, rgb = m.ega.render()
    write_png(path, w, h, rgb)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--program", default="EOB.EXE")
    ap.add_argument("--args", default=DEFAULT_ARGS)
    ap.add_argument("--max-insns", type=int, default=50_000_000)
    ap.add_argument("--shot-every", type=int, default=0, help="instructions between screenshots")
    ap.add_argument("--out", default=str(ROOT / "build" / "oracle" / "run"))
    a = ap.parse_args(argv)
    out = Path(a.out)
    (out / "shots").mkdir(parents=True, exist_ok=True)
    for p in (out / "shots").glob("*.png"):
        p.unlink()
    m = build_machine(a.program, a.args)
    shots = []

    def on_chunk(mach):
        if a.shot_every and mach.insns // a.shot_every > len(shots) and mach.ega.mode == 0x13:
            p = out / "shots" / ("%04d.png" % len(shots))
            shot(mach, p)
            shots.append(p.name)

    code, reason = 0, ""
    try:
        reason = m.run(a.max_insns, on_chunk=on_chunk if a.shot_every else None)
    except Unimplemented as e:
        code, reason = 3, "unimplemented: %s" % e
    except GuestCrash as e:
        code, reason = 4, "guest fault: %s" % e
    except OracleError as e:
        code, reason = 4, "oracle: %s" % e
    shot(m, out / "shots" / "final.png")
    summary = {"program": a.program, "args": a.args, "stop": reason, "instructions": m.insns,
               "video_mode": "%02Xh" % m.ega.mode, "digest": m.state_digest(), "ints": {"%02X" % k: v for k, v in sorted(m.int_counts.items())},
               "files": sorted({p for op, p, _ in m.vfs.log if op in ("open", "create")}), "shots": shots + ["final.png"]}
    (out / "summary.json").write_text(json.dumps(summary, indent=1))
    print("%s: %s after %d instructions, video mode %s, state %s" % (
        a.program, reason, m.insns, summary["video_mode"], summary["digest"][:16]))
    return code


if __name__ == "__main__":
    sys.exit(main())
