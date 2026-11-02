# Review: i_text

Reviewer: rev-haiku
Commit reviewed: f05a54b (main when I started). This branch was at 991e9b2; src/I_TEXT.ASM, re/symbols.toml and re/globals.toml are identical on both for this unit.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Checked: `make match FILE=src/I_TEXT.ASM` 44 of 44 functions match; `make readability UNIT=i_text` 0 findings. The addresses the comments cite were checked with tools/re/disasm.py (the ISR, PIT, sample, dispatch and tempo addresses); the curve bytes, the block bytes and the "Runtime overlay error" string were read from eob/INTRO.EXE (load module at file offset 0E00h).

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [ ] must-fix src/I_TEXT.ASM:313 (the 424D header): "counts gBusyCountdown down to zero" describes a byte this build does not decrement. The handler (lines 319 to 372) has no reference to it, and the unit header (lines 6 to 8) says the wait was dropped. E_TEXT.ASM:349 to 351 has the wait. Fix: delete the bullet.

- [ ] must-fix src/I_TEXT.ASM:182 (the 3D2C header): "QUIRK 3EFC" should be 3EFB. The 34h is at 0000:3EFB; 0000:3EFC holds 41h (eob/INTRO.EXE). Curve 12 starts at 3EDE (3D5E + 12 * 32) and the 34h is its index 29. The 3E6E quirk on the same line is right (19h at 3E6E).

- [ ] must-fix src/I_TEXT.ASM:493 (the 43A5 header): "(E07C8.C:65)" is a comment on gTandyOn (E07C8.C:63 to 65), not a caller of StopAdlibDriver. The INTRO caller is ShutdownGame at 0892:0835 (src/I0892.C:420, declared at I0892.C:127). The EOB caller is E07C8.C:451. Cite the address, not a line of another file.

- [ ] must-fix src/I_TEXT.ASM:87 (the 03B0 SetSoundByte header): "idx selects the 300-word table at 0416" is wrong. idx selects one word of that table (lines 94 to 95: the near pointer at cs:0416 + 2 * idx), and reg is the offset into the record that word points to (lines 96 to 98). E177D.C:251 to 252 passes idx as the sound number and reg as 3 or 1.

- [ ] must-fix src/I_TEXT.ASM:240 (gRhythmKeyBits): "the key bits last written to register BDh (bit 5 is the rhythm mode)" does not describe the byte. C2h stores `operand | old value` (lines 1502 to 1503), so the byte is the OR of every C2h operand since the last C3h, which clears it (line 1512). It is not the last value written to BDh, which also carries gRegBDDepth and 20h (lines 1504 to 1506). Its readers test it as rhythm mode (lines 1829 and 2282).

- [ ] must-fix src/I_TEXT.ASM:1826 (the 4EF7 VoiceKeyOff header): "skips the first six voices while a sample plays" is wrong. The routine returns at once when gRhythmKeyBits is nonzero (rhythm mode on) and the voice index is 6 or more (lines 1829 to 1833). Voice 9 gets no B0 write (lines 1835 to 1836). No sample test appears in the routine. Say: skips voices 6 to 9 while rhythm mode is on.

- [ ] must-fix src/I_TEXT.ASM:1847 (the 4F1F StartVoiceNote header): "records the level byte" is wrong. AH, the note's duration, goes to gNoteDuration (line 1850) and to the voice's delay counter [di+5] (lines 1852 to 1857, and 1874), with a random part added when [di+36h] is set. gNoteDuration's own comment (line 236) says it is the duration.

- [ ] must-fix src/I_TEXT.ASM:2096 (the 50C8 WriteVoiceReg header): "when the voice is two-operator, register 40h" is wrong. The test is record byte 31h (line 2106), the connection bit. Register 40h (the modulator level) is written only for an additive voice, where both operators sound (the 512F header, line 2155). Every AdLib melodic voice is two-operator, so that cannot be the test. "the voice 422C-table register 43h" names nothing: 422C is gOperatorOffsets (lines 303 to 304). Write "register 43h (carrier level) of the voice's operator (gOperatorOffsets, 422C)".

- [ ] must-fix src/I_TEXT.ASM:2279 (the 520C VoiceSilence header), two wrong claims. (a) "writes ffh to its level registers": the ffh goes to 60h, 63h, 80h and 83h (lines 2292 to 2307), which are attack/decay and sustain/release. The level registers 40h and 43h are not written. (b) "then zeroes its gate and key-on": B0 is written 0 (lines 2308 to 2311) and then 20h (lines 2312 to 2313), and 20h is the key-on bit, so the voice ends up keyed on. The header also leaves out the guard at lines 2282 to 2286: with rhythm mode on, voices 6 and up return at once.

- [ ] must-fix src/I_TEXT.ASM:1878 (the 4F57 StartSample header): "programs the PIT and the speakers" is wrong. StartSample reads or writes no speaker port (no port 61h between lines 1879 and 1978); StartAdlibDriver clears the speaker gate (lines 480 to 482). StartSample writes AdLib registers for the sample's voice (lines 1891 to 1910, 1929 to 1932), writes register 40h = 0 (lines 1947 to 1954), and programs the PIT (lines 1912 to 1917 read it, lines 1963 to 1970 set the 4Bh divisor). The header also does not say its arguments: a far sample pointer at [bp+6] and a length at [bp+0ah] (lines 1934 to 1941).

- [ ] should src/I_TEXT.ASM:6 to 8 (the unit header): "three instructions dropped" lists six instructions: the cld, the cmp, je and dec of the busy wait, and the push di and pop di pair (the dispatch has one pair fewer than E_TEXT.ASM: 13 pairs here, 14 there). Say six instructions in three places.

- [ ] should src/I_TEXT.ASM:163 to 164 (the 0416 block header): "zeros but for 12 bytes in three places" is loose. The nonzero bytes are the word 066Eh at 0416, the word 0671h at 0542 (gPatchTable's first word), and 12 bytes at 0670, of which 10 are nonzero. "adlib.dat's first word is 0258h" has no source here: adlib.dat is not in eob/, and no document gives the value. Cite it or drop it.

- [ ] should src/I_TEXT.ASM:170 to 171 (gPatchTable): no comment. Its name is backed: the script command 90h reads patch pointers at [bx+gPatchTable] (line 1017), and GetDriverTable arg 3 returns it (line 713). Add a comment with those.

- [ ] should src/I_TEXT.ASM:178 to 179 (gEntryTable): "in the order the original links them" has no evidence in the unit. Tag it [H] or drop it.

- [ ] should src/I_TEXT.ASM:241 (gTempoRate): "the followers' voice speed" is vague. The byte is copied into a voice's speed byte [di+25h] when that voice's follow flag [di+40h] is set (lines 1783 to 1784), and by commands A9h and BAh (lines 1118 and 1340). Say so.

- [ ] should src/I_TEXT.ASM:246 (gEffectIndex): "L_4AAC clears it" holds only for effect type 2 (lines 1378 to 1381). Say so.

- [ ] should src/I_TEXT.ASM:251 to 254 (gRhythmLevelB): "B is stored by L_4CC7" is incomplete. C6h (L_4D6B, lines 1659, 1670, 1681 and 1692) also stores into B.

- [ ] should src/I_TEXT.ASM:449 (the 4352 header): "clears the ring" is loose. StartAdlibDriver resets the queue indices gQueueRead and gQueueWrite (lines 483 to 484); the ring's bytes are left as they were.

- [ ] should src/I_TEXT.ASM:543 to 545 (QueueSound): "its high byte lands in the next ring byte" holds for indices 0 to 14. Index 15's high byte lands at 3F7A, which is gVoiceRecord0 byte 0. ACh, ADh and VoiceCarrierLevel read or write that byte (lines 1133, 1149 and 2128). That is a quirk: mark it `QUIRK 43E7:` and say what happens.

- [ ] should src/I_TEXT.ASM:565 to 568 (ProcessSoundQueue): omits that VoiceSilence is skipped for voice 9. The test at line 608 compares the doubled voice number (line 595 shifts bx left) with 12h, so it is voice 9.

- [ ] should src/I_TEXT.ASM:692 (GetDriverTable): "returns a near driver table pointer for arg 0 to 4" is loose. Arg 0 returns cs, a segment (line 699). Args 1 and 2 both return gSoundTable (lines 704 and 709). Arg 3 is gPatchTable (0542h, line 713) and arg 4 is gCurveTablePtrs (020Ch, line 717). The header lists four values for five args.

- [ ] should src/I_TEXT.ASM:724 (ResetVoices): omits the level writes that silence voices 0 to 8: 3Fh to registers 40h and 43h (lines 740 to 750).

- [ ] should src/I_TEXT.ASM:1714 (SetVoicePitch): on entry AL is the note, which is stored at +41h (line 1718). The header does not say that, and it omits the pitch-curve adjust from +42h (lines 1749 to 1781).

- [ ] should src/I_TEXT.ASM:1847 (StartVoiceNote): "the 2dh shift" is a multiplier. The routine adds (AH shr 3) [di+2dh] times into [di+21h] (lines 1860 to 1872), which is the key-off point. Say that.

- [ ] should src/I_TEXT.ASM:2011 (TickTempo): "the step byte 3F45" is the beat count, gTempoBeat (line 258).

- [ ] should src/I_TEXT.ASM:2191 (ClearVoice): "resets its delay and script pointers". The script pointer [di+3] is in the zeroed range. [di+29h] and [di+2Bh] are the per-tick handlers, set to L_5255 (a ret, lines 2210 to 2211). Say so.

- [ ] should src/I_TEXT.ASM:2223 (LoadVoicePatch): "E177D.C:31" is ATTENUATION_ADLIB_LIMIT. The full-attenuation define and the whitepaper quote it cites are at E177D.C:32 to 33.

- [ ] should src/I_TEXT.ASM:2437 (SoundRandom): "the driver random byte" does not say the function returns the new seed in AX. Its caller at line 1231 uses AX.

- [ ] should src/I_TEXT.ASM:121 (GetSoundData): "returns the far driver routine pointer cs:[0416 + 2*idx]" reads as if the pointer were at that address. The function returns dx = cs and ax = the word stored there (lines 127 to 132).

- [ ] should src/I_TEXT.ASM:2096 (WriteVoiceReg): the name is vague. It writes the two total-level registers (43h and 40h). A name such as WriteVoiceLevels says what it does; use `make rename` for it.

- [ ] should src/I_TEXT.ASM:1 to 19 (the unit header): no whitepaper section is cited, and READABILITY asks for one. Cite WHITEPAPER 2.1.2 (what segment 0 holds) and 2.4 (INTRO.EXE).

- [ ] should src/I_TEXT.ASM:281 to 282 (the voice records): the record fields (+3, +5, +8, +20h, +25h, +28h, +29h, +2Bh, +40h and the rest) are documented only where the code uses them. One comment, or EQU names, listing the fields would let the next reader follow the code without searching.

- [ ] note src/I_TEXT.ASM:309 to 316 and E177D.C:249: nothing in this build decrements a busy byte, and 3B36 is zero spare data here (inside gBlockSpare). If INTRO's code reaches PlaySoundAtCell's busy wait (E177D.C:249), that wait cannot end. The callers in src/I0*.C do not show whether it does. This is a question for the human, not a finding against this file.

- [ ] note the script commands (80h to C9h, the table at 536C) are described only in the code. WHITEPAPER does not define the script format. Each handler's label is an L_ placeholder; a one-line description of what each command does would help the next reader.

- [ ] note gScriptFlag (3A0): named for its one writer (C7h, line 1702) and its reader GetScriptFlag (03A1), which has no known caller. The name is acceptable as it stands.

- [ ] note registry (question for the human): re/symbols.toml has 7 entries for these 44 functions. Example: i0000_03fa `_RebaseDriverBlock` has size 16216, which covers 03FA to 4352, including both INT 08h handlers. This is the registry problem escalated for e17a6, fixed by task/split-on-match. Please run that for i_text before review-pass, so each function has its own entry.

- [ ] note checked and right, no action: the QUIRK 4627 (cmp al,70h against a 74-entry table), with "Runtime overlay error" starting at 0000:5400 just after the table; the PIT addresses 437D to 438B and the int 21h 2508h addresses 4372 to 4378 and 5021 to 5027; the write to 3A0h at 4DFB; the gEffectMode clear at 4923; the QUIRK 02F9 (6Fh where the ramp wants 5Fh); no far call or far pointer to 0000:03A1 in INTRO.EXE (search for A1 03 00 00 and 9A A1 03 00 00); gBlockSpare at 36DE; the variable offsets from 3F1E to 3F7A; and the names of the shared routines against I0892.C:127 to 132 (GetDriverBlock, RebaseDriverBlock, StartAdlibDriver, StopAdlibDriver), which agree.

## Summary

The unit reads well at the level of its structure: the header, the pitch-curve and driver tables, the INT 08h comments, and the script interpreter are understandable from the source alone, and the addresses it cites check out. The errors are concentrated in four voice-level routines (VoiceKeyOff, VoiceSilence, StartVoiceNote and WriteVoiceReg), in one INT 08h bullet carried over from the EOB build (gBusyCountdown), in one QUIRK address (3EFC should be 3EFB), and in one stale cross-file citation (E07C8.C:65). The placeholder names are the L_ labels of the script handlers and the spare variables, which is fine. GetScriptFlag and gEntryTable keep their "Purpose unknown" with the evidence cited. The registry question in the note above is for the human, not the decompiler.
