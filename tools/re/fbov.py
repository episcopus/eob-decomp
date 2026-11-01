#!/usr/bin/env python3
"""fbov.py: manifest of a Borland C++ (VROOMM) executable: TLINK segment table and FBOV overlays.

Turbo C++ appends the overlay code to the EXE after the load module (whitepaper 2.3):

  tail +00 "FBOV"
       +04 u32  bytes of overlay data after this 16-byte header
       +08 u32  file offset of the segment table (inside the load module)
       +0C u32  segment table entries
       +10      overlay records, one per overlaid segment: code, then u16 fixup offsets,
                then zero padding up to the next record

  segment table entry (8 bytes): u16 paragraph, u16 last offset (size - 1, FFFF for none),
                                 u16 kind, u16 flags (meaning not established)
  kind: 0 data, 1 code, 3 overlay stub, 4 meaning not established (stack, BSS?)

  overlay stub segment (in the load module), same layout as Turbo Pascal's:
       +00 CD 3F, +02 u16 runtime, +04 u32 offset of the record (from tail +10),
       +08 u16 code bytes, +0A u16 fixup bytes, +0C u16 entries, +0E u16 link,
       +10..+1F runtime fields (zero on disk), +20 entries: CD 3F, u16 code offset, u8 0

Usage:
  fbov.py EXE [-o manifest.json] [--extract DIR]
Exit code 0 when the manifest validates, 1 otherwise.
"""
from __future__ import annotations

import argparse, hashlib, json, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mzutil import read_mz  # noqa: E402

KIND = {0: "data", 1: "code", 3: "overlay-stub", 4: "kind4"}


def segment_table(data: bytes, mz: dict, offset: int, count: int) -> list[dict]:
    out = []
    for i in range(count):
        para, last, kind, flags = struct.unpack_from("<4H", data, offset + 8 * i)
        out.append({"index": i, "para": para, "size": 0 if last == 0xFFFF else last + 1,
                    "last": last, "kind": kind, "kind_name": KIND.get(kind, "kind%d" % kind),
                    "flags": flags})
    return out


def stub(image: bytes, para: int) -> dict:
    p = para * 16
    fo, code, fix, nent, link = struct.unpack_from("<IHHHH", image, p + 4)
    ents = []
    for i in range(nent):
        q = p + 0x20 + 5 * i
        op, off, z = struct.unpack_from("<2sHB", image, q)
        ents.append({"index": i, "code_offset": off, "ok": op == b"\xcd\x3f" and z == 0})
    return {"stub_para": para, "record_offset": fo, "code_bytes": code, "fixup_bytes": fix,
            "entry_count": nent, "link": link, "runtime_zero": not any(image[p + 0x10:p + 0x20]),
            "entries": ents}


def manifest(data: bytes, extract: str | None = None) -> tuple[dict, list[str]]:
    mz = read_mz(data)
    img, tail, errors = mz["image"], mz["tail"], []
    m = {"schema": "eob-fbov-v1", "sha256": hashlib.sha256(data).hexdigest(),
         "tlink": mz["tlink"], "load_module_bytes": len(img), "relocations": mz["nreloc"],
         "entry": "%04X:%04X" % (mz["cs"], mz["ip"]), "stack": "%04X:%04X" % (mz["ss"], mz["sp"]),
         "segments": [], "overlays": []}
    if not tail:
        m["fbov"] = None
        return m, errors
    if tail[:4] != b"FBOV":
        return m, ["data after the load module does not start with FBOV"]
    size, tab, count = struct.unpack_from("<III", tail, 4)
    body = tail[16:]
    m["fbov"] = {"bytes": size, "segment_table_offset": tab, "segment_table_entries": count}
    if size != len(body):
        errors.append("FBOV size %d != %d bytes after its header" % (size, len(body)))
    if not mz["header_bytes"] <= tab and tab + 8 * count <= mz["size"]:
        errors.append("segment table outside the load module")
        return m, errors
    m["segments"] = segs = segment_table(data, mz, tab, count)
    paras = [s["para"] for s in segs]
    if paras != sorted(paras):
        errors.append("segment table is not in address order")
    if (tab - mz["header_bytes"]) % 16 or (tab - mz["header_bytes"]) // 16 not in paras:
        errors.append("segment table does not start a segment")
    stubs = sorted({s["para"] for s in segs if s["kind"] == 3 and s["size"] > 0})
    pos = 0
    for n, para in enumerate(stubs):
        o = stub(img, para)
        o["overlay"] = n
        if img[para * 16:para * 16 + 2] != b"\xcd\x3f":
            errors.append("overlay stub %04X has no INT 3Fh" % para)
        if not o["runtime_zero"] or not all(e["ok"] for e in o["entries"]):
            errors.append("overlay stub %04X: malformed header or entries" % para)
        start, end = o["record_offset"], o["record_offset"] + o["code_bytes"] + o["fixup_bytes"]
        if start < pos:
            errors.append("overlay %d at %d overlaps the previous record" % (n, start))
        elif any(body[pos:start]):
            errors.append("non-zero padding before overlay %d" % n)
        code = body[start:start + o["code_bytes"]]
        fix = list(struct.unpack_from("<%dH" % (o["fixup_bytes"] // 2), body, start + o["code_bytes"]))
        o["fixups"] = fix
        if o["fixup_bytes"] % 2 or any(f + 2 > len(code) for f in fix):
            errors.append("overlay %d has fixups outside its code" % n)
        if any(e["code_offset"] >= len(code) for e in o["entries"]):
            errors.append("overlay %d has an entry outside its code" % n)
        o["code_sha256"] = hashlib.sha256(code).hexdigest()
        m["overlays"].append(o)
        pos = end
        if extract:
            os.makedirs(extract, exist_ok=True)
            open(os.path.join(extract, "ovl%02d.bin" % n), "wb").write(code)
    if any(body[pos:]):
        errors.append("non-zero bytes after the last overlay")
    m["overlay_code_bytes"] = sum(o["code_bytes"] for o in m["overlays"])
    m["overlay_entries"] = sum(o["entry_count"] for o in m["overlays"])
    return m, errors


def summary(m: dict) -> str:
    kinds = {}
    for s in m["segments"]:
        kinds[s["kind_name"]] = kinds.get(s["kind_name"], 0) + 1
    return "%d segments (%s), %d overlays, %d entries, %d overlay code bytes" % (
        len(m["segments"]), ", ".join("%d %s" % (v, k) for k, v in sorted(kinds.items())),
        len(m["overlays"]), m.get("overlay_entries", 0), m.get("overlay_code_bytes", 0))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("exe")
    ap.add_argument("-o", "--out")
    ap.add_argument("--extract")
    a = ap.parse_args(argv)
    m, errors = manifest(open(a.exe, "rb").read(), a.extract)
    m["path"] = a.exe
    if a.out:
        os.makedirs(os.path.dirname(a.out) or ".", exist_ok=True)
        json.dump(m, open(a.out, "w"), indent=1)
    print("%s: %s; %s" % (os.path.basename(a.exe), summary(m),
                          "OK" if not errors else "%d ERRORS" % len(errors)))
    for e in errors:
        print("  error: " + e)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
