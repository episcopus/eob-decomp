# Review: i_text

Reviewer: rev-haiku
Commit reviewed: f05a54b (main when I started). This branch was at 991e9b2; src/I_TEXT.ASM, re/symbols.toml and re/globals.toml are identical on both for this unit.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Checked: `make match FILE=src/I_TEXT.ASM` 44 of 44 functions match; `make readability UNIT=i_text` 0 findings. The addresses the comments cite were checked with tools/re/disasm.py (the ISR, PIT, sample, dispatch and tempo addresses); the curve bytes, the block bytes and the "Runtime overlay error" string were read from eob/INTRO.EXE (load module at file offset 0E00h).

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

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

- [ ] must-fix src/I_TEXT.ASM:241 (gRhythmKeyBits; round 1 must-fix reopened). Two claims in the comment are wrong.
  (a) "its readers test bit 5 as rhythm mode (VoiceKeyOff line 1829, VoiceSilence line 2282)": both readers test the whole
  byte for nonzero (`cmp byte ptr ds:[gRhythmKeyBits], 0` at lines 1829 and 2282); bit 5 is not tested anywhere.
  (b) "the OR of every C2h operand since the last C3h" omits the direct store `mov byte ptr ds:[gRhythmKeyBits], 20h` at
  line 1481, which is inside the C1h handler (L_4B01 at line 1398, L_4BE0 at line 1487). Fix: say C1h sets it to 20h
  (line 1481), C2h ORs its operand into it (lines 1502 to 1503), C3h clears it (line 1512), and VoiceKeyOff (1829) and
  VoiceSilence (2282) test it for nonzero.

- [ ] must-fix src/I_TEXT.ASM:242 (gTempoRate; a round 1 should, changed by the fix, now wrong). Two citations name the
  wrong code. (a) "copied to a voice's speed byte [di+25h] when its follow flag [di+40h] is set (line 1783 to 1784)": the
  copy is at lines 781 to 784 (cmp [di+40h] at 781, mov al,gTempoRate at 783, mov [di+25h],al at 784, in the UpdateVoices
  loop before L_45A5); lines 1783 and 1784 are the A0 register setup in SetVoicePitch. (b) "by commands A9h and BAh (lines
  1118 and 1340)": line 1118 (L_4881, script byte A9h) stores its own operand AH into [di+25h] and does not read
  gTempoRate. The byte is written only by command A6h (L_4871, line 1111), and copied by BAh (L_4A7B, line 1340). Fix:
  "written by A6h (line 1111); copied into a voice's speed byte [di+25h] by BAh (line 1340) and, when [di+40h] is set,
  by the UpdateVoices copy (lines 781 to 784)".

- [ ] should src/I_TEXT.ASM:493 (the 43A5 header): the address is right, but "src/I0892.C:420" and "declared at
  I0892.C:127" are stale. ShutdownGame is at I0892.C:444 and calls StopAdlibDriver at I0892.C:454; the extern is at
  I0892.C:149. Cite the address (0892:0835) and, if a line is kept, the current lines. E07C8.C:451 is right.

- [ ] should src/I_TEXT.ASM:254 (gRhythmLevelC): the C6h list "lines 1659, 1670, 1681 and 1692" omits line 1648, the store
  `mov byte ptr ds:[gRhythmLevelB+2], al` inside L_4D6B (line 1643). L_4D6B does store into B; add 1648 to the list.

- note src/I_TEXT.ASM:2106 (WriteVoiceReg): the test is `cmp byte ptr [di + 31h], 0`, a nonzero test of the whole byte.
  It is "the connection bit" only if byte 31h holds bit 0 and nothing else (the C0h comment says LoadVoicePatch keeps bit 0
  there). Not a finding; say "nonzero" if the bit is not the only content.

- note registry (round 1 note, still open): re/symbols.toml has 7 i_text entries for 44 functions (i0000_03fa, the
  `_RebaseDriverBlock` entry, covers 16216 bytes, including both INT 08h handlers). review-pass stamps only the entries
  that exist, so the split (one entry per function) is still needed before a passing review. Not changed in this round.

- [ ] should carried from round 1 (still open): rename `WriteVoiceReg` to `WriteVoiceLevels` (make rename); cite WHITEPAPER
  2.1.2 and 2.4 in the unit header; list the voice-record fields once (src/I_TEXT.ASM:281 to 282).

### Round 1

- [x] must-fix src/I_TEXT.ASM:313 (the 424D header): "counts gBusyCountdown down to zero" describes a byte this build does not decrement. The handler (lines 319 to 372) has no reference to it, and the unit header (lines 6 to 8) says the wait was dropped. E_TEXT.ASM:349 to 351 has the wait. Fix: delete the bullet. **Fixed:** Deleted the gBusyCountdown bullet; the handler has no such byte (the unit header already says the wait was dropped).

- [x] must-fix src/I_TEXT.ASM:182 (the 3D2C header): "QUIRK 3EFC" should be 3EFB. The 34h is at 0000:3EFB; 0000:3EFC holds 41h (eob/INTRO.EXE). Curve 12 starts at 3EDE (3D5E + 12 * 32) and the 34h is its index 29. The 3E6E quirk on the same line is right (19h at 3E6E). **Fixed:** Changed QUIRK 3EFC to 3EFB; the 34h is at 3EFB and 3EFC holds 41h.

- [x] must-fix src/I_TEXT.ASM:493 (the 43A5 header): "(E07C8.C:65)" is a comment on gTandyOn (E07C8.C:63 to 65), not a caller of StopAdlibDriver. The INTRO caller is ShutdownGame at 0892:0835 (src/I0892.C:420, declared at I0892.C:127). The EOB caller is E07C8.C:451. Cite the address, not a line of another file. **Fixed:** Replaced the E07C8.C:65 citation with the INTRO caller ShutdownGame at 0892:0835 (src/I0892.C:420, declared at I0892.C:127); kept the EOB caller E07C8.C:451.

- [x] must-fix src/I_TEXT.ASM:87 (the 03B0 SetSoundByte header): "idx selects the 300-word table at 0416" is wrong. idx selects one word of that table (lines 94 to 95: the near pointer at cs:0416 + 2 * idx), and reg is the offset into the record that word points to (lines 96 to 98). E177D.C:251 to 252 passes idx as the sound number and reg as 3 or 1. **Fixed:** Corrected the SetSoundByte comment: idx selects one word of the 300-word table (cs:0416 + 2*idx); reg offsets into the record that word points to.

- [ ] must-fix src/I_TEXT.ASM:240 (gRhythmKeyBits), REOPENED in round 2 (round 2 item 1): "the key bits last written to register BDh (bit 5 is the rhythm mode)" does not describe the byte. C2h stores `operand | old value` (lines 1502 to 1503), so the byte is the OR of every C2h operand since the last C3h, which clears it (line 1512). It is not the last value written to BDh, which also carries gRegBDDepth and 20h (lines 1504 to 1506). Its readers test it as rhythm mode (lines 1829 and 2282). **Fixed:** Rewrote gRhythmKeyBits as the OR of every C2h operand since the last C3h (which clears it), not the last BDh write; its readers test bit 5 as rhythm mode.

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

- [ ] should src/I_TEXT.ASM:2096 (WriteVoiceReg): the name is vague. It writes the two total-level registers (43h and 40h). A name such as WriteVoiceLevels says what it does; use `make rename` for it.

- [ ] should src/I_TEXT.ASM:1 to 19 (the unit header): no whitepaper section is cited, and READABILITY asks for one. Cite WHITEPAPER 2.1.2 (what segment 0 holds) and 2.4 (INTRO.EXE).

- [ ] should src/I_TEXT.ASM:281 to 282 (the voice records): the record fields (+3, +5, +8, +20h, +25h, +28h, +29h, +2Bh, +40h and the rest) are documented only where the code uses them. One comment, or EQU names, listing the fields would let the next reader follow the code without searching.

- [ ] note src/I_TEXT.ASM:309 to 316 and E177D.C:249: nothing in this build decrements a busy byte, and 3B36 is zero spare data here (inside gBlockSpare). If INTRO's code reaches PlaySoundAtCell's busy wait (E177D.C:249), that wait cannot end. The callers in src/I0*.C do not show whether it does. This is a question for the human, not a finding against this file.

- [ ] note the script commands (80h to C9h, the table at 536C) are described only in the code. WHITEPAPER does not define the script format. Each handler's label is an L_ placeholder; a one-line description of what each command does would help the next reader.

- [ ] note gScriptFlag (3A0): named for its one writer (C7h, line 1702) and its reader GetScriptFlag (03A1), which has no known caller. The name is acceptable as it stands.

- [ ] note registry (question for the human): re/symbols.toml has 7 entries for these 44 functions. Example: i0000_03fa `_RebaseDriverBlock` has size 16216, which covers 03FA to 4352, including both INT 08h handlers. This is the registry problem escalated for e17a6, fixed by task/split-on-match. Please run that for i_text before review-pass, so each function has its own entry.

- [ ] note checked and right, no action: the QUIRK 4627 (cmp al,70h against a 74-entry table), with "Runtime overlay error" starting at 0000:5400 just after the table; the PIT addresses 437D to 438B and the int 21h 2508h addresses 4372 to 4378 and 5021 to 5027; the write to 3A0h at 4DFB; the gEffectMode clear at 4923; the QUIRK 02F9 (6Fh where the ramp wants 5Fh); no far call or far pointer to 0000:03A1 in INTRO.EXE (search for A1 03 00 00 and 9A A1 03 00 00); gBlockSpare at 36DE; the variable offsets from 3F1E to 3F7A; and the names of the shared routines against I0892.C:127 to 132 (GetDriverBlock, RebaseDriverBlock, StartAdlibDriver, StopAdlibDriver), which agree.

## Summary

Round 2 (bb3f897): not passed. Two must-fix, both in changed comment lines: gRhythmKeyBits (241; reopened from round 1, its readers test nonzero and C1h also sets it) and gTempoRate (242; its copy and command citations name the wrong lines). Two shoulds (the stale INTRO caller lines at 493; the C6h list at 254). Nothing escalated: no matcher, spec or pin question.

Round 1 summary:

The unit reads well at the level of its structure: the header, the pitch-curve and driver tables, the INT 08h comments, and the script interpreter are understandable from the source alone, and the addresses it cites check out. The errors are concentrated in four voice-level routines (VoiceKeyOff, VoiceSilence, StartVoiceNote and WriteVoiceReg), in one INT 08h bullet carried over from the EOB build (gBusyCountdown), in one QUIRK address (3EFC should be 3EFB), and in one stale cross-file citation (E07C8.C:65). The placeholder names are the L_ labels of the script handlers and the spare variables, which is fine. GetScriptFlag and gEntryTable keep their "Purpose unknown" with the evidence cited. The registry question in the note above is for the human, not the decompiler.
