# Review: e1d8c

Reviewer: rev-sonnet
Commit reviewed: 427c22f1f43a9db1d4fd16c14b6b09a1fbd4586f

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix src/E1D8C.ASM:2-3: "the caller is the C runtime's random helper at 0EF3:2EBC" is wrong. Segment 0EF3 is unit e0ef3 (`re/units.toml`, kind c, 13219 bytes, 104 functions), game code; the Borland runtime is at segment 0000 (whitepaper 2.x; `rand()` is 0000:55B7 per unit e1aaa). The bytes at 0EF3:2EBC are `55 8b ec 83 ec 3c`, a normal function with 0x3C bytes of locals, not a helper. The far call `9a 00 00 8c 1d` occurs exactly twice in EOB.EXE, at 0EF3:2FA2 (followed by `mov ah,0 / and ax,3 / mov bx,ax`, so the result is used as a 2 bit value) and 0EF3:305F (followed by `test al,1`). Name those call sites, and drop "C runtime".
- [ ] must-fix src/E1D8C.ASM:3-5: the algorithm description does not match the code. (a) "rotate the low two bits out": only one bit leaves the low byte into carry, bit 1 of [si], by the second `shr al,1` (1D8C:0016); the first shift only discards bit 0 of the copy. (b) "shift the 3-byte state left": only [si+2],[si+1] shift left (`rcl` 001B, 001F, carry in = bit 1 of byte 0, carry out = top bit of byte 1); byte 0 shifts right (`shr al,1; rcr [si],1`, 002A..002E, carry in = bit 0 of `(b0>>2) - b0 - !CF`). (c) "fold the high bytes together" is `xor al,[si+1]`, i.e. new byte 0 xor byte 1 only; byte 2 does not take part in the result. Suggest: "AL = b0 xor b1 of the updated state; AH = 0".
- [ ] should src/E1D8C.ASM:15: `mov ax,212Eh` is DGROUP (whitepaper 2.x: DGROUP is paragraph 212E), and the file image carries a segment relocation for it (MZ relocation table entry 1D8C:0006). Say so in the header: the literal is DGROUP, relocated at load, not a constant. Also say the state is the 3 bytes at DGROUP:8484 and that nothing else in the EXE references the offset 8484 (a scan of the whole image finds only this function's `be 84 84`), with the static initial value `34 12 76` in the file at that address (so the first outputs are deterministic unless something seeds it through another pointer).
- [ ] note The name `_f1D8C_0000` is a placeholder; a call site that uses the result as `& 3` and `& 1` supports "random byte" but not a more specific name. Do not rename to rand-like names until the 0EF3 callers are understood.
- [ ] note The function saves ES and SI and clobbers AX only (plus flags); DS is not changed. It never reads DS, so it is independent of the caller's data segment.

## Summary

The unit is one 50 byte far function and the code is straightforward, but its header comment contains two wrong claims: the caller is game code in e0ef3 and not the C runtime, and the description of the shift/rotate steps does not match the instructions. Both are comment-only fixes (the match is not in question). The name is an acceptable placeholder.
