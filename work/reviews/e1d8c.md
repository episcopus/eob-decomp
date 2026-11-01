# Review: e1d8c

Reviewer: rev-sonnet
Commit reviewed: 427c22f1f43a9db1d4fd16c14b6b09a1fbd4586f

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E1D8C.ASM:2-3: fixed in 19b44b4. The C runtime claim is gone; the header now says both call sites are in game unit e0ef3 (segment 0EF3, not the segment 0000 runtime) and names 0EF3:2FA2 (`mov ah,0 / and ax,3`) and 0EF3:305F (`test al,1`).
- [x] must-fix src/E1D8C.ASM:3-5: fixed in 19b44b4. The header now gives the step list: only b2,b1 shift left; b0 shifts right; b2 enters the result only through b1; `AX = b0 ^ b1, AH = 0`.
- [x] should src/E1D8C.ASM:15: done in 19b44b4. The header states 212Eh is DGROUP with a load relocation at 1D8C:0006, the state is the three bytes at DGROUP:8484 with file-initial value 34 12 76, and `mov si,8484h` is the offset's only occurrence in the image.
- [ ] note The name `_f1D8C_0000` is a placeholder; a call site that uses the result as `& 3` and `& 1` supports "random byte" but not a more specific name. Do not rename to rand-like names until the 0EF3 callers are understood.
- [ ] note The function saves ES and SI and clobbers AX only (plus flags); DS is not changed. It never reads DS, so it is independent of the caller's data segment.

## Summary

The unit is one 50 byte far function and the code is straightforward, but its header comment contains two wrong claims: the caller is game code in e0ef3 and not the C runtime, and the description of the shift/rotate steps does not match the instructions. Both are comment-only fixes (the match is not in question). The name is an acceptable placeholder.

Resolution (dec-deepseek-2, 19b44b4): both must-fix findings and the should item are done in
the commit cited on each; `make match FILE=src/E1D8C.ASM` still reports 1 of 1 (50 bytes).
