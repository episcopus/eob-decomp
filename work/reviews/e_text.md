# Review: e_text

Reviewer: rev-haiku
Commit reviewed: 2ba0b7c (the review branch; src/E_TEXT.ASM is unchanged on main at 4381e9d, the main changes since 2ba0b7c that touch re/symbols.toml are other units)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

Round 1: `make match FILE=src/E_TEXT.ASM` 44 of 44 functions ok (unit complete, 19996 bytes); `make readability UNIT=e_text` 0 findings. The unit was read against the other files of `src/` (the declarations and comments of E177D.C and E07C8.C, the extern names they use), against WHITEPAPER 2.7, and against the bytes of `eob/EOB.EXE` for the claims that turn on instruction order (`tools/re/disasm.py eob/EOB.EXE 0000 --start 0x400C --len 24`, `--start 0x4E34 --len 224`; read only).

Checked and right in round 1:

- The data layout of 020Ch to 4F94 agrees with the comments: gCurveRamp at 029Ah, so QUIRK 02F9 is its index 5Fh (value 6Fh, neighbours 5Eh and 60h); gDriverTablePtrs 039Ch; gScriptFlag 03A0h; the block 0416h to 36DDh and gBlockSpare 36DEh to 3943h (13000 + 614 bytes); gDriverTables 3944h (498 bytes), the variables 3B36h to 3B92h (93 bytes), the voice records 3B93h (ten of 43h), the voice tables 3E31h.
- The command table: 74 entries, and exactly 20 name L_4311 (the comment's count holds). QUIRK 424C: the bound is 70h, the table has 4Ah entries. Entry numbers in the function comments hold (SetNote is the ninth entry, GetSampleOffset the seventh, GetVoiceState the sixth).
- The shared names match their declarations in src/E177D.C (GetDriverBlock, SetSoundByte, GetSoundByte, GetSoundData, RebaseDriverBlock, QueueSound) and src/E07C8.C (StartAdlibDriver, StopAdlibDriver). E177D.C:152 (FileRead into GetDriverBlock), E177D.C:249 (the busy wait) and E177D.C:268 (gBusyCountdown set to 0Bh) hold.
- The INT 08h facts: divisor 0555h (874 Hz, 3FA8 to 3FB0), 0Ch passes (72.8 Hz) and 30h passes (18.2 Hz) with the chain to the BIOS handler, the sample divisor 004Bh (15.9 kHz), the end condition of the sample, and QUIRK 3EE5 (no chain to the BIOS handler).
- The gEffectMode clear at 454B (the command B3h, L_4541) is where the comment puts it.
- WHITEPAPER 2.7 says "40h to 55h = 3Fh", which is what the LoadVoicePatch comment relies on.

Not checked: the claim that the block's first word in ADLIB.DAT is 0258h (the file is inside eobdata1.pak and is not decoded in this worktree; `make data` would show it).

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Round 1: must-fix items

- [ ] must-fix src/E_TEXT.ASM:83 (SetSoundByte, 0000:03B0): "idx selects the 300-word table at 0416" is wrong. The code (03B0 to 03C4: `mov bx,[bp+6]; shl bx,1; mov bx,cs:[bx+0416h]; add bx,[bp+8]; mov [bx],al`) reads one word of the 300-word offset table, at 0416h + 2*idx, and that word is the base of a sub-table; reg is added to that base. Say that. The same wording is in src/E177D.C:117 (the E177D owner's line, a note for that unit).

- [ ] must-fix src/E_TEXT.ASM:159 (DriverBlock, 0000:0416): "zeros but for 12 bytes in three places" is the wrong count. The source holds dw 066Eh at 0416h (2 bytes, line 164), dw 0671h at 0542h (2 bytes, gPatchTable, line 167) and a 12-byte run at 0670h (line 169): 16 non-zero bytes in three places. Say so.

- [ ] must-fix src/E_TEXT.ASM:495 (StopAdlibDriver, 0000:3FCA): "stops the PIT" is wrong. At 3FEA to 3FF0 the code writes 00h twice to port 40h after the 36h mode write, which sets divisor 0000h: the BIOS default of 18.2 Hz, not a stop. Say "returns the PIT to its default divisor (0000h, 18.2 Hz)". The citation "(E07C8.C:65)" is also the wrong code: E07C8.C:65 is the gTandyOn declaration; the call is at E07C8.C:451 (in ShutdownGame, 07C8:058A). Cite the address 07C8:058A instead.

- [ ] must-fix src/E_TEXT.ASM:1830 (VoiceKeyOff, 0000:4B1F): "skips the first six voices while a sample plays" is wrong on three counts. The test is gRhythmKeyBits (3B40, nonzero while rhythm mode is on; C1h sets it to 20h), it skips voices 6 and up (index 6 or more, `cmp gVoiceIndex,6; jl`, so voices 0 to 5 are written), and gSampleEnable is not read. Voice 9 is also skipped by the next test (`cmp gVoiceIndex,9; je`). Say that.

- [ ] must-fix src/E_TEXT.ASM:1851 (StartVoiceNote, 0000:4B47): "records the level byte" is wrong. AH is the note duration: it is stored in gNoteDuration (3B3B; the variables comment at line 233 agrees), which the script uses to decide whether to go on (L_423E). "the 2dh shift" is a count: when [di+2dh] is nonzero, [di+21h] becomes (duration >> 3) summed [di+2dh] times (4B5F to 4B7B), the key-off count UpdateVoices compares against. When [di+36h] is nonzero the duration gets SoundRandom masked with [di+36h] added to it. Rewrite the comment from those three paths, with the names of the fields (see the record-layout should below).

- [ ] must-fix src/E_TEXT.ASM:1882 (StartSample, 0000:4B7F): "programs the PIT and the speakers" is wrong about the speakers. StartSample never touches port 61h (the speaker gate); it writes AdLib (OPL2) registers 20h to B0h for voice 0 (the sample setup at 4B7F to 4BD8) and sets the PIT to 004Bh. The speaker gate is cleared by StartAdlibDriver (3FAE, see the should below). The comment also does not describe the two arguments: a far pointer to the sample at [bp+6] (offset, then segment) and its length in bytes at [bp+0Ah] (the code stores them at gSamplePtrOff, gSampleStart and gSampleLength).

- [ ] must-fix src/E_TEXT.ASM:2283 (VoiceSilence, 0000:4E34): two wrong claims. (a) "writes ffh to its level registers": 0FFh goes to 60h, 63h, 80h and 83h (4E4A to 4E6B), which are attack/decay and sustain/release (the names LoadVoicePatch's comment gives them), not the level registers 40h and 43h. (b) "then zeroes its gate and key-on": at 4E74 B0h+voice is written 0, and then at 4E79 `mov al,20h; call _AdlibWrite` writes 20h to the same register (AdlibWrite keeps AX, so AH is still the register). The voice is left with key-on set and F-number 0. Describe what the bytes do, and mark `QUIRK 0000:4E79:` with that effect; the source is the truth here, so the comment must not claim otherwise.

- [ ] must-fix src/E_TEXT.ASM:545 (QueueSound, 0000:400C): "(stored as a word, so its high byte lands in the next ring byte)" is wrong at the last index. The ring gQueueRing is 16 bytes (3B83h to 3B92h). At index 15 the word store (0000:401B, `mov [bx+3B83h],ax`) writes 3B92h and 3B93h, and 3B93h is the first byte of gVoiceRecord0 (line 282). Mark `QUIRK 0000:401B:` and say that a sound queued at index 15 also writes voice 0's first record byte.

### Round 1: should

- [ ] should src/E_TEXT.ASM:174 (gEntryTable): "lists the eleven near entry points of the driver in the order the original links them". The routines end in retf (they are far routines entered through an offset), and nothing in the unit shows the order is the link order. Say "eleven routine offsets, far routines (each ends in retf), in the order listed; no reader in this file".

- [ ] should src/E_TEXT.ASM:166 (gPatchTable): no comment at all. What is known: a table of near pointers to the instrument patches (11 bytes each, read by LoadVoicePatch at 4DC8 through `mov si,[bx+gPatchTable]`, from the 90h command at 43AD and the C1h command at 4729). It sits inside the 300-word table that RebaseDriverBlock rebases (word 150), so its entries are rebased too. Say so.

- [ ] should src/E_TEXT.ASM:451 (StartAdlibDriver, 0000:3F77): the comment omits the speaker gate. After the PIT divisor write (3FAE onward) the code does `in al,61h; and al,0FEh; out 61h,al`, clearing the gate of PIT channel 2, so the PC speaker is silenced while the AdLib driver plays. Add it.

- [ ] should src/E_TEXT.ASM:26 (gScriptFlag, 020Ch header): "read back by GetScriptFlag". GetScriptFlag (03A1) has no caller in the EXE; its own comment says Purpose unknown. Say "read by GetScriptFlag (03A1), which no caller names".

- [ ] should src/E_TEXT.ASM:237 (gRhythmKeyBits): "the key bits last written to register BDh" is not what the code does. C2h ORs the operand into it (`or al,[gRhythmKeyBits]; mov [gRhythmKeyBits],al`, 4808 to 4837), so it accumulates the key bits written since C3h (4838) clears it; C1h sets it to 20h. Say so.

- [ ] should src/E_TEXT.ASM:694 (GetDriverTable, 0000:4102): "returns a near driver table pointer for arg 0 to 4: cs, block table, 542h, 20ch". Argument 0 returns cs, which is a segment, not a pointer; arguments 1 and 2 both return gSoundTable (the list names "block table" once); arguments above 4 return whatever AX held. Say each.

- [ ] should src/E_TEXT.ASM:2441 (SoundRandom, 0000:4F84): "the driver random byte". The routine adds 9248h to gRandomSeed (the word at 3B47), rotates it right three times, stores it and returns the new 16-bit seed in AX. Callers use it as a word (the B5h command, 45AD) and as a byte (StartVoiceNote). Say "advances gRandomSeed and returns the new seed".

- [ ] should src/E_TEXT.ASM:2015 (TickTempo, 0000:4C8D): "advances the tempo counters at 3B5D/3B5C and the step byte 3B5E". 3B5E is gTempoBeat, the beat count (the variables comment at line 255 says so); TickTempo adds one to it each time gTempoCounter reaches zero. Use the name.

- [ ] should src/E_TEXT.ASM:2100 (WriteVoiceReg, 0000:4CF0): "writes the voice 3E45-table register 43h and, when the voice is two-operator, register 40h" is garbled, and "two-operator" is not what byte 31h records. It is the connection bit (LoadVoicePatch keeps bit 0 of the C0h byte there; 1 is additive, both operators sound). Say: "writes the carrier level register (43h plus the voice's operator offset); when +31h is set (additive connection) it also writes the modulator level (40h)".

- [ ] should src/E_TEXT.ASM:2195 (ClearVoice, 0000:4D95): "resets its delay and script pointers". The code sets +25h (the voice's speed, UpdateVoices copies gTempoRate into it) to FFh, +35h to 1, +08h (the priority) to 0, and both handler pointers +29h and +2Bh to L_4E7D (a ret). The delay (+05h) is cleared only by the zeroing. Name the fields.

- [ ] should src/E_TEXT.ASM:2045 (GetVoiceState, 0000:4CB1): the name is vague: it returns the note delay counter (+05h), which the comment says. "GetVoiceDelay" would be specific. The decompiler can apply it with `make rename OLD=GetVoiceState NEW=GetVoiceDelay WHY=...`, since the name is shared.

- [ ] should src/E_TEXT.ASM:761 (UpdateVoices, 0000:418F): the comment says the loop runs "its command through the 4F94 jump table". It does not say that the note delay +05h counts down each tick and the script runs only when it reaches zero (41D8, L_41F9), or that every voice also calls its two handlers, +29h and then +2Bh, each tick (L_425F). Say so.

- [ ] should src/E_TEXT.ASM:2227 (LoadVoicePatch): the citation "E177D.C:31" points at ATTENUATION_ADLIB_LIMIT (line 31 of src/E177D.C); LEVEL_FULL_ATTENUATION, the define meant, is on line 33 and is 177D:01AF in the E177D comment. Cite the address (177D:01AF) or the whitepaper section.

- [ ] should src/E_TEXT.ASM:4, 157, 231 and 310 (the header and comments citing src/E177D.C by line: "E177D.C:147-149", ":152", ":249", ":268"): these lines move whenever E177D.C changes. Cite the addresses instead: the card test and LoadSoundDriver at 177D:000A, the busy wait at 177D:0135, the set of gBusyCountdown at 177D:0201 (the E177D.C comments give these).

- [ ] should src/E_TEXT.ASM:282 to 299 (the voice records and tables): the record fields are never named, yet UpdateVoices, its helpers, SetVoicePitch and the ISR address them by raw offsets ([di+05h], [di+08h], [di+25h], [di+26h], [di+28h], [di+29h], [di+2Bh], [di+2Dh], [di+2Eh], [di+2Fh], [di+35h], [di+41h] and so on). The record layout is the largest readability gap in this unit. Give each field a name (EQU or a comment table next to the records), with the code that proves it. Evidence already in the file: +03h the script pointer (402C: `mov [di+3],si`), +05h the note delay (418F, 41D8), +08h the priority (402C), +25h the speed, +26h the phase accumulator (41CA to 41D8), +28h the B0h image (3E66, 4AEF), +29h and +2Bh the per-tick and per-voice handlers (425F), +35h the key-off count (41F7).

- [ ] should src/E_TEXT.ASM:1718 (SetVoicePitch, 0000:4A3A): the comment says "builds the voice frequency and octave from its record" but the note argument (AL, stored at +41h: octave in the high nibble, semitone in the low nibble) is not described, and the comment does not say that the tune offsets at +07h and +12h are added. Name the argument and say what it holds.

### Round 1: notes

- [ ] note src/E07C8.C:157 to 159 (another unit's comment): "stops PIT channel 0" repeats the wrong claim in the must-fix above, and it cites src/E_TEXT.ASM:495-522 by line. The owner of e07c8 should fix both; this unit does not edit it.

- [ ] note src/E177D.C:117 (another unit's comment): "idx indexes the block's 300-word table" is the same wording as the must-fix at line 83 here; the e177d owner should replace it with the address-based wording.

- [ ] note src/E_TEXT.ASM:637 (SetNote, 0000:409E): `and ax,7fffh` strips bit 15 from the note argument before it is used as an index, so bit 15 is a flag the caller sets. Nothing in the unit says what the flag means. Write "Purpose unknown:" with that fact.

- [ ] note src/E_TEXT.ASM:165 (the block's first word 0258h in ADLIB.DAT): not checked in this worktree (see the Round 1 header). Check it with `make data` before the review passes.

## Summary

The unit is understandable from its source alone at the top level: the header, the data sections and the names all carry evidence (the curve tables, the 3944 and 3B36 tables, the INT 08h handlers, and the shared names with E177D.C and E07C8.C all check out against the bytes and the other units). The problems are in the function comments that describe behaviour: eight must-fix items are wrong about what the code does (the PIT's end state, the key-off test, the note duration, the speaker, the silencing registers and the final key-on, the queue's last index, the block's byte count, and the wording of the block's index). The largest gap is the voice record layout, which is never named although almost every routine in the driver addresses it by raw offset. Names that rest on no evidence are placeholders only where the comment says Purpose unknown (GetScriptFlag, gEntryTable, the block's bytes); none is a confident name without evidence.
