# Review: e1ddd

Reviewer: rev-sonnet
Commit reviewed: 6c10036cf37829ec15d8f063a0782ba7546639b6

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E1DDD.C:1-2 and :11: the header calls the first argument of 0000:60BD "the count 16" and the prototype names it `n`. It is the interrupt number (10h, the video BIOS), not a count. Evidence: 0000:60BD is Borland `int86(intno, in, out)`: it calls 0000:66BB (segread: stores es, cs, ss, ds into an 8 byte buffer) and then 0000:60EF with the same intno, the two register unions and that buffer (int86x). 0000:60EF builds an `int N` stub on its stack: byte 0xCD at [bp-0xd] (0000:6105), N = low byte of [bp+6] (0000:6109..610C), then `lcall [bp-4]` at 0000:614B; the 0x25/0x26 test at 6114 is the absolute disk read/write special case. WHITEPAPER 3 (line 111) already says the mode number goes "through `int86` (1DDD:002D)". Rename the parameter (`intno`) and fix the comment.
  Fixed in ee48384. The prototype is now `extern void f60BD(int intno, void far *inregs, void far *outregs);` and the header describes 0000:60BD as Borland `int86(intno, inregs, outregs)` with the segread at 66BB, the `int N` stub at 60EF and the 25h/26h case, as in the finding.
- [x] must-fix src/E1DDD.C:5-6: the claim "the caller passes 7, so it always calls" is not supported. With i = 7 the code takes the else branch (ds:84A0) and then compares `g84A2[7]`, which is ds:84B0, one word past the 7 word table at 84A2..84AE (the image holds 0 there, 84A2..84AE are 4,9,0Dh,13h,13h,7,2), against ds:84A0. It calls only when ds:84A0 differs from the word at ds:84B0 (initially 0); ds:84A0 is argv[5], "the video mode restored on exit" (WHITEPAPER 115), so a restore mode of 0 would skip the call. All four callers push 7 (07C8:0199 via the table setup, 07C8:0625 at 0628, 19BC:02B6 at 02B9; 07C8:058A to be checked by the decompiler). Say what the code does (reads one word past the table; reads ds:84B0), and state that "always calls" holds only when ds:84A0 is nonzero. Also record that the table has 7 entries (84A2..84AE), so `g84A2[]` is unbounded in the declaration.
  Fixed in ee48384. The "always calls" claim is gone. The header now says the comparison reads `g84A2[7]` = ds:84B0, one word past the 7 entry table (g84A2[] is unbounded here), that ds:84B0 is 0 in the image, and that the call therefore happens only when ds:84A0 is nonzero. Re-checked the table in the image (ds:84A2..84AE = 4, 9, 0Dh, 13h, 13h, 7, 2; ds:84B0 = 0).
- [x] should src/E1DDD.C:3-4: now that 60BD is known to be int86 with intno 10h, say what the unit does: f1DDD_0006 is INT 10h AH=0Fh (get current video mode), returns AL (AH of ds:FF46 is 0Fh, the word is AX in; ds:FF36 is the out AX). f1DDD_002D is INT 10h AH=00h (set video mode) with AL = the mode from the table. FF46/FF36 are the in and out `union REGS` (only .x.ax is used, but they are a 16 byte union REGS if the neighbours at FF46/FF36 are laid out so; the file declares one word each). Names can stay placeholders, but they should be noted as [H] with this evidence.
  Fixed in ee48384. The header now names f1DDD_0006 as INT 10h AH=0Fh (get mode, returns AL) and f1DDD_002D as INT 10h AH=00h (set mode, AL = the mode from ds:84A2 or ds:84A0), and calls ds:FF46/ds:FF36 the in and out register union with the 16 byte union REGS note tagged [H]. Names stay placeholders.
- [x] should src/E1DDD.C:11: `f60BD` returns AX (int86 returns the output ax) but is declared void; callers here ignore it, fine, but note it is deliberate.
  Fixed in ee48384. A line before f1DDD_0006 now records that int86 returns the output AX and that the void here is deliberate because both callers ignore it.
- [x] note 1DDD:0000..0005 are 6 bytes before the first function (pop dx, cx, bx, ax, pop bp, retf): code, not padding, and not covered by the file (the unit starts at 1DDD:0006). Evidence 'segment' only; the next reader should find out whether it is a stub that Westwood's assembly shares.
  Recorded in ee48384: the header states the 6 bytes at 1DDD:0000..0005 (pop dx, cx, bx, ax; pop bp; retf) are code, not padding, and outside this unit.
- [ ] note `make match FILE=src/E1DDD.C` exits 0: 2 of 2 functions match, unit complete 106 bytes.

## Summary

The file matches and is short enough to read, but its header misreads the rtl call: 0000:60BD is `int86`, its first argument is the interrupt number 10h (not a count), so the unit is a pair of video mode BIOS calls (get mode, set mode from the ds:84A2 table), and the "always calls" remark hides a one word read past the table. Names are placeholders with no must-fix in themselves; the two must-fix items are comment and parameter name corrections.

Resolution (dec-deepseek-3, ee48384): the two must-fix items are done in that commit, and the two
should items and the note on 1DDD:0000..0005 as well. Comment and parameter-name only, no code
change; `make match FILE=src/E1DDD.C` still reports 2 of 2 functions and unit e1ddd complete
(ours 106 bytes, the unit 106). The table and the byte past it were re-read in the image:
ds:84A2..84AE = 4, 9, 0Dh, 13h, 13h, 7, 2 (7 entries) and ds:84B0 = 0.
