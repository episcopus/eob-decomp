#!/usr/bin/env python3
"""tc_toolchain.py: install and run the candidate Borland toolchains (docs/setup/SETUP-tc.md).

  install [--force]           unpack every release in tools/re/tc_toolchain.toml from build/toolchain/dist/
                              (7z -> FAT12 floppy images -> PKZIP archives) into build/toolchain/<release>/
  check                       verify the installed files against the pinned hashes
  run RELEASE TOOL [ARGS..]   run BIN/<TOOL>.EXE of a release under emu2 in the current directory,
                              with INCLUDE and LIB of that release on drive D: (paths D:\\INCLUDE, D:\\LIB)

Needs: py7zr (uv sync), Info-ZIP `unzip` (PKZIP 1.x implode), emu2 (make emu2).
"""
from __future__ import annotations

import argparse, hashlib, io, os, shutil, subprocess, sys, tempfile, tomllib
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fat12 import Fat12  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
DIST = ROOT / "build" / "toolchain" / "dist"
TOOLCHAIN = ROOT / "build" / "toolchain"
MANIFEST = ROOT / "tools" / "re" / "tc_toolchain.toml"
EMU2 = ROOT / "tools" / "vendor" / "emu2" / "emu2"


def sha256(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def manifest() -> dict:
    return tomllib.loads(MANIFEST.read_text())


def disk_files(archive: Path) -> dict[str, bytes]:
    """Every file on every floppy image in a 7z archive, by upper-case 8.3 name."""
    import py7zr
    out: dict[str, bytes] = {}
    with tempfile.TemporaryDirectory() as tmp:
        with py7zr.SevenZipFile(archive) as z:
            z.extractall(tmp)
        for img in sorted(Path(tmp).rglob("*.img")):
            for f in Fat12(img.read_bytes()).files():
                out[f["path"].upper()] = f["data"]
    return out


def unzip_all(data: bytes, dest: Path) -> list[str]:
    if not shutil.which("unzip"):
        raise SystemExit("install: Info-ZIP `unzip` not found")
    dest.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(suffix=".zip") as t:
        t.write(data)
        t.flush()
        r = subprocess.run(["unzip", "-o", "-qq", "-LL", t.name, "-d", str(dest)], capture_output=True)
        if r.returncode not in (0, 1):
            raise SystemExit("install: unzip failed: %s" % r.stderr.decode()[:300])
        names = subprocess.run(["unzip", "-Z1", t.name], capture_output=True, text=True).stdout.split()
    # DOS tools want upper-case names; -LL lower-cased them, so rename back
    for p in sorted(dest.rglob("*"), key=lambda p: -len(p.parts)):
        if p.name != p.name.upper():
            p.rename(p.with_name(p.name.upper()))
    return names


def install(force: bool) -> int:
    m, bad = manifest(), 0
    for rel, r in m["release"].items():
        target = TOOLCHAIN / rel
        archive = DIST / r["archive"]
        if not archive.exists():
            print("skip    %s: %s not in build/toolchain/dist" % (rel, r["archive"]))
            continue
        if sha256(archive.read_bytes()) != r["archive_sha256"]:
            print("CHANGED %s: %s does not match its pinned hash" % (rel, r["archive"]))
            bad += 1
            continue
        if target.exists() and not force:
            print("ok      %s: already installed (--force to redo)" % rel)
            continue
        shutil.rmtree(target, ignore_errors=True)
        files = disk_files(archive)
        for zipname, sub in r["zips"].items():
            if zipname not in files:
                print("MISSING %s: %s on the disks" % (rel, zipname))
                bad += 1
                continue
            unzip_all(files[zipname], target / sub)
        print("ok      %s: installed in %s" % (rel, target.relative_to(ROOT)))
    return check() if not bad else 1


def check() -> int:
    m, bad, n = manifest(), 0, 0
    for rel, r in m["release"].items():
        target = TOOLCHAIN / rel
        if not target.exists():
            continue
        for path, h in r.get("files", {}).items():
            p = target / path
            n += 1
            if not p.exists():
                print("MISSING %s/%s" % (rel, path)); bad += 1
            elif sha256(p.read_bytes()) != h:
                print("CHANGED %s/%s" % (rel, path)); bad += 1
    for key in ("pinned", "pinned_asm"):
        if not (TOOLCHAIN / m[key]).exists():
            print("MISSING %s release %s (make tc-install)" % (key, m[key])); bad += 1
    print("toolchain: %d pinned files checked, pinned %s + %s, %s" % (
        n, m["pinned"], m["pinned_asm"], "OK" if not bad else "%d problems" % bad))
    return 1 if bad else 0


def run(release: str, tool: str, args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess:
    """Run a DOS tool of a release under emu2; drive C: is `cwd`, drive D: the release."""
    target = TOOLCHAIN / release
    exe = target / "BIN" / (tool.upper() + ("" if "." in tool else ".EXE"))
    if not exe.exists():
        raise SystemExit("run: %s not installed (make tc-install)" % exe.relative_to(ROOT))
    if not EMU2.exists():
        raise SystemExit("run: emu2 not built (make emu2)")
    env = dict(os.environ, EMU2_DRIVE_C=str(cwd or Path.cwd()), EMU2_DRIVE_D=str(target),
               EMU2_DEFAULT_DRIVE="C")
    return subprocess.run([str(EMU2), str(exe)] + args, env=env, capture_output=True, cwd=cwd)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sp = ap.add_subparsers(dest="cmd", required=True)
    i = sp.add_parser("install"); i.add_argument("--force", action="store_true")
    sp.add_parser("check")
    r = sp.add_parser("run"); r.add_argument("release"); r.add_argument("tool"); r.add_argument("args", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    if a.cmd == "install":
        return install(a.force)
    if a.cmd == "check":
        return check()
    p = run(a.release, a.tool, a.args)
    sys.stdout.write(p.stdout.decode("latin-1"))
    sys.stderr.write(p.stderr.decode("latin-1"))
    return p.returncode


if __name__ == "__main__":
    sys.exit(main())
