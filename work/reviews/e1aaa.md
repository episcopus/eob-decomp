# Review: <unit id>

Reviewer: rev-sonnet
Commit reviewed: 427c22f1f43a9db1d4fd16c14b6b09a1fbd4586f

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] should src/E1AAA.C:1-3: the header calls `rand` "the LCG" without saying why; the evidence is on our bytes. 0000:55B7 loads the 32 bit seed at ds:8784/8786, multiplies by 015A4E35h (`ba 5a 01 b8 35 4e`, call 0000:7737 pattern of the long multiply), adds 1, stores it back, and returns the high word masked with 7FFFh (`25 ff 7f`, `cb`). That is the Borland `rand()` with range 0..32767 inclusive, which also justifies the divisor 32767 and the `hi+1` rejection. One line in the header would let the next reader skip re-deriving it. (Symbol 55B7 has size 155, so it also holds `srand` and neighbours; only the first 0x24 bytes are `rand`.)
- [ ] note The comment "rejects the one overflow value hi+1" is correct: n = hi-lo+1, rand()*n/32767 equals n only when rand() == 32767, so r == hi+1 then and `r > hi` loops. Checked against the function bytes at 1AAA:0005..003B (`33 c0 ba ff 7f 50 52` pushes the long 32767, `9a 37 77` multiply, `9a 51 77` divide, `3b 46 08 77 d0` loops back).
- [ ] note The callers support the signature (lo, hi) with unsigned inclusive bounds. Six call sites in the 0EF3 code (file offsets 0x142A0, 0x142C2, 0x142FA, 0x146B1, 0x147D0, 0x147FC) each push `xor ax,ax; push ax` (lo = 0) immediately before `call far 1AAA:0005`, after pushing hi first (for example `push [bp+8]`, or `ax>>2`, or `0x400 * x`). So in practice lo is always 0 and hi is an inclusive maximum. The name `RandomRange` states what the code does and is fine; the file does not need to claim more.
- [ ] note Edge case, not a bug to fix (original behaviour): with lo=0 and hi=65535 the 16 bit `hi-lo+1` is 0, so the result is always lo. `hi < lo` wraps similarly. No caller seen passes either.
- [ ] note `i0a15` in INTRO.EXE is `same_as = "e1aaa"` in re/units.toml (same 55 bytes), so a change here applies there too.

## Summary

One 55 byte far function, understandable from its source alone. The name `RandomRange` and the types follow from the code and the call sites (lo is always 0 at the six callers seen). The only suggestion is a one line statement of the evidence for `rand` being the Borland LCG. No must-fix.
