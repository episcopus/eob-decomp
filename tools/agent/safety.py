#!/usr/bin/env python3
"""safety.py: what a branch may not change (AGENTS.md ground rules; the pre-commit hook and `make handoff`).

Everywhere: no change under eob/ (the pinned originals) or build/, no en or em dash in an added line,
no TOML file that does not parse (a registry every tool reads; e122e, 2026-11-02, escaped a backtick in a note).
On unit/ and review/ branches also: not the spec (docs/spec/), re/inputs.toml, the toolchain pin
(tools/re/tc_toolchain.toml), the matcher (tools/match/) or its negative fixtures (tests/match/negative/),
the agent tools (tools/agent/): those change on a task branch the human asked for, so a decompiler
can never make its own work pass by changing the proof.

  safety.py --staged         check the index (tools/agent/hooks/pre-commit)
  safety.py --branch NAME    check NAME against main (make handoff, make integrate)
"""
from __future__ import annotations

import argparse
import sys
import tomllib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402

ALWAYS = ("eob/", "build/")
WORKER_BRANCH = ("docs/spec/", "re/inputs.toml", "tools/re/tc_toolchain.toml", "tools/match/",
                 "tests/match/negative/", "tools/agent/")
DASHES = ("–", "—")


def findings(paths: list[str], added: list[tuple[str, str]], branch: str, read=None) -> list[str]:
    """`read(path)`: the file as committed or staged (None: deleted), to check that TOML parses."""
    out = []
    worker = branch.startswith(("unit/", "review/"))
    for p in paths:
        if p.startswith(ALWAYS):
            out.append("%s: never changed by a commit (pinned originals or build output)" % p)
        elif worker and p.startswith(WORKER_BRANCH):
            out.append("%s: not changed on %s (a task branch the human asked for changes it)" % (p, branch))
    for path, line in added:
        if any(d in line for d in DASHES):
            out.append("%s: an en or em dash in an added line: %s" % (path, line.strip()[:80]))
    for p in paths:
        text = read(p) if read and p.endswith(".toml") else None
        if text is not None:
            try:
                tomllib.loads(text)
            except tomllib.TOMLDecodeError as e:
                out.append("%s: not valid TOML: %s (in a basic string, a backslash starts an escape: "
                           "write a backtick as it is, a backslash as \\\\)" % (p, e))
    return out


def diff_added(args: list[str], cwd: Path | None = None) -> list[tuple[str, str]]:
    out, path = [], ""
    for line in common.git("diff", "--no-color", "-U0", *args, cwd=cwd).splitlines():
        if line.startswith("+++ "):
            path = line[6:] if line.startswith("+++ b/") else line[4:]
        elif line.startswith("+") and not line.startswith("+++"):
            out.append((path, line[1:]))
    return out


def branch_findings(branch: str, cwd: Path | None = None, rev: str | None = None) -> list[str]:
    """What `branch` (or its commit `rev`) changes since it left main, judged by the branch's kind."""
    rev = rev or branch
    base = common.git("merge-base", "main", rev, cwd=cwd)
    paths = common.git("diff", "--name-only", base, rev, cwd=cwd).splitlines()
    def read(p):
        return common.git("show", "%s:%s" % (rev, p), cwd=cwd, check=False) or None
    return findings(paths, diff_added([base, rev], cwd), branch, read)


def staged_findings() -> list[str]:
    paths = common.git("diff", "--cached", "--name-only").splitlines()
    branch = common.git("branch", "--show-current", check=False)
    def read(p):
        return common.git("show", ":" + p, check=False) or None
    return findings(paths, diff_added(["--cached"]), branch, read)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--staged", action="store_true")
    ap.add_argument("--branch")
    a = ap.parse_args(argv)
    problems = staged_findings() if a.staged else branch_findings(a.branch or common.git("branch", "--show-current"))
    for p in problems:
        print("safety: " + p, file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
