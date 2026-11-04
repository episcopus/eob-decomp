# Review: <unit id>

Reviewer: p6-rev-5
Commit reviewed: ce0608f367ed01b1b1df84e254eef04d1a734b27 (review/e1aaa HEAD at the start of this round; src/E1AAA.C last changed in 2e8c6a1c, the P6 readability fix; `make match FILE=src/E1AAA.C`: 1 of 1, 55 bytes, exit 0; `make readability UNIT=e1aaa`: 0 findings)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

## Round P6 (2026-11-04), commit ce0608f367ed01b1b1df84e254eef04d1a734b27

Reviewer: p6-rev-5. Commit reviewed: ce0608f367ed01b1b1df84e254eef04d1a734b27 (review/e1aaa HEAD at the start of this round; src/E1AAA.C last changed in 2e8c6a1c, the P6 readability fix). The diff from the commit 427c22f1 to HEAD touches src/E1AAA.C in the P6 readability pass: it removes the "placeholders" sentence from the header, adds the function comment at 1AAA:0005 and renames the local `r` to `roll`. `make match FILE=src/E1AAA.C`: 1 of 1, 55 bytes, exit 0; `make readability UNIT=e1aaa`: 0 findings.

### Findings (round P6)

- [ ] should src/E1AAA.C:1-3 (header): the header still calls `rand` "the LCG" without the evidence, and the P6 diff removed the "placeholders" sentence but did not add the one-line justification the round 1 review asked for. The evidence is the Borland LCG at 0000:55B7 (seed ds:8784/8786, multiplier 015A4E35h, add 1, high word & 0x7FFF; range 0..32767), which justifies the divisor 32767 and the hi+1 rejection. Say that in the header.
- [ ] note src/E1AAA.C:13-14: the new function comment "a random number from lo to hi inclusive: rand() scaled into the range, drawn again in the one case the scaling gives hi + 1" matches the code (:21-26).
- [ ] note src/E1AAA.C:18: the local `r` is now `roll`; the name is a role, not a placeholder, and fixes the readability rule R4 for single-letter locals.

Checked and right (no finding): the name `RandomRange`, the `(lo, hi)` signature and the unsigned types are unchanged and still backed by the call sites (lo is 0 at the six callers in 0EF3, hi is the inclusive maximum); the edge-case note (lo=0, hi=65535 gives 0; hi < lo wraps) is unchanged and matches the 16-bit arithmetic.

### Summary (round P6)

The P6 diff is a readability fix: the function comment is accurate and the local `roll` is a better name. No must-fix: the one should is the round 1 request to state the `rand` LCG evidence in the header, which the P6 diff left open. No matcher or spec question; nothing escalated. The reviewer did not edit src/.

- [ ] should src/E1AAA.C:1-3: the header calls `rand` "the LCG" without saying why; the evidence is on our bytes. 0000:55B7 loads the 32 bit seed at ds:8784/8786, multiplies by 015A4E35h (`ba 5a 01 b8 35 4e`, call 0000:7737 pattern of the long multiply), adds 1, stores it back, and returns the high word masked with 7FFFh (`25 ff 7f`, `cb`). That is the Borland `rand()` with range 0..32767 inclusive, which also justifies the divisor 32767 and the `hi+1` rejection. One line in the header would let the next reader skip re-deriving it. (Symbol 55B7 has size 155, so it also holds `srand` and neighbours; only the first 0x24 bytes are `rand`.)
- [ ] note The comment "rejects the one overflow value hi+1" is correct: n = hi-lo+1, rand()*n/32767 equals n only when rand() == 32767, so r == hi+1 then and `r > hi` loops. Checked against the function bytes at 1AAA:0005..003B (`33 c0 ba ff 7f 50 52` pushes the long 32767, `9a 37 77` multiply, `9a 51 77` divide, `3b 46 08 77 d0` loops back).
- [ ] note The callers support the signature (lo, hi) with unsigned inclusive bounds. Six call sites in the 0EF3 code (file offsets 0x142A0, 0x142C2, 0x142FA, 0x146B1, 0x147D0, 0x147FC) each push `xor ax,ax; push ax` (lo = 0) immediately before `call far 1AAA:0005`, after pushing hi first (for example `push [bp+8]`, or `ax>>2`, or `0x400 * x`). So in practice lo is always 0 and hi is an inclusive maximum. The name `RandomRange` states what the code does and is fine; the file does not need to claim more.
- [ ] note Edge case, not a bug to fix (original behaviour): with lo=0 and hi=65535 the 16 bit `hi-lo+1` is 0, so the result is always lo. `hi < lo` wraps similarly. No caller seen passes either.
- [ ] note `i0a15` in INTRO.EXE is `same_as = "e1aaa"` in re/units.toml (same 55 bytes), so a change here applies there too.

## Summary

One 55 byte far function, understandable from its source alone. The name `RandomRange` and the types follow from the code and the call sites (lo is always 0 at the six callers seen). The only suggestion is a one line statement of the evidence for `rand` being the Borland LCG. No must-fix.
