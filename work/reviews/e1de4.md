# Review: e1de4

Reviewer: p6-rev-2
Commit reviewed: b08c0a8d7bcd68a111e511064630da3a2512de67 (P6 round; the earlier round read cbfe8c4)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Round P6 (2026-11-04), commit b08c0a8d7bcd68a111e511064630da3a2512de67

Judged the diff cbfe8c4..b08c0a8 on src/E1DE4.ASM: _f1DE4_0000 -> _IsInVerticalRetrace
(1DE4:0000), _f1DE4_000D -> _WaitRetrace (1DE4:000D). The comments are unchanged.

Evidence checked against re/renames.toml (1DE4:0000=IsInVerticalRetrace, 1DE4:000D=WaitRetrace;
no struct field in re/fields.toml). The names match their WHYs: IsInVerticalRetrace reads port
3DAh bit 3 once and returns 8 or 0; WaitRetrace polls bit 3 twice, for the start and end of
retrace, or with argument 1 in the other order. The callers in src/E1A65.C and src/E1A16.C use
the same names (CalibrateRetrace calls IsInVerticalRetrace, WaitFrames calls WaitRetrace).

The earlier round's three shoulds are unchanged by this diff and still stand: the
"(into active display)" parenthetical is wrong; the caller comment says "rate" where the caller
does a polarity test; and the VGA/vertical-retrace labels are prior knowledge and should be
tagged [H].

- [ ] should (carried) src/E1DE4.ASM:23: "(into active display)" is wrong; say "waits until
  bit 3 is clear, then until it is set", as the earlier round asked.
- [ ] should (carried) src/E1DE4.ASM:9: "counts set and clear samples to measure the retrace
  rate" overstates; the caller does a polarity test. Say so and name the gRetraceInverted link.
- [ ] should (carried) src/E1DE4.ASM:1-3: "VGA retrace support" and "(vertical retrace)" are
  prior knowledge, not bytes; tag them [H], as the earlier round asked.
- [ ] note The new names are backed and the callers use them (src/E1A65.C CalibrateRetrace and
  src/E1A16.C WaitFrames).
- [ ] note The earlier round's notes still hold: the byte match (AL & 8, AH cleared; only the
  low byte of [bp+6] compared), the no-timeout loops, and the `je done` after the first loop.

No open must-fix. The names are backed by the evidence and the callers use them consistently.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] should src/E1DE4.ASM:14: "(into active display)" is wrong. Mode 1 waits for bit 3 to clear and then to set again (1DE4:002A..0032), so it returns at the start of the bit-set phase, not in active display. Say "waits until bit 3 is clear, then until it is set". The comment also calls the phases "end/start of retrace", which is only true when bit 3 means retrace; see the next finding.
- [ ] should src/E1DE4.ASM:10-11: "counts set and clear samples to measure the retrace rate" overstates what the caller does. 1A65:000E..00CA waits for the BIOS tick counter at 0040:006C to change, then calls 1DE4:0000 10000 times (cmp 0x2710 at 1A65:00BB), counts nonzero results in [bp-0xc] and zero results in [bp-0xa], and sets gFE6C (ds:FE6C) to 1 only if the set count is larger (1A65:00C8..00CA); otherwise it stays 0 (1A65:0022). That is a polarity test of bit 3 (mostly set or mostly clear over a timer tick), not a rate. gFE6C is what WaitFrames (1A16:0010) pushes as the [bp+6] argument of 1DE4:000D, which is why the comment on the mode argument matters. A short note on the polarity link would help the next reader.
- [ ] should src/E1DE4.ASM:1-3: "VGA retrace support" and "(vertical retrace)" come from the usual meaning of port 3DAh and bit 3, not from anything in the bytes. The code only proves "read port 3DAh, test bit 3". The caller's polarity test (finding above) suggests the author was unsure which way bit 3 reads on some hardware. Tag the VGA claim [H], as the "kind asm" claim is.
- [ ] note Bytes match the comments: 1DE4:0000 returns AL & 8 with AH cleared, so 0 or 8, only tested for zero by its caller (1A65:00AC `or ax,ax`). 1DE4:000D saves AX, BX and DX (not CX, ES or flags), and only the low byte of [bp+6] is compared with 1 (`mov bl,[bp+6]`, `cmp bl,1`), so the argument is an int in the caller (E1A16.C `int mode`) but a byte here. The `je done` after the first wait loop (1DE4:0028) is an unconditional jump in effect (ZF is set after the `jne` falls through); the source's `je done` is what the original has, not a typo.
- [ ] note Both loops have no timeout. If bit 3 never changes (a port that reads a constant) 1DE4:000D hangs; that is the original's behaviour, not to be fixed. The same applies to the 10000-sample loop in the caller, which is bounded.
- [ ] note Names `_f1DE4_0000` and `_f1DE4_000D` are address placeholders and E1A16.C already uses `f1DE4_000D` in its extern, so they are consistent across src/. The caller name `WaitFrames` is in E1A16.C and fits the use. A real name for 1DE4:0000 should wait for the 1A65 unit (kind unknown, not yet lifted); a name for 1DE4:000D such as WaitRetrace would be justified by the bytes, but it stays a placeholder here, which is fine.

## Summary

The unit is two small far functions (57 bytes) and is understandable from its source alone. The code is accurately described apart from one wrong parenthetical ("into active display") and a caller description that says "rate" where the caller does a polarity test; the "VGA" and "vertical retrace" labels rest on prior knowledge of port 3DAh, not on bytes. Names are address placeholders, which is fine. No must-fix.
