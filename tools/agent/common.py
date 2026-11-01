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


def install_git_config() -> None:
    """Per-clone settings the process needs (shared by every worktree of the clone): the
    pre-commit guard and the registry's merge driver (.gitattributes)."""
    main_dir = main_checkout()
    git("config", "core.hooksPath", "tools/agent/hooks", cwd=main_dir)
    git("config", "merge.eob-registry.name", "registry entries by id (tools/agent/merge_registry.py)", cwd=main_dir)
    git("config", "merge.eob-registry.driver", "python3 tools/agent/merge_registry.py %O %A %B", cwd=main_dir)


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


# ---------------------------------------------------------------- attempts and agent status

TIER1_ATTEMPTS = 2           # sessions of tier 1 on a unit without progress before tier 2 may take it


def _locked_json(name: str, update=None):
    """Read (and with `update`, rewrite) a JSON file of the clone's git directory under an flock."""
    import fcntl
    path = common_dir() / name
    with open(common_dir() / (name + ".lock"), "w") as guard:
        fcntl.flock(guard, fcntl.LOCK_EX)
        data = json.loads(path.read_text()) if path.exists() else {}
        if update is not None:
            update(data)
            atomic_write(path, json.dumps(data, indent=1, sort_keys=True))
        return data


def attempts() -> dict[str, dict[str, int]]:
    """unit -> {"1": sessions of tier 1 without progress, "2": the same for tier 2}."""
    return _locked_json("eob-attempts.json")


def record_attempt(uid: str, tier: int, progressed: bool) -> None:
    """A decompiler session ended: progress (more functions at L2) clears the unit's count, so a big
    unit keeps going as long as sessions keep matching; no progress counts one attempt of its tier."""
    def up(d):
        if progressed:
            d.pop(uid, None)
        else:
            e = d.setdefault(uid, {})
            e[str(tier)] = e.get(str(tier), 0) + 1
    _locked_json("eob-attempts.json", up)


def l2_count(root: Path, uid: str) -> int:
    """Functions of the unit at L2 or L3 in the registry of the checkout `root`."""
    fs = tomllib.loads((root / "re/symbols.toml").read_text())["function"]
    return sum(1 for f in fs if f["unit"] == uid and f.get("level") in ("L2", "L3"))


def agent_status(agent: str, **fields_) -> None:
    """What a worker is doing now, for `make watch-agents` (eob-agents/<agent>.json)."""
    d = state_dir("eob-agents")
    atomic_write(d / ("%s.json" % identifier(agent, "agent")), json.dumps({"agent": agent, "time": now(), **fields_}))


def agent_statuses() -> dict[str, dict]:
    out = {}
    for p in state_dir("eob-agents").glob("*.json"):
        try:
            out[p.stem] = json.loads(p.read_text())
        except ValueError:
            pass
    return out
