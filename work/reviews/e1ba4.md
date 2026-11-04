# Review: e1ba4

Reviewer: p6-rev-2
Commit reviewed: d3391d6da34006ee3c1476339fac7460db591f8e (P6 round; the earlier round read df97906)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Round P6 (2026-11-04), commit d3391d6da34006ee3c1476339fac7460db591f8e

Judged the diff df97906..d3391d6 on src/E1BA4.C: the header drops "Placeholder names";
gFn80CA -> gDrawText (ds:80CA); f1BA4_000C -> PrintLong (1BA4:000C); the parameters are
renamed a, b, c, d -> x, y, colour, flag; a PrintLong comment is added.

Evidence checked against re/renames.toml (ds:80CA=gDrawText: the callers pass a far string,
x, y, a colour and a flag; 1BA4:000C=PrintLong: ltoa, gDrawText, the long twin of PrintInt,
RedrawMember 1547:0991 prints the class experience totals; no struct field in re/fields.toml).
The names match: gDrawText is the text routine, PrintLong is the long twin of PrintInt, and
x, y, colour, flag are the caller's arguments at 1547:1075 (expValue, x, y, 0xf, gAltPanelFill).

The earlier round's duplicate-name should is partly resolved: the name is gDrawText in both
files, but E1BA4.C still declares it with (); use the typed prototype from src/E1AA6.C:7. The
other should also remains: the header still says "installed at run time" although no direct
store to ds:80CA was found.

- [ ] should (carried) src/E1BA4.C:7: gDrawText is now the name in both files, but this
  declaration is still () while src/E1AA6.C:7 has the typed prototype; use the typed one.
- [ ] should (carried) src/E1BA4.C:1-4: "installed at run time" still implies a known
  installer; no direct store to ds:80CA was found, so say "set by something not yet found".
- [ ] note The new names x, y, colour, flag match the caller at 1547:1075 and the typed
  gDrawText prototype; the PrintLong comment is accurate.
- [ ] note The earlier round's note that this is the long twin of PrintInt is now reflected in
  the name PrintLong; the types note (long value, 30 byte buffer) still holds.

No open must-fix. The names are backed by the evidence and the callers use them consistently.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] should src/E1BA4.C:6-7: the same global ds:80CA is declared twice in `src/` with different names and types: `gFn80CA` as `void (far *)()` here, `gDrawText` as `void (far *)(char far *text, int a, int b, int c, int d)` in src/E1AA6.C:7. Both units call it the same way (far pointer to the buffer, then four words, caller cleans 0Ch bytes: 1BA4:0029..003E and the E1AA6 twin). Reuse `gDrawText` and its prototype so the name is one in `src/`; the packet says to reuse names src/ already gives.
- [ ] should src/E1BA4.C:1-4: "installed at run time (its static value in EOB.EXE is 0)": the 0 is right (the four bytes at file offset 0x2800+0x212E0+0x80CA are 00 00 00 00), but I found no direct store to ds:80CA or ds:80CC in EOB.EXE code (only `lcall [80CA]` sites, 48 of them, byte scan for `ca 80` and `cc 80`). Who installs it is not established; say "set by something not yet found" rather than imply it is known.
- [ ] note The caller claim is true: `lcall 1BA4:000C` is at 1547:1075, inside 1547:0991. There the stack args are: the long taken from the record at [bp-14h]/[bp-12h]; `a` = F7h minus the result of `lcall 082C:0974` on that same long (looks like a width, so a right aligned x at 247; [H]); `b` = 7*i+91h (a row y); `c` = 0Fh; `d` = byte ds:FB7B. So `a` and `b` are probably x and y, `c` a colour and `d` a mode or flag. Names `a`..`d` are fine as placeholders until that is proven.
- [ ] note Types: `long value` is right (ltoa at 0000:62F1 gets `[bp+8]:[bp+6]` then the near-in-large-model `ss:bp-1Eh` buffer, radix 10 pushed first); the 30 byte buffer (`sub sp, 1Eh`) holds the 12 character worst case of a long. The "draws screen text" label of the callee rests on E1AA6's guess and the argument shape (string plus x, y, colour like values), not on the callee body; it is plausible and the file does call it a placeholder.
- [ ] note This is the `long` twin of src/E1AA6.C `PrintInt` (itoa at 0000:62AA); the two differ only in the conversion and the argument width. A later name could be `PrintLong`.

## Summary

The unit is one 57 byte function and is understandable from its source alone; the header comment matches the bytes (ltoa, 30 byte buffer, far pointer call with four word args, one caller at 1547:1075). Nothing claims more than the evidence except the "installed at run time" remark, and the global ds:80CA has a second, better typed name in E1AA6.C. `f1BA4_000C` is a placeholder with no evidence yet for a real name. No must-fix.
