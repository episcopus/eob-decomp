#!/usr/bin/env python3
"""run.py: start and stop a run of the roster (playbook 90).

  make start-run [DRY=1]      check, then start every entry of work/roster.toml in the background
  make run-status             each entry: alive or not, its claim, the last line of its log
  make run-add NAME=<name>    start (or restart) one entry in the running run
  make stop-run [WAIT=30]     drain: workers finish their session and stop (WAIT minutes at most,
                              then they are stopped and settle their claims), the integrator
                              finishes its merge, a last `make integrate`, the run's state cleared

Before starting: main is clean and on main; the toolchain, .venv and the overlays are there; the
session command's program is on the PATH; `make match-gate` passes; no run is active. The run's
state (process ids) is in the clone's git directory (eob-run.json), the logs in build/run/.
"""
from __future__ import annotations

import argparse
import json
import os
import shlex
import shutil
import signal
import subprocess
import sys
import time
import tomllib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402

ROLES = ("decompiler", "reviewer", "integrator")


def state_path() -> Path:
    return common.common_dir() / "eob-run.json"


def stop_file() -> Path:
    return common.main_checkout() / "build/worker/STOP"


def roster() -> tuple[dict, list[dict]]:
    r = tomllib.loads((common.ROOT / "work/roster.toml").read_text())
    run, entries = r.get("run", {}), r.get("session", [])
    names = [e.get("name", "") for e in entries]
    for e in entries:
        common.identifier(e.get("name", ""), "roster name")
        if e.get("role") not in ROLES:
            raise ValueError("%s: role must be one of %s" % (e["name"], ", ".join(ROLES)))
    if len(set(names)) != len(names):
        raise ValueError("roster names must be unique")
    if sum(e["role"] == "integrator" for e in entries) > 1:
        raise ValueError("one integrator at most")
    return run, entries


def command(run: dict, e: dict) -> list[str]:
    py = str(common.main_checkout() / ".venv/bin/python")
    if e["role"] == "integrator":
        return [py, "tools/agent/integrator.py", "--interval", str(e.get("interval", 120)),
                "--idle-stop", str(e.get("idle_stop", 60))]
    return [py, "tools/agent/worker.py", "--agent", e["name"], "--role", e["role"],
            "--sessions", str(e.get("sessions", run.get("sessions", 50))),
            "--timeout", str(e.get("timeout", run.get("timeout", 60))),
            "--session-cmd", e.get("session_cmd", run.get("session_cmd", ""))]


def alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except (ProcessLookupError, PermissionError):
        return False
    try:                                   # a zombie child of this process is not alive
        return os.waitpid(pid, os.WNOHANG) == (0, 0)
    except ChildProcessError:
        return True


def load_state() -> dict:
    p = state_path()
    return json.loads(p.read_text()) if p.exists() else {}


def save_state(s: dict) -> None:
    common.atomic_write(state_path(), json.dumps(s, indent=1))


def workers_alive() -> bool:
    s = load_state()
    return any(v["role"] != "integrator" and alive(v["pid"]) for v in s.get("sessions", {}).values())


def preflight(run: dict, entries: list[dict]) -> list[str]:
    out = []
    main_dir = common.main_checkout()
    if common.ROOT.resolve() != main_dir.resolve() or common.git("branch", "--show-current") != "main":
        out.append("run it from the main checkout on branch main")
    if common.git("status", "--porcelain", "--untracked-files=no"):
        out.append("main has uncommitted changes")
    for rel in (".venv/bin/python", "build/toolchain", "build/re/eob.json"):
        if not (main_dir / rel).exists():
            out.append("%s is missing (make setup, make tc-install, make overlays)" % rel)
    for e in entries:
        if e["role"] != "integrator":
            cmd = e.get("session_cmd", run.get("session_cmd", ""))
            prog = shlex.split(cmd)[0] if cmd.strip() else ""
            if not prog or not shutil.which(prog):
                out.append("%s: the session command %r is not runnable here (set session_cmd in work/roster.toml)" % (
                    e["name"], cmd))
    if any(alive(v["pid"]) for v in load_state().get("sessions", {}).values()):
        out.append("a run is active (make run-status; make stop-run)")
    if not out:
        r = subprocess.run(["make", "-s", "match-gate"], cwd=main_dir, capture_output=True, text=True)
        if r.returncode:
            out.append("make match-gate fails:\n" + (r.stdout + r.stderr)[-1500:])
    return out


def launch(run: dict, e: dict) -> dict:
    main_dir = common.main_checkout()
    log = main_dir / "build/run" / ("%s.log" % e["name"])
    log.parent.mkdir(parents=True, exist_ok=True)
    out = open(log, "a")
    out.write("\n=== %s start %s\n" % (common.now(), " ".join(shlex.quote(c) for c in command(run, e))))
    out.flush()
    p = subprocess.Popen(command(run, e), cwd=main_dir, stdout=out, stderr=subprocess.STDOUT,
                         stdin=subprocess.DEVNULL, start_new_session=True,
                         env=dict(os.environ, PYTHONUNBUFFERED="1"))
    common.event("run-launch", name=e["name"], role=e["role"], pid=p.pid)
    return {"pid": p.pid, "role": e["role"], "log": str(log), "started": common.now()}


def start(dry: bool) -> int:
    run, entries = roster()
    if dry:
        for e in entries:
            print("%-8s %s" % (e["name"], " ".join(shlex.quote(c) for c in command(run, e))))
        problems = preflight(run, entries)
        for p in problems:
            print("start-run: would refuse: " + p)
        return 0
    problems = preflight(run, entries)
    for p in problems:
        print("start-run: " + p, file=sys.stderr)
    if problems:
        return 1
    common.install_git_config()
    stop_file().unlink(missing_ok=True)
    state = {"started": common.now(), "sessions": {}}
    for e in entries:
        state["sessions"][e["name"]] = launch(run, e)
        print("start-run: %s (%s) pid %d, log %s" % (e["name"], e["role"], state["sessions"][e["name"]]["pid"],
                                                    state["sessions"][e["name"]]["log"]))
    save_state(state)
    common.event("run-start", sessions=list(state["sessions"]))
    print("start-run: %d sessions; make run-status, make stop-run" % len(entries))
    return 0


def add(name: str) -> int:
    run, entries = roster()
    e = next((e for e in entries if e["name"] == name), None)
    if e is None:
        print("run-add: no %s in work/roster.toml" % name, file=sys.stderr)
        return 1
    state = load_state()
    if not state:
        print("run-add: no run is active (make start-run)", file=sys.stderr)
        return 1
    cur = state["sessions"].get(name)
    if cur and alive(cur["pid"]):
        print("run-add: %s is running (pid %d)" % (name, cur["pid"]), file=sys.stderr)
        return 1
    state["sessions"][name] = launch(run, e)
    save_state(state)
    print("run-add: %s started, pid %d" % (name, state["sessions"][name]["pid"]))
    return 0


def last_line(path: str) -> str:
    try:
        lines = [l for l in Path(path).read_text().splitlines() if l.strip()]
        return lines[-1][:110] if lines else ""
    except OSError:
        return ""


def status() -> int:
    state = load_state()
    if not state:
        print("run-status: no run (make start-run)")
    else:
        print("run started %s%s" % (state["started"], "; STOP requested" if stop_file().exists() else ""))
        locks = {common.fields(p.read_text()).get("agent"): p.stem for p in common.state_dir("eob-locks").glob("*.lock")}
        for name, v in state["sessions"].items():
            print("  %-8s %-10s %-7s %-14s %s" % (name, v["role"], "alive" if alive(v["pid"]) else "stopped",
                                                 locks.get(name, ""), last_line(v["log"])))
    subprocess.run([sys.executable, str(Path(__file__).with_name("progress.py"))])
    return 0


def stop(wait: float) -> int:
    import worker
    state = load_state()
    sessions = state.get("sessions", {})
    stop_file().parent.mkdir(parents=True, exist_ok=True)
    stop_file().touch()
    workers = {n: v for n, v in sessions.items() if v["role"] != "integrator"}
    deadline = time.time() + wait * 60
    print("stop-run: STOP set; waiting up to %g minutes for %d workers to finish their session" % (wait, len(workers)))
    while time.time() < deadline and any(alive(v["pid"]) for v in workers.values()):
        time.sleep(5)
    for n, v in workers.items():
        if alive(v["pid"]):
            print("stop-run: stopping %s (pid %d); it settles its claim" % (n, v["pid"]))
            os.kill(v["pid"], signal.SIGTERM)
    end = time.time() + 120
    while time.time() < end and any(alive(v["pid"]) for v in workers.values()):
        time.sleep(2)
    for n, v in workers.items():
        if alive(v["pid"]):
            os.killpg(v["pid"], signal.SIGKILL)
    for n, v in sessions.items():
        if v["role"] == "integrator" and alive(v["pid"]):
            print("stop-run: the integrator finishes its merge, then stops")
            os.kill(v["pid"], signal.SIGTERM)
            end = time.time() + 600
            while time.time() < end and alive(v["pid"]):
                time.sleep(2)
    left = []
    for p in sorted(common.state_dir("eob-locks").glob("*.lock")):
        f = common.fields(p.read_text())
        if f.get("agent") in workers:
            wt = common.worktree_of(p.stem)
            outcome = worker.settle(f["agent"], p.stem, f["branch"], wt) if wt.exists() else "left"
            print("stop-run: claim %s of %s: %s" % (p.stem, f["agent"], outcome))
            if outcome == "left":
                left.append(p.stem)
    r = subprocess.run(["make", "-s", "integrate"], cwd=common.main_checkout(), capture_output=True, text=True)
    print((r.stdout + r.stderr).strip())
    state_path().unlink(missing_ok=True)
    stop_file().unlink(missing_ok=True)
    common.event("run-stop", left=left)
    if left:
        print("stop-run: claims left for the human (uncommitted or unmatched work): %s; make locks, make unlock"
              % ", ".join(left))
    subprocess.run([sys.executable, str(Path(__file__).with_name("progress.py"))])
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("action", choices=("start", "stop", "status", "add"))
    ap.add_argument("name", nargs="?", default="")
    ap.add_argument("--dry", action="store_true")
    ap.add_argument("--wait", type=float, default=30)
    a = ap.parse_args(argv)
    sys.stdout.reconfigure(line_buffering=True)
    try:
        if a.action == "start":
            return start(a.dry)
        if a.action == "status":
            return status()
        if a.action == "add":
            return add(common.identifier(a.name, "NAME"))
        return stop(a.wait)
    except (ValueError, RuntimeError, OSError) as e:
        print("%s-run: %s" % (a.action, e), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
