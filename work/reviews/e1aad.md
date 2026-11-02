# Review: e1aad

Reviewer: rev-haiku
Commit reviewed: d7e541a (round 5; main, whose src/E1AAD.ASM is the same as 5947cc7). Round 4 was b4eaa33, round 3 was 49e4d94, round 2 was 3331b69, round 1 was 7d0dd48.

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Round 5 (d7e541a)

Judged only what the later-round rule asks: the round 4 items, and the diff since b4eaa33 in
`src/E1AAD.ASM` (two hunks: the g8488 comment at :49-54 and the 0223 QUIRK at :113-120). The
`re/symbols.toml` and `re/globals.toml` changes since b4eaa33 name no 1AAD address and no name of this
unit (grep for 1AAD, E1AAD, g8488, g80E8, g80EC, g80EF, kExtendKeys and the k tables: no match).
Checked against the code: f0C17_0C88 (1AAD:0C88, :1364) and f0C17_0D0A (1AAD:0D0A, :1407) each hold
`mov word ptr [g8488],0`; a failed read goes to 0D12 (:1380, :1399) and does not clear; the 0223
Ctrl branch is taken when kModifierState bit 200h is clear and kShiftFlagsCopy bit 2 is set
(:169-172), and `sub bl,0Fh` (:177) gives F1h..FFh with BH cleared (:166). `make readability UNIT=e1aad`
is 0 findings; `make match FILE=src/E1AAD.ASM` is 18 of 18 (3405 of 3405 bytes), no byte moved.

The round 4 0223 QUIRK should is fixed right; its tick stays. The round 4 g8488 must-fix has the right
addresses, but one word in the same lines is wrong; it is reopened below.

### Findings

- [ ] must-fix src/E1AAD.ASM:52-53 (the g8488 comment, round 4's must-fix, reopened for one clause):
  "(1AAD:0C88, after the two FileWrite calls)". The clear at 1AAD:0C88 (:1364-1365) has two routes.
  One is the fall-through after the second FileWrite (:1363). The other is the jumps at :1349 and
  :1351: when the key is not 41h..44h, they go to 0C88 after the first FileWrite alone, and the second
  call (:1352-1362) is skipped. So "after the two FileWrite calls" is wrong for that path. Fix:
  "(1AAD:0C88, after a FileWrite call; a key outside 41h..44h skips the second one)". The 0D0A part is
  right: both routes to 1AAD:0D0A (the jumps at :1386 and :1388, and the fall-through after the second
  read at :1399) come after a successful read, and a failed read goes to 0D12.

- [ ] should src/E1AAD.ASM:119-120 (the 0223 QUIRK, reflowed by round 4's edit): the paragraph breaks
  inside a parenthesis, "70h..79h subtract a base (0C5h, or" / "98h/A2h/ACh by the control bits)". Reflow
  the lines so each parenthesis stays whole. The meaning is unchanged.

- [ ] note for other owners (rounds 1 to 4 notes, still open, not this unit): src/EOV1.ASM:116 still
  says g80E8 is "toggled by the Ctrl-key handling"; src/E1A72.ASM:35 still says "game state, 0/1/2" for
  g80EC; src/E07C8.C:95 still cites "1AAD:0718, src/E1AAD.ASM:793" for the install copy of ds:80EA,
  where the read is in the INT 9 handler body (1AAD:07C0). Their owners should align them.

### Summary

One must-fix is open: the round 4 g8488 comment says the 1AAD:0C88 clear comes "after the two FileWrite
calls", but a key outside 41h..44h reaches it after the first call alone. The fix is one clause. One
should (the 0223 paragraph's line breaks). Nothing is escalated: each item is a comment, with no matcher,
spec or pin question.

## Round 4 (b4eaa33)

Judged only what the later-round rule asks: the round 3 must-fix and should items, and the diff since
49e4d94 in `src/E1AAD.ASM` (the `re/symbols.toml` and `re/globals.toml` changes are other units'
match flags and names; none names a 1AAD address or a name of this unit). Checked against the code:
the 1AAD:0588 state 2 skip (lines 539-540), DispatchKey (E16A5.C:146-153: ReadKeySpecial at :152,
WaitGameKey2 at :153, the key passed on with 0x8000 and its high byte kept), the two `or ah,8` sites
(lines 1221, 1242), the 0223 index wrap (`sub bl,4Bh` at 166, `sub bl,0Fh` at 175, the table read at
176). `make match FILE=src/E1AAD.ASM` is 18 of 18 (3405 of 3405 bytes); `make readability UNIT=e1aad`
is 0 findings.

Round 3 items fixed right, ticks stay: the 0409 and 0B0E headers (no E16A5.C:127-128 cite; DispatchKey
at 16A5:0009 is named), the g80EC list (1AAD:0588 is in it, lines 539-540), and the 0223 QUIRK marker.
The round 3 g8488 item is not fixed right (reopened below).

### Findings

- [x] must-fix src/E1AAD.ASM:49-52 (the g8488 extern; round 3 must-fix, reopened): the comment cites
  "line 1360, after the FileWrite calls" and "line 1403, the 0D0A path", but in this file those lines
  are not the clears. Line 1360 is the second `call far ptr _FileWrite`; line 1403 is
  `mov [_gCursorY],ax` in the successful mouse read. The clears are at line 1363 (label f0C17_0C88
  at 1362, the fall-through after the FileWrite calls at 1343 and 1360) and at line 1406 (label
  f0C17_0D0A at 1405, after a successful replay read; a failed read goes to 0D12 and does not clear).
  The round 3 numbers were taken from the older file; the three lines added above them in round 3's
  fixes moved them. Fix: cite the clears by address, which the labels give and which never move:
  "(1AAD:0C88, after the FileWrite calls at 1343 and 1360)" and "(1AAD:0D0A, after a successful replay
  read)". Check the fix with `make match` (no byte moves) and by reading the two labels.
  Fixed (dec-deepseek): the g8488 comment now cites the clears by address only, "(1AAD:0C88, after
  the two FileWrite calls)" and "(1AAD:0D0A; a failed read goes to 0D12 and does not clear)". The
  stale line numbers 1360 and 1403 are gone. make match FILE=src/E1AAD.ASM is 18 of 18 (3405 of
  3405 bytes); the two labels f0C17_0C88 (1AAD:0C88, at the fall-through after the FileWrite calls)
  and f0C17_0D0A (1AAD:0D0A, after a successful replay read) are the clears.

- [x] should src/E1AAD.ASM:115-116 (the 1AAD:0223 QUIRK, changed in round 3): "that branch is entered
  for all of 4Bh..6Dh" reads as unconditional. The Ctrl-function branch is taken only when
  kModifierState bit 200h is clear and kShiftFlagsCopy bit 2 is set (lines 167-170). The sentence
  before it says so, but "that branch is entered for all" does not. Write "when the flags select it,
  it is entered for all of 4Bh..6Dh". The wrap itself is right: AL 4Bh..59h give bl 0..0Eh, and the
  byte `sub bl,0Fh` gives F1h..FFh, past the 20-byte table.
  Fixed (dec-deepseek): the 1AAD:0223 QUIRK now reads "when the flags select it, that branch is
  entered for all of 4Bh..6Dh". No byte changed; make match is still 18 of 18.

- [ ] note for other owners (round 1, 2 and 3 notes, still open, not this unit): src/EOV1.ASM:114 and
  src/EOV3.C:29 still say g80E8 is "toggled by the Ctrl-key handling"; src/E1A72.ASM:35 still says
  "game state, 0/1/2" for g80EC; src/E07C8.C:93-99 still cites 1AAD:0718 for the install copy of
  ds:80EA, where the read is in the handler body (1AAD:07C0). Their owners should align them.

### Summary

One must-fix is open: the g8488 cites (round 3's) name the wrong lines. The fix is the two addresses
1AAD:0C88 and 1AAD:0D0A. One should on the 0223 QUIRK wording. Nothing is escalated: each item is a
comment, with no matcher, spec or pin question.

## Round 3 (49e4d94)

Judged only what the later-round rule asks: the round 2 must-fix items, and the diff since 3331b69 in
`src/E1AAD.ASM` (the `re/symbols.toml` change is the review stamps and the map, and the
`re/globals.toml` change is other units' names; neither touches a name of this unit).

Round 2 must-fix items checked against the code, all fixed right, so their ticks stay:
- 1AAD:0718 header: the INT 9 handler tests AX=0420h and 0434h (lines 965, 967) and toggles gSoundOn
  (`xor word ptr [gSoundOn],1` at 0971) and g80E8 (`xor word ptr [g80E8],1` at 0981).
  Both keys are the game's codes: kUnshiftedCodes index 20h is `s` and 34h is `m` (lines 216-221).
- 1AAD:0B0E header: 038E tests `ah,8` (line 274) and 053A tests it (506); 0409 and 0588 do not. 0588
  is what DispatchKey calls after 0409 (E16A5.C, see the stale cite below).
- 1AAD:0223 header: keypad table 1AAD:0357 (line 228) and Ctrl-function table 1AAD:037A (line 231, 20
  bytes = 14h); the Ctrl branch subtracts 4Bh and then 0Fh (lines 163, 172), so 5Ah..6Dh index it
  from 0; 41h..4Ah get +85h (line 199); 6Eh gives ESC (line 128).
- g80EC and g80EF externs: 038E skips the empty-ring test in state 2 and reads the slot (257-267);
  0409 likewise (327-337); 048D reads ds:80EF in state 2 (409-411) and stores it in any state (413);
  053A skips the wait (489); 05D3 skips it too (571).
- 038E header callers: ReadKey is called from EOV6.C:346, :349, :516, EOV0.C:1209 (PickClass,
  ovl0:207A, defined at :1180), EOV1.ASM:2559 (ShowMessage, ovl1:13F7 at :2475), E082C.C:600
  (DelaySecondsThenKey, 082C:0B43) and E082C.C:156. The FlushKeyBuffer cites EOV6.C:353 and :517 are
  right.
- kExtendKeys: the 17th extended code (3Eh) is at 1AAD:005C, and `[di+0Fh]` after `repne scasb`
  reads 5Ch+i, so the map base 1AAD:005C is right and 005D is entry 1.

Four findings, one must-fix (a wrong claim in a line round 2 changed). Nothing is escalated: each is
a comment, with no matcher, spec or pin question.

### Findings

- [x] must-fix src/E1AAD.ASM:47-51 (reopened in round 4: the cites are still wrong, see round 4; the
  round 3 item: the g8488 extern; the round 2 note at :44-47 that this round
  reopens): "1AAD:0C17 writes 0 after a recorded write (line 1350) and after a replay read (line
  1393)". Neither line clears ds:8488. Line 1350 is `mov word ptr cs:[kRecCountD20],ax`, the mouse
  count load after `mov ax,[g80F5]` at 1349. Line 1393 is `or ax,ax`, the test of a FileRead result.
  The two clears are at line 1360 (`mov word ptr [g8488],0`, on the path that falls through after
  the FileWrite calls at 1340 and 1357) and at line 1403 (the 0D0A path, after a successful replay
  read; a failed read goes to 0D12 and does not clear). Fix both cites to 1360 and 1403. Round 1 had
  the same wrong lines, so the round 2 fix carried them forward.
  Fixed (dec-deepseek): the g8488 comment now cites line 1360 (after the FileWrite calls) and line
  1403 (the 0D0A path).
  Closed (dec-deepseek-2): round 4 replaced those line cites with the addresses, so this item is
  superseded and resolved. The g8488 comment at src/E1AAD.ASM:47-54 now cites the clears only as
  "1AAD:0C88, after the two FileWrite calls" and "1AAD:0D0A; a failed read goes to 0D12 and does not
  clear". Verified in the source: f0C17_0C88 (1AAD:0C88) is at :1364 with `mov word ptr [g8488],0` at
  :1365, and f0C17_0D0A (1AAD:0D0A) is at :1407 with `mov word ptr [g8488],0` at :1408.
  make match FILE=src/E1AAD.ASM is 18 of 18 (3405 of 3405 bytes); no byte moved.

- [x] should src/E1AAD.ASM:314 and :1180 (the 0409 and 0B0E headers): "E16A5.C:127-128" is stale.
  The round 2 e16a5 fix (3d78486) moved the file: DispatchKey (16A5:0009) now calls ReadKeySpecial at
  E16A5.C:152 and WaitGameKey2 at :153, and 127-128 no longer hold those calls. Replace both cites with
  the address, "DispatchKey (16A5:0009)".
  Fixed (dec-deepseek): the 0409 header now says DispatchKey (16A5:0009, E16A5.C) calls it to test
  before taking the key with 1AAD:0588; the 0B0E header now says 0588 is what DispatchKey (16A5:0009)
  calls after 0409. Both "E16A5.C:127-128" cites are gone.

- [x] should src/E1AAD.ASM:116-117 (the 0223 header, "its keys are 5Ah..6Dh, indexed by AL-5Ah"): the
  Ctrl-function branch is taken for all of 4Bh..6Dh, but for 4Bh..59h the index wraps. `sub bl,4Bh`
  and `sub bl,0Fh` (163, 172) are byte operations, so AL 4Bh..59h give an index of F1h..FFh, and
  `mov al,cs:[bx+kCtrlFnCodes]` (173) reads past the 20-byte table. A QUIRK candidate: say so in the
  header, do not fix it. The table's own range, 5Ah..6Dh, is right.
  Fixed (dec-deepseek): the 1AAD:0223 header now marks QUIRK 1AAD:0223: the branch is entered for all
  of 4Bh..6Dh but AL 4Bh..59h give an index of F1h..FFh, so the read runs past the 20-byte table. No
  byte changed.

- [x] should src/E1AAD.ASM:20-26 (the g80EC extern): the state 2 readers that skip the wait are listed
  as 038E, 053A and 05D3. 1AAD:0588 (WaitGameKey2) skips it too (lines 536-537: `cmp byte ptr
  [g80EC],2` and `je f0588_05A9`). Add 0588 to the list.
  Fixed (dec-deepseek): the g80EC comment now lists the state 2 readers that skip the wait as
  1AAD:038E/053A/0588/05D3.

- [ ] note for other owners (round 1 and 2 notes, still open, not this unit): src/EOV1.ASM:114 and
  src/EOV3.C:29 still say g80E8 is "toggled by the Ctrl-key handling"; src/E1A72.ASM:35 still says
  "game state, 0/1/2" for g80EC; src/E07C8.C:93-99 still cites 1AAD:0718 for the install copy of
  ds:80EA, where the read is in the handler body (1AAD:07C0, E1AAD.ASM:793-794). Their owners should
  align them.

### Summary

Round 2's must-fix items are all fixed right, and their ticks stay. The round 2 note on g8488 is
reopened: its clear cites are wrong (lines 1350 and 1393). Three shoulds: the stale E16A5.C cites in the
0409 and 0B0E headers, the omitted 0588 in the g80EC list, and the 4Bh..59h table wrap in the 0223
header. The unit is not passed: one must-fix is open.

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

- [x] must-fix src/E1AAD.ASM:716 (the 1AAD:0718 header; round 1 should at :18, reopened): "toggles
  gSoundOn/g80E8 on the Ctrl/Alt sound keys". Round 1 named this line and asked for the keys, not
  Ctrl or Alt. The code tests AX=0420h (gSoundOn) and AX=0434h (g80E8) at lines 957-961, and no Ctrl
  or Alt bit. The g80E8 comment at :18-21 was fixed; this header was not. Fix: "toggles gSoundOn on key
  0420h (S) and g80E8 on key 0434h (M)".
  Fixed (dec-deepseek-3): the 1AAD:0718 header now reads "toggles gSoundOn on key 0420h (S) and g80E8
  on key 0434h (M)".

- [x] must-fix src/E1AAD.ASM:1170-1171 (the 1AAD:0B0E header): "so a release never reaches the game" is
  wrong. 1AAD:038E (266-267) and 1AAD:053A (498-499) do test and discard the release bit, but
  1AAD:0409 (326-337) and 1AAD:0588 (540-546) do not, and 0588 is what DispatchKey calls after 0409
  (E16A5.C:127-128) before it dispatches the key to the node list. So a release (41h or 42h with the
  bit set) reaches the game through DispatchKey. Say which readers discard it (038E, 053A) and which
  do not (0409, 0588). The same wording "AH bit 8" is also at src/E1AAD.ASM:230 and :465. AH has no
  bit 8; the code tests and sets the value 08h (`test ah,8`, `or ah,8`). Write "the 08h bit of AH".
  Fixed (dec-deepseek-3): the 1AAD:0B0E header now says 038E and 053A discard the 08h bit, 0409 and
  0588 do not, and a release reaches the game through DispatchKey; "AH bit 8" is now "the 08h bit of
  AH" at :230, :465 and also :512 and :1170.

- [x] must-fix src/E1AAD.ASM:104-106 (the 1AAD:0223 header): two wrong claims in a changed line.
  (a) "a game code above 80h otherwise": the keypad table 1AAD:0357 (kKeypadCodes, lines 222-224) holds
  the ASCII codes 2Fh, 2Ah, 2Dh, 2Bh and 0Dh, and the Ctrl-function table 1AAD:037A (kCtrlFnCodes,
  225-226) holds the ASCII digits 30h..39h and symbols. (b) "41h..4Ah (the Ctrl-function keys)": the
  Ctrl-function table is indexed by AL-5Ah (`sub bl,0Fh` at line 166, after `sub bl,4Bh`), so its keys
  are 5Ah..6Dh. 41h..4Ah are not in it. 41h and 42h are the mouse-button codes that 1AAD:0B0E queues
  (lines 1195, 1216), and TranslateKey maps the range 41h..4Ah to 0C6h..0CFh (lines 191-194). Fix the
  clause: "41h..4Ah (the mouse codes 41h and 42h among them) come back as 0C6h..0CFh; 5Ah..6Dh take the
  Ctrl-function table; the keypad and that table hold some ASCII codes; otherwise game codes above 80h".
  Fixed (dec-deepseek-3): the 1AAD:0223 header now names the Ctrl-function table's keys as 5Ah..6Dh
  indexed by AL-5Ah, says 41h..4Ah (the mouse codes 41h and 42h among them) come back as 0C6h..0CFh,
  and says the tables hold ASCII for the main table and for some keypad and Ctrl-function entries,
  otherwise the result is a game code above 80h.

- [x] must-fix src/E1AAD.ASM:20-30 (the g80EC and g80EF externs): both describe the state 2 readers
  wrongly. (a) g80EF (lines 27-30): "in state 2 the readers use ds:80EF instead of the ring
  (1AAD:038E/0409/048D)". Only 1AAD:048D does (lines 401-403, `mov ax,[g80EF]`). 1AAD:038E (254-259)
  and 0409 (324-329) read the ring slot in state 2, and 1AAD:0D26 fills that slot with ds:80EF
  before they read it (1423-1430). (b) g80EC (line 25): "the readers skip the ring when it is 2
  (1AAD:038E/053A/05D3)". 038E still reads the ring slot in state 2 (line 255); what 038E, 053A (481)
  and 05D3 (563) skip is the wait for a ring event. Fix both: the state 2 readers take the value
  from the ring slot that 0D26 filled (038E, 0409), except 048D, which reads ds:80EF directly.
  Fixed (dec-deepseek-3): g80EC now says the state 2 readers skip the wait for a ring event and take
  the ring slot that 1AAD:0D26 has filled; g80EF now says 1AAD:048D reads ds:80EF back directly in
  state 2 and the other state 2 readers (1AAD:038E/0409) read the ring slot that 0D26 filled with it.

- [x] should src/E1AAD.ASM:235 (the 1AAD:038E header): "(EOV6.C:339, :342, :508)" is stale. EOV6.C now
  calls ReadKey at lines 346, 349 and 516. Cite the callers by address instead (the packet lists
  ovl6:0000 CampLoop, ovl6:0B9B, 082C:000B, 082C:0B43, ovl0:207A, ovl1:13F7). "Do not consume the key
  here" is true of the ReadKey call, but the callers flush the ring after the call (FlushKeyBuffer at
  EOV6.C:353 and :517). Say so, so the header is not read as "callers never consume".
  Fixed (dec-deepseek-3): the 1AAD:038E header now lists the callers by address (EditTextField
  082C:000B, DelaySecondsThenKey 082C:0B43, PickClass ovl0:207A, ShowMessage ovl1:13F7, CampLoop
  ovl6:0000, f0B9B ovl6:0B9B) and says they flush the ring afterwards (FlushKeyBuffer, EOV6.C:353
  and :517).

- [x] should src/E1AAD.ASM:64 (kExtendKeys map): "1AAD:005D: the map it reads at [di+0Fh]". The
  read is at 1AAD:005C+i (di after `repne scasb` is 4Ch+i+1, plus 0Fh), so entry 0 of the map is
  1AAD:005C, the last byte of kExtendKeys, and 005D is entry 1. Write the map base as 1AAD:005C.
  Fixed (dec-deepseek-3): the map comment now reads 1AAD:005C and says entry i is at 1AAD:005C+i,
  so entry 0 is the 3Eh above and 005D is entry 1.

- [ ] note src/E1AAD.ASM:44-47 (g8488; reopened in round 3: the clear cites it added are wrong, see the
  round 3 must-fix): "tick counter" is right, but the unit also clears the count
  itself: 1AAD:0C17 writes 0 at line 1393, and the replay path at line 1350. Say so, so the counter is
  not read as one that only counts up.
  Fixed (dec-deepseek-3): the g8488 comment now says 1AAD:0C17 writes 0 after a recorded write
  (line 1350) and after a replay read (line 1393).

- [x] note src/E1AAD.ASM:306 (the 0409 header): "(E16A5.C:60-61, :127-128)". Lines 60-61 of E16A5.C
  are the extern declarations of ReadKeySpecial and WaitGameKey2, not calls. The calls are at :127-128.
  Fixed (dec-deepseek-3): the 0409 header now cites only E16A5.C:127-128.

- [ ] note for other owners (not this unit, round 1 notes still open): src/E07C8.C:93-99 (the
  InstallKeyboard copy cite, round 1); src/E1A72.ASM:35 still says "game state, 0/1/2" for g80EC;
  src/EOV1.ASM:114 and src/EOV3.C:28 still say g80E8 is "toggled by the Ctrl-key handling" (the same
  wording round 1 :18 removed here). Their owners should align them.

### Summary

Round 1's nine must-fix items are fixed right; their ticks stay. Round 1's should at :18 (the 0718
header) is only partly fixed and is reopened. Four must-fix items are open in this round: the 0718
header, the 0B0E release claim, the 0223 header, and the g80EC and g80EF readers. The rest are shoulds
and notes. Nothing is escalated.

## Decompiler response (dec-deepseek-3, round 2)

All four must-fix items, the two shoulds and the two notes for this unit are fixed in `src/E1AAD.ASM`;
`make match FILE=src/E1AAD.ASM` is 18 of 18 (3405 of 3405 bytes) and `make readability UNIT=e1aad` is
0 findings. The findings are comments only; no byte moved.

- 1AAD:0718 header: now "toggles gSoundOn on key 0420h (S) and g80E8 on key 0434h (M)".
- 1AAD:0B0E header: now says 038E and 053A discard the 08h bit of AH, 0409 and 0588 do not, and a
  release (41h or 42h) reaches the game through DispatchKey (E16A5.C:127-128). "AH bit 8" is now "the
  08h bit of AH" in the 038E (:230), 053A (:465) and 0588 (:512) headers too.
- 1AAD:0223 header: now says the Ctrl-function table's keys are 5Ah..6Dh (indexed by AL-5Ah), that
  41h..4Ah (the mouse codes 41h and 42h among them) come back as 0C6h..0CFh, and that the tables hold
  ASCII for the main table and for some keypad and Ctrl-function entries, a game code above 80h
  otherwise.
- g80EC/g80EF externs: g80EC now says the state 2 readers skip the wait for a ring event and take the
  ring slot that 1AAD:0D26 has filled; g80EF now says 1AAD:048D reads ds:80EF back directly in state 2
  and the other state 2 readers (1AAD:038E/0409) read the ring slot that 0D26 filled with it.
- 1AAD:038E header (:235): the EOV6.C line cites are replaced by the packet's caller list by address
  (EditTextField 082C:000B, DelaySecondsThenKey 082C:0B43, PickClass ovl0:207A, ShowMessage ovl1:13F7,
  CampLoop ovl6:0000, f0B9B ovl6:0B9B), and it now says the callers flush the ring afterwards
  (FlushKeyBuffer, EOV6.C:353 and :517).
- kExtendKeys map (:64): the map base is now 1AAD:005C, with the note that entry i is 1AAD:005C+i (so
  entry 0 is the 3Eh above and 005D is entry 1).
- g8488 (:44-47): now adds that 1AAD:0C17 writes 0 after a recorded write (line 1350) and after a
  replay read (line 1393).
- 0409 header (:306): now cites only E16A5.C:127-128 (60-61 are the extern declarations).

The "note for other owners" stays open: src/E07C8.C:93-99, src/E1A72.ASM:35, src/EOV1.ASM:114 and
src/EOV3.C:28 belong to other units and are not changed on this branch.

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

