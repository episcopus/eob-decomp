# Review: e1ba8

# Round P6 (2026-11-04), commit ce0608f

Reviewer: p6-rev-1
Commit reviewed: ce0608f (main; review/e1ba8 was ce0608f when this round began and no fast-forward was needed). The last review round was Round 1 at 8a768d4. Since 8a768d4, src/E1BA8.ASM changed by the Phase 6 rename: f1BA8_0005 is PadBeforeUnpack0, f1BA8_0006 is Unpack0, f1BA8_0072 is NormalizePtr and f1BA8_0081 is NormFar, in the PUBLIC/PROC/ENDP labels. The diff is 16 insertions and 16 deletions. The header comment is not changed. The names do not change the machine code.

Later round (playbook 70). Judged: the diff since 8a768d4 for src/E1BA8.ASM (the rename) and the names it introduces. The Round 1 shoulds 11 and 12 (the header argument layout and the DX:AX return) are unchanged by the P6 diff and remain shoulds.

Checked: `make match FILE=src/E1BA8.ASM` 4 of 4 (140 bytes), exit 0; `make readability UNIT=e1ba8` 0 findings. Evidence used this round: `git diff 8a768d4 HEAD -- src/E1BA8.ASM` (the rename); `grep -n -A7 "addr = \"1BA8:0005\"" re/renames.toml` and the 1BA8:0006, :0072 and :0081 entries (the four whys); `sed -n '1,12p' src/E1BA8.ASM` (the header).

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E1BA8.ASM:6 (the header): "Placeholder names." is false after the P6 rename. The four functions are PadBeforeUnpack0 (1BA8:0005, the lone 00 pad before Unpack0; re/renames.toml:7143), Unpack0 (1BA8:0006, the format 0 copy Unpack calls; re/renames.toml:24), NormalizePtr (1BA8:0072, the by-value far pointer normalizer returning DX:AX; re/renames.toml:7602) and NormFar (1BA8:0081, the register-convention normalizer; re/renames.toml:33). Drop "Placeholder names." or say the names and their evidence in the header. Fixed in 6af4a72: the header names the four functions with what backs each, in the same six lines.
- [ ] note the Round 1 shoulds 11 and 12 (the header should give the argument layout and say NormalizePtr returns DX:AX) are still open; the P6 diff did not change the header. The P6 names make the missing argument layout more visible, but the shoulds are not must-fix.
- [ ] note the four names are backed: PadBeforeUnpack0 (the 00 byte before the even entry Unpack0; the whitepaper 2.6 word-alignment pad in E1BBE, I0CDA and E1D9A), Unpack0 (the format 0 stored copy), NormalizePtr (the by-value normalizer) and NormFar (the register normalizer). The P6 diff's changed lines are the PUBLIC/PROC/ENDP labels; `make match` is 4 of 4 (140 bytes), so the bytes are unchanged.
- [ ] note the header's line 2 "one byte the map calls a function (a segment-start marker, not code)" is consistent with the name PadBeforeUnpack0; the name says the byte is a pad, the header says a segment-start marker. Both are readings of the same 00; no change needed.

## Summary

One must-fix item is open. The P6 rename named all four functions (PadBeforeUnpack0, Unpack0, NormalizePtr and NormFar), but the header still ends "Placeholder names." The names are backed by re/renames.toml and the callers. The matcher is 4 of 4 and readability is 0 findings; the bytes are unchanged. The unit does not pass (review-failed) until the header sentence is rewritten.

Reviewer: rev-sonnet
Commit reviewed: 8a768d4e9ef29cba163e92f97450590f9d2c6d35

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

- [ ] should src/E1BA8.ASM:1-6: the header describes what 1BA8:0006 does but not how it is called. The frame shows (far proc, `bp+6` is the first argument): `[bp+6],[bp+8]` = source offset, segment (loaded into ds:si), `[bp+0Ah],[bp+0Ch]` = destination offset, segment (es:di), `[bp+0Eh],[bp+10h]` = a 32-bit byte count, low word in CX (copied first, as words plus a trailing `movsb`), high word in BX (the number of 64 KiB blocks, `cx = 8000h` words each). The caller 1B82:00E7..0107 confirms it: it pushes `[src+4]`, `[src+2]` (the 32-bit size from the packed header), then dst seg and off, then `src+8` (seg, off), and cleans up 0Ch bytes. Add this to the header; it is the whole point of the routine.
- [ ] should src/E1BA8.ASM:1-6: 1BA8:0072 returns the normalized pointer in DX:AX (offset in AX, which is `& 0Fh`, and segment in DX). Evidence: its caller MemAlloc at 19BC:0080..0088 pushes a far pointer (seg, off), calls 1BA8:0072, cleans up 4 bytes and stores AX, DX as the new pointer ([bp-4], [bp-2]). Say "returns DX:AX" in the comment.
- [ ] should src/E1BA8.ASM:6: "Placeholder names" is fair, but the evidence for a role name is already there: 1BA8:0006 is the copy used by case 0 (stored, no compression) of the unpacker at 1B82:00C7, and 1BA8:0072 is the pointer normalization applied by MemAlloc (19BC:0007) and FileRead (1C0E:0400). A later pass could name them (for example a huge-pointer copy and a huge-pointer normalize); not required now.
- [ ] note 1BA8:0006 assumes the direction flag is clear and does not handle overlapping ranges (a forward copy only). It saves and restores AX, BX, CX, DX, SI, DI, DS and ES, so it is safe to call from code that keeps values in registers. These facts are visible in the code and need no change.
- [ ] note 1BA8:0081 (register convention: AX = offset, DX = segment in; AX = offset & 0Fh, DX = segment + (offset >> 4) out) is correct as read: `push ax; shr ax,1 x4; add dx,ax; pop ax; and ax,0Fh`. It is called with `push cs; call near ptr` even though it is declared `FAR` (ends in `retf`); this is how the original bytes are, so keep it.
- [ ] note 1BA8:0005 is one byte `00`. The map calls it a function only because it is the start of the segment (evidence `segment` in tools/re/functions.py); the neighbouring bytes at 1BA8:0000..0004 (`0c 8b .. e5 5d cb`, ending in `pop bp; retf`) are the tail of the previous module (E1BA4), so this module starts at 1BA8:0005 and the file is right to keep the byte as data in a `LABEL BYTE`. The header says this; the claim is supported.
- [ ] note The caller of the 16-byte normalizer 1BA8:0081 from other segments (19FC:0007, 1D9A:0013) shows it is also used as a plain far routine, so keeping it `PUBLIC` is correct.
- [ ] note src/E1B82.C:18 declares 1BA8:0006 as `Unpack0(char far *src, void far *dst, long arg)`. `arg` is the byte count (32 bit); that unit could name it `len`. Not a finding for this file.

## Summary

The unit is four small functions that are understandable from the source: a far memory copy with a 32-bit byte count that renormalizes both far pointers before the first copy and before each 64 KiB block, a by-value far pointer normalizer (DX:AX result), and the register-convention normalizer they share. The header text is correct (checked against the code and the callers at 1B82:00E7, 19BC:0080); it is only missing the argument layout and the return registers, which are suggestions. Names are placeholders and no name claims more than the evidence shows. No must-fix. `make match FILE=src/E1BA8.ASM` reports 4 of 4 functions matching (140 bytes, unit complete).
