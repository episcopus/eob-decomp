"""common.py: what the agent tools share (docs/process/playbooks/90-orchestrate.md).

Coordination state lives in the clone's common git directory (`git rev-parse --git-common-dir`),
never in a working tree and never in a commit, so every worktree of this clone sees the same locks
and hand-offs:

  eob-locks/<key>.lock            one agent's claim on a unit (e0cf7), a review (review-e0cf7) or a
                                  task; created atomically
  eob-handoffs/<key>.md           a branch waiting for `make integrate`, or an escalation
  eob-handoff-archive/            hand-offs that were merged or settled
  eob-events.log                  one line per claim, hand-off, merge, release (JSON)

A unit is complete when every function of it is at L2 in re/symbols.toml.
"""
from __future__ import annotations

import datetime as dt
import json
import os
import re
import subprocess
import tempfile
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
# What is not in git but every worktree needs, linked from the main checkout: the Python tools, the
# installed compiler, the extracted overlays, emu2.
LINKS = (".venv", "build/toolchain", "build/re", "tools/vendor")
UNIT_STATES = ("complete", "partial", "blocked")
REVIEW_STATES = ("review-passed", "review-failed")
STATES = UNIT_STATES + REVIEW_STATES + ("escalate",)


def identifier(value: str, label: str = "identifier") -> str:
    """Keys and agent names are names, never paths or options."""
    if not re.fullmatch(r"[a-z0-9][a-z0-9_-]{0,79}", value or ""):
        raise ValueError("%s must be 1 to 80 lower-case letters, digits, '_' or '-', not %r" % (label, value))
    return value


def single_line(value: str, label: str) -> str:
    if any(ord(c) < 32 or ord(c) == 127 for c in value):
        raise ValueError("%s must be one line without control characters" % label)
    return value


def fields(text: str) -> dict[str, str]:
    out = {}
    for line in text.splitlines():
        if " = " in line:
            k, v = line.split(" = ", 1)
            out[k] = v
    return out


def record(d: dict) -> str:
    return "".join("%s = %s\n" % (k, v) for k, v in d.items())


def now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")


def atomic_write(path: Path, text: str) -> None:
    fd, name = tempfile.mkstemp(prefix=".%s." % path.name, dir=path.parent)
    try:
        with os.fdopen(fd, "w") as f:
            f.write(text)
        os.replace(name, path)
    finally:
        Path(name).unlink(missing_ok=True)


def git(*args: str, cwd: Path | None = None, check: bool = True) -> str:
    r = subprocess.run(["git", *args], cwd=cwd or ROOT, capture_output=True, text=True)
    if check and r.returncode:
        raise RuntimeError("git %s: %s" % (" ".join(args), r.stderr.strip()))
    return r.stdout.strip()


def common_dir() -> Path:
    p = Path(git("rev-parse", "--git-common-dir"))
    return p if p.is_absolute() else (ROOT / p).resolve()


def main_checkout() -> Path:
    """The working tree that owns the clone's .git directory (where build/toolchain lives)."""
    return common_dir().parent


def state_dir(name: str) -> Path:
    d = common_dir() / name
    d.mkdir(exist_ok=True)
    return d


def lock_path(key: str) -> Path:
    return state_dir("eob-locks") / ("%s.lock" % identifier(key, "key"))


def handoff_path(key: str) -> Path:
    return state_dir("eob-handoffs") / ("%s.md" % identifier(key, "key"))


def try_lock(key: str, body: dict) -> bool:
    """Create the lock atomically; False when someone holds it."""
    try:
        fd = os.open(lock_path(key), os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o644)
    except FileExistsError:
        return False
    with os.fdopen(fd, "w") as f:
        f.write(record(body))
    return True


def lock_owner(key: str) -> str | None:
    p = lock_path(key)
    return fields(p.read_text()).get("agent") if p.exists() else None


def event(kind: str, **kw) -> None:
    line = json.dumps({"time": now(), "event": kind, **kw}, sort_keys=True)
    with open(common_dir() / "eob-events.log", "a") as f:
        f.write(line + "\n")


def require_main(operation: str) -> None:
    if ROOT.resolve() != main_checkout().resolve() or git("branch", "--show-current") != "main":
        raise ValueError("%s runs in the main checkout on branch main" % operation)


def worktree_of(key: str) -> Path:
    return main_checkout().parent / "eob-worktrees" / key


def link_untracked(wt: Path) -> None:
    for rel in LINKS:
        src, dst = main_checkout() / rel, wt / rel
        if src.exists() and not dst.exists() and not dst.is_symlink():
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.symlink_to(src)


# ---------------------------------------------------------------- the registry

def units() -> list[dict]:
    return tomllib.loads((ROOT / "re/units.toml").read_text())["unit"]


def functions() -> list[dict]:
    return tomllib.loads((ROOT / "re/symbols.toml").read_text())["function"]


def unit(uid: str) -> dict:
    u = next((u for u in units() if u["id"] == uid), None)
    if u is None:
        raise ValueError("no unit %s in re/units.toml" % uid)
    return u


def unit_functions(uid: str, fs: list[dict] | None = None) -> list[dict]:
    return [f for f in (fs or functions()) if f["unit"] == uid]


def unit_complete(uid: str, fs: list[dict] | None = None) -> bool:
    rs = unit_functions(uid, fs)
    return bool(rs) and all(f.get("level") in ("L2", "L3") for f in rs)


def unit_reviewed(uid: str, fs: list[dict] | None = None) -> bool:
    rs = unit_functions(uid, fs)
    return bool(rs) and all(str(f.get("review", "")).startswith("passed") for f in rs)


def queue() -> list[str]:
    p = ROOT / "work/queue.toml"
    return [u["id"] for u in tomllib.loads(p.read_text()).get("unit", [])] if p.exists() else []


def source_of(uid: str) -> Path | None:
    for ext in (".C", ".ASM"):
        p = ROOT / "src" / (uid.upper() + ext)
        if p.exists():
            return p
    return None


def open_must_fix(uid: str) -> list[str]:
    """Open must-fix findings of the unit's review (work/reviews/<unit>.md): `- [ ] must-fix ...`."""
    p = ROOT / "work/reviews" / ("%s.md" % uid)
    if not p.exists():
        return []
    return [l.strip() for l in p.read_text().splitlines() if re.match(r"\s*-\s*\[ \]\s*must-fix\b", l, re.I)]
