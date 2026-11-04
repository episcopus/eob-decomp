#!/usr/bin/env python3
"""run.py: run the original Eye of the Beholder in the oracle (playbook 25; the machine is poolrad-decomp's).

  .venv/bin/python tools/oracle/run.py [--script tests/sessions/x.keys] [--mouse] [--sound adlib]
        [--args "3 4 0 homie 3"] [--max-insns N] [--out build/oracle/run] [--shot-every N]

EOB.EXE takes its configuration on the command line (whitepaper 2.6), as START1.EXE passes it through
INTRO.EXE: graphics digit ('3': VGA), sound digit ('1': AdLib, '4': none), mouse digit ('1': yes, '0':
no), the key "homie", and the video mode to restore on exit (3: text). --mouse and --sound set those
digits and install the device. Writes summary.json and shots/*.png into --out: the stop reason, the
instruction count, the state digest, and every `shot` and `digest` checkpoint of the script. Exit code 0
when the run stopped for an expected reason (instruction budget, script quit, program exit), 3 on an
unimplemented service, 4 on a guest fault.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from oracle.machine import Machine, Unimplemented, GuestCrash, OracleError, ROOT  # noqa: E402
from oracle.ega import write_png  # noqa: E402
from oracle.input import Script, Mouse, no_mouse  # noqa: E402
from oracle import layout  # noqa: E402

GAME = ROOT / "eob"
DEFAULT_ARGS = "3 4 0 homie 3"
PROGRAM_ARGS = {"EOB.EXE": DEFAULT_ARGS, "INTRO.EXE": "3 4 0 homie"}   # INTRO.EXE: as START1.EXE runs it


def command_line(args: str, mouse: bool, sound: str | None) -> str:
    a = args.split()
    if mouse:
        a[2] = "1"
    if sound == "adlib":
        a[1] = "1"
    return " ".join(a)


def pinned() -> set[str]:
    """The names of the pinned files of eob/ (re/inputs.toml)."""
    import tomllib
    files = tomllib.loads((ROOT / "re" / "inputs.toml").read_text())["file"]
    return {Path(k).name.upper() for k in files}


def hide_unpinned(m: Machine) -> None:
    """A run sees only the pinned files of eob/: EOBDATA.SAV and LEVELS.TMP, which the game writes,
    differ from one checkout to the next (SPEC 2), so a session that needs them says which
    (`option save`, use_save)."""
    keep = pinned()
    for full in list(m.vfs.host_index):
        if full.rsplit("\\", 1)[-1] not in keep:
            m.vfs.deleted.add(full)


def use_save(m: Machine, save: Path) -> None:
    """Serve `save` as EOBDATA.SAV, and LEVELS.TMP as the game leaves it after saving that game: the
    save's 12 level states (whitepaper 4.4; loading a game reads LEVELS.TMP before it rewrites it,
    and stalls when the file is missing)."""
    sys.path.insert(0, str(ROOT / "tools" / "re"))
    import eobfmt
    m.vfs.alias("EOBDATA.SAV", save)
    m.vfs.deleted.discard(m.vfs.mount + "\\EOBDATA.SAV")
    levels = eobfmt.save(save.read_bytes())["levels"]
    m.vfs.layer[m.vfs.mount + "\\LEVELS.TMP"] = bytearray(levels)
    m.vfs.deleted.discard(m.vfs.mount + "\\LEVELS.TMP")


def build_machine(program: str = "EOB.EXE", args: str = DEFAULT_ARGS, *, mouse: bool = False,
                  sound: str | None = None, insn_per_sec: int = 1_000_000) -> Machine:
    args = command_line(args, mouse, sound)
    lay = layout.current()       # the originals unless layout.use() chose the rebuild (playbook 50)
    m = Machine(lay.exe(program), GAME, cmdline=b" " + args.encode("latin-1"), insn_per_sec=insn_per_sec)
    hide_unpinned(m)
    lay.serve(m)
    if not lay.is_original:
        m.canonical = lay.canonical
    if mouse:
        Mouse(m)
    else:
        no_mouse(m)
    if sound == "adlib":
        from oracle.adlib import AdLib
        AdLib(m)
    return m


def shot(m: Machine, path: Path) -> str:
    w, h, rgb = m.ega.render()
    write_png(path, w, h, rgb)
    return hashlib.sha256(rgb).hexdigest()


def sound_digest(m: Machine) -> str:
    """sha256 of everything sent to the sound devices: AdLib registers, PC speaker, Tandy chip."""
    h = hashlib.sha256()
    for name, log in (("adlib", getattr(m, "adlib_log", [])), ("speaker", m.speaker_log), ("tandy", m.tandy_log)):
        h.update(name.encode())
        h.update(repr(log).encode())
    return h.hexdigest()


def run_session(script_text: str | None, *, out: Path, max_insns: int, args: str | None = None,
                mouse: bool = False, sound: str | None = None, shot_every: int = 0,
                save: Path | None = None, program: str = "EOB.EXE") -> dict:
    args = args or PROGRAM_ARGS[program]
    (out / "shots").mkdir(parents=True, exist_ok=True)
    for p in (out / "shots").glob("*.png"):
        p.unlink()
    m = build_machine(program, args=args, mouse=mouse, sound=sound)
    if save is not None:                   # the saved game the run sees (EOBDATA.SAV is not pinned)
        use_save(m, save)
    checkpoints = []
    script = Script(script_text or "")

    def on_checkpoint(mach, kind, name):
        cp = {"kind": kind, "name": name, "instructions": mach.insns, "digest": mach.state_digest()}
        if kind == "shot":
            cp["image"] = shot(mach, out / "shots" / ("%s.png" % name))
        checkpoints.append(cp)
    script.on_checkpoint = on_checkpoint
    m.keys = script
    shots = []

    def on_chunk(mach):
        if shot_every and mach.insns // shot_every > len(shots):
            p = out / "shots" / ("%04d.png" % len(shots))
            shot(mach, p)
            shots.append(p.name)

    code, reason = 0, ""
    try:
        reason = m.run(max_insns, on_chunk=on_chunk if shot_every else None)
    except Unimplemented as e:
        code, reason = 3, "unimplemented: %s" % e
    except GuestCrash as e:
        code, reason = 4, "guest fault: %s" % e
    except OracleError as e:
        code, reason = 4, "oracle: %s" % e
    final = shot(m, out / "shots" / "final.png")
    summary = {"program": program, "args": command_line(args, mouse, sound), "stop": reason, "code": code,
               "instructions": m.insns, "video_mode": "%02Xh" % m.ega.mode, "digest": m.state_digest(),
               "final_image": final, "checkpoints": checkpoints, "script_done": script.done(),
               "ints": {"%02X" % k: v for k, v in sorted(m.int_counts.items())},
               "files": sorted({p for op, p, _ in m.vfs.log if op in ("open", "create")}),
               "writes": sorted({p for op, p, _ in m.vfs.log if op == "create"}),
               "adlib_writes": len(getattr(m, "adlib_log", [])), "sound_digest": sound_digest(m),
               "shots": shots + ["final.png"]}
    (out / "summary.json").write_text(json.dumps(summary, indent=1))
    return summary


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--script")
    ap.add_argument("--program", default="EOB.EXE", help="EOB.EXE or INTRO.EXE")
    ap.add_argument("--args", help="default: %s" % PROGRAM_ARGS)
    ap.add_argument("--mouse", action="store_true")
    ap.add_argument("--sound", choices=["adlib"])
    ap.add_argument("--max-insns", type=int, default=50_000_000)
    ap.add_argument("--shot-every", type=int, default=0, help="instructions between screenshots")
    ap.add_argument("--out", default=str(ROOT / "build" / "oracle" / "run"))
    ap.add_argument("--save", help="a saved game to use as EOBDATA.SAV")
    ap.add_argument("--rebuilt", action="store_true", help="run the programs `make rebuild` linked (playbook 50)")
    a = ap.parse_args(argv)
    if a.rebuilt:
        layout.use(layout.rebuilt())
    text = Path(a.script).read_text() if a.script else None
    s = run_session(text, out=Path(a.out), max_insns=a.max_insns, args=a.args, mouse=a.mouse, sound=a.sound,
                    shot_every=a.shot_every, save=Path(a.save) if a.save else None,
                    program=a.program)
    print("%s: %s after %d instructions, video mode %s, state %s, %d checkpoints" % (
        s["program"], s["stop"], s["instructions"], s["video_mode"], s["digest"][:16], len(s["checkpoints"])))
    return s["code"]


if __name__ == "__main__":
    sys.exit(main())
