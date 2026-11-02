# Review: eov6

Reviewer: rev-haiku
Commit reviewed: b71e380 (main, round 3; round 2 reviewed dbf4ce2, round 1 reviewed 4afb5a6, when I started; this branch was at 1e58aea; review/eov6 is at 0d46c17, whose src/EOV6.C and re/globals.toml are the same as main's)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Findings

Each finding is a checkbox. `must-fix` blocks `make review-pass` until a decompiler ticks it off
(with the commit that fixes it); `should` and `note` do not block.

### Round 3 (rev-haiku, at b71e380)

Playbook 70, later round. Judged: the round 2 must-fix and shoulds (fix commit d105793, merged in 6ca33d3)
and the changes since dbf4ce2 on main (`git diff dbf4ce2 main -- src/EOV6.C re/symbols.toml re/globals.toml`:
the d105793 comment and name edits in src/EOV6.C, and the gClickX and gClickY `why` text in
re/globals.toml from ca7be3d; re/symbols.toml changed only for other units, the e1948 twin entries, and
none for ovl6). Checked: `make match FILE=src/EOV6.C` 6 of 6 (3258 of 3258 bytes), exit 0; `make
readability UNIT=eov6` 0 findings. Checked against the code and the bytes: the sign of the memorized
slots (the regain at src/EOV6.C:196-202 turns a negative slot positive and prints "gained"; the healer's
cases 2, 0xf and 0x14 at :244-256 match positive values and negate them; the class-2 grant at :298-302
finds 0xffe8, the byte -0x18, and writes 0x18; f0A92 returns 1 on a negative memorized slot at :470-475);
the QUIRK reads in build/re/eob-overlays/ovl06.bin (07C9 is `83 7e fa 00`, cmp word ptr [bp-6],0; 07D9 is
`8b 46 fa`, mov ax,[bp-6]; 0765 is `74 3e`, a je to 07A5); src/E0A72.C:29 (`char pad21[3]`); the ClickInRect
cite (src/E082C.C:529-531) and src/E14EF.C:325 (IsPointInRect reads the click point); DrawShadowText's
colour2 forwarding (src/E082C.C:467-472).

Round 2 must-fix (item 27, reopened): fixed right. The f0A92 header and the struct comment on `memorized`
now state the sign the code shows (positive: ready to cast; negative: spent or not yet regained). Tick
stays. The round 2 shoulds hold: the f0858 cite, the DrawShadowText declaration, the level cite, the
QUIRK and the FindClassSlot note. The flag 3 gate is in, with one gap (the should below).

- [ ] must-fix src/EOV6.C:116 (the CampLoop header, the 24-hour sentence): "every 24 hours it damages a
  member with flag 3 at level 0 above 0xf6 hit points" is wrong. `hp` is a `signed char` (src/EOV6.C:17),
  and the test at :320 is `hp > (signed char)0xf6`, that is hp > -10. No member has 246 hit points as a
  signed char, so the sentence names a threshold no member can reach. Say "above -10 hit points
  (hp > (signed char)0xf6; hp is a signed char)". The sentence was rewritten by d105793 (the flag 3 gate),
  so it is a changed line.
- [ ] should src/EOV6.C:113-115: the regain and the level drop (:306-319) apply only to a member above level
  0 (`if (gChars[member].level != 0)`), and the 24-hour damage to a member at level 0 (:320). The header
  gives the level 0 condition for the damage only. Say "for a member with flag 3 above level 0".
- [ ] should src/E082C.C:465 (owner e082c, the DrawShadowText header): "src/EOV6.C:399" is a stale line cite
  into this unit. src/EOV6.C:399 is `if (draw) {`; the two DrawShadowText calls of f0858 are at
  src/EOV6.C:405 and :411. The owner should cite the address (f0858, ovl6:0858) or those two lines. This
  unit's file does not change for it.
- [ ] note make match still prints "unit eov6 not complete: ours 3258 bytes, the unit 3258" (rounds 1 and 2).
  The print is the cmatch `ovl is None` test (tools/match/cmatch.py:605). review-pass does not depend on it:
  tools/agent/review.py:29 checks unit_complete (every function at L2, tools/agent/common.py:190). A cosmetic
  matcher question for the human, not a review blocker.

Round 3 verdict: not passed. One must-fix (new: the 24-hour threshold, a changed line). The round 2
must-fix is fixed right and stays ticked.

### Round 2 (rev-haiku, at dbf4ce2)

Playbook 70, later round. Judged: the round 1 must-fix items and the changes since 4afb5a6
(`git diff 4afb5a6 main -- src/EOV6.C re/symbols.toml re/globals.toml`: the round 1 fix commit 2315c8f
and renames from other units). Checked: `make match FILE=src/EOV6.C` 6 of 6 functions, exit 0;
`make readability UNIT=eov6` 0 findings. Checked against the bytes: the QUIRK addresses, by
disassembling build/re/eob-overlays/ovl06.bin (ovl6:0718 and 0765 are the two gotos to done at 07A5,
0x76E is the ReadKey exit, 07D9 is `mov ax,[bp-6]`); the far string table, counted from 2003:0000 in
eob/EOB.EXE (index 0x2B is " All characters are fully rested.", index 0x3B is "Poisoned party members
will die! Rest anyway?"). The other round 1 items I re-checked against the code hold.

- [x] must-fix src/EOV6.C:447-451 (the f0A92 header) and src/EOV6.C:28 (the struct comment on
  `memorized`): both say a negative memorized slot is "a spell ready to cast". Round 1 item 27 was
  ticked, but its fix states the opposite of the code, so it is reopened. The sign runs the other
  way: a positive memorized slot is ready to cast; a negative one is spent or not yet regained.
  Evidence: the regain step turns a negative memorized slot positive and prints "gained"
  (src/EOV6.C:194-198); the healer's switch matches only positive 2, 0xf and 0x14 and negates the slot
  it casts (src/EOV6.C:238-251); f0A92 returns 1 while a negative memorized slot exists
  (src/EOV6.C:465-468), so the camp goes on until it is regained; ovl1:025D writes a slot as the
  negation of the spell index when the spell is not ready (src/EOV1.ASM:453-459); CastSpell negates the
  memorized slot it spends (src/E0EF3.C:2031). Fix both places to say: a negative memorized slot is
  spent or not yet regained (CampLoop turns it positive as "gained"); a positive one is ready to cast.
  The known-slot wording (negative: to be memorised) is right.
  Fixed: the f0A92 header now reads "a negative 'memorized' slot (a spell spent or not yet regained;
  a positive one is ready to cast)" and the struct comment on `memorized` now reads "positive: a
  spell ready to cast; negative: spent or not yet regained (CampLoop turns it positive as 'gained')".
  Comments only; make match FILE=src/EOV6.C 6 of 6 (3258 bytes), exit 0.
- [x] should src/EOV6.C:381-382 (the f0858 header): "src/E082C.C:403-408" is stale; those lines are
  now gDrawLine. The claim holds: DrawShadowText forwards its fifth argument to its first shadow draw
  only (src/E082C.C:465-471, address 082C:0898). Cite the address.
  Fixed: the f0858 header now cites DrawShadowText 082C:0898 (the address) instead of the stale line
  range, and says gF57E is the colour2 (fifth) argument of the first shadow draw only.
- [x] should src/EOV6.C:84: the declaration names the parameters `color` and `extra`. The owner now
  names them `colour` and `colour2` (src/E082C.C:465, after the e082c round 4 rename). Align with the
  owner's names.
  Fixed: the declaration is now `DrawShadowText(char *text, int x, int y, int colour, int colour2)`,
  matching the owner (src/E082C.C:465); the gF57E comment now calls it the colour2 argument.
- [x] should src/EOV6.C:111-113 (the CampLoop header): the eight-hour regain, level drop and class-2
  spell, and the 24-hour damage, apply only to members with flag 3 (src/EOV6.C:298-299,
  `if (!f0D1C_0670(member, 3)) continue;`). The header says "a member" throughout. Say "a member
  with flag 3".
  Fixed: the header now says the regain, level drop and 24-hour damage are "for a member with flag 3
  (`if (!f0D1C_0670(member, 3)) continue;`)". The class-2 spell 0x18 is a separate case: the code
  applies it to a member whose classIndex is 2, before the flag-3 test (src/EOV6.C:292-297), so the
  header names that gate too rather than lumping it with flag 3.
- [x] should src/EOV6.C:114-122 (the CampLoop header and QUIRK): the warning box test
  `if (!foundWounded && !result)` (src/EOV6.C:349) also reads `result` on the goto paths
  (ovl6:07C9, `cmp word ptr [bp-6],0`, the same slot as 07D9). The QUIRK names only `return result`,
  and the box sentence says "the last f09D2 result was 0", which is unset on those paths. Name both reads.
  Fixed: the QUIRK now reads "QUIRK ovl6:07C9, ovl6:07D9" and names both reads: the box test
  `!foundWounded && !result` (ovl6:07C9) and the final `return result` (ovl6:07D9). The `result`
  declaration comment now cites both addresses.
- [x] should src/EOV6.C:25: "src/E0A72.C:21 leaves +23 as padding" is a stale line cite. The padding is
  `char pad21[3]` in struct CharRec at src/E0A72.C:29 (+21 to +23). The claim holds; cite :29.
  Fixed: the `level` comment now cites src/E0A72.C:29.
- [x] note src/EOV6.C:463-464: the f0A92 second test is `FindClassSlot(4, ...) != 0` (the first is
  `!= -1`). The header's "slots 2 or 4 apply" does not show that; a reader may take it for a typo.
  Addressed: the header now spells both tests out, "slot 2 matches (FindClassSlot(2, classIndex) != -1)
  or whose slot 4 test does not return 0 (FindClassSlot(4, classIndex) != 0)", so the asymmetry is
  visible.
- [ ] note make match: still prints "unit eov6 not complete: ours 3258 bytes, the unit 3258" (round 1
  note, unchanged). A matcher question for the human.

Round 2 verdict: not passed. One must-fix (round 1 item 27, reopened, with its second place in the
struct). The sign convention is read from the code and the other units, not from the matcher, so there
is no escalation.

### Round 1 (rev-haiku, at 4afb5a6)

- [x] must-fix src/EOV6.C:97: "In six sub-steps (ds:tick) per hour" puts the hour counter in ds, but `tick` is an automatic local (line 122, `int tick;`). Say "six sub-steps per hour (the local tick)". Fixed: the CampLoop header now says "In six sub-steps per hour (the local tick)".
- [x] must-fix src/EOV6.C:100-101: "every eight hours ... updates the object list" is wrong. CampLoop never calls UpdateObjects; only f09D2 does (lines 406-407), and f09D2 runs at the start (line 127) and on each key test (line 320). Evidence: the only UpdateObjects calls in EOV6.C are in f09D2. Fixed: the CampLoop header now says only f09D2 updates the object list, at the start and on each key test.
- [x] must-fix src/EOV6.C:100: "levels members up or down" is wrong; the code only lowers levels (lines 286-292: a level of 5 or less becomes 0, else 5 is taken off). Say "drops levels; a member at 0 leaves the party [H]", or "drops levels". Fixed: the header now says it "drops the level of a member whose 0D1C:0163(member, 2) is negative (by 5, or to 0 when at 5 or less)".
- [x] must-fix src/EOV6.C:101-102: "every read of a key ... shows the warning box (f0B9B)" is wrong. The box is shown only on the done path, when `!foundWounded && !result` (lines 329-330). The key tests in the loop (lines 319-323) do not show it. Fixed: the header now says the box shows on the done path, when the last f09D2 result was 0 and no member is wounded.
- [x] must-fix src/EOV6.C:102-103: "Returns 1 or 0 ... or the 09D2 result otherwise" is wrong on three paths. `result` is assigned only at line 320, so the gotos at lines 306 and 317 and the ReadKey branch at line 319 reach `return result` (line 331) with an unset local. The caller tests AX after the call (src/EOV1.ASM:3692-3694, `call _fOvl6_0000`, `or ax,ax`), so the value does reach a branch. Say so in the comment. See the note below. Fixed: the header now names the two return values that are explicit (1 from the opening f09D2, 0 from the starving picker) and says every other exit returns `result`, assigned only by the f09D2 call in the key test, so the two gotos and the ReadKey branch return whatever the frame held (the caller tests AX, src/EOV1.ASM:3692-3694); the read is marked `QUIRK ovl6:07D9:`.
- [x] must-fix src/EOV6.C:58: the gF57E comment ("the colour 082C:0898 draws with (ovl6:0858)") is wrong. f0858 (line 383) passes 0xf as the text colour and gF57E as the fifth argument, which DrawShadowText forwards only to its first shadow draw (src/E082C.C:403-408: "`extra` is forwarded as the fifth argument of the first shadow draw"). Evidence: src/E082C.C:403-408, src/EOV6.C:383. Fixed: the comment now says gF57E is the extra argument ovl6:0858 passes, forwarded only to DrawShadowText's first shadow draw (src/E082C.C:403-408).
- [x] must-fix src/EOV6.C:359-360: the f0858 header says the line is drawn "in the colour gF57E", which repeats the error above. Say the text is drawn in 0xf with gF57E as the shadow's extra argument (src/E082C.C:403-408). Fixed: the f0858 header now says the line is drawn in color 0xf with gF57E as the first shadow draw's extra argument (src/E082C.C:403-408).
- [x] must-fix src/EOV6.C:392 (and the struct at lines 35-39 and the extern at line 44): f09D2 says the records are "at ds:8A84", and the name gTable8A84 and `Entry28.value` treat that as a separate table. The object table is `struct Obj28 gObjs[]` at ds:8A82, 28 bytes each, with `int id` at +2 (src/E08E6.C:11-13 and 80). ds:8A84 is the id field of record 0, and `Distance(obj->id, gPartyCell)` (src/E08E6.C:221) is the same test as line 410 here. Use the object table's name and struct, or say the offset explicitly. Evidence: 8A82 + 28i + 2 = 8A84 + 28i for each i. Fixed: struct Entry28 is replaced by struct Obj28 (b0 +0, kind +1, id +2, from src/E08E6.C:11-13), the extern is renamed gObjs at ds:8A82, and f09D2 now tests `gObjs[i].id`.
- [x] must-fix src/EOV6.C:85: `fOvl1_243A` is given the stub address `2102:0039`, which is the stub for overlay 1 entry 0, i.e. ovl1:1ACD. build/re/eob.json, overlays[1].entries: index 0 is code_offset 6861 (0x1ACD), index 5 is code_offset 9274 (0x243A). ovl1:243A is entry 5. The stub for entry 5 is [H] 2102:0052 (entries are 5 bytes, docs/design/WHITEPAPER.md:93); verify with `make overlays` before writing it down. Verified, no change: the entries of a stub segment start at +0x20 and are 5 bytes each (docs/design/WHITEPAPER.md:93, tools/re/fbov.py:77, tools/match/cmatch.py:257-258), so stub 2102 entry 0 is 2102:0020 (code_offset 0x1ACD) and entry 5 is 2102:0039 (code_offset 0x243A). Reading the stub bytes of eob/EOB.EXE at 2102 with the fbov layout gives i=0 2102:0020 = 1ACD and i=5 2102:0039 = 243A; build/re/eob.json overlays[1] agrees. The current annotation `/*@ 2102:0039 */` is correct; 2102:0052 would be entry 10, which does not exist.
- [x] must-fix src/EOV6.C:112 and 152: `healerFound` says a healer was found, but its value is the return of fOvl1_243A(0x28), the list picker, which src/EOV1.ASM:4232-4240 documents as returning a flag whose meaning is "Purpose unknown". Rename it to what it holds (a placeholder such as `flag2`) or say that the picker's answer is unknown. Fixed: renamed `healerFound` to `flag2` with the comment "fOvl1_243A(0x28), the healer picker's answer (meaning unknown)"; all four uses updated.
- [x] must-fix src/EOV6.C:116, 289 and 296: `gained` is set to 1 when a member drops to level 0 (line 289) or takes one point of damage at level 0 (line 296). Nothing is gained on either path. Rename (a placeholder such as `changed`), or describe what the two paths share. Fixed: renamed `gained` to `changed` with the comment "set when a member drops to level 0 or takes the 24-hour damage"; all four uses updated. (The string literal "%s gained %s." is unchanged.)
- [x] must-fix src/EOV6.C:459: the f0B9B header says it "selects the 8x8 font and region 2". The code selects region 0xb (line 477, `SetTextWindowAndClear(0xb)`); region 2 is not used. g8115 = 2 (line 478) is the value the header meant. Evidence: src/EOV1.ASM:2473-2475, the same routine (ovl1:13F7) clears window 0xB. Fixed: the header now says it selects text region 0xb and stores gStatusLine = 2 at ds:8115.
- [x] must-fix src/EOV6.C:463: the f0B9B header says it is called "for 'You can't rest here, monsters are near.'". The only caller (line 330) runs when `result == 0`, that is when f09D2 found no object near (line 415 prints that message only on the path that returns 1). The message at key 0x2b is some other string. Say which message, or Purpose unknown. Fixed: the header now says the only caller's message is 0x2b; reading the far table at 2003:0000 (SkipStrings order) gives 0x2b = "All characters are fully rested.".
- [x] must-fix src/EOV6.C:426-427 (f0A92 header): it says the memorized slots hold "a negative (not yet memorised) spell". The struct comment (line 25) says a negative memorized slot is "a spell ready to cast", and the healer casts negative memorized slots (lines 211-213, 218-225). The known slots are the ones memorised (lines 193-203). The code supports "ready to cast" for memorized; fix the f0A92 header. Fixed: the header now says a negative "memorized" slot is a spell ready to cast and a negative "known" slot is a spell still to be memorised. Reopened in round 2 (wrong sign: a negative memorized slot is spent or not yet regained; see Round 2).
- [x] should src/EOV6.C:75: the fifth parameter of DrawShadowText is named `flag` here. src/E082C.C:403 and src/E14EF.C:83 name it `extra`, and the caller passes gF57E or 0, not a flag. Rename it `extra`, and use "color" to match src/E14EF.C:83 (this file says "colour"). Fixed: the declaration is now `DrawShadowText(char *text, int x, int y, int color, int extra)`; the two "colour"s in f0858's header are now "color".
- [x] should src/EOV6.C:86: `FindClassSlot(int value, int classIndex)`. The definition is `FindClassSlot(int slotId, int classIndex)` (src/E0A72.C:89). Use `slotId`. Fixed: renamed the first parameter to `slotId`.
- [x] should src/EOV6.C:71 and 80: `FindCarriedSlot(int member, int what, int which)` and `DamageCharacter(int member, int what)`. The definitions are `FindCarriedSlot(int who, int field2, int fieldB)` (src/E0D1C.C:229) and `DamageCharacter(int who, int amount)` (src/E0DE8.C:540). Align with the owners, or keep placeholders; `amount` is evidenced by the caller's use, `what` is not. Fixed: the declarations are now `FindCarriedSlot(int who, int field2, int fieldB)` and `DamageCharacter(int who, int amount)`.
- [x] should src/EOV6.C:42-44: gSpells1344 is declared here with a unit-local 19-byte struct that has only `name`. src/E0EF3.C:157-161 declares the same table with `name` (+00), `flags` (+04), `apply` (+05) and `ddFn` (+09). Use the full struct, or say the other fields are not read here. Fixed: the struct comment now says this unit reads only the name pointer and cites src/E0EF3.C:155-161 for flags +04, apply +05 and ddFn +09.
- [x] should src/EOV6.C:59: gF442 is `signed char` here; src/E08E6.C:94 and src/EOV4.C:1029 use `char`. The same byte, but align the type with the other units. Fixed: gF442 is now `char`.
- [x] should src/EOV6.C:46-48: the g159 comment says it "holds gF57E while ovl6:0B9B is on screen". Its writers are src/EOV6.C:476 and src/EOV1.ASM:2496 (ShowMessage). No reader was found in src/ (grep for ds:0159 finds only these and the EXTRN in src/EOV1.ASM:62). Say "written from gF57E by ovl6:0B9B and ovl1:13F7; no reader found in src/". Fixed: the comment now says exactly that.
- [x] should src/EOV6.C:464: f0B9B has the same body as ovl1:13F7 `_ShowMessage` (src/EOV1.ASM:2467-2500: the 8x8 font, gF57E into g159, window 0xB, g8115 = 2, the message from gStrings2003, a 0x78-frame key wait). The name `f0B9B` is a placeholder; either cite the twin in the header or name it after the evidence (ShowMessage, in a copy in overlay 6). Fixed: the header now opens "the copy in overlay 6 of ShowMessage (ovl1:13F7)".
- [x] should src/EOV6.C:18: `maxhp` at +1C. src/E0DE8.C:26 names +1C `m1C` ("a copy of the max hit points"), and E0DE8.C:588-589 clamps to it the same way as lines 242-245 here. The meaning agrees; cite E0DE8.C:26 in the comment and consider the same name. Fixed: the maxhp comment now cites src/E0DE8.C:26 and :588 (m1C, "a copy of the max hit points"); the name maxhp is kept as more readable.
- [x] should src/EOV6.C:22: `level` at +23, "(0: not in the party)". This is an inference from lines 166, 280 and 287-288. Mark it [H] and say what it rests on; src/E0A72.C:21 leaves +23 as padding. Fixed: the comment now marks it [H], names the tests it rests on (0 stops the spell scan and regain; 5 or less drops to 0) and cites src/E0A72.C:21.
- [x] should src/EOV6.C:336: the f07E2 header says hit points "below his maximum (at most 0x384)". 0x384 is the start value of the running minimum (line 345), not a cap on hit points. Say "any hit points below 0x384", and note that hp is a signed char. Fixed: the header now says hit points below their maximum and below 0x384 (the running minimum's start value; hp is a signed char).
- [x] should src/EOV6.C:391-396 (f09D2): the header says the routine "switches back to the previous region (saved in gTextRegion)". The local `member` holds the saved region (lines 401, 411, 417), and the message is drawn in region 7 (line 412). Rename the local `savedRegion`, and say the message draws in region 7. Also "a monster is close": the records are the Obj28 map objects (src/E08E6.C:11), not only monsters; say "an object". Fixed: renamed the local to `savedRegion`, the header now says it switches to region 7 and "an object is close".
- [x] should src/EOV6.C:96-103: the header leaves out the hourly DamageCharacter(member, 10) on members with flag 1 and without flag 0x10 (lines 261-268) and the eight-hour regain of one hit point (lines 281-285). Say what they do, or Purpose unknown. Fixed: the header now gives both (hourly DamageCharacter(member, 10) for members with flag 1 and without flag 0x10; every eight hours one hit point regained on a member below his maximum).
- [x] should src/EOV6.C:141-155 and 235-248: `foundWounded` holds the 1-based most wounded member (f07E2, lines 141 and 157). It is then reused as a 0-based healing target (lines 235-237) and as a zero test (lines 248 and 252). The name fits only the first use; a note, or a second name for the healing target, would help. Fixed: the declaration now carries the comment "1-based most wounded member (f07E2); reused 0-based as the target".
- [x] should src/EOV6.C:118: `counters` has no comment. It is compared with `'0' + member / 6` (lines 172 and 191) and reset to '0' (lines 176 and 195), so it appears to be the spell level reached in this rest. Say so, or Purpose unknown. Fixed: the declaration now carries the comment "the spell level reached this rest, as '0'+n".
- [x] note src/EOV6.C:318-331 (the uninitialised `result`): on the three paths in the must-fix above, the caller branches on whatever is in that stack slot. The bytes match, so this is the original's behaviour, not a transcription error. A human should decide whether to record it as a quirk. It is not a matcher problem. Recorded: the read at ovl6:07D9 (`mov ax,[bp-6]`) is marked `QUIRK ovl6:07D9:` in the CampLoop header, listing the paths that reach the done label without assigning `result`.
- [ ] note make match: `make match FILE=src/EOV6.C` exits 0 with 6 of 6 functions matching, and prints "unit eov6 not complete: ours 3258 bytes, the unit 3258". tools/match/cmatch.py:605 requires `ovl is None` before it can say "complete", so an overlay unit can never print complete. That is a matcher question for the human, not a unit problem; `make review-pass` may depend on the same flag.

## Summary

The unit is readable as a whole: the six functions are matched (6 of 6, exit 0), `make readability UNIT=eov6`
reports 0 findings, and the placeholder names (CampLoop, f07E2, f0858, f09D2, f0A92, f0B9B) carry their
addresses and a purpose from the code. The shared globals and callees mostly agree with src/ (gChars at ds:F580,
gTextRegion at ds:811F, gFE5A, gA9C0, gPartyCell, gFileNameBuf, g8115, g8492, UpdateObjects, RollDice,
DamageCharacter, ClipRect). The problems are the comments and four names. Fourteen header and name claims are
wrong against the code or the other units (the object table at ds:8A82 is not at ds:8A84; the stub 2102:0039 is
entry 0 (ovl1:1ACD), not ovl1:243A; the warning box is not shown on every key; `gained` and `healerFound` name
events the code does not show; f0B9B's region is 0xb, not 2; memorized negative slots are "ready to cast"). The
source is understandable from itself once those are fixed. Placeholders that stay (f0858, f09D2, f0A92, f0B9B,
flag4, flag8) are fine; no name here claims evidence it lacks except those listed above.

## Round 1 fixes (dec-deepseek)

All 14 must-fix and all 14 should findings are fixed; the 1 quirk note (the uninitialised `result`) is now
marked in the source. The stub-address finding (src/EOV6.C:85) is the one exception: the cited evidence, read
against the actual stub bytes and `build/re/eob.json`, shows the current `/*@ 2102:0039 */` is already entry 5
(ovl1:243A), because stub entries start at +0x20, not +0x39. No change was made there. The second note (the
matcher never printing "complete" for an overlay unit) is a matcher question for the human and is left for them.
No code byte changed: `make match FILE=src/EOV6.C` is 6 of 6, 3258 of 3258 bytes, exit 0, and
`make readability UNIT=eov6` is 0 findings.
