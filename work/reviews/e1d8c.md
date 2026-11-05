# Review: e1d8c

# Round P6 re-review (2026-11-04), commit 24711ec

Reviewer: p6-rev-r2
Commit reviewed: 24711ec (P6 re-review; the P6 round read ce0608f)

Judged: the open P6 must-fix (the header's placeholder sentence) and the diff since ce0608f for src/E1D8C.ASM. `make match FILE=src/E1D8C.ASM`: 1 of 1 (50 bytes), exit 0. `make readability UNIT=e1d8c`: 0 findings.

- [x] must-fix src/E1D8C.ASM:18: fixed right. The "placeholder name stays" sentence is gone. The header now says RandomByte comes from the algorithm and the INTRO twin (0CDA:0006, src/I0CDA.ASM), the same 50-byte generator, with re/renames.toml as the evidence. The twin and algorithm back the name.
- [ ] note No new false claim in the changed lines.

# Round P6 (2026-11-04), commit ce0608f

# Round P6 (2026-11-04), commit ce0608f

Reviewer: p6-rev-1
Commit reviewed: ce0608f (main; review/e1d8c was ce0608f when this round began and no fast-forward was needed). The last review round was Round 1 at a7f9bf7. Since a7f9bf7, src/E1D8C.ASM changed by the Phase 6 rename: f1D8C_0000 is RandomByte, in the PUBLIC/PROC/ENDP labels. The diff is 3 insertions and 3 deletions. The header comment is not changed. The names do not change the machine code.

Later round (playbook 70). Judged: the diff since a7f9bf7 for src/E1D8C.ASM (the rename) and the name it introduces. The Round 1 note 18 (the placeholder name) is superseded by the rename.

Checked: `make match FILE=src/E1D8C.ASM` 1 of 1 (50 bytes), exit 0; `make readability UNIT=e1d8c` 0 findings. Evidence used this round: `git diff a7f9bf7 HEAD -- src/E1D8C.ASM` (the rename); `grep -n -A8 "addr = \"1D8C:0000\"" re/renames.toml` (re/renames.toml:4965, the RandomByte why); `sed -n '1,30p' src/E1D8C.ASM` (the header comment).

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/E1D8C.ASM:18 (the header comment): "The placeholder name stays: those uses support 'random byte' but nothing more specific." is false after the P6 rename. The name is RandomByte now, backed by re/renames.toml:4965: "The same 50-byte 24-bit generator as INTRO.EXE's RandomByte (0CDA:0006, src/I0CDA.ASM), stepping the state at ds:8484 (seed 34 12 76) and returning new b0 xor b1; its two callers in AnimateConeOfCold mask it (and 3, test 1)." Say the name comes from the twin (0CDA:0006, src/I0CDA.ASM) and the algorithm; drop "The placeholder name stays". The "those uses support 'random byte' but nothing more specific" is also stale: the twin and the algorithm back the name. Fixed in 6af4a72: the name is said to come from the algorithm and the INTRO twin 0CDA:0006.
- [ ] note the P6 diff's changed lines are the PUBLIC/PROC/ENDP labels; `make match` is 1 of 1 (50 bytes), so the bytes are unchanged. The header's algorithm description (the 24-bit step, the state at ds:8484, the seed 34 12 76, the two far calls) still holds.
- [ ] note the Round 1 note 18 said the name was a placeholder and the callers supported only "small random value"; the P6 rename's why adds the INTRO.EXE twin (0CDA:0006) and the state step, so the name RandomByte is backed. The Round 1 note is superseded.

## Summary

One must-fix item is open. The P6 rename replaced f1D8C_0000 with RandomByte, backed by the INTRO.EXE twin and the state step, but the header comment still says "The placeholder name stays: those uses support 'random byte' but nothing more specific." The matcher is 1 of 1 and readability is 0 findings; the bytes are unchanged. The unit does not pass (review-failed) until the header comment is rewritten.

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
- [ ] note The name `_f1D8C_0000` is a placeholder. Both call sites (EOB.EXE file offsets 0x146D2 and 0x1478F, i.e. 0EF3:2FA2 and 0EF3:305F, inside the function at 0EF3:2EBC) use the result as `& 3` and `& 1`, which supports "small random value" and nothing more specific.
- [ ] note Registers: ES, SI and BP are saved and restored; AX and the flags are clobbered; DS is never read, so the routine does not depend on the caller's data segment (it loads DGROUP 212E into ES itself).
- [ ] note Checked and correct: the step list in the header (shr/shr leaves bit 1 of b0 in carry; rcl b2, rcl b1 propagate it; cmc then sbb gives b0>>2 - b0 - !bit7(old b1); shr moves bit 0 into carry; rcr b0 inserts it at bit 7; result b0 ^ b1, AH 0); the two far call sites (`9a 00 00 8c 1d` occurs exactly twice in EOB.EXE); DGROUP is 212E (whitepaper section on the startup); the initial state bytes at DGROUP:8484 are `34 12 76` in the file image (file offset 0x2800 + 0x212E0 + 0x8484); the only `84 84` pair in the image is the `mov si,8484h` operand, and no `85 84` or `86 84` (the other two state bytes) occurs anywhere.

## Summary

The unit is one 50 byte far function and is understandable from its source alone: the header gives the state location and initial value, the step list (verified against each instruction), both call sites with evidence, and the relocation. Nothing in it claims more than the bytes and the image show. The only name is the placeholder `_f1D8C_0000`, which is fine because the two callers give no basis for a specific name. No must-fix.
