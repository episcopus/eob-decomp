# Review: i_text

Reviewer: rev-haiku
Commit reviewed: f05a54b (main when I started). This branch was at 991e9b2; src/I_TEXT.ASM, re/symbols.toml and re/globals.toml are identical on both for this unit.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Checked: `make match FILE=src/I_TEXT.ASM` 44 of 44 functions match; `make readability UNIT=i_text` 0 findings. The addresses the comments cite were checked with tools/re/disasm.py (the ISR, PIT, sample, dispatch and tempo addresses); the curve bytes, the block bytes and the "Runtime overlay error" string were read from eob/INTRO.EXE (load module at file offset 0E00h).

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Round 4 (rev-haiku, 2026-11-02)

Reviewed at fa015bd (main). The unit's changes since cd26f41 (round 3) are in src/I_TEXT.ASM only, from commit 528956d (the round 3 fixes), and all are comment lines; no code byte changed. re/symbols.toml has no i_text row that changed (its diff is other units), and re/globals.toml changed only the gFn1EE2 and gBuiltinPalette rows (i0892's, not this unit).

Checked: `make match FILE=src/I_TEXT.ASM` 44 of 44, exit 0; `make readability UNIT=i_text` 0 findings. Each changed line was read against the code at the label or address it names:
- gPatchTable: the 90h handler L_4785 (line 1062) reads `[bx + gPatchTable]` at line 1068.
- gRhythmKeyBits: C1h L_4B01 stores 20h (line 1532); C2h L_4BE0 ORs the operand in (1553) and stores (1554); C3h L_4C10 clears it (1563); VoiceKeyOff tests it (1880) and VoiceSilence tests it (2333), both for nonzero.
- gTempoRate: A6h L_4871 stores it (1162); BAh L_4A7B loads it (1390) and stores it into [di+25h] (1391); the UpdateVoices copy is at 832 to 835 under the [di+40h] test.
- gEffectIndex: L_4AAC clears it at 1431 to 1432, under the type-2 test at 1429 to 1430.
- gRhythmLevelC: C5h L_4CC7 stores B (1641 and the others); C6h L_4D6B stores B at 1699; C by L_4C23 at 1573.
- 43A5 header: src/I0892.C:451 is the ShutdownGame definition, its StopAdlibDriver call is line 461, the extern is line 155; E07C8.C:451 is the EOB call.
- 4407 header: the `cmp bl, 12h` at 659 with the `je` at 660 skips the VoiceSilence call for voice 9.
- 4516 header: the 3Fh writes to 40h and 43h are at 794 to 801.
- 4E12 header: the store to [di+41h] is line 1769; the [di+42h] pitch adjust runs 1800 to 1832.
- 516D header: L_5255 is a `ret` at 2367; the stores to [di+29h] and [di+2Bh] are at 2261 and 2262.
- 535C header: the call to SoundRandom at 1282 is followed by `and ax, bx` at 1283 (see R4-2).
- Field list: +05h, _GetVoiceState (2093) reads [di+5] and returns it, the sixth dispatch entry (line 188); +07h, L_46E3 stores it (990); +08h, the 82h handler body at L_4663 compares it (940) and stores it (943); +17h, +1Ah and +1Bh, L_526C (2379) negates or reloads only when `dec [di+1ah]` reaches zero (the `jne L_5281` at 2382); +24h, L_47BF stores it (1087); +26h, a carry goes to L_45B3 (842), which decrements [di+5] (843) and jumps to L_45D4 (844) at zero; +34h, L_45F5 (874) stores the next script byte (876) when the word at +32h or +33h is nonzero (868, 872); +3Bh to +3Eh, L_52BB (2412) decrements +3Ch and reloads it at 2415 to 2416, L_52C6 (2417) adds gOperatorOffset and runs xlatb (2422).

Round 3 items:
- Round 3 must-fix R3-1 to R3-7: fixed right; ticks stay.
- R3-8 (the 43A5 header), R3-9 (the moved in-file cites at 171, 248, 619, 775, 1765, 2242 and 2488) and R3-10 (+3Bh to +3Eh): fixed right; ticks stay.
- R3-11: +07h, +08h, +24h and the B5h clause of +28h are right. The +28h writer list is partial, see R4-1, so R3-11 stays ticked and R4-1 is the open item.

Findings:

- [ ] should R4-1 src/I_TEXT.ASM:312 to 313 (+28h): the list of writers is partial. Round 3 named the writers as SetVoicePitch, VoiceKeyOn, VoiceKeyOff, the INT 08h handler and the B9h tail; the fix dropped the B9h tail. The tail is `mov byte ptr [di + 28h], ch` at line 1376 (the code after L_49C1, reached at L_4A52). Two more writers are not named: the oscillator step L_526C (`mov byte ptr [di + 28h], al` at line 2396, on the path through L_5281) and VoiceSilence (line 2484). The sentence is not false, but it reads as the full list. Fix: name the B9h command's tail (L_4A52), L_526C and VoiceSilence, or say the list is partial.

- [ ] note R4-2 src/I_TEXT.ASM:2488 (the 535C header): "caller line 1282 uses AX" names the call; the use is at 1283 (`and ax, bx`). Right enough: the call returns the seed in AX.

- [ ] note R4-3 src/I_TEXT.ASM:291 (the +08h entry): the 82h handler's entry is L_4657 (line 921); L_4663 is the sound-number path inside it, which is where the compare and store are. The label is right for the claim; a reader following the 82h script byte will meet L_4657 first.

- [ ] note registry (round 1 to 3 note, still open; not a review finding): re/symbols.toml has 7 i_text rows for 44 functions (i0000_03fa covers 03FA to 4352, both INT 08h handlers; i0000_43a5 is 4240 bytes). `review-pass` stamps only the existing rows, so 37 functions get no `review` stamp in this round. The split (task/split-on-match) is for the human; it is not a text finding and does not block this round.

### Round 3 (rev-haiku, 2026-11-02)

Reviewed at cd26f41 (main). src/I_TEXT.ASM is identical on main and on branch review/i_text (0c8ba51); no i0000 or I_TEXT row of re/symbols.toml or re/globals.toml differs between them. The diff read is bb3f897 to main for src/I_TEXT.ASM: commit e4b4fe5 (the round 2 fixes and the 43h-byte field list at lines 283 to 334) and commit d05bca0 (the WriteVoiceLevels rename).

Checked: `make match FILE=src/I_TEXT.ASM` 44 of 44, exit 0; `make readability UNIT=i_text` 0 findings. Every claim of the new field list was checked against the `[di + Nh]` accesses and the script handler labels (the `; script byte` comments); the round 2 claims were checked against the code at the lines they now cite.

Main finding: the field list adds 51 lines above the code, and the fixes kept the bb3f897 line numbers, so every in-file line citation at or after line 283 now names code 51 lines below its target (the round 2 claims themselves are right). Cite labels or the current lines.

Round 2 items, in order:
- gRhythmKeyBits (round 2 item 1): the claims are right, the line citations are not. Reopened as R3-1.
- gTempoRate (round 2 item 2): the claims are right, the line citations are not. Reopened as R3-2.
- gRhythmLevelC (round 2 should): its changed line cites lines that are now wrong. Reopened as R3-3.
- The 43A5 header (round 2 should): the address 0892:0835 is right; the I0892.C lines are stale. Now R3-8.
- The WriteVoiceLevels rename and the WHITEPAPER 2.1.2 and 2.4 citations: right, ticks stay.
- The field list (round 2 should): the list is useful, but four of its claims are wrong (R3-4 to R3-7).
- The round 2 note on 2106 (nonzero test): the field list says LoadVoicePatch keeps bit 0 only (`and al, 1`), so the nonzero test is the connection bit. Resolved.

- [x] must-fix R3-1 src/I_TEXT.ASM:242 (gRhythmKeyBits, round 2 item 1 reopened). The claims are right: C1h sets 20h, C2h ORs its operand in, C3h clears it, VoiceKeyOff and VoiceSilence test it for nonzero. The line numbers name other code: the C1h store `mov byte ptr ds:[gRhythmKeyBits], 20h` is line 1532 (the cited 1481 is `shl bx, 1`); the C2h `or al, byte ptr ds:[gRhythmKeyBits]` and store are lines 1553 to 1554 (cited 1502 to 1503 is `mov al, [si + 5]`); the C3h clear is line 1563 (cited 1512 is `call _AdlibWrite`); the VoiceKeyOff test is line 1880 (cited 1829 is `mov al, [bx + si]`); the VoiceSilence test is line 2333 (cited 2282 is `add ah, cl`). Fix: cite the labels, which do not move (L_4B01 for C1h, L_4BE0 for C2h, L_4C10 for C3h, VoiceKeyOff, VoiceSilence), or the current lines. **Fixed:** gRhythmKeyBits now cites the labels L_4B01 (C1h), L_4BE0 (C2h) and L_4C10 (C3h) and the names VoiceKeyOff and VoiceSilence, with no in-file line numbers.

- [x] must-fix R3-2 src/I_TEXT.ASM:243 (gTempoRate, round 2 item 2 reopened). The claims are right: A6h writes it, BAh copies it into [di+25h], the UpdateVoices copy when [di+40h] is set. The lines are wrong: the A6h store `mov byte ptr ds:[gTempoRate], ah` is line 1162 (cited 1111 is the gTempoBeat store); the BAh load `mov al, byte ptr ds:[gTempoRate]` is line 1390 and its store to [di+25h] is 1391 (cited 1340 is `xor bh, bh`); the UpdateVoices copy is lines 834 to 835 (cited 781 to 784 are AdlibWrite calls in ResetVoices). Fix: cite L_4871 (A6h), L_4A7B (BAh) and UpdateVoices, or the current lines. **Fixed:** gTempoRate now cites A6h (L_4871), BAh (L_4A7B) and UpdateVoices, with no in-file line numbers.

- [x] must-fix R3-3 src/I_TEXT.ASM:255 (gRhythmLevelC, round 2 should, now a must-fix because this line changed). The C6h stores to gRhythmLevelB (label L_4D6B) are at lines 1699, 1710, 1721, 1732 and 1743; the list 1648, 1659, 1670, 1681 and 1692 is the bb3f897 numbering. Fix: the current lines, or cite L_4D6B. **Fixed:** gRhythmLevelC now cites L_4D6B and drops the stale line list.

- [x] must-fix R3-4 src/I_TEXT.ASM:288 (the +05h field): "GetVoiceDelay returns it" names no function. Nothing in src/ or the driver is called GetVoiceDelay. The function that returns the byte at [di+5] is `_GetVoiceState` (line 2093, a driver table entry at line 188; it does `mov al, [di+5]`). Fix: name _GetVoiceState. **Fixed:** the +05h field now says GetVoiceState returns it.

- [x] must-fix R3-5 src/I_TEXT.ASM:298 (+17h) and 300 to 301 (+1Ah, +1Bh). "L_5256 negates it at a carry" and "L_5256 reloads it from +1Bh" are wrong as written. The step routine at L_5256 falls through to L_526C on a carry from +1Eh (lines 2368 to 2379), and L_526C does `dec [di+1ah]` and goes to L_5281 when the count is not zero. The negation of +17h and the reload of +1Ah from +1Bh happen only when the count runs out, not at every carry. Fix: say so, for both fields. **Fixed:** +17h, +1Ah and +1Bh now say L_526C negates +17h and reloads +1Ah from +1Bh only when the count at +1Ah runs out.

- [x] must-fix R3-6 src/I_TEXT.ASM:310 (+26h): "starts the script on a carry" is wrong. A carry out of the +26h add goes to L_45B3, which counts the note delay +05h down; the script resumes at L_45D4 (`mov si, [di+3]`) only when that count reaches zero. Fix: say a carry counts the note delay down and the script resumes when it reaches zero. **Fixed:** +26h now says a carry counts the note delay +05h down and the script resumes at L_45D4 when it reaches zero.

- [x] must-fix R3-7 src/I_TEXT.ASM:323 (+34h): "scripts A0h and A1h store it" is wrong. A0h (L_483F, line 1138) and A1h (L_4852, line 1147) call StartVoiceNote (A1h also calls VoiceKeyOn) and store nothing in +34h. The store is in UpdateVoices, at L_45F5 (line 876): it reads the next script byte into +34h when the word at +32h or +33h is nonzero (the cmp word tests at lines 868 and 872). Fix: name UpdateVoices and that test. **Fixed:** +34h now says UpdateVoices (L_45F5) stores the next script byte in it when the word at +32h or +33h is nonzero.

- [x] should R3-8 src/I_TEXT.ASM:544 (the 43A5 header, round 2 should reopened). The address 0892:0835 is right (src/I0892.C:451 carries `/*@ 0892:0835 */`), but the I0892.C lines are stale: ShutdownGame is at I0892.C:451, its call to StopAdlibDriver is line 461, and the extern is line 155 (the header says 444, 454 and 149). Fix: cite the address, or the current lines. E07C8.C:451 is right. **Fixed:** the 43A5 header now cites src/I0892.C:451, the call at line 461 and the extern at I0892.C:155.

- [x] should R3-9 in-file line citations that the 51-line insertion moved (untouched by the fixes): src/I_TEXT.ASM:171 (line 1017 is now 1068; line 713 is now 764), :248 (1378 to 1381 is now 1429 to 1432), :619 (608 is now 659), :775 (740 to 750 is now about 791 to 801), :1765 (1718 is now 1769; 1749 to 1781 is now about 1800 to 1832), :2242 (2210 to 2211 is now 2261 to 2262), :2488 (1231 is now 1282). Cite labels, or the current lines. **Fixed:** :171 cites the script 90h label L_4785 and drops both lines; :248 cites 1429 to 1432; :619 cites 659; :775 cites 794 to 801; :1765 cites 1769 and 1800 to 1832; :2242 drops the line pair (L_5255 is named); :2488 cites 1282.

- [x] should R3-10 src/I_TEXT.ASM:328 to 331 (+3Bh, +3Ch, +3Dh, +3Eh): "L_52AF reloads, counts, adds, indexes" describes the carry path. L_52AF carries into L_52BB (line 2412: the count `dec [di+3ch]` and the reload from +3Bh) and into L_52C6 (line 2417: the add of gOperatorOffset and the xlatb). A reader who follows L_52AF alone sees only the +39h add. Fix: name L_52BB and L_52C6. **Fixed:** +3Bh and +3Ch now name L_52BB; +3Dh and +3Eh now name L_52C6.

- [x] note R3-11 src/I_TEXT.ASM:290, 291, 308 and 312: the field list is right but not complete. +07h is also set by script 87h (L_46E3), +08h is also compared and stored in the 82h handler (L_4663), +24h is set by script 93h (L_47BF). For +28h, script B5h (L_4985) reads the byte and writes only the registers; the writes are by SetVoicePitch, VoiceKeyOn, VoiceKeyOff, the INT 08h handler and the B9h tail. **Fixed:** +07h names script 87h (L_46E3); +08h names script 82h (L_4663); +24h names script 93h (L_47BF); +28h says B5h reads it and writes only the registers.

- [ ] note registry (round 1 and round 2 note, still open): re/symbols.toml still has 7 i_text rows for 44 functions (i0000_03fa is 16216 bytes, covering 03FA to 4352, both INT 08h handlers; i0000_43a5 is 4240 bytes). review-pass stamps only the existing rows, so the split still has to be done before a passing review (the human's task/split-on-match run).

Checked and right, no action: the gRhythmKeyBits reader claims (nonzero tests, not bit 5); the gTempoRate writer and the BAh copy; the INT 08h handler reads and writes voice 0's +28h (through bx); the L_5255 ret; the connection bit (`and al, 1` in LoadVoicePatch); the WriteVoiceLevels name; the other field claims (+00h, +03h, +06h, +09h to +0Ah, +12h to +15h, +19h to +1Ch, +1Eh, +1Fh, +20h to +21h, +25h, +27h, +29h, +2Bh, +2Dh, +2Eh, +2Fh, +30h, +32h, +33h, +35h, +36h, +39h, +3Ah, +40h to +42h).

### Round 2 (rev-haiku, 2026-11-02)

Reviewed at bb3f897 (main). The three files of this unit (src/I_TEXT.ASM, re/symbols.toml, re/globals.toml) are the
same on branch review/i_text (3b0319b) and on main for the I_TEXT lines; the round 1 must-fix items were judged against
the current text, and the diff from f05a54b to main was read for the changed lines. No code byte changed: the diff is
comments, the gPatchTable comment, and registry and globals entries.

Checked: `make match FILE=src/I_TEXT.ASM` 44 of 44, exit 0; `make readability UNIT=i_text` 0 findings. In eob/INTRO.EXE
(load module at file offset 0E00h): 3EFB holds 34h and 3EFC holds 41h; the 12 bytes at 0670 are 88 01 01 08 01 00 18 00
54 F2 60 46 (10 nonzero); the words are 066Eh at 0416 and 0671h at 0542. The queue reset (lines 483 and 484), the 3F7A
ring overflow (gQueueRing at 3F6A, 16 bytes), the SetSoundByte, GetSoundData, SoundRandom, ClearVoice, ResetVoices,
GetDriverTable, ProcessSoundQueue (line 608), StartSample and WriteVoiceReg claims were read against the code.

Round 1 must-fix items: 313 (gBusyCountdown), 182 (3EFB), 87 (SetSoundByte), 1826 (VoiceKeyOff), 1847 (StartVoiceNote),
2096 (WriteVoiceReg), 2279 (VoiceSilence) and 1878 (StartSample) are fixed right, ticks stay. The 493 (E07C8.C:65 to
the INTRO caller) fix is right in the address, its line cites are stale (round 2 should, item 3). 240 (gRhythmKeyBits) is NOT
fixed right: reopened (round 2 item 1).

- [x] must-fix src/I_TEXT.ASM:241 (gRhythmKeyBits; round 1 must-fix reopened). Two claims in the comment are wrong.
  (a) "its readers test bit 5 as rhythm mode (VoiceKeyOff line 1829, VoiceSilence line 2282)": both readers test the whole
  byte for nonzero (`cmp byte ptr ds:[gRhythmKeyBits], 0` at lines 1829 and 2282); bit 5 is not tested anywhere.
  (b) "the OR of every C2h operand since the last C3h" omits the direct store `mov byte ptr ds:[gRhythmKeyBits], 20h` at
  line 1481, which is inside the C1h handler (L_4B01 at line 1398, L_4BE0 at line 1487). Fix: say C1h sets it to 20h
  (line 1481), C2h ORs its operand into it (lines 1502 to 1503), C3h clears it (line 1512), and VoiceKeyOff (1829) and
  VoiceSilence (2282) test it for nonzero. **Fixed:** Rewrote gRhythmKeyBits: C1h sets it to 20h (line 1481), C2h ORs
  its operand into it (lines 1502 to 1503), C3h clears it (line 1512); VoiceKeyOff (1829) and VoiceSilence (2282) test it
  for nonzero (commit e4b4fe5).

- [x] must-fix src/I_TEXT.ASM:242 (gTempoRate; a round 1 should, changed by the fix, now wrong). Two citations name the
  wrong code. (a) "copied to a voice's speed byte [di+25h] when its follow flag [di+40h] is set (line 1783 to 1784)": the
  copy is at lines 781 to 784 (cmp [di+40h] at 781, mov al,gTempoRate at 783, mov [di+25h],al at 784, in the UpdateVoices
  loop before L_45A5); lines 1783 and 1784 are the A0 register setup in SetVoicePitch. (b) "by commands A9h and BAh (lines
  1118 and 1340)": line 1118 (L_4881, script byte A9h) stores its own operand AH into [di+25h] and does not read
  gTempoRate. The byte is written only by command A6h (L_4871, line 1111), and copied by BAh (L_4A7B, line 1340). Fix:
  "written by A6h (line 1111); copied into a voice's speed byte [di+25h] by BAh (line 1340) and, when [di+40h] is set,
  by the UpdateVoices copy (lines 781 to 784)". **Fixed:** Rewrote gTempoRate: written by A6h (line 1111); copied into a
  voice's speed byte [di+25h] by BAh (line 1340) and, when [di+40h] is set, by the UpdateVoices copy (lines 781 to 784)
  (commit e4b4fe5).

- [x] should src/I_TEXT.ASM:493 (the 43A5 header): the address is right, but "src/I0892.C:420" and "declared at
  I0892.C:127" are stale. ShutdownGame is at I0892.C:444 and calls StopAdlibDriver at I0892.C:454; the extern is at
  I0892.C:149. Cite the address (0892:0835) and, if a line is kept, the current lines. E07C8.C:451 is right. **Fixed:**
  The header now says ShutdownGame at 0892:0835 (src/I0892.C:444, calls it at line 454; the extern is at I0892.C:149)
  (commit e4b4fe5).

- [x] should src/I_TEXT.ASM:254 (gRhythmLevelC): the C6h list "lines 1659, 1670, 1681 and 1692" omits line 1648, the store
  `mov byte ptr ds:[gRhythmLevelB+2], al` inside L_4D6B (line 1643). L_4D6B does store into B; add 1648 to the list.
  **Fixed:** The gRhythmLevelC comment now lists 1648 with 1659, 1670, 1681 and 1692 (commit e4b4fe5).

- note src/I_TEXT.ASM:2106 (WriteVoiceReg): the test is `cmp byte ptr [di + 31h], 0`, a nonzero test of the whole byte.
  It is "the connection bit" only if byte 31h holds bit 0 and nothing else (the C0h comment says LoadVoicePatch keeps bit 0
  there). Not a finding; say "nonzero" if the bit is not the only content.

- note registry (round 1 note, still open): re/symbols.toml has 7 i_text entries for 44 functions (i0000_03fa, the
  `_RebaseDriverBlock` entry, covers 16216 bytes, including both INT 08h handlers). review-pass stamps only the entries
  that exist, so the split (one entry per function) is still needed before a passing review. Not changed in this round.

- [x] should carried from round 1 (still open): rename `WriteVoiceReg` to `WriteVoiceLevels` (make rename); cite WHITEPAPER
  2.1.2 and 2.4 in the unit header; list the voice-record fields once (src/I_TEXT.ASM:281 to 282). **Fixed:** Renamed
  intro/0000:50C8 via `make rename` (re/renames.toml, commit d05bca0; 7 uses in src/I_TEXT.ASM); the unit header cites
  WHITEPAPER 2.1.2 and 2.4; the 43h-byte voice-record field list is at src/I_TEXT.ASM:283 to 334 (offsets +00h to +42h
  with their setters and readers) (commit e4b4fe5).

### Round 1

- [x] must-fix src/I_TEXT.ASM:313 (the 424D header): "counts gBusyCountdown down to zero" describes a byte this build does not decrement. The handler (lines 319 to 372) has no reference to it, and the unit header (lines 6 to 8) says the wait was dropped. E_TEXT.ASM:349 to 351 has the wait. Fix: delete the bullet. **Fixed:** Deleted the gBusyCountdown bullet; the handler has no such byte (the unit header already says the wait was dropped).

- [x] must-fix src/I_TEXT.ASM:182 (the 3D2C header): "QUIRK 3EFC" should be 3EFB. The 34h is at 0000:3EFB; 0000:3EFC holds 41h (eob/INTRO.EXE). Curve 12 starts at 3EDE (3D5E + 12 * 32) and the 34h is its index 29. The 3E6E quirk on the same line is right (19h at 3E6E). **Fixed:** Changed QUIRK 3EFC to 3EFB; the 34h is at 3EFB and 3EFC holds 41h.

- [x] must-fix src/I_TEXT.ASM:493 (the 43A5 header): "(E07C8.C:65)" is a comment on gTandyOn (E07C8.C:63 to 65), not a caller of StopAdlibDriver. The INTRO caller is ShutdownGame at 0892:0835 (src/I0892.C:420, declared at I0892.C:127). The EOB caller is E07C8.C:451. Cite the address, not a line of another file. **Fixed:** Replaced the E07C8.C:65 citation with the INTRO caller ShutdownGame at 0892:0835 (src/I0892.C:420, declared at I0892.C:127); kept the EOB caller E07C8.C:451.

- [x] must-fix src/I_TEXT.ASM:87 (the 03B0 SetSoundByte header): "idx selects the 300-word table at 0416" is wrong. idx selects one word of that table (lines 94 to 95: the near pointer at cs:0416 + 2 * idx), and reg is the offset into the record that word points to (lines 96 to 98). E177D.C:251 to 252 passes idx as the sound number and reg as 3 or 1. **Fixed:** Corrected the SetSoundByte comment: idx selects one word of the 300-word table (cs:0416 + 2*idx); reg offsets into the record that word points to.

- [x] must-fix src/I_TEXT.ASM:240 (gRhythmKeyBits), REOPENED in round 2 (round 2 item 1): "the key bits last written to register BDh (bit 5 is the rhythm mode)" does not describe the byte. C2h stores `operand | old value` (lines 1502 to 1503), so the byte is the OR of every C2h operand since the last C3h, which clears it (line 1512). It is not the last value written to BDh, which also carries gRegBDDepth and 20h (lines 1504 to 1506). Its readers test it as rhythm mode (lines 1829 and 2282). **Fixed (round 2):** Rewrote gRhythmKeyBits: C1h sets it to 20h (line 1481), C2h ORs its operand into it (lines 1502 to 1503), C3h clears it (line 1512); VoiceKeyOff (1829) and VoiceSilence (2282) test it for nonzero (commit e4b4fe5).

- [x] must-fix src/I_TEXT.ASM:1826 (the 4EF7 VoiceKeyOff header): "skips the first six voices while a sample plays" is wrong. The routine returns at once when gRhythmKeyBits is nonzero (rhythm mode on) and the voice index is 6 or more (lines 1829 to 1833). Voice 9 gets no B0 write (lines 1835 to 1836). No sample test appears in the routine. Say: skips voices 6 to 9 while rhythm mode is on. **Fixed:** Corrected VoiceKeyOff: while rhythm mode is on it skips voices 6 to 9 (not the first six while a sample plays); voice 9 gets no B0 write.

- [x] must-fix src/I_TEXT.ASM:1847 (the 4F1F StartVoiceNote header): "records the level byte" is wrong. AH, the note's duration, goes to gNoteDuration (line 1850) and to the voice's delay counter [di+5] (lines 1852 to 1857, and 1874), with a random part added when [di+36h] is set. gNoteDuration's own comment (line 236) says it is the duration. **Fixed:** Corrected StartVoiceNote: AH is the note duration, stored in gNoteDuration and the voice's delay counter [di+5], with a random part when [di+36h] is set; the 2dh shift multiplies into the key-off point [di+21h].

- [x] must-fix src/I_TEXT.ASM:2096 (the 50C8 WriteVoiceReg header): "when the voice is two-operator, register 40h" is wrong. The test is record byte 31h (line 2106), the connection bit. Register 40h (the modulator level) is written only for an additive voice, where both operators sound (the 512F header, line 2155). Every AdLib melodic voice is two-operator, so that cannot be the test. "the voice 422C-table register 43h" names nothing: 422C is gOperatorOffsets (lines 303 to 304). Write "register 43h (carrier level) of the voice's operator (gOperatorOffsets, 422C)". **Fixed:** Corrected WriteVoiceReg: the test is record byte 31h (the connection bit), not two-operator; register 40h is written only for an additive voice; 422C is gOperatorOffsets.

- [x] must-fix src/I_TEXT.ASM:2279 (the 520C VoiceSilence header), two wrong claims. (a) "writes ffh to its level registers": the ffh goes to 60h, 63h, 80h and 83h (lines 2292 to 2307), which are attack/decay and sustain/release. The level registers 40h and 43h are not written. (b) "then zeroes its gate and key-on": B0 is written 0 (lines 2308 to 2311) and then 20h (lines 2312 to 2313), and 20h is the key-on bit, so the voice ends up keyed on. The header also leaves out the guard at lines 2282 to 2286: with rhythm mode on, voices 6 and up return at once. **Fixed:** Corrected VoiceSilence: ffh goes to 60h, 63h, 80h and 83h (attack/decay and sustain/release, not the level registers); B0 is written 0 then 20h (key-on set); rhythm mode makes voices 6 and up return at once.

- [x] must-fix src/I_TEXT.ASM:1878 (the 4F57 StartSample header): "programs the PIT and the speakers" is wrong. StartSample reads or writes no speaker port (no port 61h between lines 1879 and 1978); StartAdlibDriver clears the speaker gate (lines 480 to 482). StartSample writes AdLib registers for the sample's voice (lines 1891 to 1910, 1929 to 1932), writes register 40h = 0 (lines 1947 to 1954), and programs the PIT (lines 1912 to 1917 read it, lines 1963 to 1970 set the 4Bh divisor). The header also does not say its arguments: a far sample pointer at [bp+6] and a length at [bp+0ah] (lines 1934 to 1941). **Fixed:** Corrected StartSample: it reads or writes no speaker port; it writes the sample's AdLib registers and programs the PIT (divisor 4Bh); added the far pointer at [bp+6] and the length at [bp+0ah].

- [x] should src/I_TEXT.ASM:6 to 8 (the unit header): "three instructions dropped" lists six instructions: the cld, the cmp, je and dec of the busy wait, and the push di and pop di pair (the dispatch has one pair fewer than E_TEXT.ASM: 13 pairs here, 14 there). Say six instructions in three places. **Fixed:** Said six instructions in three places.

- [x] should src/I_TEXT.ASM:163 to 164 (the 0416 block header): "zeros but for 12 bytes in three places" is loose. The nonzero bytes are the word 066Eh at 0416, the word 0671h at 0542 (gPatchTable's first word), and 12 bytes at 0670, of which 10 are nonzero. "adlib.dat's first word is 0258h" has no source here: adlib.dat is not in eob/, and no document gives the value. Cite it or drop it. **Fixed:** Listed the nonzero bytes precisely (word 066Eh at 0416, word 0671h at 0542, 12 bytes at 0670 with 10 nonzero) and dropped the unsourced adlib.dat first-word claim.

- [x] should src/I_TEXT.ASM:170 to 171 (gPatchTable): no comment. Its name is backed: the script command 90h reads patch pointers at [bx+gPatchTable] (line 1017), and GetDriverTable arg 3 returns it (line 713). Add a comment with those. **Fixed:** Added the gPatchTable comment: script command 90h reads it at line 1017 and GetDriverTable arg 3 returns it at line 713.

- [x] should src/I_TEXT.ASM:178 to 179 (gEntryTable): "in the order the original links them" has no evidence in the unit. Tag it [H] or drop it. **Fixed:** Tagged the link order [H].

- [x] should src/I_TEXT.ASM:241 (gTempoRate): "the followers' voice speed" is vague. The byte is copied into a voice's speed byte [di+25h] when that voice's follow flag [di+40h] is set (lines 1783 to 1784), and by commands A9h and BAh (lines 1118 and 1340). Say so. **Fixed:** Rewrote gTempoRate: copied to a voice's speed byte [di+25h] when its follow flag [di+40h] is set, and by commands A9h and BAh.

- [x] should src/I_TEXT.ASM:246 (gEffectIndex): "L_4AAC clears it" holds only for effect type 2 (lines 1378 to 1381). Say so. **Fixed:** Said L_4AAC clears gEffectIndex only for effect type 2 (lines 1378 to 1381).

- [x] should src/I_TEXT.ASM:251 to 254 (gRhythmLevelB): "B is stored by L_4CC7" is incomplete. C6h (L_4D6B, lines 1659, 1670, 1681 and 1692) also stores into B. **Fixed:** Added the C6h (L_4D6B, lines 1659, 1670, 1681 and 1692) writes to B.

- [x] should src/I_TEXT.ASM:449 (the 4352 header): "clears the ring" is loose. StartAdlibDriver resets the queue indices gQueueRead and gQueueWrite (lines 483 to 484); the ring's bytes are left as they were. **Fixed:** Said StartAdlibDriver resets the queue indices gQueueRead and gQueueWrite; the ring's bytes are left as they were.

- [x] should src/I_TEXT.ASM:543 to 545 (QueueSound): "its high byte lands in the next ring byte" holds for indices 0 to 14. Index 15's high byte lands at 3F7A, which is gVoiceRecord0 byte 0. ACh, ADh and VoiceCarrierLevel read or write that byte (lines 1133, 1149 and 2128). That is a quirk: mark it `QUIRK 43E7:` and say what happens. **Fixed:** Added the QUIRK 43E7 index-15 high-byte note (3F7A, gVoiceRecord0 byte 0) and its readers.

- [x] should src/I_TEXT.ASM:565 to 568 (ProcessSoundQueue): omits that VoiceSilence is skipped for voice 9. The test at line 608 compares the doubled voice number (line 595 shifts bx left) with 12h, so it is voice 9. **Fixed:** Added that VoiceSilence is skipped for voice 9 (the doubled voice number 12h at line 608).

- [x] should src/I_TEXT.ASM:692 (GetDriverTable): "returns a near driver table pointer for arg 0 to 4" is loose. Arg 0 returns cs, a segment (line 699). Args 1 and 2 both return gSoundTable (lines 704 and 709). Arg 3 is gPatchTable (0542h, line 713) and arg 4 is gCurveTablePtrs (020Ch, line 717). The header lists four values for five args. **Fixed:** Listed what each GetDriverTable argument returns: arg 0 cs, args 1 and 2 gSoundTable, arg 3 gPatchTable, arg 4 gCurveTablePtrs.

- [x] should src/I_TEXT.ASM:724 (ResetVoices): omits the level writes that silence voices 0 to 8: 3Fh to registers 40h and 43h (lines 740 to 750). **Fixed:** Added the 3Fh writes to registers 40h and 43h that silence voices 0 to 8.

- [x] should src/I_TEXT.ASM:1714 (SetVoicePitch): on entry AL is the note, which is stored at +41h (line 1718). The header does not say that, and it omits the pitch-curve adjust from +42h (lines 1749 to 1781). **Fixed:** Added the AL note storage at [di+41h] and the pitch-curve adjust from [di+42h].

- [x] should src/I_TEXT.ASM:1847 (StartVoiceNote): "the 2dh shift" is a multiplier. The routine adds (AH shr 3) [di+2dh] times into [di+21h] (lines 1860 to 1872), which is the key-off point. Say that. **Fixed:** Said the 2dh shift multiplies into the key-off point [di+21h].

- [x] should src/I_TEXT.ASM:2011 (TickTempo): "the step byte 3F45" is the beat count, gTempoBeat (line 258). **Fixed:** Named 3F45 gTempoBeat (the beat count).

- [x] should src/I_TEXT.ASM:2191 (ClearVoice): "resets its delay and script pointers". The script pointer [di+3] is in the zeroed range. [di+29h] and [di+2Bh] are the per-tick handlers, set to L_5255 (a ret, lines 2210 to 2211). Say so. **Fixed:** Said the script pointer [di+3] is in the zeroed range and [di+29h]/[di+2Bh] are set to L_5255 (a ret).

- [x] should src/I_TEXT.ASM:2223 (LoadVoicePatch): "E177D.C:31" is ATTENUATION_ADLIB_LIMIT. The full-attenuation define and the whitepaper quote it cites are at E177D.C:32 to 33. **Fixed:** Changed E177D.C:31 to E177D.C:32 to 33 (the full-attenuation define and whitepaper quote).

- [x] should src/I_TEXT.ASM:2437 (SoundRandom): "the driver random byte" does not say the function returns the new seed in AX. Its caller at line 1231 uses AX. **Fixed:** Said SoundRandom returns the new seed in AX (caller at line 1231 uses it).

- [x] should src/I_TEXT.ASM:121 (GetSoundData): "returns the far driver routine pointer cs:[0416 + 2*idx]" reads as if the pointer were at that address. The function returns dx = cs and ax = the word stored there (lines 127 to 132). **Fixed:** Said the function returns dx = cs and ax = the word at cs:[0416 + 2*idx], not a pointer stored there.

- [x] should src/I_TEXT.ASM:2096 (WriteVoiceReg): the name is vague. It writes the two total-level registers (43h and 40h). A name such as WriteVoiceLevels says what it does; use `make rename` for it. **Fixed:** Renamed to WriteVoiceLevels (make rename, intro/0000:50C8, commit d05bca0).

- [x] should src/I_TEXT.ASM:1 to 19 (the unit header): no whitepaper section is cited, and READABILITY asks for one. Cite WHITEPAPER 2.1.2 (what segment 0 holds) and 2.4 (INTRO.EXE). **Fixed:** The header now cites WHITEPAPER 2.1.2 and 2.4 (commit e4b4fe5).

- [x] should src/I_TEXT.ASM:281 to 282 (the voice records): the record fields (+3, +5, +8, +20h, +25h, +28h, +29h, +2Bh, +40h and the rest) are documented only where the code uses them. One comment, or EQU names, listing the fields would let the next reader follow the code without searching. **Fixed:** Added the 43h-byte field list (offsets +00h to +42h, each with its setters and readers) at src/I_TEXT.ASM:283 to 334 (commit e4b4fe5).

- [ ] note src/I_TEXT.ASM:309 to 316 and E177D.C:249: nothing in this build decrements a busy byte, and 3B36 is zero spare data here (inside gBlockSpare). If INTRO's code reaches PlaySoundAtCell's busy wait (E177D.C:249), that wait cannot end. The callers in src/I0*.C do not show whether it does. This is a question for the human, not a finding against this file.

- [ ] note the script commands (80h to C9h, the table at 536C) are described only in the code. WHITEPAPER does not define the script format. Each handler's label is an L_ placeholder; a one-line description of what each command does would help the next reader.

- [ ] note gScriptFlag (3A0): named for its one writer (C7h, line 1702) and its reader GetScriptFlag (03A1), which has no known caller. The name is acceptable as it stands.

- [ ] note registry (question for the human): re/symbols.toml has 7 entries for these 44 functions. Example: i0000_03fa `_RebaseDriverBlock` has size 16216, which covers 03FA to 4352, including both INT 08h handlers. This is the registry problem escalated for e17a6, fixed by task/split-on-match. Please run that for i_text before review-pass, so each function has its own entry.

- [ ] note checked and right, no action: the QUIRK 4627 (cmp al,70h against a 74-entry table), with "Runtime overlay error" starting at 0000:5400 just after the table; the PIT addresses 437D to 438B and the int 21h 2508h addresses 4372 to 4378 and 5021 to 5027; the write to 3A0h at 4DFB; the gEffectMode clear at 4923; the QUIRK 02F9 (6Fh where the ramp wants 5Fh); no far call or far pointer to 0000:03A1 in INTRO.EXE (search for A1 03 00 00 and 9A A1 03 00 00); gBlockSpare at 36DE; the variable offsets from 3F1E to 3F7A; and the names of the shared routines against I0892.C:127 to 132 (GetDriverBlock, RebaseDriverBlock, StartAdlibDriver, StopAdlibDriver), which agree.

## Summary

Round 4 (fa015bd): passed. No must-fix open: the round 3 must-fix items are fixed right (ticks stay), and the changed lines are sound against the code, the labels and the addresses they name. Open, not blocking: R4-1 (should, the +28h writer list is partial: the B9h tail, L_526C and VoiceSilence also write +28h), and the notes R4-2, R4-3 and the registry note (7 i_text rows for 44 functions; `review-pass` stamps those 7 rows only, so the split is for the human). Evidence: make match FILE=src/I_TEXT.ASM 44 of 44, exit 0; make readability UNIT=i_text 0 findings; the changed lines checked against src/I_TEXT.ASM, src/I0892.C and src/E07C8.C as listed above. Nothing escalated: no matcher, spec or pin question.

Round 2 (bb3f897): not passed. Two must-fix, both in changed comment lines: gRhythmKeyBits (241; reopened from round 1, its readers test nonzero and C1h also sets it) and gTempoRate (242; its copy and command citations name the wrong lines). Two shoulds (the stale INTRO caller lines at 493; the C6h list at 254). Nothing escalated: no matcher, spec or pin question.

Round 1 summary:

The unit reads well at the level of its structure: the header, the pitch-curve and driver tables, the INT 08h comments, and the script interpreter are understandable from the source alone, and the addresses it cites check out. The errors are concentrated in four voice-level routines (VoiceKeyOff, VoiceSilence, StartVoiceNote and WriteVoiceReg), in one INT 08h bullet carried over from the EOB build (gBusyCountdown), in one QUIRK address (3EFC should be 3EFB), and in one stale cross-file citation (E07C8.C:65). The placeholder names are the L_ labels of the script handlers and the spare variables, which is fine. GetScriptFlag and gEntryTable keep their "Purpose unknown" with the evidence cited. The registry question in the note above is for the human, not the decompiler.
