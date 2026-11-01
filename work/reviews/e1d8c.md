# Review: e1d8c

Reviewer: rev-sonnet
Commit reviewed: a7f9bf773118dfbb201d1ac06ec31417aae0c893

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

Second review. The first review (two must-fix in the header comment) was fixed in 19b44b4 and
ticked in 8c0b096; this pass rechecks the header from scratch against the bytes and the image.

- [ ] note src/E1D8C.ASM:20: the header says `mov ax,212Eh` is at 1D8C:0006; the instruction starts at 1D8C:0005 and 0006 is its immediate, which is where the MZ relocation entry (2272) points. The claim "carries a relocation" is right; the address is of the field.
- [ ] note The name `_f1D8C_0000` is a placeholder. Both call sites (EOB.EXE file offsets 0x14652 and 0x14711, i.e. 0EF3:2FA2 and 0EF3:305F, inside the function at 0EF3:2EBC) use the result as `& 3` and `& 1`, which supports "small random value" and nothing more specific.
- [ ] note Registers: ES, SI and BP are saved and restored; AX and the flags are clobbered; DS is never read, so the routine does not depend on the caller's data segment (it loads DGROUP 212E into ES itself).
- [ ] note Checked and correct: the step list in the header (shr/shr leaves bit 1 of b0 in carry; rcl b2, rcl b1 propagate it; cmc then sbb gives b0>>2 - b0 - !bit7(old b1); shr moves bit 0 into carry; rcr b0 inserts it at bit 7; result b0 ^ b1, AH 0); the two far call sites (`9a 00 00 8c 1d` occurs exactly twice in EOB.EXE); DGROUP is 212E (whitepaper section on the startup); the initial state bytes at DGROUP:8484 are `34 12 76` in the file image (file offset 0x2800 + 0x212E0 + 0x8484); the only `84 84` pair in the image is the `mov si,8484h` operand, and no `85 84` or `86 84` (the other two state bytes) occurs anywhere.

## Summary

The unit is one 50 byte far function and is understandable from its source alone: the header gives the state location and initial value, the step list (verified against each instruction), both call sites with evidence, and the relocation. Nothing in it claims more than the bytes and the image show. The only name is the placeholder `_f1D8C_0000`, which is fine because the two callers give no basis for a specific name. No must-fix.
