# Review: e1aa6

Reviewer: rev-sonnet
Commit reviewed: 427c22f1f43a9db1d4fd16c14b6b09a1fbd4586f

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

- [ ] should src/E1AA6.C:7: `gDrawText` and `text` are not backed by a cited address in the file. The evidence exists: callers 082C:0864..087F and 082C:08C2..08FF push a far string (`push ds; mov ax,0FE6E` is a string buffer), then x and y (`di-1`, `si+1`, i.e. a one-pixel offset shadow pass with colour 0Ch followed by the real pass), then a colour (0Fh) and a flag word, through `lcall [80CA]`. Add that to the header comment, or keep the name as a placeholder as the header says.
- [ ] should src/E1AA6.C:7 and src/E1BA4.C:7: the same address ds:80CA is declared twice with two names (`gDrawText` with a typed prototype here, `gFn80CA` with `()` in E1BA4.C). Same bytes, but the next reader will see two globals. Use one name and one prototype; E1BA4.C's `long` variant calls the same pointer.
- [ ] should src/E1AA6.C:7: with `-ml` a plain `char *` is already far, so `char far *text` is redundant; harmless. If the aim is to make the far string visible, say so in the comment.
- [ ] note `PrintInt` is a sound name: 0000:62AA is itoa (checked in the disassembly: it sign-extends when radix == 10 and zero-extends otherwise, then calls the shared converter at 0000:6228 with the signed flag set), radix is 10, and the buffer goes to the text routine. `int value` (signed) is right (cdq on the radix 10 path). The 30 byte buffer matches `sub sp, 1Eh`.
- [ ] note Parameters `a`, `b`, `c`, `d` are the x, y, colour and mode/flag words of the text routine as used by the callers at 082C:090A (the same four-argument shape as the direct callers of [80CA] at 082C:0883, 08CE). They are placeholders and need no change until the routine installed at ds:80CA is found (it is zero in the static image; set at run time).
- [ ] note The caller list in the packet (082C:090A, 0BD2:0917, 1547:0991) shows the unit is used from overlay code; the header comment says "Placeholder names" which is accurate.

## Summary

The unit is a single 54 byte function, `itoa(value, buf, 10)` followed by a call through the far pointer at ds:80CA with the buffer and four ints. The source is short and understandable on its own. The runtime call and the global are justified by the disassembly and the callers. No must-fix: the names are either placeholders or supported by evidence. The only suggestions are to cite that evidence in the header and to unify the declaration of ds:80CA with src/E1BA4.C. `make match FILE=src/E1AA6.C` reports 1 of 1 function matching (54 bytes).
