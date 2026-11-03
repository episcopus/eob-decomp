# Review: s_text

Reviewer: rev-haiku
Commit reviewed: 9a2b2fc (HEAD of review/s_text; the last change to the unit's files is aef98e7)
Round 2 commit reviewed: 3e16e87 (main, the merge of unit/s_text; the diff from 9a2b2fc is round 1's fixes plus the probes and console routines, aa0e56a)
Round 3 commit reviewed: 3ec2009 (main; the diff from 3e16e87 is the round 2 fixes, b31cf07)
Round 4 commit reviewed: fbf7526 (main; the diff from 3ec2009 is the round 3 fixes, b233ff9)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Round 4 (rev-haiku, fbf7526)

Judged only: the round 3 must-fix and should items, and the changed lines since 3ec2009
(`git diff 3ec2009 main -- src/S_TEXT.C src/S_TEXT.ASM re/symbols.toml re/globals.toml`; only the two
src files changed, no map or globals change). The bytes were not re-proved; I checked the claims in the
changed lines against eob/START1.EXE (code address + 0x200 is the file offset: the call at 0487 is
`E8 6C 01`, whose target 0487 + 3 + 016Ch is 05F6, PutChar).

Evidence, per changed line:
- TestCrtcRegister (0000:04A3): `B0 0F EE 42 EC 8A E0 B0 66 EE B9 00 01 E2 FE EC 86 E0`. `in al,dx` is at
  04A7, `mov ah,al` at 04A8, the second `in al,dx` at 04B2 and `xchg` at 04B3. The header's account of AL
  and AH is right: AL the first read-back after the swap, AH the second; AL and AH are set by the routine.
- Int2FHandler (0000:0620): `55 8B EC 57 56 1E CF`: push bp, mov bp,sp, push di, push si, push ds, iret at
  0626. The iret pops DS, SI and DI as IP, CS and FLAGS; BP stays on the stack. Right.
- InstallInt2FHandler (0000:062C): `push ds` at 0631, `mov ax,offset Int2FHandler` at 0632, `mov dx,ax` at
  0635; nothing in the routine sets DS. The vector is DGROUP:0620h. Right.
- Nothing reaches the two routines: START1.EXE has no `CD 2F` (count 0); no near call, jump, short jump or
  far call targets 0620 or 062C; the one reference to 0620 is the immediate at 0632. Right.
- PrintString (0000:0474): `push si` at 0477, `xor si,si` at 0478, `mov ax,si` at 047C, `inc si` at 047E,
  the call at 0487, the condition `cmp byte ptr [bx+si],0` at 048E. PutChar's `push si` is at 05FA and its
  `pop si` at 0604. The increment precedes the call and i survives it. Right.
- gMode: `cmp byte ptr [0534h],5` at 025E, `jle` at 0263 (7Eh, a signed compare), `mov byte ptr [0534h],0` at
  0265. So a gDetectedMode of 80h to FFh is not clamped, and 6 to 127 become 0. gModeMaskTable is at ds:00A9
  (S_TEXT.C:23), so index -128 is ds:0029, before the table. Right.
- No em or en dash in the changed lines.

The round 3 must-fix items (the Int2FHandler iret, the Int2FHandler header's vector sentence, and gMode)
are fixed right and stay ticked. The round 3 shoulds (TestCrtcRegister's inputs, PrintString's reason and
the InstallInt2FHandler QUIRK's cause) are fixed right and stay ticked.

### Findings

- [ ] should src/S_TEXT.ASM:347-348 (InstallInt2FHandler QUIRK): "even an INT 2Fh would jump to
  DGROUP:0620h, a data address, because of the DS:DX mismatch" holds only after InstallInt2FHandler has run.
  Until then the vector is whatever DOS holds, so the sentence reads as if the vector were already
  DGROUP:0620h. Say "once InstallInt2FHandler has run, an INT 2Fh would jump to DGROUP:0620h". Clarity only:
  the claim is true with that condition, so it does not block.

### Summary (round 4)

The unit passes. The round 3 must-fix items are fixed right, every changed line checks against the
bytes, and the one new finding is a should (the INT 2Fh sentence needs its condition) that does not block.

## Round 3 (rev-haiku, 3ec2009)

Judged only: the round 2 must-fix items, and the changed lines since 3e16e87
(`git diff 3e16e87 main -- src/S_TEXT.C src/S_TEXT.ASM re/symbols.toml re/globals.toml`; only the two
src files changed). The bytes were not re-proved: `make match FILE=src/S_TEXT.ASM` 13 of 13 and
`FILE=src/S_TEXT.C` 2 of 2 both exit 0, and `make readability UNIT=s_text` reports 0 findings. I read the
code and the EXE bytes for the claims in the changed lines.

Evidence: code address + 0x200 is the file offset in eob/START1.EXE (PutChar 05F6 is file 0x7F6,
Int2FHandler 0620 is file 0x820). `CD 2F` occurs nowhere in START1.EXE. The strings at ds:0172 and
ds:01A6 begin 0Dh 0Ah (file 0x1E12 and 0x1E46). Round 2's fixes for TestCrtcRegister's write-back, GetKey's
three returns, InstallInt2FHandler's DS:DX, PutChar, DetectMouse, gDetectedMode, gNote and gModePrompt are
right; the three must-fix items below are not.

### Findings

- [x] must-fix src/S_TEXT.ASM:319-320 (Int2FHandler header; reopens round 2's item at :317-319): "the iret
  takes the first three of those words as the return IP, CS and FLAGS" names the wrong three. The pushes
  are BP, DI, SI, DS in that order (0000:0620 to 0625: `push bp; mov bp,sp; push di; push si; push ds`), so
  DS is on top of the stack. iret (0000:0626) pops IP from the top, so it takes the last three pushed,
  DS, SI and DI, as IP, CS and FLAGS; BP stays on the stack. Say that.
  Fixed: the header now says the four pushes are in that order, that DS is on top, so `iret` (0000:0626)
  pops the last three pushed, DS, SI and DI, as the return IP, CS and FLAGS, and BP stays on the stack.
  Evidence: 0000:0620 `55 8B EC 57 56 1E CF` (push bp; mov bp,sp; push di; push si; push ds; iret).
- [x] must-fix src/S_TEXT.ASM:321 (Int2FHandler header): "InstallInt2FHandler sets it as the interrupt 2Fh
  vector" is wrong. InstallInt2FHandler (0000:062C) puts DGROUP:0620h in the vector (DS:DX, as its own
  header says at :339 to :343). It does not install this handler. Say that.
  Fixed: the header now says the install routine does not put this handler in the vector:
  InstallInt2FHandler (0000:062C) installs DGROUP:0620h (DS:DX), not the code's 0620h. The citation of
  InstallInt2FHandler's header line range was dropped (the header moved when this one grew two lines).
- [x] must-fix src/S_TEXT.C:33 (gMode): "or 0 when it is above 5" is wrong for 80h to FFh. The clamp is a
  signed compare: `cmp byte ptr [0534h],5` then `jle` (0000:025E to 0263), so the `gMode = 0` store at
  0000:0265 runs only for 6 to 127. The "above 5" wording is mine (round 2's should at :26), and it is
  imprecise. It matters: the CGA/EGA path of DetectVideoAdapter (0000:04E1 to 04F8) stores AX = 0180h into
  the first word, so gDetectedMode can be 80h. Then gMode is -128, not clamped, and `gModeMaskTable[gMode]`
  (:78) reads ds:0029, before the table. Say: "index into gModeMaskTable (:78): gDetectedMode as a signed
  byte, or 0 when it is 6 to 127 (:72 to :73, a signed compare), or 6 when the byte at F000:C000 is 21h
  (:76 to :77); a gDetectedMode of 80h to FFh is not clamped and reads before the table."
  Fixed: the gMode comment now reads as given above. Evidence: 0000:025E `80 3E 34 05 05 7E 05` (cmp
  byte [0534h],5; jle), so the signed compare leaves 80h to FFh unclamped.
- [x] should src/S_TEXT.ASM:46-50 (TestCrtcRegister header): "DX holds the CRTC base, AL the first read-back
  (written back at the end), AH the second read-back" reads as if AL and AH came in. The routine sets them
  itself (the first read-back `in al,dx` at :56, the second in AL and swapped into AH at :63 to :64). Say
  that DX is the input and AL and AH are the routine's own values at those points.
  Fixed: the header now says DX is the input (the CRTC base) and AL and AH are the routine's own values,
  not inputs: the first read-back `in al,dx` (0000:04A7) is kept in AH (0000:04A8) and the `xchg ah,al`
  (0000:04B3) leaves AL the first read-back and AH the second. The source-line citations were replaced by
  addresses (the header grew, so the old :56/:57/:64 line numbers would have shifted).
- [x] should src/S_TEXT.C:145-148 (PrintString comment): the because clause says why the condition reads
  after the call, not why i must survive it. The bytes (0000:047C to 0491) show `mov ax,si; inc si` at
  047C to 047E, then the call (0487), then the condition `cmp byte ptr [bx+si],0` (048E) after the return.
  i survives the call because PutChar saves and restores SI (0000:05FA push si, 0604 pop si). Say that, and
  that the increment comes before the call because the source is text[i++].
  Fixed: the comment now says the source is text[i++], so the increment precedes the call, and i survives
  it because PutChar saves and restores SI (0000:05FA push si, 0604 pop si); the condition reads after the
  return. The comment keeps four lines, so the file's line numbers do not shift.
- [x] should src/S_TEXT.ASM:343 (InstallInt2FHandler QUIRK): "because of that DS:DX mismatch the handler is
  never reached" gives the mismatch as the whole reason. The handler is unreached first because nothing
  calls InstallInt2FHandler and START1.EXE has no INT 2Fh (`CD 2F` nowhere in the file). The mismatch
  means that even an INT 2Fh would jump to DGROUP:0620h, a data address. Say both.
  Fixed: the QUIRK now says the handler is unreached first because nothing calls this routine and
  START1.EXE issues no INT 2Fh (the code bytes contain no CD 2F); even an INT 2Fh would jump to
  DGROUP:0620h, a data address, because of the DS:DX mismatch.

### Summary (round 3)

The unit does not pass. Round 2's Int2FHandler item is reopened (its iret wording is wrong), and the
Int2FHandler header also says InstallInt2FHandler sets the handler as the vector, which its own header
contradicts. The gMode "above 5" wording is wrong for the signed byte 80h to FFh; that wording is mine, and
the must-fix gives the exact clamp. The remaining three items are clarity: TestCrtcRegister's inputs, the
PrintString reason, and the InstallInt2FHandler QUIRK's cause. The unit is complete and matches; only the
comments need fixing.

Round 3 resolved (decompiler): the three must-fix and three should items are fixed in
src/S_TEXT.ASM/src/S_TEXT.C. Int2FHandler: the four pushes are BP, DI, SI, DS, DS is on top, and `iret`
(0000:0626) pops the last three pushed, DS, SI and DI, as IP, CS and FLAGS (BP stays); the header no
longer says InstallInt2FHandler installs this handler, it installs DGROUP:0620h. gMode: the clamp is the
signed `jle` at 0000:0263, so only 6 to 127 become 0 and 80h to FFh are not clamped. TestCrtcRegister: DX
is the input and AL and AH are the routine's own values. PrintString: i survives the call because PutChar
saves and restores SI, and the increment precedes the call because the source is text[i++].
InstallInt2FHandler QUIRK: unreached first because nothing calls it and START1.EXE has no `CD 2F`, and the
DS:DX mismatch would send even an INT 2Fh to DGROUP:0620h. Source-line citations that the edits would have
shifted were replaced by code addresses. Evidence: make match FILE=src/S_TEXT.C 2 of 2 and
FILE=src/S_TEXT.ASM 13 of 13, both exit 0; make readability UNIT=s_text 0 findings.

## Round 2 (rev-haiku, 3e16e87)

Judged only: the round 1 must-fix items, and the changed lines since 9a2b2fc (`git diff 9a2b2fc main -- src/S_TEXT.C src/S_TEXT.ASM re/symbols.toml`). Round 1's must-fix items are fixed right and stay ticked: the three probes and the four console and mouse routines are written and match their bodies; the header's table claim (record 1 is ProbeVga at 056A, enabled in the data) holds; the Main argument forms are right (:112, :120, :134); the DetectVideoAdapter values, the signed compare in NormalizeVideoInfo and the three gSelection uses (:90, :92, :104) match the code; and the library names are right (PutChar, GetKey and DetectMouse are this unit's labels, and build/re/rtl_start1.json gives `_execl` the value 2103, which is 0837h).

Evidence for the data claims: the DGROUP base that the unit's strings and tables use is file offset 0x1CA0 in eob/START1.EXE (not 0x1CA2, which I tried first). At that base the probe table at ds:0334 reads `01 6A 05`, `00 BA 05`, `00 E6 05`, `00 BD 04` (records start at 0334, 0337, 033A, 033D), ds:031A reads 0000h, 0101h, 0202h, 0000h, 0303h, ds:00AF is 03h, and the marks at ds:016E and ds:0170 are `*` and space, as the comments say.

### Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off with the commit that fixes it; `should` and `note` do not block.

- [x] must-fix src/S_TEXT.ASM:48 (TestCrtcRegister header): "DX holds the CRTC base, AH the first read-back and AL the value to write back" is wrong for AH at the write-back. `mov ah,al` (:56) keeps the first read-back in AH, then `xchg ah,al` (:63) swaps the two, so at `out dx,al` (:64) AL is the first read-back (the value written back) and AH is the second read-back, which `cmp ah,66h` (:65) tests. Say: "AL the first read-back, written back at the end; AH the second read-back, tested against 66h". This reopens round 1's should on TestCrtcRegister: the wording I asked for ("AH keeps the first read-back") was imprecise. Fixed: the header now says "DX holds the CRTC base, AL the first read-back (written back at the end), AH the second read-back (tested against 66h)".
- [x] must-fix src/S_TEXT.ASM:292-293 (GetKey header): "so the two ranges do not collide" is wrong. `cbw` (:309) sign-extends AL, so ASCII codes 128 to 255 come back as -128 to -1, the same values as the negated scan codes 1 to 127 (-1 to -127). Only ASCII 1 to 127 stays apart. Say what the code returns: ASCII 1 to 127 as they are, ASCII 128 to 255 negative, and an extended key as its negated scan code. Fixed: the header now gives all three returns: ASCII 1 to 127 as it is, ASCII 128 to 255 negative, and an extended key as its negated scan code.
- [x] must-fix src/S_TEXT.ASM:317-319 (Int2FHandler header; reopened in round 3, see round 3): "does nothing but iret" is wrong. The handler pushes BP, DI, SI and DS (:322 to :326) before `iret` (:327), and `iret` then pops those words as the return IP, CS and FLAGS, so it cannot return to what it interrupted. The pops after `iret` are dead, as the comment says; the four pushes are not. Describe the pushes and the iret. Fixed: the header now describes the four pushes and that the iret takes the first three as the return IP, CS and FLAGS, so it cannot return to what it interrupted; the QUIRK keeps the dead pops and ret. Resolved further by round 3: the "first three" wording was wrong; the iret takes the last three pushed, DS, SI and DI (see round 3, fixed there).
- [x] must-fix src/S_TEXT.ASM:335-336 (InstallInt2FHandler header): "install Int2FHandler (0000:0620) as interrupt 2Fh" and "it only installs a no-op handler" are wrong. DOS function 25h takes the handler as DS:DX. The code sets DX to 0620h (:347 to :348) and never sets DS (:346 only pushes it), and the header gives DS as DGROUP (:14). So the vector is DGROUP:0620h, a data address, not Int2FHandler at the code segment's 0620h. The comment must say so. The "no call" claim (src/S_TEXT.ASM:318) has evidence to cite: the code bytes of START1.EXE contain no `CD 2F` (INT 2Fh). Fixed: the header now says the code puts offset Int2FHandler in DX (0000:0632 to 0636) and never sets DS, so the vector is DGROUP:0620h, a data address; the "no call" claim cites the absent `CD 2F`.
- [x] should src/S_TEXT.ASM:272-274 (PutChar header) and :362 (DetectMouse header): "because the DOS call may clobber them" and "the registers the DOS and BIOS calls may clobber" give a reason with no evidence. Say the frame reproduces the original's register saves and that the reason is not known, or cite what DOS 06h and INT 33h are known to preserve. Fixed: both headers now say the frame reproduces the original's register saves and that why the calls need them saved is not known.
- [x] should src/S_TEXT.C:145-147 (PrintString comment): the last sentence restates the loop. Say why `i` must survive the PutChar call, or drop it. Fixed: the comment now says the compiler keeps i in SI (a register variable) and increments it before each PutChar call because the loop condition reads the next character only after the call returns (0000:047C to 0491).
- [x] should src/S_TEXT.C:33 (gMode): "from gDetectedMode" also means 0 when gDetectedMode is above 5 (`if (gMode > 5) gMode = 0`, :72 to :73). Say so. Fixed: the comment now reads "index 0 to 6 into gModeMaskTable (:78): gDetectedMode, or 0 when it is above 5 (:72 to :73), or 6 when the byte at F000:C000 is 21h (:76 to :77)".
- [x] should src/S_TEXT.C:35 (gDetectedMode): "NormalizeVideoInfo ... reads the second" is incomplete: it reads both words and byte +1 (src/S_TEXT.ASM:124 to :135). Say "reads the second word and byte +1". Fixed: the comment now says NormalizeVideoInfo reads both words and byte +1.
- [x] should src/S_TEXT.C:40-41 (gNote, gModePrompt): the strings at ds:0172 and ds:01A6 begin with CR LF (the bytes are "\r\nNOTE - " and "\r\nMode?"). Say so, so that the printed text is not read as starting on the same line. Fixed: both comments now show the leading CR LF: "\r\nNOTE - ..." and "\r\nMode?".
- [x] note src/S_TEXT.ASM:347 (the literal 0620h): the decompiler's note reports a matcher limitation (`make match` builds the ASM module alone, so `offset Int2FHandler` resolves to 03C8h there, and cmatch reads that near-label offset as a data global because it falls in the DGROUP pad range). I did not reproduce it. Whether the literal stays or the matcher changes is for the human (escalated in this handoff); the must-fix on :335 is needed either way. Resolved by task/small-code-pad (merged): the small model's code pad is now 133h, outside the data pad range, so the source says `offset Int2FHandler` and the literal 0620h is gone (make match FILE=src/S_TEXT.ASM 13 of 13); the must-fix on :335 was fixed above.
- [x] note re/symbols.toml: the new entries match their addresses (s0000_04bd is 63 bytes, 04BD to 04FC; s0000_056a is 80 bytes, 056A to 05BA). This is map data, not editable from the unit. Confirmed: re/symbols.toml has s0000_04bd addr 0000:04BD size 63 and s0000_056a addr 0000:056A size 80, both L2, match_flags "tasm /mx".

### Summary (round 2)

The round 1 must-fix items are fixed right, and the table, string and mark claims check against the original's bytes. Four changed lines are wrong and block the pass: TestCrtcRegister's AH at the write-back, GetKey's range claim, Int2FHandler's "does nothing but iret", and InstallInt2FHandler's vector (DS:0620h, not the handler). The unit does not pass. The matcher question about the 0620h literal goes to the human (STATE=escalate); the four comment fixes do not depend on the answer, except the InstallInt2FHandler QUIRK line, which should say what the literal is either way.

Round 2 resolved (decompiler): the four must-fix comment errors are fixed in src/S_TEXT.ASM (TestCrtcRegister: AL the first read-back written back, AH the second read-back tested against 66h; GetKey: the three returns the cbw produces; Int2FHandler: the four pushes and the iret that consumes them; InstallInt2FHandler: DS:DX, so the vector is DGROUP:0620h). The four should items are fixed too (PutChar and DetectMouse frames reproduce the original's saves, reason unknown; PrintString says why i survives the call; gMode gives the clamp to 0 and the F000:C000 override; gDetectedMode names both words and byte +1; gNote and gModePrompt show their leading CR LF). The 0620h literal note is resolved by task/small-code-pad: the source now says offset Int2FHandler. Evidence: make match FILE=src/S_TEXT.C 2 of 2 and FILE=src/S_TEXT.ASM 13 of 13, both exit 0; make readability UNIT=s_text 0 findings.

## Round 1

### Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

- [x] must-fix src/S_TEXT.ASM:8 (header): "The probes at 056A, 05BA and 05E6 (reached only through the table, and past the unit's end) are not written here." Fixed: the human settled the boundary (task/s_text-bounds, merged): the unit runs 0000:0239 to 0682, the first library module is ATEXIT, and 05F6 to 0642 are the unit's own. The three probes are now written in src/S_TEXT.ASM (ProbeVga 056A, ProbeEga 05BA, ProbeColorCrtc 05E6), the header says 1097 bytes and lists them, and make match FILE=src/S_TEXT.ASM is 13 of 13. No comment alone could fix the 10 unwritten bytes; they are now code.
- [x] must-fix src/S_TEXT.C:49-50 (Main header): says the three execl arguments are one-character arguments `"0" + the choice`. Fixed: the Main header now says the graphics and mouse arguments are "0" plus the choice (:112, :134) and the sound argument is the prompt's own key '1' to '4' (:120).
- [x] must-fix src/S_TEXT.ASM:62-63 (DetectVideoAdapter comment): says it stores "0080h, 0180h or 0283h". Fixed: the comment now gives the three values the bytes produce and what selects them: AX = 0101h on a steady status, else 0180h when the sampled status bits 70h are zero, 0181h when they are 10h, and 0382h otherwise.
- [x] must-fix src/S_TEXT.ASM:103-106 (NormalizeVideoInfo comment): "either word's low byte is 4 or more" is wrong for low bytes 80h and up. Fixed: the comment now says the compares are signed (4 to 127) and that a low byte of 80h to FFh is negative, so the 80h, 81h and 82h DetectVideoAdapter stores do reach the mode test.
- [x] must-fix src/S_TEXT.C:26 (`gSelection`: "the key the menu loop last read"): the variable holds three things. Fixed: the declaration comment now names all three uses: the key read (:90), the zero-based menu index (:92, `gSelection -= '1'`) and 'N' as the restart flag (:104).
- [x] must-fix src/S_TEXT.C:9-12 (library routines `PutChar`, `GetKey`, `DetectMouse`, `execl`; header line 4 "the library routines are named by their address"): Fixed: PutChar, GetKey and DetectMouse are now functions of this unit's assembly module (src/S_TEXT.ASM, 05F6/0608/0642) with their behaviour in the extern comments (prints a character at :91, :117, :127, :153; reads a key at :90, :101, :116, :126; gates the mouse prompt at :122). execl is settled as the library's `_execl`: build/re/rtl_start1.json names the library entry at 0000:0837 `_execl`, cited in the header and the extern comment. The header no longer claims the routines are named by address.
- [x] should src/S_TEXT.C:33-45 (placeholder globals gBanner to gHomie): Fixed: every declaration now carries its string (gBanner the title and mode question, gStar "*", gSpace " ", gNote the note, gModePrompt "Mode?", gWarningHead/Tail, gSurePrompt, gSoundPrompt, gMousePrompt, gIntroPath/gIntroName "INTRO.EXE", gHomie "homie"), and gHomie says INTRO.EXE's main strcmp's it (whitepaper 2.6).
- [x] should src/S_TEXT.C:31 (`gDetectedMode`): Fixed: the comment says it is the first byte of the four-byte video info record, InitVideo zeroes both words and NormalizeVideoInfo reads the second.
- [x] should src/S_TEXT.C:29 (`gMode`): Fixed: the comment now says it is the index 0 to 6 into gModeMaskTable, from gDetectedMode or 6 when the byte at F000:C000 is 21h.
- [x] should src/S_TEXT.C:69 (`0xF000C000` and `0x21`): Fixed: the constants now carry a `Purpose unknown:` comment saying the byte at F000:C000 is unidentified and that 21h selects table entry 6 (ds:00AF = 03h, the CGA and Tandy mask).
- [x] should src/S_TEXT.C:138-139 (PrintString comment): Fixed: the comment now says what the loop does (writes the string one character at a time through PutChar) and why the index i stays a local, instead of describing the stack frame.
- [x] should src/S_TEXT.ASM:21-23 (StoreVideoWord): Fixed: the comment now says "the first word when its low byte is zero" and names the three callers (DetectVideoAdapter, ProbeEga, ProbeColorCrtc).
- [x] should src/S_TEXT.ASM:35-38 (TestCrtcRegister): Fixed: the comment now says register 0Fh is the one probed and that AH keeps the first read-back and AL the value written back (it was the "VGA probe" that was wrong; the caller is ProbeColorCrtc).
- [x] should src/S_TEXT.ASM:61-64 (DetectVideoAdapter): Fixed: the MDA and CGA/EGA identifications are now marked `[H: ... not checked against hardware]`.
- [x] should src/S_TEXT.ASM:103-106 (NormalizeVideoInfo, the swap rule): Fixed: the comment now reads "swap the two words when the mode is 7 and byte +1 is not 1, or the mode is not 7 and byte +1 is 1".
- [x] should src/S_TEXT.ASM:1-9 (header): "runs the four probe routines through the table" but only one of them (04BD) is in this file. Fixed: the header now names ProbeVga (056A) as record 1, already enabled in the data and running first, and says InitVideo enables records 2 to 4.
- [x] note re/symbols.toml (map data, not editable from the unit): `s0000_04a3` (TestCrtcRegister) has size 89. Fixed by the map update (commit 921e8da): s0000_04a3 is size 26, s0000_052d size 61, and DetectVideoAdapter 04BD, the probes 056A, 05BA, 05E6 have their own entries. All 15 functions of the unit are now recorded L2.
- [x] note docs/design/WHITEPAPER.md 2.4 (line 117, outside the unit; not edited by the reviewer): says the call is `spawnl(.., ...)` at 0000:0447 and that each choice is "'0' + index". The unit's comments now cite build/re/rtl_start1.json for `_execl` and give the corrected argument forms; the whitepaper line is outside this unit and still needs the same fix, for whoever owns it.
- [x] note The names of the shared globals agree with the rest of src/: no other file declares ds:0113 to ds:0536 (`grep` over src/ finds only S_TEXT.C), so the one-name rule holds for this unit.

## Summary

The matched code reads well and the names that are backed by evidence are fine: StoreVideoWord, TestCrtcRegister, NormalizeVideoInfo and InitVideo each describe what their bytes do, and the menu strings back the global names. The unit is not yet understandable from its source alone, for three reasons. First, four comments say something the bytes do not: the sound argument is not "0" plus the choice, the DetectVideoAdapter values are wrong, the low-byte test in NormalizeVideoInfo is signed, and gSelection is described as one thing when it holds three. Second, the library routine names (PutChar, GetKey, DetectMouse, execl) are asserted without evidence, and the whitepaper calls the same call spawnl. Third, and most important, the unit boundary is not right: the video probe at 056A is inside the unit's declared bytes, runs past the 0574 end, and is not written in any file, so `make match` reports the unit incomplete (10 bytes). Escalated to the human (STATE=escalate) because the boundary is a map and spec question. Placeholder names left as they are: the globals gBanner to gHomie and the record name gDetectedMode, until the findings above are answered.

Round 1 resolved (decompiler): the human settled the boundary (0239 to 0682, ATEXIT starts the library), the probes and console and mouse routines are written, every finding above is addressed, all 15 functions are at L2, and make match FILE=src/S_TEXT.C and FILE=src/S_TEXT.ASM both pass (readability 0, match-gate OK).
