# Review: e0de8

Reviewer: p6-rev-6
Commit reviewed: ce0608f (HEAD; the previous round read 5529429)

Playbook 70. Judge what a matching file can still get wrong: names, types, comments, structure.
The matcher already proved the bytes; do not re-prove them, read for meaning.

## Round P6 (2026-11-04), commit ce0608f

Judged: the changes since 5529429 in src/E0DE8.C (543 lines), the e0de8 entries in re/symbols.toml,
and the e0de8 entries in re/renames.toml and re/fields.toml. The matcher still proves 20 of 20
functions; `make readability UNIT=e0de8` 0 findings. The previous round's must-fix items are fixed
and stay ticked; no new must-fix.

- [ ] should: the comments still use the old item and character field names in several places:
  `m02` for the ItemRecord `type` in ApplyItemAttack (:271), MoveCarriedItem (:279), UseItemByKind
  (:347) and UseItem (:395); `m0B` for the ItemRecord `value` in ComputeArmourClass (:660); `m0E`
  for the ItemType `m0E.word` in ComputeArmourClass (:663); `m01` for the CharRec `status` in
  DamageCharacter and ApplyStatusEffect; `mE2`, `mE3` and `m77` for `damageReduction`, `effectFlags`
  and `items`. Use the new field names so one word names one thing.
- [ ] should: the gViewFrontSide `why` cites "src/E122E.C:104"; the line is a stale file line cite
  in another unit's file. Cite the address 122E:xxxx instead.
- [ ] note: the P6 names are backed by detailed `why` entries: gViewToCellPos (ds:1A1B), gViewRelativePos
  (ds:1A2B), gClassSlotCount (ds:115F), gClassBaseBits (ds:1231), gWallHandlerIndex (ds:1905),
  gLauncherAmmoType (ds:552F), gArmourSlots (ds:553E), EndInvisibility (0EF3:12A1), RollMemberSave
  (0D1C:059B), IsWeaponDamageHalved (0D1C:0BEB), gPanelHighlight (ds:FB4C), gPanelShadow (ds:F53A)
  and gPanelFillColour (ds:F57E).
- [ ] note: the previous Round 2 should called g1A1B "the sound ids"; the P6 name gViewToCellPos
  says "Not a sound table" and the table values back it. The owner's AddDDEffect comment
  (src/E0BD2.C:506) still calls the fourth argument a sound kind, which is the contradiction to
  resolve in E0BD2's review, not here.

## Round 3 (5529429)

Judged: the round 2 must-fix (the HalveSaveDamageA and HalveSaveDamageB headers and HalveSaveDamageB's
parameter and body), the round 2 shoulds at their changed lines, and the diff since 64798e7 in
src/E0DE8.C (the round 2 fix, commit e7f5f0c). src/E0DE8.C is the same on HEAD (c619e8f) and on main
(5529429). On main, re/symbols.toml differs from HEAD only by two E0D1C renames (0D1C:02DC to
GetCurrentItem, 0D1C:070F to DrawItemIcon); this unit calls neither. Matcher on this tree: `make match
FILE=src/E0DE8.C` 20 of 20, exit 0; `make readability UNIT=e0de8` 0 findings (not re-judged).

Round 2 must-fix: fixed right, tick stays. HalveSaveDamageB is `struct Obj28 far *objRec` (:782), its
header (:775-781) says it reads the record's b0 as the gTable1A3B index, and the body (:788) reads
`gTable1A3B[objRec->b0].m02`. struct Obj28's b0 is at +0 (:50).

Round 2 shoulds, checked at their changed lines: RollToHit's `itemType` (:188-200; the bonus is m0B
when itemType is 1..3); UseItemByKind returns 0 once an item is found (:421-423; the code at :470-474
has gDF4C inside the `if`, return 0 after it, -4 when nothing is found); RunRecordAttack (:275-276, code
:291-294) and TryRecordAttack (:307, code :322-325) say 1 when IsKind2Or3 is false, which is what the
code does; gA9C6's cite 0ACD:0644 (Action0644 reads gA9C6 at E0ACD.C:555); the gB894 +4 cite 0D1C:0080
(E0D1C.C:36 names it). All sound except the items below.

## Findings

- [ ] should src/E0DE8.C:779-781 (HalveSaveDamageB's header): "Its chance is a real threshold". The
  chance is `GetIndexedByte(0, classByte, 4)` (:788), which returns a byte from the far buffer
  g3406[0] (0A72:0006, src/E0A72.C:69-81). Nothing in this unit or in docs/, re/ or work/ shows what
  those bytes hold: src/E0A72.C:38-41 only declares the four tables. The sentence contrasts B with A,
  whose chance is the 0/1 result of 059B (E0D1C.C:416-443), so A always halves. The contrast is
  shown; "real threshold" is not. Say what B's chance is (the byte 0A72:0006 returns from g3406), and
  either cite the buffer contents or say their range is not established here. I could not show the
  sentence wrong, so this is a should, not a must-fix.
- [ ] should src/E0DE8.C:142-145 (gDF4C): "set to 1 when an action took effect (0DE8:0359, 0DE8:0537,
  0DE8:05BB)" reads as the full list of setters. Those are the three routines in this file that set it
  (:352, :411, :470), but the owner's file sets it too: src/E0ACD.C:437, :479, :490, :565 and :571
  (and E1414.C:352 says the routine there sets it). Add "in this file", or name the others.
- [ ] should, for the owner of src/E0EF3.C: :316 still declares `HalveSaveDamageB(char far *type, int
  value)`, and the call at :891 casts `(char far *)&gObjs[*list]`. The definition is now
  `struct Obj28 far *objRec` (E0DE8.C:782). The bytes match (both are far pointers), but the owner's
  declaration names a different type and parameter from the definition. Not this unit's to change.

## Summary

No must-fix open. The round 2 must-fix is fixed right, and the changed lines are sound (the header
claims about RunRecordAttack, TryRecordAttack, UseItemByKind, gA9C6 and gB894 match the code and the
callees). The three shoulds are one unevidenced sentence in the 0FCA header, a gDF4C list that reads as
complete, and a declaration in the E0EF3.C owner's file. The round 1 and round 2 notes for other owners
stay open and are not re-judged. Nothing escalated: no matcher, spec or pin question.

## Round 2 (64798e7)

Judged: the eighteen round 1 must-fix items against the code, and the diff since a3d753d in
src/E0DE8.C (re/symbols.toml and re/globals.toml carry only the renames and the names above). Matcher
state on this commit: `make match FILE=src/E0DE8.C` 20 of 20, exit 0 (not re-judged).

Round 1 must-fix items: each re-checked against the code and the owner's file. All eighteen are fixed
right, ticks stay. The gDF4C cite below is stale but its wording is right.

Changed lines: each new header claim was checked against its body and its callers: RollToHit (callers
0DE8:02D4, 0DE8:0359, E0EF3.C:889 in 0A3F and E0EF3.C:980 in 0E1B), RunRecordAttack, IsKind2Or3,
ApplyItemAttack, UseCarriedItem (the AddDDEffect arguments), UseItemByKind (the m77[16] path),
UseItem (the -2 branch), CheckPartyWiped, DamageCharacter, AddExperience (caller 0C2B only),
LevelUpSlot (E1DE7.C:332 and :379, gTextAttr1 at ds:811D per E07C8.C:103), ComputeArmourClass (the
branches at :728, :738, :741, :750), HalveSaveDamageA (059B returns 0 or 1, E0D1C.C:416-443; its
callers 0A3F and 20A8 at E0EF3.C:905, :921 and :1794). These are sound except the items below.

## Findings

- [x] must-fix src/E0DE8.C:765 and :779: "B takes a pointer to an item type's class byte" is a wrong
  claim, in both headers (the sentence is in the 0F98 header and the 0FCA header). The only caller of
  HalveSaveDamageB, src/E0EF3.C:891, passes `(char far *)&gObjs[*list]`: a 28-byte object record
  (struct Obj28, :49). The byte it reads is that record's +0 `b0`, "an index into the ds:1A3B table"
  (:50), and the body does `gTable1A3B[*type].m02` (:786). Say that: B takes a pointer to an object
  record and reads its b0 as the gTable1A3B index. A takes a character index (unchanged, correct).
  Fixed: both headers now say B takes a pointer to a 28-byte object record and reads its b0 as the
  gTable1A3B index. HalveSaveDamageB's parameter is typed `struct Obj28 far *objRec` and the body
  reads `gTable1A3B[objRec->b0].m02`; make match still 20 of 20.
- [x] should src/E0DE8.C:193 (RollToHit header) and its local `kind` (`kind = gTable8DD2[slot].m02`):
  "its m02 kind selects the m0B weapon value when it is 1..3". m02 is the item type index (the Entry14
  comment, :75), and the code tests the value 1..3 on it. `kind` is the m0E meaning (:112). Say the
  item type index, and rename the local (for example `itemType`).
  Fixed: the header says "its m02 item type index selects the m0B weapon value"; the local is now
  `itemType`.
- [x] should src/E0DE8.C:421 (UseItemByKind header): "Returns 0 when an item was found and dispatched".
  The code returns 0 once an item is found (:471), whether or not AddDDEffect took it; the sound and
  gDF4C are inside the `if` at :464. Say "returns 0 once an item was found, whether or not AddDDEffect
  took it".
  Fixed: the header now says "Returns 0 once an item was found, whether or not AddDDEffect took it".
- [x] should src/E0DE8.C:275 (RunRecordAttack header): "(or 1 when the type does not attack)" repeats
  the claim round 1 removed from IsKind2Or3. Say "(or 1 when IsKind2Or3 is false)".
  Fixed: the RunRecordAttack and TryRecordAttack headers now say "(or 1 when IsKind2Or3 is false)".
- [x] should src/E0DE8.C:143-144 (gA9C6, gDF4C): "src/E0ACD.C:56" and "src/E0ACD.C:53" are stale. The
  declarations are now E0ACD.C:77 (gA9C6) and :74 (gDF4C); the wording agrees with both. Replace the
  lines with the addresses of the routines that set or read them, or the current lines.
  Fixed: the stale file line cites are gone. gA9C6 now cites 0ACD:0644 (its reader in E0ACD.C) and gDF4C
  cites the three routines in this file that set it (0DE8:0359, 0DE8:0537, 0DE8:05BB).
- [x] should src/E0DE8.C:149 (gB894): "the +4 byte is read at src/E0D1C.C:36". Line 36 is the gB894
  declaration; the comment there says 0D1C:0080 tests the byte. Cite 0D1C:0080 by address.
  Fixed: the comment now cites 0D1C:0080.
- [x] note src/E0DE8.C:29 (struct CharRec): `char maxhp` against src/EOV5.C:32, `signed char maxhp`.
  Same bytes under Turbo C++'s signed char default; the two units should spell one type.
  Fixed: the field is now `signed char maxhp`, matching src/EOV5.C:32.
- [x] note src/E0DE8.C:7 (file header): "It walks the" is on its own line; reflow the paragraph.
  Fixed: the file header paragraph is reflowed.
- [x] note src/E0DE8.C:306 (TryRecordAttack header, unchanged text): "(or 1 when the type does not
  attack)" is the same claim as :275; reword with the :275 fix. src/E0DE8.C:794 (ApplyStatusEffect
  header) runs past 100 columns.
  Fixed: TryRecordAttack is reworded with the :275 fix (above); the ApplyStatusEffect header is reflowed
  under 100 columns.
- [ ] note for other owners: src/E0EF3.C:313 declares RollToHit with `int flag` (this file names the
  fourth parameter `useStrength`, :201). src/E0A72.C:276-281 cites E0DE8.C:612-615 and :613 for the
  AddExperience call; it is now :640-643 (the threshold call is :640, LevelUpSlot :642). The owner of
  E0A72.C should cite 0DE8:0B45 by address.

## Summary

The eighteen round 1 must-fix items are fixed right. The changed lines are sound except one wrong
claim: the HalveSaveDamageB sentence in two headers says B takes an item type's class byte, but its
only caller passes an object record and it reads that record's b0. That sentence is must-fix; the
shoulds are stale or loose citations and one item-type naming issue. Nothing escalated: no matcher,
spec or pin question.

## Round 1 (a3d753d)

`make match FILE=src/E0DE8.C` reports 20 of 20 functions matching and `make readability UNIT=e0de8`
reports 0 findings. Those gates cannot see the items below. Line numbers are src/E0DE8.C unless
another file is named; addresses are `0DE8:xxxx` unless marked otherwise. Each owner's own file was
read for every shared global and callee, and the packet's caller lists were checked against the
headers.

## Findings

- [x] must-fix src/E0DE8.C:255-285: RunRecordAttack's header says the base is "doubled at random", but
  the code (line 277) is `result = roll + 2`. `roll` (265, 270-271) and `result` (266, 277-281) both
  hold a character index: `f0D1C_0670(result, 1)` and `DamageCharacter(result, damage)` (line 281;
  DamageCharacter's first parameter is `who`, line 540). Fix the header to say +2, and rename the two
  locals to what they hold (the member index read from g1A2B, and the member that is hit).
- [x] must-fix src/E0DE8.C:180-181 and :205 (and the struct comment at :52): RollToHit says it "marks
  gObjs[obj].m16 bit 0 as hit". Line 205 sets the bit before the roll (line 207), whatever the outcome,
  so a miss sets it too. DamageObject (08E6:0FBE, src/E08E6.C:830) sets the same bit. Say it is set on
  every call (the object was attacked), not that it was hit.
- [x] must-fix src/E0DE8.C:178-183, :187, :206, :214: RollToHit's local `ac` is documented as "that
  armour class". GetHitTarget returns "the d20 number a to-hit roll must reach" (0A72:0522,
  src/E0A72.C:306-309), a to-hit target, not an armour class. Rename `ac` (for example `toHit`) and
  fix the last sentence of the header.
- [x] must-fix src/E0DE8.C:125: gPartyCell is documented as "the global sound volume (src/E177D.C)".
  src/E177D.C:92 says it is "the party's map cell" and passes it as the sound's cell; src/EOV6.C:438
  uses Distance(gPartyCell, gObjs[i].id); src/E0D1C.C:336 indexes the nine-byte cell records at
  ds:B894 with gPartyCell * 9. Write "the party's map cell (src/E177D.C:92)". The same wrong wording is
  in other units (see Notes).
- [x] must-fix src/E0DE8.C:132: gDCA4 is documented as "the object acting this step, set by 0359". 0359
  records the target (its header, :315, and `gDCA4 = target;` at :342), and UseItem damages
  `gObjs[gDCA4]` (:496). It is the target object, not the actor.
- [x] must-fix src/E0DE8.C:131: gDF4C is documented as "set to 1 when a hit lands". It is also set on
  a successful UseCarriedItem (:389) and UseItemByKind (:446), and on the -1 bump in ApplyItemAttack
  (:332), where no hit lands. The owner's wording (src/E0ACD.C:53) is "set to 1 when an action took
  effect"; use that.
- [x] must-fix src/E0DE8.C:120: g552F is documented as "a byte table indexed by a class, used by 05BB".
  UseItemByKind indexes it by the item's m02 (`kind = g552F[m02]`, :409), and the function's own header
  says so (:394-395, "The item's m02 indexes g552F to a `kind`"). It is indexed by item type, not by class.
- [x] must-fix src/E0DE8.C:117: g1A3D is documented as "a class-byte table (0x1B per row) used by 0FCA".
  This file never reads it (the only mention is this declaration), and 0FCA (HalveSaveDamageB, :751)
  reads gTable1A3B[*type].m02. src/E08E6.C:39 gives ds:1A3D as the +2 byte of the 27-byte entries at
  ds:1A3B, that is gTable1A3B[0].m02. Remove the declaration, or keep it only as that alias.
- [x] must-fix src/E0DE8.C:156 and :327: f08E6_15B4 is the function src/E08E6.C:1039 defines as
  `Obj15B4(char who, int value)`, with the same parameters, and the packet's callee list names it
  Obj15B4. Rename it with `make rename OLD=f08E6_15B4 NEW=Obj15B4 WHY="src/E08E6.C:1039"`, or the
  name that is final for 08E6:15B4 at the merge.
- [x] must-fix src/E0DE8.C:146: RollHitPoints's parameters are named `who` and `level`. The owner
  (src/E0A72.C:164-179, and the definition at :180) names them `classIndex` and `m17`, says the first is
  the class index m20, and says the evidence does not show m17 is a level. LevelUpSlot passes
  gChars[who].m17 as the second argument (:652-656). Rename them to `classIndex` and `m17`.
- [x] must-fix src/E0DE8.C:374: UseCarriedItem's header says it acts "against the item type `field`".
  The code indexes gTable8DD2 with `field` (:385, `gTable8DD2[field].m02`) and passes that record's m02,
  the item type, to AddDDEffect. `field` is the 14-byte item record, not the item type.
- [x] must-fix src/E0DE8.C:394-396 (the tests at :413, :419, :425): UseItemByKind's header says kind 0x10
  takes "the first m77[0], m77[1] or the special m77[16] slot whose own m02 is 0x10". The m02 == 0x10
  test is on m77[0] and m77[1] only. The m77[16] branch (:424-425) pulls the item through RemoveListItem
  with no m02 test. Fix the sentence.
- [x] must-fix src/E0DE8.C:600: AddExperience's header says "Called by 082C:... and 0DE8:0C2B". The
  packet's caller list for 0DE8:0B45 has one entry, 0DE8:0C2B (AddPartyExperience). Remove "082C:..."
  (a placeholder, not a caller).
- [x] must-fix src/E0DE8.C:730-732, :737-738, :768: HalveSaveDamageA's header says it halves "when the
  f0D1C_059B test for character who succeeds". The code does not test that result. It takes 0D1C:059B's
  return value as `chance` (:737) and halves when a fresh RollDice(1, 20, 0) is at least `chance`
  (:738). src/E0D1C.C:416-443 says 059B returns 1 when a d20 roll is at least its value, so its return
  is 0 or 1, and the test at :738 holds for every roll. Either the callee is misread here, or the code
  is not what the comment says. The decompiler must work out which, and write the comment for what the
  bytes do. (ApplyStatusEffect at :768 uses 059B as a boolean, which fits E0D1C.C:416-443.) The header's
  "Called by ... 0DE8:101A" is also wrong: ApplyStatusEffect does not call HalveSaveDamageA (its only
  call to 059B is at :768), and the packet lists HalveSaveDamageA's callers as 0EF3:0A3F and 0EF3:20A8.
- [x] must-fix src/E0DE8.C:244-253: IsWeaponKind's name and header say that item types whose m0E kind is
  2 or 3 are weapons that attack. The dispatch in UseItem (:471-485) sends kind 1 to ApplyItemAttack,
  which its header calls an "item/effect attack" (:310), kind 2 to UseCarriedItem and kind 3 to
  UseItemByKind. Nothing in this file says kinds 2 or 3 attack. Either cite the evidence (which DD
  records or item types use 2 or 3 as attacks, from 1414:0172), or rename it to what it tests (m0E low
  seven bits equal 2 or 3) and drop "attacks" from the header.
- [x] must-fix src/E0DE8.C:530-531, against :537-539 and :549: DamageCharacter's header says "it derates
  mE2 (clamped at 0)", and the QUIRK on the next lines (and the test at :549) says the clamp never runs.
  The first claim is wrong as stated. Write that the derate happens and the clamp to 0 is dead
  (QUIRK 0DE8:09FD).
- [x] must-fix src/E0DE8.C:162: RedrawMember's parameter is named `dir`. The owner, src/E1547.C:456,
  defines `RedrawMember(int who)`, and every call in this file passes a member index (:432, :483, :492,
  :500, :571, :593). Rename it `who`. Other files use `dir` too (see Notes).
- [x] must-fix src/E0DE8.C:154-155: AddDDEffect is declared twice with different parameter names. The
  second line (:155, `slot, a, b, c, d`) is stale. The first (:154) names its fourth argument
  `tableByte`, but the owner (src/E0BD2.C:506, with its comment just above) is
  `(who, item, volume, kind, page, type)` and says the fourth is a sound `kind | 4`. The calls at :384
  and :441 pass the g1A1B sound ids (src/E0D1C.C:337). Delete :155 and name the parameters as E0BD2.C:506.
- [x] should src/E0DE8.C:1-4: the file header lists combat, damage, experience and armour class, but not
  item use (0359 ApplyItemAttack, 0474 MoveCarriedItem, 0537 UseCarriedItem, 05BB UseItemByKind,
  0763 UseItem), which is about half the unit.
- [x] should src/E0DE8.C:183: RollToHit's "Called by" lists 0DE8:02D4 and 0DE8:0359 only. The packet also
  lists 0EF3:0A3F DamageTargetCell and 0EF3:0E1B CastMeleeEffect as callers of 0DE8:0007.
- [x] should src/E0DE8.C:536: "0E08E6" is not an address; it means 08E6. The same caller list omits
  0DE8:01FF RunRecordAttack, which is in this unit.
- [x] should src/E0DE8.C:180: "the derived stat A" is the old name for GetStrengthToHitBonus (0A72:00CF,
  src/E0A72.C:100-107: the attacker's strength to-hit bonus). Say that, with the address.
- [x] should src/E0DE8.C:184, :222, :763-764, :766: the `flag` parameters are named for nothing they select:
  RollToHit (strength bonus or m15 value, :200-203), RollAttackDamage (strength damage bonus, :227),
  ApplyStatusEffect (skip when the bit is already set, :766). Rename them for the effect, for example
  `useStrength` and `skipIfSet`.
- [x] should src/E0DE8.C:186, :204: the local `b0` is the Obj28 field copied out, an index into gTable1A3B.
  Name it for that, for example `entry`.
- [x] should src/E0DE8.C:310-321: in ApplyItemAttack, `value` is the row from f0D1C_00DA, used as `value * 9`
  into gB894 (:329-331). Name it `row`. The header (:312-313) also omits that the -1 bump sets gDF4C (:332).
- [x] should src/E0DE8.C:310-316, :450, :501-504: ApplyItemAttack's return codes -1, -2 and -3, UseItemByKind's
  -4 (:450), and UseItem's `result < -2` (:501) are magic numbers with meaning. Name them, or say in the
  headers what each value means. UseItem's header (:457) gives the slot choice only as "slot or slot + 2";
  say it is `slot` when result is below -2 and `slot + 2` otherwise.
- [x] should src/E0DE8.C:398-399: UseItemByKind's header says a found item is "dispatched through
  AddDDEffect (sound 0x1A when m02 is 7, else 0x0B, setting gDF4C), returning 0". The sound and gDF4C
  (:441-446) happen only when AddDDEffect returns nonzero. Say so.
- [x] should src/E0DE8.C:373-376, :384, :459, :351, :377: `slot` means two things here: an entry of
  gTable8DD2 (RollToHit :178, RollAttackDamage :222, ApplyItemAttack :317, UseItemByKind :400), and a
  carried-item index m77[slot] (MoveCarriedItem :351, UseCarriedItem :377, UseItem :459, where it is also
  the m1E bit and the mE8 index). `field` (UseCarriedItem, UseItem) is the gTable8DD2 index too. Name the
  gTable8DD2 index `itemRec` (or `itemSlot`) and the carried index `carriedSlot`, or say in each header
  which one it is.
- [x] should src/E0DE8.C:703-704, :711: `max` and `second` are the two bonuses from m77[25] and m77[26].
  Name them for that (for example `bonus25` and `bonus26`). The header (:665-670) should say they count
  only items whose m0E kind is 0 (:706, :709).
- [x] should src/E0DE8.C:665-670, :696-697, :717-718: ComputeArmourClass's header omits three rules the code
  applies: (a) for slots 1 and 2 only item type 0x1B counts (:696-697); (b) when m1D is not above
  base + 6 the mDF branch clears mDF (the else at :717-718), a side effect; (c) the m0E kind test on
  m77[25] and m77[26] (:706, :709).
- [x] should src/E0DE8.C:597: "accepts" is wrong for GetClassSlotByte: the code skips a slot whose byte is
  -1 (:610-611), so write "whose byte is not -1 (the class has that slot)". The `value != -1` test on the
  threshold (:614) is undocumented: the level-up happens only when the threshold is not -1 and the total is past it.
- [x] should src/E0DE8.C:605: `unsigned long value` receives GetClassSlotExpThreshold's `long` result, which is
  -1 for "no threshold" (src/E0A72.C:282-292). Type it `long`.
- [x] should src/E0DE8.C:730 and :743: HalveSaveDamageA and HalveSaveDamageB differ only by an "A" or "B"
  suffix, which claims nothing. Name them for what their test uses (the first takes a character, the second
  an item record's class byte), or say in the headers why there are two.
- [x] should src/E0DE8.C:646, :659-660: LevelUpSlot's `attr` is the class index (src/E0A72.C:177, and
  AddExperience passes its `cls` at :615). Name it `cls`. The header quotes the sprintf format without its
  control bytes (\x06\x01 and \x06\x0f at :659); say what they are.
- [x] should src/E0DE8.C:159-160: ClearEffectFlag4 and RecomputeArmourClass take `void far *arg`, but the
  owners (src/E0EF3.C:1017 and :1153) take `struct CharRec far *rec`, and every call here passes
  &gChars[who]. Use that type (same bytes).
- [x] should src/E0DE8.C:158: DamageObject's `int n` should be `damage`, the owner's name (src/E08E6.C:816).
- [x] should src/E0DE8.C:143-144: GetClassSlotExpThreshold's parameters are `indexA, indexB`; the owner
  (src/E0A72.C:282) has `row, slot, level`. GetArmourClassBase's `classIndex` should be `m15` (src/E0A72.C:159).
- [x] should src/E0DE8.C:26: the field at +1C is `m1C` here, but src/EOV5.C:32 calls it `maxhp` ("maximum hit
  points") and EOV5.C:825-829 uses it that way. Rename the field in this file's struct (a field, so no
  `make rename` is needed).
- [x] should src/E0DE8.C:28, :32, :40, :37-42: struct comments that do not say what the code shows. m1E (:28)
  "a bit set/cleared by the per-slot helper": UseItem toggles it itself (:498). m24 (:32) "three per-class-slot
  counters": they are levels (AddExperience passes m24[i] + 1 as the level reached, :613; src/E0A72.C:276-281).
  mE3 (:40) "high word folded at 09B9": 09B9 clears bit 2 only (:557). mDF, mE2, mE7, mE8 (:37-42) have no
  comment, although DamageCharacter uses mDF (:563-569) and mE7 (:553).
- [x] should src/E0DE8.C:69-75 and :88, :107: Entry14 flags (:69) bits 8 and 0x10 are read by UseItem (:487,
  :494); m02 (:71) is the item type index into gItemTypes (:471, :409); m0B (:75) is the weapon or slot bonus
  used by RollToHit and RollAttackDamage (:197, :237) and ComputeArmourClass (:699). DDRec m0B (:88) is the
  attacker (TryRecordAttack :300 says so). ItemType m0E (:107) is also UseItem's dispatch kind (:471) and
  ComputeArmourClass's test (:694), not only 01D7's.
- [x] should src/E0DE8.C:123: gFBD2 says "bit 0 gates the icon colour row". RollToHit reads bits 4 and 5
  (:208). Add that use (the owner's comment, src/E0D1C.C:50, names bit 0 only).
- [x] should src/E0DE8.C:129: gFB44 is "a mode byte (src/E0D1C.C)". The owner says 0 and 1 gate 0D1C:01CA's
  redraws (src/E0D1C.C:108); here DamageCharacter redraws the member when it is 0 and the portrait otherwise
  (:570-573). Say so.
- [x] should src/E0DE8.C:130: gA9C6 is "an offset added into gEF6A's targets". In this file it is an offset within
  a nine-byte gB894 record (:312, :329-331), which src/E0ACD.C:56 also says.
- [x] should src/E0DE8.C:134: gB894's comment ("+0x19C bytes read via B894 (0359)") says nothing checkable.
  Say what 0359 uses: the nine-byte records at ds:B894 indexed by row * 9 (:329-331). The +4 byte is read
  elsewhere (src/E0D1C.C:36).
- [x] should src/E0DE8.C:119, :121: g1231 is "a byte table used by 0D5E", but it is indexed by the class m20
  (:693; src/E0BD2.C:102 says so). g553E is "a byte table used by 0D5E", but it holds the four m77 slot
  indexes that ComputeArmourClass walks (:687). Say what each indexes.
- [x] should src/E0DE8.C:122: g54F6 is declared but not used in this file (grep). Its use is in
  src/E0D1C.C:437 and :441. Remove the declaration or keep it with that note.
- [x] should src/E0DE8.C:506-528: CheckPartyWiped's header omits the LockShowPointer and DelaySecondsThenKey(1)
  calls (:525-526). "creatures with hit points" is f0D1C_0670(n, 5), which src/E0D1C.C:445-448 defines as
  m01 bit 0 and m1B > 0. Say that.
- [x] should src/E0DE8.C:153: RemoveListItem's third parameter is `item`; the owner (src/E0BD2.C:282) has
  `target`. Use the owner's name.
- [ ] note the same mistakes in other units, for their owners: the "global sound volume" wording for gPartyCell
  at src/E0D1C.C:39, src/E1414.C:94 and src/E0ACD.C:46 (E0ACD.C:478 says "volume is the global gPartyCell");
  the RedrawMember parameter `dir` at src/E0ACD.C:31, src/E0D1C.C:125, src/E0EF3.C:307 and src/E08E6.C:146;
  the RollHitPoints parameters `who, level` at src/EOV0.C:100; and AddDDEffect's third parameter `volume`
  at src/E122E.C:42, src/E0ACD.C:66 and src/E08E6.C:149, although src/E122E.C:729 passes an id there.
- [ ] note the struct CharRec m17 byte (:23) is also the m17 argument of RollHitPoints (src/E0A72.C:173-177,
  and LevelUpSlot at :652). The struct comment could say so.
- [ ] note the QUIRK at 0DE8:09FD (:537-539, :549) describes the compiled shape. The match proves the bytes;
  I did not check the disassembly claim itself, only that the source matches it.
- [ ] note the open question in the first must-fix (HalveSaveDamageA, :730-738) is a question about 0D1C:059B's
  return value, which src/E0D1C.C:416-443 owns. The owner of that unit should confirm what 059B returns and what
  its other callers do with it. It is not a matcher question.

## Summary

The unit is understandable for its arithmetic: all 20 functions match, the struct layouts agree with the
owning units, and the gates pass. It is not yet readable on meaning, and there are 18 must-fix items.
Several global comments contradict the owner's file or the unit's own code (gPartyCell, gDCA4, gDF4C,
g1A3D, g552F). Several function headers describe code that is not there or works differently
(RunRecordAttack's doubling, UseCarriedItem's item type, UseItemByKind's m77[16] test, DamageCharacter's
clamp, AddExperience's caller). HalveSaveDamageA reads a callee's 0/1 result as a threshold, which the
decompiler must check against the callee. The names IsWeaponKind, `ac`, RunRecordAttack's `roll` and
RollHitPoints's parameters claim more than the evidence shows. The placeholder names that remain
(f0D1C_0670's `who, mask`, and the A/B halving helpers) are acceptable only where their headers say so.
Nothing escalated: no matcher, spec or pin question.

## Decompiler response (dec-deepseek, round 1)

All 18 must-fix and all 30 should items are fixed in src/E0DE8.C (comments, names and types only; no
code byte changed). `make match FILE=src/E0DE8.C` reports 20 of 20 functions and 4279 of 4279 bytes;
`make readability UNIT=e0de8` reports 0 findings. The two renames were applied with `make rename`
(they are logged in re/renames.toml and follow in re/symbols.toml).

- RunRecordAttack (:255-285): header now says the base member gets +2, and `roll`/`result` are
  `baseMember`/`hitMember`, both character indexes (`f0D1C_0670` and `DamageCharacter` take a member
  index).
- RollToHit m16 (:180-181, :205, struct :52): now "bit 0 set on every 0007 call (the object was
  attacked)", whatever the outcome; DamageObject (08E6:0FBE) sets the same bit.
- RollToHit `ac` (:178-183, :187, :206, :214): renamed `toHit`, the d20 number GetHitTarget (0A72:0522)
  returns; the header says "to-hit target", not armour class.
- gPartyCell (:125): now "the party's map cell (src/E177D.C:92)".
- gDCA4 (:132): now "the target object 0359 records (:361); UseItem damages gObjs[gDCA4] (:520)".
- gDF4C (:131): now "set to 1 when an action took effect (src/E0ACD.C:53)".
- g552F (:120): now "indexed by the item's m02 (UseItemByKind :432)".
- g1A3D (:117): the declaration is removed (this file never reads it; it is gTable1A3B[0].m02).
- f08E6_15B4 (:156, :327): renamed Obj15B4 (make rename; the owner src/E08E6.C:1039).
- RollHitPoints (:146): parameters are now `classIndex, m17` per src/E0A72.C:164-179.
- UseCarriedItem (:374): `field` is now the item record (a gTable8DD2 index), not the item type.
- UseItemByKind (:394-396): the m02 == 0x10 test is on m77[0] and m77[1] only; m77[16] is pulled
  through RemoveListItem with no m02 test.
- AddExperience (:600): the "082C:..." placeholder caller is gone; the packet lists only 0DE8:0C2B.
- HalveSaveDamageA (:730-738): 0D1C:059B returns 0/1 (src/E0D1C.C:416-443), so the `RollDice(1, 20, 0)
  >= chance` test holds on every roll and the function always halves. The header now says so and marks
  QUIRK 0DE8:0F98; the "0DE8:101A" caller was wrong (ApplyStatusEffect only calls 059B) and is removed.
- IsWeaponKind (:244-253): renamed IsKind2Or3 (make rename); the header drops "attacks" and says it
  tests m0E low seven bits 2 or 3.
- DamageCharacter (:530-531, :537-539, :549): the header now says the derate happens and the clamp to 0
  is dead (QUIRK 0DE8:09FD).
- RedrawMember (:162): parameter renamed `who` (owner src/E1547.C:456).
- AddDDEffect (:154-155): the stale second prototype is deleted; parameters are
  `(who, item, volume, kind, page, type)` per src/E0BD2.C:506.

Should items: the file header now lists item use; RollToHit's caller list adds 0EF3:0A3F and 0EF3:0E1B;
"0E08E6" is 08E6 and 0DE8:01FF is added; "derived stat A" is GetStrengthToHitBonus (0A72:00CF); the
`flag` parameters are `useStrength`/`skipIfSet`; `b0` is `entry`; ApplyItemAttack's `value` is `row`;
the -1/-2/-3/-4 codes are spelled out in the headers and UseItem says slot below -2, slot + 2 otherwise;
UseItemByKind says the sound and gDF4C happen only when AddDDEffect returns nonzero; the two `slot`
meanings are named in each header (gTable8DD2 index vs carried index); `max`/`second` are
`bonus25`/`bonus26` and the header says they count only m0E kind 0; ComputeArmourClass's header adds the
0x1B, mDF-clear and m0E-kind rules; GetClassSlotByte says "whose byte is not -1" and documents the -1
threshold; `value` is `long`; the A/B headers say what each takes; LevelUpSlot's `attr` is `cls` and the
sprintf control bytes are named; ClearEffectFlag4/RecomputeArmourClass take `struct CharRec far *`;
DamageObject's `n` is `damage`; GetClassSlotExpThreshold is `(row, slot, level)` and
GetArmourClassBase is `m15`; the struct field `m1C` is `maxhp` and the m1E/m24/mE3/mDF/mE2/mE7/mE8 and
Entry14/DDRec/ItemType comments say what the code shows; gFBD2/gFB44/gA9C6/gB894/g1231/g553E/g54F6 are
documented; CheckPartyWiped names LockShowPointer and DelaySecondsThenKey(1) and the f0D1C_0670(n, 5)
meaning; RemoveListItem's third parameter is `target`.

Left unticked: the four `note` items, which are for the owners of the other units named there (gPartyCell
wording in E0D1C.C/E1414.C/E0ACD.C, `dir` in E0ACD.C/E0D1C.C/E0EF3.C/E08E6.C, RollHitPoints in EOV0.C,
AddDDEffect's `volume` in E122E.C/E0ACD.C/E08E6.C) or informational (the 0DE8:09FD QUIRK shape, the
CharRec m17 note). A unit branch must not edit another unit's file; `make integrate` applies the logged
renames there. The question about 0D1C:059B's return value is settled by src/E0D1C.C:416-443 (it returns
0/1) and is recorded in the HalveSaveDamageA header.
