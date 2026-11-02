# Review: e1aad

Reviewer: rev-haiku
Commit reviewed: 3331b69 (round 2; main, with the round 1 fixes merged in 01237b0). Round 1 was 7d0dd48.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Round 2 (3331b69)

Judged only what the playbook's later-round rule asks: the round 1 must-fix items, and the diff since
7d0dd48 in `src/E1AAD.ASM` (the `re/symbols.toml` changes are the map and other units' stamps, not
this unit's names). Each round 1 must-fix was checked against the code it cites and is fixed right, so
those ticks stay. Checked against the code: the kModifierState writes (lines 603, 826); kKeyFlag204 has
one writer (install, 741); the install's 40:17 mapping (730-741); the INT 9 paths (E0 846-850, E1
834-844, break 938-941, sound keys 957-961); INT 23h (784-786, 1034); the 0409 peek (316-358); 038E
(243-298); 0471 and 0D26 (361-373, 1407-1434); 053A and 0588 (470-555); the 05F1 and 0693 wrapper
(589-708); the 0B0E release sites (1206-1229; 0B56 and 0B88); the 0D04 latch (1391); the g8136 latch at
1A72:01DC (E1A72.ASM:413-415); the tick counter at 1D8F:0055 (E1D8F.ASM:76-90). The caller lists in
the comments match `make packet UNIT=e1aad` (build/packets/e1aad.md).

Four must-fix items are new (three wrong claims in changed lines, and the round 1 g80E8 item only
partly fixed). Nothing is escalated: each is a comment, with no matcher, spec or pin question.

### Findings

- [ ] must-fix src/E1AAD.ASM:716 (the 1AAD:0718 header; round 1 should at :18, reopened): "toggles
  gSoundOn/g80E8 on the Ctrl/Alt sound keys". Round 1 named this line and asked for the keys, not
  Ctrl or Alt. The code tests AX=0420h (gSoundOn) and AX=0434h (g80E8) at lines 957-961, and no Ctrl
  or Alt bit. The g80E8 comment at :18-21 was fixed; this header was not. Fix: "toggles gSoundOn on key
  0420h (S) and g80E8 on key 0434h (M)".

- [ ] must-fix src/E1AAD.ASM:1170-1171 (the 1AAD:0B0E header): "so a release never reaches the game" is
  wrong. 1AAD:038E (266-267) and 1AAD:053A (498-499) do test and discard the release bit, but
  1AAD:0409 (326-337) and 1AAD:0588 (540-546) do not, and 0588 is what DispatchKey calls after 0409
  (E16A5.C:127-128) before it dispatches the key to the node list. So a release (41h or 42h with the
  bit set) reaches the game through DispatchKey. Say which readers discard it (038E, 053A) and which
  do not (0409, 0588). The same wording "AH bit 8" is also at src/E1AAD.ASM:230 and :465. AH has no
  bit 8; the code tests and sets the value 08h (`test ah,8`, `or ah,8`). Write "the 08h bit of AH".

- [ ] must-fix src/E1AAD.ASM:104-106 (the 1AAD:0223 header): two wrong claims in a changed line.
  (a) "a game code above 80h otherwise": the keypad table 1AAD:0357 (kKeypadCodes, lines 222-224) holds
  the ASCII codes 2Fh, 2Ah, 2Dh, 2Bh and 0Dh, and the Ctrl-function table 1AAD:037A (kCtrlFnCodes,
  225-226) holds the ASCII digits 30h..39h and symbols. (b) "41h..4Ah (the Ctrl-function keys)": the
  Ctrl-function table is indexed by AL-5Ah (`sub bl,0Fh` at line 166, after `sub bl,4Bh`), so its keys
  are 5Ah..6Dh. 41h..4Ah are not in it. 41h and 42h are the mouse-button codes that 1AAD:0B0E queues
  (lines 1195, 1216), and TranslateKey maps the range 41h..4Ah to 0C6h..0CFh (lines 191-194). Fix the
  clause: "41h..4Ah (the mouse codes 41h and 42h among them) come back as 0C6h..0CFh; 5Ah..6Dh take the
  Ctrl-function table; the keypad and that table hold some ASCII codes; otherwise game codes above 80h".

- [ ] must-fix src/E1AAD.ASM:20-30 (the g80EC and g80EF externs): both describe the state 2 readers
  wrongly. (a) g80EF (lines 27-30): "in state 2 the readers use ds:80EF instead of the ring
  (1AAD:038E/0409/048D)". Only 1AAD:048D does (lines 401-403, `mov ax,[g80EF]`). 1AAD:038E (254-259)
  and 0409 (324-329) read the ring slot in state 2, and 1AAD:0D26 fills that slot with ds:80EF
  before they read it (1423-1430). (b) g80EC (line 25): "the readers skip the ring when it is 2
  (1AAD:038E/053A/05D3)". 038E still reads the ring slot in state 2 (line 255); what 038E, 053A (481)
  and 05D3 (563) skip is the wait for a ring event. Fix both: the state 2 readers take the value
  from the ring slot that 0D26 filled (038E, 0409), except 048D, which reads ds:80EF directly.

- [ ] should src/E1AAD.ASM:235 (the 1AAD:038E header): "(EOV6.C:339, :342, :508)" is stale. EOV6.C now
  calls ReadKey at lines 346, 349 and 516. Cite the callers by address instead (the packet lists
  ovl6:0000 CampLoop, ovl6:0B9B, 082C:000B, 082C:0B43, ovl0:207A, ovl1:13F7). "Do not consume the key
  here" is true of the ReadKey call, but the callers flush the ring after the call (FlushKeyBuffer at
  EOV6.C:353 and :517). Say so, so the header is not read as "callers never consume".

- [ ] should src/E1AAD.ASM:64 (kExtendKeys map): "1AAD:005D: the map it reads at [di+0Fh]". The
  read is at 1AAD:005C+i (di after `repne scasb` is 4Ch+i+1, plus 0Fh), so entry 0 of the map is
  1AAD:005C, the last byte of kExtendKeys, and 005D is entry 1. Write the map base as 1AAD:005C.

- [ ] note src/E1AAD.ASM:44-47 (g8488): "tick counter" is right, but the unit also clears the count
  itself: 1AAD:0C17 writes 0 at line 1393, and the replay path at line 1350. Say so, so the counter is
  not read as one that only counts up.

- [ ] note src/E1AAD.ASM:306 (the 0409 header): "(E16A5.C:60-61, :127-128)". Lines 60-61 of E16A5.C
  are the extern declarations of ReadKeySpecial and WaitGameKey2, not calls. The calls are at :127-128.

- [ ] note for other owners (not this unit, round 1 notes still open): src/E07C8.C:93-99 (the
  InstallKeyboard copy cite, round 1); src/E1A72.ASM:35 still says "game state, 0/1/2" for g80EC;
  src/EOV1.ASM:114 and src/EOV3.C:28 still say g80E8 is "toggled by the Ctrl-key handling" (the same
  wording round 1 :18 removed here). Their owners should align them.

### Summary

Round 1's nine must-fix items are fixed right; their ticks stay. Round 1's should at :18 (the 0718
header) is only partly fixed and is reopened. Four must-fix items are open in this round: the 0718
header, the 0B0E release claim, the 0223 header, and the g80EC and g80EF readers. The rest are shoulds
and notes. Nothing is escalated.

## Round 1 (7d0dd48)

`make readability UNIT=e1aad` reports 0 findings. That gate checks structure only; the items below
are what it cannot see. Every citation was checked against the code (`src/E1AAD.ASM` line numbers
unless another file is named). Addresses are `1AAD:xxxx` unless marked otherwise.

## Findings

- [x] must-fix src/E1AAD.ASM:57: the kModifierState comment says it is "copied from ds:80EA at handler install".
  The install (1AAD:0718, lines 693-762) never reads ds:80EA. The copy happens on every INT 9 event
  (1AAD:07C0, lines 793-794) and on every 1AAD:05F1 call (lines 573-574). The initial 0110h is the
  load-time value before any copy. The g80EA extern on line 19 ("copied into 1AAD:0180") is right and
  can stay.

- [x] must-fix src/E1AAD.ASM:73: the kKeyFlag204 comment says "copy of 40:96 bit 4". The install code
  (1AAD:0718, lines 700-711) reads BIOS 40:17 (`mov al,byte ptr es:[17h]`, line 700) and sets bit 10h
  when 40:17 bit 1 (0x02, left Shift) is set (lines 709-711). Nothing clears it afterwards. Fix the
  comment to say this: set once at install from 40:17 bit 1 (left Shift), never cleared.

- [x] must-fix src/E1AAD.ASM:27: g8136 is declared "1 while a mouse button is held". The owner says
  otherwise: src/E1A72.ASM:62-64 says it is "latched to 1 once any button has been seen; this unit
  never clears it", and nothing in src/ clears it. This unit also sets it in replay (line 1356). Use
  the E1A72 evidence in this comment.

- [x] must-fix src/E1AAD.ASM:278-282: the 1AAD:0409 header describes the filter backwards. The code
  (lines 307-313) returns a key that is NOT in the special list 1AAD:0219 and is below 7Ah
  (`jb f0409_045A` after the `or cx,cx` at line 310); a key that is in the list is consumed (lines
  314-317). The header also says 0409 "reads" the key, but the return path (lines 322-325) does not
  store kKeyBufRead, so 0409 leaves the key in the ring. The filter matches 1AAD:0588's (E16A5.C:60-61
  and 127-128 use 0409 to test, then 0588 to take the key). Fix the header to say so: a peek that
  returns the first key that 0588 would take, without consuming it.

- [x] must-fix src/E1AAD.ASM:281-282 and src/E1AAD.ASM:487: "called by the text input helper 16A5:0009"
  is wrong. 16A5:0009 is DispatchKey (src/E16A5.C:116-121), which reads one pending key and dispatches
  it to the node list; E16A5.C:1-4 calls it "the event dispatch of the node lists". Name it so:
  "DispatchKey (16A5:0009, E16A5.C), the node-list dispatcher".

- [x] must-fix src/E1AAD.ASM:439-442: "Called by StartupGame, LoadGamePanel and the camp path (EOV6.C)".
  EOV6.C never calls WaitGameKey; the camp loop calls ReadKey (1AAD:038E, src/EOV6.C:339, :342, :508).
  Callers of 1AAD:053A per the packet: 07C8:0199 StartupGame, 07C8:0625 AllocationFailure, 082C:000B
  EditTextField, 082C:081A ShowMemoryDiagnostics, 19BC:02B6 OutOfMemory, 1C0E:0ADD DisplayFileError,
  ovl1:1ACD, ovl2:0846 LoadGamePanel. Replace the camp path with the full list, or say "for example".

- [x] must-fix src/E1AAD.ASM:767: the INT 9 handler comment says it "either swallows it (E1/E0 prefix,
  break code) or translates and buffers it". The code does not do that for E0 or break codes. An E0
  prefix (lines 814-818, ending in `jmp 1AAD:09BD` at line 818) and a break code (line 822 clears
  BL; lines 906-909 send BL=0 to 1AAD:09BD) both chain to the old INT 9 vector. Only E1 bytes are
  swallowed (acknowledged at 1AAD:09CA, reached from lines 810-812 unless DX bit 80h is set). Fix the
  comment to name each path.

- [x] must-fix src/E1AAD.ASM:1002: "the INT 23h handler installed at 1AAD:07B2". The install at
  1AAD:0718 sets the INT 23h vector to 09F5 (lines 754-756: `mov dx,09F5h`, `mov ax,2523h`). 07B2 is not
  an entry point.

- [x] must-fix src/E1AAD.ASM:337-338: the 1AAD:0471 header says the return is the xor of the ring
  indices and that 1AAD:0D26 is called "for its side effect". In state 2 (replay) 1AAD:0D26 overwrites
  AX (lines 1384-1388: AX is ds:80EF or 0), so 0471 returns the injected key or 0, not the xor. Its
  callers test the return (src/E14EF.C UpdateOptionList, src/E16A5.C:150 DispatchKeyWait). Say what it
  returns in state 0 and 1 (the xor) and in state 2 (the key or 0).

- [ ] should src/E1AAD.ASM:18 (reopened in round 2 as the must-fix above, :716): g80E8 is "toggled by Ctrl-key handling, role unknown", and the 1AAD:0718
  header (line 688) says the handler "toggles gSoundOn/g80E8 on the Ctrl/Alt sound keys". The handler
  tests AX=0420h (gSoundOn) and AX=0434h (g80E8) at lines 925-928. Key code 20h is S and 34h is M
  (kUnshiftedCodes at 1AAD:02DB, index 20h and 34h), both with AH=04. Name the keys and do not call
  g80E8 a sound flag: the options code toggles it (src/EOV1.ASM:3724-3731), and SaveGame saves it
  next to gSoundOn (src/EOV3.C:808-840). Ctrl or Alt is not shown by this code.

- [x] should src/E1AAD.ASM:20: g80EC is "game state, 0/1/2". The code uses it as an input mode: 1
  writes the recording (1AAD:0C17, lines 1284-1317), 2 replays it (lines 1319-1361), and 038E, 053A and
  05D3 skip the ring when it is 2. The whitepaper has no ds:80EC. "Game state" claims more than the
  code shows. Either a mode name with this evidence, or keep the placeholder and say what is known.
  src/E1A72.ASM:35 repeats the "game state" wording and needs the same change.

- [x] should src/E1AAD.ASM:22: g80EF is "key of the recorded event last decoded". 1AAD:048D copies every
  event taken from the ring to ds:80EF (line 380), in any state. In state 2 it reads ds:80EF instead of
  the ring (lines 376-378). Say that.

- [x] should src/E1AAD.ASM:34: g8488 is "counter that paces the recorded input replay". The evidence is
  in src/E1D8F.ASM (the INT 1Ch tick, `inc word ptr ds:[8488h]` at its L80): a tick counter, 18.2 per
  second. 1AAD:0C17 stores its value with each recorded event (lines 1287-1288) and 1AAD:0D26 waits
  until it reaches the stored count (lines 1384-1386). Cite the tick and say so.

- [x] should src/E1AAD.ASM:68: kKeyBitTable1 is "tested with bit 0". Shift bit 0 is Caps Lock
  (1AAD:0718 lines 701-703, from 40:17 bit 6). The table (1AAD:01DF) has bits set for the letter keys,
  codes 11h..1Ah, 1Fh..27h, 2Eh..2Fh and 30h..34h, and 1AAD:0693 (lines 658-662) and 07C0 (lines
  883-887) flip the shift result for them. Say "letter keys: Caps Lock inverts shift for them".

- [x] should src/E1AAD.ASM:70: kKeyBitTable2 is "tested with bit 1" and nothing more. Bit 1 is Num Lock
  (40:17 bit 5, lines 705-707), but this table holds the codes 1Bh..1Dh ([ ] \) and 20h..28h except
  24h (S, D, F, G, J, K, L, ;), not keypad codes. Either say Purpose unknown or check the bit with the
  oracle.

- [x] should src/E1AAD.ASM:48 and :50: kExtendKeys is "for 1AAD:0718", but 1AAD:07C0 searches it (lines
  829-832: `mov di,4Ch`, `mov cx,11h`, `repne scasb`) and then reads the byte at `[di+0Fh]`, that is
  1AAD:005C+i for the match index i. The unnamed bytes at 1AAD:005D (lines 50-53) are that map. Cite 07C0
  and name the map.

- [x] should src/E1AAD.ASM:85-91: the 1AAD:0223 header says the function maps to "an ASCII code in AL".
  Key codes 41h to 4Ah come back with 85h added (lines 170-173, so 0C6h to 0CFh), and the keypad and
  Ctrl-function codes come back with high bits set. "Other codes come back with bit 80h set" (line 91)
  is not quite it either. Say that the result is ASCII for the main table and a game code above 80h
  otherwise.

- [x] should src/E1AAD.ASM:210: 1AAD:038E says it advances "by 4 when AL is 41h..44h". The code adds 4
  more (line 252), so a mouse event moves the index by 6 in all, as 1AAD:048D does. Also the header
  does not say the returned key stays in the ring: the return path (lines 261-265) skips the store of
  kKeyBufRead, as 0409 does. Check the callers (src/EOV6.C:339, :342, :508) and say whether they
  consume the key.

- [x] should src/E1AAD.ASM:556-559: the 1AAD:0693 wrapper is described with a "near call". The call
  (line 628, `call _GetKeyState`) is FAR, and _GetKeyState is a FAR proc ending in `retf`. The wrapper
  pushes BX, CX, DI (lines 624-626) and passes the key in AX/BX, but GetKeyState reads its key from
  [bp+6] (line 564), which after the far call is the DI the wrapper pushed. Candidate QUIRK 1AAD:0693:
  mark it, do not fix. Confirm with the oracle before writing it down.

- [x] should src/E1AAD.ASM:1135-1137: the 1AAD:0B0E header does not say that a button release sets AH
  bit 8 (lines 1171-1174 and 1192-1195, `or ah,8` when BL is 0). Readers 038E (lines 243-244) and 053A
  (lines 471-472) discard bit 8, so releases never reach the game. Say it.

- [x] should src/E1AAD.ASM:688-689: "the two data bytes at 1AAD:09F2" are three bytes (kKeyMode9F2 at 09F2 is
  one byte, kScanCode9F3 at 09F3 is a word). Minor.

- [x] should src/E1AAD.ASM:1374-1375: 1AAD:0D26 writes 0 into the ring slot at the read index when no
  recorded key is due (lines 1387-1394), so during replay it overwrites a queued event. The header says
  "and 0 otherwise"; say that this overwrites the slot.

- [x] note src/E1AAD.ASM:47: kChainFarPtr is zero in this unit and no file in src/ writes it, so the call
  at 1AAD:099B never runs yet. Say "Purpose unknown; no writer found in src/".

- [x] note src/E1AAD.ASM:1032-1033: 1AAD:0A1F has no caller in src/ (grep finds none); "probably a far
  pointer call" is the right level of doubt. Leave it.

- [x] note src/E1AAD.ASM:1-9 and the k prefix: the data names here use a `k` prefix (kKeyBuf, kShiftFlags)
  that no other unit uses and that docs/process/READABILITY.md does not define for unit-private data.
  Decide the convention in phase 6, or say in this header that `k` means unit-private data.

- [ ] note src/E07C8.C:93-99 (not this unit, for its owner): it says "the install copy (InstallKeyboard,
  1AAD:0718, src/E1AAD.ASM:793) takes the load-time value". Line 793 is inside the INT 9 handler at
  1AAD:07C0, and 1AAD:0718 never reads ds:80EA. The comment should cite 1AAD:07C0.

- [x] note the names that are right and need no change: ReadKey (038E: non-blocking read, its callers
  are in EOV6.C and the overlays), ReadKeySpecial (0409, but see the must-fix), WaitGameKey2 (0588),
  WaitKeyEvent (05D3), FlushKeyBuffer (0480), DequeueKeyEvent (048D), InjectRecordedKey (0D26),
  InstallKeyboard and RemoveKeyboard (0718 and 09F6, both with callers in E07C8.C), HandleMouseButtons
  (0B0E, called from the INT 33h handler at 1A72:017A, E1A72.ASM:406) and GetKeyState (05F1, used by
  E16A5.C:205-209). Each has its evidence in its header.

## Summary

The unit is understandable from its source alone, and most names are backed by an address, a caller or
a table: TranslateKey, ReadKey, DequeueKeyEvent, FlushKeyBuffer, GetKeyState and the INT 9 install are
all clear. The problems are in the comments, not the names. Nine must-fix items describe code wrongly
(the kModifierState copy, the kKeyFlag204 source, the g8136 meaning, the inverted 0409 filter, the
DispatchKey caller, the EOV6 caller of 053A, the E0 and break-code paths in the INT 9 handler, the INT 23h
address and the IsKeyPending return in replay). Several names are still placeholders (g80E8, g80EC,
g80EF, g8488, kKeyBitTable2); each is a should, with the evidence that is already in the tree. Nothing is
escalated: every item is a comment or a name, with no matcher, spec or pin question.

## Decompiler response (dec-deepseek-3, round 1)

All nine must-fix items and the should and note items are fixed in `src/E1AAD.ASM`; `make match
FILE=src/E1AAD.ASM` is 18 of 18 (3405 of 3405 bytes) and `make readability UNIT=e1aad` is 0
findings after every change. The findings are comments and names only; no byte moved.

- kModifierState (:57): now says it is copied from ds:80EA by the INT 9 handler body (1AAD:07C0)
  and by GetKeyState (1AAD:05F1), and that 0110h is the load-time value.
- kKeyFlag204 (:73): now "set once at install (1AAD:0718) when 40:17 bit 1 (left Shift) is set;
  never cleared".
- g8136 (:27): now "latched to 1 once any button has been seen (1A72:01DC); nothing in src/ clears
  it, and the recorded replay sets it too (1AAD:0D04)".
- 1AAD:0409 header (:278-282): now a peek of the first key 1AAD:0588 would take, without consuming
  it, called by DispatchKey (16A5:0009, E16A5.C) at E16A5.C:60-61, :127-128.
- "text input helper 16A5:0009" (:281-282, :487): now "DispatchKey (16A5:0009, E16A5.C), the
  node-list dispatcher".
- 1AAD:053A header (:439-442): the camp path is replaced by the full caller list from the packet
  (07C8:0199, 07C8:0625, 082C:000B, 082C:081A, 19BC:02B6, 1C0E:0ADD, ovl1:1ACD, ovl2:0846).
- 1AAD:07C0 header (:767): names each path: E0 prefix and break codes chain to the old INT 9 vector
  (1AAD:09BD), only E1 bytes are swallowed (1AAD:09CA unless kModifierState bit 80h diverts).
- INT 23h (:1002): now the bare IRET at 1AAD:09F5, with the install at 1AAD:0718 pointing INT 23h
  there.
- 1AAD:0471 header (:337-338): the xor in state 0/1, and in state 2 the injected key (ds:80EF) or 0
  because 1AAD:0D26 overwrites AX.
- g80E8 (:18): now an option flag saved next to gSoundOn (EOV3.C:808-840), toggled by the options
  code (EOV1.ASM:3724-3731) and by the INT 9 handler on key 0434h (M), as gSoundOn on 0420h (S).
- g80EC (:20): now an input mode (1 writes, 2 replays, readers skip the ring at 2). The wording in
  src/E1A72.ASM:35 is another unit's file and was left for its owner.
- g80EF (:22): now says 1AAD:048D copies every event to ds:80EF in any state and the readers use it
  instead of the ring in state 2.
- g8488 (:34): now a tick counter, 18.2 per second, incremented by the INT 1Ch handler
  (1D8F:0055, E1D8F.ASM); 1AAD:0C17 stores it and 1AAD:0D26 waits for it.
- kKeyBitTable1 (:68): now "letter keys: shift bit 0 (Caps Lock) inverts the shift result". The
  kKeyBitTable2 comment (:70) now says Purpose unknown, Num Lock bit but non-keypad codes.
- kExtendKeys (:48, :50): now cites the 1AAD:07C0 search and the map read at [di+0Fh] (1AAD:005C+i).
- 1AAD:0223 header (:85-91): now ASCII for the main table, a game code above 80h otherwise, with
  3Eh..40h and 7Ah+ taking bit 80h set and 6Eh ESC.
- 1AAD:038E header (:210): now advances by 2 (6 for a mouse event), leaves the key in the ring, and
  its callers test the return without consuming (EOV6.C:339, :342, :508).
- 1AAD:0693 (:556-559): now a FAR call, marked Candidate QUIRK 1AAD:0693 (the wrapper's key is
  never read because 1AAD:05F1 takes [bp+6], the pushed DI), oracle confirmation pending.
- 1AAD:0B0E header (:1135-1137): now says a button release sets AH bit 8 (1AAD:0B56/0B88) and the
  readers discard bit 8, so a release never reaches the game.
- "two data bytes at 1AAD:09F2" (:688-689): now "three data bytes (kKeyMode9F2 one byte,
  kScanCode9F3 a word)".
- 1AAD:0D26 header (:1374-1375): now says the 0 write overwrites the slot at the read index.
- kChainFarPtr (:47): now notes it is zero here and no writer in src/, so the call never runs.
- 1AAD:0A1F (:1032-1033): left as the reviewer asked (no caller in src/, probably a far pointer
  call).
- k prefix (:1-9): the header now says the `k` names are this unit's private variables and tables.

The src/E07C8.C:93-99 note is that unit's file, so it was not changed on this branch.

