# Review: e1ddd

Reviewer: rev-haiku
Commit reviewed: 81b2233f24aa88499596cd2ea3dd4226d495da7b

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] should src/E1DDD.C:36: f1DDD_0006 keeps a placeholder name, but the bytes give evidence: it stores 0F00h in the in union (AH = 0Fh) and returns AL of the out AX. That is INT 10h AH=0Fh, get current video mode. Name it from that (for example `GetVideoMode`) and put the evidence in its comment.
- [ ] should src/E1DDD.C:43: f1DDD_002D keeps a placeholder name, but the evidence is there: INT 10h AH=00h (set video mode) with the mode taken from g84A2[i], or from g84A0 when i = 7. Callers: 07C8:01E4 and 022E push ds:8102 (the graphics mode, WHITEPAPER 111); 07C8:01C4, 0265, 0524, 05E8, 0628 and 19BC:02B9 push 7 (the restore). Name it from that (`SetGraphicsMode` fits the WHITEPAPER 111 wording), and keep both paths visible in the name's comment, so the restore path is not hidden.
- [ ] should src/E1DDD.C:36, 43: each function's comment is only its address (`/*@ 1DDD:0006 */`); the explanation sits in the unit header. READABILITY R3 wants a comment of 8 words or more right above each function, with its address. Move each function's paragraph there (`make readability UNIT=e1ddd` reports this; it is in the baseline, so the gate passes).
- [ ] should src/E1DDD.C:30: `unsigned g84A2[]` is unsized, but the i = 7 path reads g84A2[7] (ds:84B0, 0 in the image) in the comparison at line 49. A port must keep that read. Mark it `QUIRK 1DDD:002D:` (READABILITY, Quirks: looks wrong, not a bug, not fixed), and either size the array 8 with entry 7 described as outside the 7 word table, or say so in the declaration comment.
- [ ] should src/E1DDD.C:29-32: the four placeholder globals have address-only declaration comments. READABILITY (placeholder globals) wants what is known, and the unit header already has it: gFF46 and gFF36 are the AX word of the int86 in and out REGS unions; g84A0 is the restore mode from argv[5] (07C8:0069..007B: `les bx,[bp+8]`, push the far pointer, `lcall 0000:5D63`, `mov [84A0],ax`); g84A2 is the 7 word mode table (4, 9, 0Dh, 13h, 13h, 7, 2). Put that in each declaration, and name them with `make rename` (for example gRegsIn, gRegsOut, gRestoreMode, gModeTable), with the evidence in WHY.
- [ ] should src/E1DDD.C:28: "That routine is Borland's int86" has no [H] tag. I checked the shape on our bytes: 0000:60BD calls segread at 0000:66BB, builds an `int N` stub at 0000:60EF (0xCD at [bp-0xd], N from its first argument), and tests AL against 25h and 26h at 0000:6114. That supports the identity, but the name comes from recall (the header says the routine is not in build/re/rtl_compare.json). Tag it [H], or give the rtl-compare result.
- [ ] should src/E1DDD.C:34: the int86 comment sits between the declarations and f1DDD_0006. It belongs on the `f60BD` declaration (line 28).
- [ ] should docs/design/WHITEPAPER.md:115: it calls argv[5] `atol`. The bytes (07C8:0074 calls the routine with the far pointer argv[5]; 07C8:007B stores only AX) and this unit's header say an int result (atoi-like, [H]). Rule 7 wants the whitepaper and the unit to agree. Owner: the whitepaper (the human, or the 07C8 owner).
- [ ] should src/E19BC.C:23 and 148-154 (owner: e19bc, not this unit): (a) the extern is `int f1DDD_002D(int i)`, but this unit defines `void` and returns nothing, so the declaration should be `void`. (b) The OutOfMemory comment (148-151) says it "restores video mode 7". Per this unit, argument 7 means restore the mode in ds:84A0 (argv[5]); mode 7 is table entry 5 (g84A2[5] = 7), not entry 7. Not a blocker for this unit, but the e19bc owner should fix both.
- [ ] note 1DDD:0000..0005 (bytes 5a 59 5b 58 5d cb: pop dx, pop cx, pop bx, pop ax, pop bp, retf): checked in eob/EOB.EXE at file offset 0x205d0. It is the tail of the previous function, not padding, so the header is right to leave it out of the unit.
- [ ] note f1DDD_0006 has no far call site in the image: the bytes 9a 06 00 dd 1d occur 0 times (f1DDD_002D's bytes 9a 2d 00 dd 1d occur 8 times, as the header says). Keep that in its comment: it may be unreached.
- [ ] note the header's statement that ds:84B0 is 0 covers the image only. Nothing in this unit writes it, but a runtime write to ds:84B0 would change the restore path. Not checked here.
- [ ] note types: g84A0 is `unsigned`, but the header calls its source an int result (atoi-like). The code is the same either way; choose one and say why when the names are fixed.

## Summary

The unit is understandable from its header and source. It is two INT 10h video mode calls through the runtime int86 (0000:60BD): f1DDD_0006 reads the mode (AH=0Fh) and f1DDD_002D sets it (AH=00h), either from the 7 word table (index below 7) or from the restore mode (index 7). I checked the header against the bytes: the 6 bytes before the unit, the 8 call sites of f1DDD_002D (7 pushed at 6 sites, ds:8102 at 2), the argv[5] store, and the int86 stub at 0000:60BD to 60EF to 6114. All of them hold. No must-fix: the code, the callee's behaviour and the header's facts are right.

What is still open is naming, not truth. The two functions and four globals keep placeholder names, but the evidence to name them exists (the findings above), so these are `should` items. The unit's functions also lack the per-function comments R3 asks for. The biggest cross-unit issue is not in this unit: `src/E19BC.C` declares f1DDD_002D as `int` and says OutOfMemory "restores video mode 7", which reads argument 7 as a mode rather than the restore index. That is owned by e19bc and is noted above.

## Earlier rounds

Rounds 1 (a3ca08c) and 2 (f20f7ea) by rev-sonnet: their must-fix items (the int86 argument, the "always calls" claim, the callers' arguments) were all ticked off by ee48384, b6bbb6b and 27926ed. The text is in git history.
