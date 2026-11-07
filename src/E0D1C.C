/* Unit at EOB.EXE 0D1C:0006 (3265 bytes, 26 functions): the party-status and character-effect
   module. It walks the six 243-byte character records at ds:F580 (struct CharRec, src/E0A72.C),
   the 28-byte monster records at ds:8A82 (struct Monster, src/E08E6.C) and a 15-word table at
   ds:A9C8, and it draws the party rows through the rectangle and region front ends. Several
   helpers are pulled in from the character unit (0A72), the monster unit (08E6) and the resident
   drawing code. Most meanings are not established, so most function names say what a function
   does rather than what the game calls it. The unit is compiled with -v, the debug information
   that keeps identical code tails apart while TCC still sizes the jumps (whitepaper 2.1.1). */

/*@flags -ml -O -Z -v */

#include "EOB.H"

extern int gViewCellIds[]; /*@ ds:A9C8 */       /* a 15-word table searched by 0D1C:0006 */
extern char gViewDirty; /*@ ds:DF4C */        /* set to 1 when 0D1C:0006 finds the value */
extern signed char gInventoryMember; /*@ ds:A99A */ /* the selected character index (src/E0ACD.C) */
extern char gCursorSlot; /*@ ds:F51A */        /* an index into the 27 word slots of gChars[gInventoryMember].items
                                            (0D1C:02DC, 0D1C:0304); slot 16 is the own-list head
                                            0D1C:035A drains. E0ACD disagrees: it reads it as an
                                            action-menu row 0..2 with 0x10 for the own list */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern signed char gDirCellOffset[]; /*@ ds:1790 */  /* byte table indexed by the second argument of 0D1C:00DA */
extern signed char gOppositeSide[]; /*@ ds:547E */  /* byte table indexed by the second argument of 0D1C:0080 */
extern char gMaze[]; /*@ ds:B894 */         /* nine-byte records; +4 (ds:B898) is tested by 0D1C:0080 and +5 (ds:B899) is a list head used by 0D1C:035A */
extern signed char gWallTypeFlags[]; /*@ ds:194B */  /* byte table read by 0D1C:0080 */
extern signed char gViewToCellPos[]; /*@ ds:1A1B */  /* the cell position seen at a view position, four per gPartyDir, read by 0D1C:035A */
extern int gPartyCell; /*@ ds:FBCA */     /* the party's map cell: 0D1C:035A indexes the nine-byte
                                            records (gMaze + gPartyCell * 9 + 5) and passes it as the
                                            InsertListItem key, and 0D1C:09CB passes it as the second
                                            point of DirectionFromPoints. MoveParty (122E:0001)
                                            sets it (settled 2026-11-03, playbook 65) */
extern int gPartyDir; /*@ ds:FBCC */            /* the party's facing, 0..3 (TurnLeft, TurnRight) */
extern int gSelectFrameIndex; /*@ ds:54F2 */            /* the index drawn by 0D1C:0454, -1 when none */
extern int gSelectFrameStep; /*@ ds:54F4 */            /* the row of the animated list 0D1C:0454 draws */
extern unsigned long gTicks; /*@ ds:848A */  /* a tick counter compared with gSelectFrameTick by 0D1C:0454 */
extern unsigned long gSelectFrameTick; /*@ ds:FC5E */  /* the next tick 0D1C:0454 may draw at */
extern signed char gSelectFrameColours[]; /*@ ds:1CE7 */  /* per-row colours for the list 0D1C:0454 draws */
extern signed char gPanelFillColour; /*@ ds:F57E */    /* a colour read by 0D1C:0C21 */
extern signed char gPanelHighlight; /*@ ds:FB4C */    /* a colour read by 0D1C:0C21 */
extern signed char gPanelShadow; /*@ ds:F53A */    /* a colour read by 0D1C:0C21 */
extern signed char gConSaveBonus[]; /*@ ds:54F6 */  /* byte table indexed by a character's constitution in 0D1C:059B */
extern unsigned char gPartySpellFlags; /*@ ds:FBD2 */  /* bit 0 gates the icon colour row in 0D1C:070F (src/E07A9.C) */
extern int gGraphicsChoice; /*@ ds:8102 */            /* nonzero: a colour row exists (src/E07A9.C) */
extern char far *gItemRemapTable; /*@ ds:F534 */   /* the 1600-byte ITEMRMP buffer (src/E07A9.C) */
extern void (far *gSetRemapTable)(void far *map); /*@ ds:80D2 */ /* the far colour-table callback (src/E07A9.C) */
extern void far *gItemIcons[]; /*@ ds:DDE4 */ /* the item icon far pointers (src/E07A9.C) */

extern struct ItemInfo gItemTypesClasses[]; /*@ ds:A9F5 */

extern struct Monster gMonsters[]; /*@ ds:8A82 */

extern struct MonsterType gMonsterTypes[]; /*@ ds:1A3B */

struct Row8 {               /* eight-byte rows at ds:5482, four words: a DrawBox rectangle */
    int x1;
    int y1;
    int x2;
    int y2;
};
extern struct Row8 gSelectFrameRects[]; /*@ ds:5482 */

extern struct ItemRecord gItems[]; /*@ ds:8DD0 */

extern char gPanelMode; /*@ ds:FB44 */            /* a mode byte: 0 and 1 gate the redraws in 0D1C:01CA and 0D1C:0304 */
extern int gMouseItem; /*@ ds:FBCE */             /* the item index shown by the mouse (src/E07A9.C) */
extern int gMouseDisabled; /*@ ds:814A */    /* nonzero disables the mouse (src/E07A9.C) */
extern int gCurLevel; /*@ ds:FBC8 */             /* the dungeon level: 2 or 7 gates the turn-undead
                                                effect (0D1C:0A43); other units set it too */
extern int gDeferMonsterHits; /*@ ds:F518 */             /* 1 while several monsters are hit at once (0D1C:0A43, 08E6:16D6): 08E6:0FBE then leaves the flashes and deaths to 08E6:0E48 */
extern char gFileNameBuf[]; /*@ ds:FE6E */   /* the text buffer sprintf formats into (src/EOV2.C) */

extern void ListMonstersAtCell(int far *list, int key, int field, char flags, char scanAll, char scanFiltered); /*@ 08E6:17FD */
/* fills `list` with the gMonsters indices whose cell equals key, terminated by -1; flags, scanAll
   and scanFiltered select the scan (Purpose unknown) */
extern void FlashViewDamage(void); /*@ 0EF3:06E7 */
extern void FinishDeferredHits(void); /*@ 08E6:0E48 */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */

extern void RedrawMember(int who); /*@ 1547:0991 */
extern void DrawHandSlot(char who, char slot); /*@ 1547:07E7 */
extern void DrawItemRow(int who, int slot, int one); /*@ 0BD2:0917 */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int RemoveListItem(void far *head, int key, int item); /*@ 0BD2:02B9 */
extern int ItemClassAllowed(int who, int item); /*@ 0BD2:11FD */
extern void SetMouseItem(int n); /*@ 07A9:010C */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void RearmCharEffects(void); /*@ 1414:0B2E */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void DrawBox(int x1, int y1, int x2, int y2, int colour); /*@ 1A1F:000C */
extern int GetSavingThrowTarget(char index, char limit, char offset); /*@ 0A72:0006 */
extern char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */
extern int FindClassSlot(int slotId, int classIndex); /*@ 0A72:007B */
/* the original sign-extends the classIndex argument at 0D1C:0942 and 0D1C:0977 (`mov al,es:[bx+0x20];
   cwde; push ax`), so the second parameter is int, not char */
extern int GetTurnUndeadTarget(int col, int row); /*@ 0A72:0597 */
extern void DrawShapeRemapped(int page, void far *src, int x, int y, int region); /*@ 1A29:008B */
extern void DrawShape(int page, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int c1, int c2, int c3); /*@ 082C:09D5 */
extern int DirectionFromPoints(int fromCell, int toCell); /*@ 17A6:0314 */
extern void DamageMonster(struct Monster far *obj, int n); /*@ 08E6:0FBE */

int MemberIs(int who, int mask);
int GetCurrentItem(void);

/*@data */
/* the data of e0d1c, ds:547E to ds:5536 (184 bytes, 0 relocations): drafted by make data-skeleton from the original's bytes; each item at its annotation, in address order. */
signed char gOppositeSide[4] = { 2, 3, 0, 1 }; /*@ ds:547E */
struct Row8 gSelectFrameRects[14] = { /*@ ds:5482 */
    { 183, 1, 247, 52 }, { 255, 1, 319, 52 }, { 183, 53, 247, 104 }, { 255, 53, 319, 104 },
    { 183, 105, 247, 156 }, { 255, 105, 319, 156 }, { 16, 63, 48, 96 }, { 80, 63, 112, 96 },
    { 16, 127, 48, 160 }, { 80, 127, 112, 160 }, { 176, 66, 208, 97 }, { 208, 66, 240, 97 },
    { 240, 66, 272, 97 }, { 272, 66, 304, 97 }
};
int gSelectFrameIndex = -1; /*@ ds:54F2 */
int gSelectFrameStep = 0; /*@ ds:54F4 */
signed char gConSaveBonus[20] = { /*@ ds:54F6 */
    0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 3,
    3, 3, 4, 4, 4, 4, 5, 5
};
/* ds:550A: the colour row DrawItemIcon passes to gSetRemapTable for a colour icon when gGraphicsChoice
   is 0 (CGA), instead of the icon's ITEMRMP row: the same 4 bytes as E07A9's gCgaIconRemap (ds:0090) */
unsigned char gCgaItemRemap[4] = { 0x00, 0x55, 0x55, 0xFF }; /*@ ds:550A */
/* ds:550E-5535: the string pool, "\x06\x06%s uses the power to turn undead!\x06\x0f\r" (TurnUndead's,
   0D1C:0A43). E0DE8 reads gLauncherAmmoType at ds:552F + the item's type, a base inside this string
   (its "dead"), so the table is read from a base below its start, as ds:FB34 was; E0DE8's data settles
   where it begins, and this unit defines nothing at ds:552F. */
/* ds:5535: the pad byte to the next module (word alignment) */

/* 0D1C:0006: Returns 1 and sets gViewDirty when the cell is one of the first 15 cells the view shows
   (ds:A9C8), so a change there is redrawn. The value `value` is looked for in the 15 words at
   ds:A9C8; when
   it is found, ds:DF4C is set to 1 and the function returns 1, otherwise it returns 0. Called
   from the monster unit (08E6) and from 1414. */
/*@ 0D1C:0006 */ int IsCellInView(int value)
{
    int far *slot;
    int i;

    slot = gViewCellIds;
    for (i = 0; i < 15; i++, slot++) {
        if (*slot == value) {
            gViewDirty = 1;
            return 1;
        }
    }
    return 0;
}

/* 0D1C:003F: selects the party member at index `who` (the signed char at ds:A99A). When
   `who` differs from the current one, the old index and the new index are passed to RedrawMember
   (which redraws that member's row), and nothing is done when they are equal. gInventoryMember indexes the
   character records read by 0D1C:0670 and is toggled 0..5 by MoveHandCursor (src/E0ACD.C). Called by
   0ACD, 0DE8, 0EF3, 1414 and overlays 1, 5 and 6. */
/*@ 0D1C:003F */ void SelectCharacter(int who)
{
    int old;

    if (gInventoryMember != who) {
        old = gInventoryMember;
        gInventoryMember = who;
        RedrawMember(old);
        RedrawMember(gInventoryMember);
    }
}

/* 0D1C:006D: Does nothing; called with the cursor slot and 9 after the keyboard cursor moves.
   Increments its two word arguments (which the caller passes by
   value, so the increment has no effect). Called by 0ACD as (gCursorSlot, 9). */
/*@ 0D1C:006D */ void NoOpAfterCursorMove(int first, int second)
{
    first++;
    second++;
}

/* 0D1C:0078: Does nothing; called with the cursor slot before the keyboard cursor moves. Increments
   its word argument (passed by value, so the increment
   has no effect). Called by 0ACD as (gCursorSlot). */
/*@ 0D1C:0078 */ void NoOpBeforeCursorMove(int value)
{
    value++;
}
/* 0D1C:0080: the cell one step from `cell` toward `dir`, or -1 when the party cannot go there:
   the wall of the target cell on the side facing back (gOppositeSide[dir]) must have bit 0 of
   gWallTypeFlags set (passable), and no monster may stand in the target (gMaze + 4, bits 0..2).
   The movement keys (0ACD:04F8 to 05FF) call it with gPartyCell before MoveParty. */
/*@ 0D1C:0080 */ int StepIfOpen(int cell, signed char dir)
{
    int next;
    int flag;

    next = StepCell(cell, dir);
    flag = gWallTypeFlags[gMaze[next * 9 + gOppositeSide[dir]]];
    if (!(flag & 1))
        return -1;
    if (gMaze[next * 9 + 4] & 7)
        return -1;
    return next;
}

/* 0D1C:00DA: the map cell one step from `cell` toward `dir` (0..3 north, east, south, west):
   cell plus gDirCellOffset[dir] (-32, 1, 32, -1) kept in the 32 x 32 maze by & 0x3FF, so a step
   off one edge wraps to the other. Called by StepIfOpen, the item and attack code and 0D1C:0A43. */
/*@ 0D1C:00DA */ int StepCell(int cell, int dir)
{
    return (cell + gDirCellOffset[dir]) & 0x3FF;
}
/* 0D1C:00F1: finds the first of the 27 word slots of gChars[who].items that holds an item (a
   nonzero index into gItems) whose type equals `field2` and whose value
   equals `fieldB`. A `field2` or `fieldB` of -1 is a wildcard. Returns the slot number, or -1.
   Called by 0D1C:01CA and from overlays 1 and 6. */
/*@ 0D1C:00F1 */ int FindCarriedSlot(int who, int field2, int fieldB)
{
    int far *slot;
    unsigned i;

    slot = gChars[who].items;
    for (i = 0; i < 27; slot++, i++) {
        if (*slot != 0) {
            if (gItems[*slot].type == field2 || field2 == -1) {
                if (gItems[*slot].value == fieldB || fieldB == -1)
                    return i;
            }
        }
    }
    return -1;
}
/* 0D1C:0163: Tests whether character who wears, in item slot 25 or 26, an item of type 0x2F with
   the given power value (1, 2 or 3 by the callers); EOV1.C:678 reads value 1 as a ring of wizardry
   [H]. Over the last two of the 27 word slots of gChars[who].items (25
   and 26), returns 1 when one of them holds an item (an index into gItems) whose
   type is 0x2F and whose value equals `fieldB`, otherwise 0. Slots 25 and 26 are the only two
   inspected. Called by 122E, 1414 and overlays 1 and 6. The empty-slot test is an early
   `continue`, not an `if` around the body: that is the source form the original has. The
   original reloads `item` into AX before the first multiply (0D1C:0194 `mov ax, di`), and the
   reload survives -O only when the body follows an early `continue` (the same redundant-load
   quirk the e08E6 note in `re/symbols.toml` records); with `if (item = rec->items[i]) { ... }`
   TCC elides the load (2 bytes short). */
/*@ 0D1C:0163 */ int IsWearingRing(int who, int fieldB)
{
    struct CharRec far *rec;
    int item;
    int i;

    rec = &gChars[who];
    for (i = 25; i <= 26; i++) {
        if (!(item = rec->items[i]))
            continue;
        if (gItems[item].type == 0x2F && gItems[item].value == fieldB)
            return 1;
    }
    return 0;
}
/* 0D1C:01CA: Purpose unknown: for every character `who` that passes MemberIs(who, 1), removes
   all carried items whose item record has type == `field2` and value == `fieldB`
   (FindCarriedSlot's match): clears the item's key (sets it to -1), clears the character's slot,
   and redraws. Returns 1 when at least one item was removed, otherwise 0. The redraw paths: when
   gPanelMode is 0 and the slot is one of the first two of a character before the sixth, DrawHandSlot;
   when gPanelMode is 1 and the character is the selected one, DrawItemRow; and when the mouse is
   enabled, the item in the slot GetCurrentItem names is cleared and redrawn through SetMouseItem.
   Called by overlays 5. What type, value and key mean is not established. */
/*@ 0D1C:01CA */ int RemoveCarriedItems(int field2, int fieldB)
{
    int slot;
    int who;
    register int item;
    register int found;

    found = who = 0;
    for (; who < 6; who++) {
        if (!MemberIs(who, 1))
            continue;
        while ((slot = FindCarriedSlot(who, field2, fieldB)) != -1) {
            item = gChars[who].items[slot];
            gItems[item].key = -1;
            gChars[who].items[slot] = 0;
            found = 1;
            if (gPanelMode == 0) {
                if (slot < 2 && who < 5)
                    DrawHandSlot(who, slot);
            }
            if (gPanelMode == 1 && gInventoryMember == who)
                DrawItemRow(who, slot, 1);
            if (!gMouseDisabled) {
                item = GetCurrentItem();
                if (gItems[item].type == field2) {
                    gItems[item].key = -1;
                    SetMouseItem(0);
                }
            }
        }
    }
    return found;
}
/* 0D1C:02DC: returns the item index of the currently displayed slot: gMouseItem when the mouse is
   enabled, otherwise gChars[gInventoryMember].items[gCursorSlot]. The getter of the SetCurrentItem pair (0D1C:0304).
   Called by 0D1C:01CA and from many other units. */
/*@ 0D1C:02DC */ int GetCurrentItem(void)
{
    if (!gMouseDisabled)
        return gMouseItem;
    return gChars[gInventoryMember].items[gCursorSlot];
}
/* 0D1C:0304: stores `n` as the item index in the current slot: gMouseItem when the mouse is enabled,
   or gChars[gInventoryMember].items[gCursorSlot] when it is disabled. When the mouse is disabled it also redraws
   the slot, through RedrawMember(gInventoryMember) when gPanelMode is 0 and DrawItemRow(gCursorSlot, 1, 0) otherwise.
   Called by SetMouseItem to set the icon shown for a selected item. */
/*@ 0D1C:0304 */ void SetCurrentItem(int n)
{
    if (!gMouseDisabled) {
        gMouseItem = n;
    } else {
        gChars[gInventoryMember].items[gCursorSlot] = n;
        if (gPanelMode == 0)
            RedrawMember(gInventoryMember);
        else
            DrawItemRow(gCursorSlot, 1, 0);
    }
}
/* 0D1C:035A: Removes party member `who`: clears its status, drops all its items onto the party's
   cell and fixes up the selected inventory member. Clears gChars[who].status and then, for each of
   the 27 word slots of
   gChars[who].items except slot 16 (which is a list head at +97), queues the item in the slot
   through InsertListItem at the head gPartyCell * 9 + 5 (the nine-byte records at ds:B894) with a
   random front cell position gViewToCellPos[gPartyDir * 4 + RollDice(1, 2, -1)]. Then it drains the slot-16 list through
   RemoveListItem and queues each returned item the same way. Finally, when `who` is the selected
   character, gInventoryMember is cleared to 0, and RearmCharEffects runs. Called by overlays 1 and 5. */
/*@ 0D1C:035A */ void RemovePartyMember(int who)
{
    struct CharRec far *rec;
    int item;
    int i;

    rec = &gChars[who];
    rec->status = 0;
    for (i = 0; i < 27; i++) {
        if (i == 16)
            continue;
        if (item = rec->items[i]) {
            InsertListItem((int far *)(gMaze + gPartyCell * 9 + 5), gPartyCell, item,
                    gViewToCellPos[gPartyDir * 4 + RollDice(1, 2, -1)]);
        }
    }
    while (rec->items[16]) {
        item = RemoveListItem(&rec->items[16], 0, -1);
        InsertListItem((int far *)(gMaze + gPartyCell * 9 + 5), gPartyCell, item,
                gViewToCellPos[gPartyDir * 4 + RollDice(1, 2, -1)]);
    }
    if (gInventoryMember == who)
        gInventoryMember = 0;
    RearmCharEffects();
}
/* 0D1C:0454: draws the animated row list at ds:5482. When `index` differs from the stored
   gSelectFrameIndex
   it hides the pointer, erases the old row (DrawBox over gSelectFrameRects[gSelectFrameIndex] in colour 0xC), shows
   the pointer, resets the row counter gSelectFrameStep to 0, stores `index` in gSelectFrameIndex and records the
   current tick (gSelectFrameTick = gTicks). Otherwise, when a row is selected and one tick has passed, it
   draws the next row gSelectFrameColours[gSelectFrameStep] in the row's colour, advances gSelectFrameStep (wrapping to 0 at a zero
   colour) and sets gSelectFrameTick = gTicks + 1. Called by overlays 0 and 1. */
/*@ 0D1C:0454 */ void DrawRowList(int index)
{
    int x1;
    int y1;
    int x2;
    int y2;

    if (gSelectFrameIndex != index) {
        if (gSelectFrameIndex != -1) {
            LockHidePointer();
            DrawBox(gSelectFrameRects[gSelectFrameIndex].x1, gSelectFrameRects[gSelectFrameIndex].y1, gSelectFrameRects[gSelectFrameIndex].x2,
                    gSelectFrameRects[gSelectFrameIndex].y2, 0xC);
            LockShowPointer();
        }
        gSelectFrameStep = 0;
        gSelectFrameIndex = index;
        gSelectFrameTick = gTicks;
    } else {
        if (gSelectFrameIndex != -1) {
            if (gTicks > gSelectFrameTick) {
                if (gSelectFrameColours[gSelectFrameStep] == 0)
                    gSelectFrameStep = 0;
                x1 = gSelectFrameRects[gSelectFrameIndex].x1;
                y1 = gSelectFrameRects[gSelectFrameIndex].y1;
                x2 = gSelectFrameRects[gSelectFrameIndex].x2;
                y2 = gSelectFrameRects[gSelectFrameIndex].y2;
                LockHidePointer();
                DrawBox(x1, y1, x2, y2, gSelectFrameColours[gSelectFrameStep]);
                LockShowPointer();
                gSelectFrameStep++;
                gSelectFrameTick = gTicks + 1;
            }
        }
    }
}
/* 0D1C:0583: calls RedrawMember (which redraws one character's row) for each of the six party
   members. Called by 0ACD, 0EF3 and overlays 1, 3 and 5 after a change that affects the whole
   party. */
/*@ 0D1C:0583 */ void RedrawCharacters(void)
{
    int i;

    for (i = 0; i < 6; i++)
        RedrawMember(i);
}
/* 0D1C:059B: Rolls a character's saving throw of the given kind and returns 1 when it succeeds. A
   per-character check against a 1d20 roll. It derives a class
   slot from gChars[who].classIndex (GetClassSlotByte, with 4 and 5 folded to 0), a value from
   GetSavingThrowTarget(classSlot, levels, `attack`), then, depending on a level-like value race * 2
   (3 or 5, and separately 4), for `attack` 4, 0 or 1 in the 3-or-5 case and for `attack` 4 or 1
   in the level-4 case, subtracts gConSaveBonus[constitution]; finally it returns 1 when RollDice(1, 20, 0) >=
   value. QUIRK 0D1C:0609/0604: race * 2 is even, so the tests for 3
   and 5 can never hold; the original still has them. Called by 0DE8:0F98 and 0DE8:101A. */
/*@ 0D1C:059B */ int RollMemberSave(int who, int attack)
{
    struct CharRec far *rec;
    int classSlot;
    int value;
    int level;

    rec = &gChars[who];
    classSlot = GetClassSlotByte(rec->classIndex, 0);
    if (classSlot == 4 || classSlot == 5)
        classSlot = 0;
    value = GetSavingThrowTarget(classSlot, rec->levels[0], attack);
    level = rec->race * 2;
    if (level == 3 || level == 5) {
        if (attack == 4 || attack == 0 || attack == 1)
            value -= gConSaveBonus[rec->constitution];
    }
    if (level == 4) {
        if (attack == 4 || attack == 1)
            value -= gConSaveBonus[rec->constitution];
    }
    return RollDice(1, 20, 0) >= value;
}
/* 0D1C:0670: whether party member `who` meets every condition `mask` selects (1 when it does,
   the AND of the selected tests). The bits: 1 the slot holds a character (status bit 0);
   2 hp above -10 (not DEAD); 4 hp above 0 (not UNCONSCIOUS); 8 not paralyzed (status bit 2);
   16 not poisoned (status bit 1). The status bits are those the status line of 1547 draws (POISONED,
   PARALYZED) and the effect ends of 1414 clear. Callers most often pass 1 (is there a character);
   0D1C:01CA and others combine them. */
/*@ 0D1C:0670 */ int MemberIs(int who, int mask)
{
    struct CharRec far *rec;
    int result;

    result = 1;
    rec = &gChars[who];
    if (mask & 1)
        result &= rec->status & 1;
    if (mask & 2)
        result &= rec->hp > -10;
    if (mask & 4)
        result &= rec->hp > 0;
    if (mask & 8)
        result &= (rec->status & 4) == 0;
    if (mask & 16)
        result &= (rec->status & 2) == 0;
    return result;
}
/* 0D1C:070F: draws the item icon of item record `n` (gItems) at (x, y), the far pointer
   gItemIcons[icon]. Colour-row path, Purpose unknown: when gPartySpellFlags bit 0 is set and the record's
   flags have 0x80, it first selects the icon's 16-byte colour row (gItemRemapTable + (icon << 4), or the
   4-byte table gCgaItemRemap at ds:550A when gGraphicsChoice is 0) through gSetRemapTable and draws through DrawShapeRemapped,
   otherwise through DrawShape. Called by 0BD2 (three sites) and 1547:07E7. */
/*@ 0D1C:070F */ void DrawItemIcon(int mode, int n, int x, int y)
{
    int icon;

    icon = gItems[n].icon;
    if ((gPartySpellFlags & 1) && (gItems[n].flags & 0x80)) {
        if (gGraphicsChoice)
            gSetRemapTable(gItemRemapTable + (icon << 4));
        else
            gSetRemapTable(gCgaItemRemap);
        DrawShapeRemapped(mode, gItemIcons[icon], x, y, 0);
    } else {
        DrawShape(mode, gItemIcons[icon], x, y, 0);
    }
}
/* 0D1C:07A7: advances the far string pointer `text` past `count` NUL-terminated strings (each
   step skips to and past the next zero byte) and returns the pointer just past the last one.
   Called by 0BD2, overlay 1, overlay 2 and overlay 6. */
/*@ 0D1C:07A7 */ char far *SkipStrings(char far *text, int count)
{
    char far *cursor;
    int i;

    i = count;
    cursor = text;
    for (; i; i--) {
        while (*cursor++)
            ;
    }
    return cursor;
}
/* 0D1C:07DB: Returns whether the member can use hand 0 or hand 1 with the items held (class and
   two-handed rules). Combines the first two carried items of gChars[who] (slots 0 and
   1). Each slot's item is passed to ItemClassAllowed and its item record's type indexes the
   16-byte records at ds:A9F5. With `flag` 0 it returns 1 when slot 0 is empty or ItemClassAllowed
   accepted its item. Otherwise it returns 0 when slot 0 holds an item whose gItemTypesClasses[type].m01 is 2, then
   1 when slot 1 is empty, and for a slot-1 item whose gItemTypesClasses[m1].m09 (low 7 bits) is 1, 2 or 3,
   0 when gItemTypesClasses[m1].m01 is nonzero, else the slot-1 result. Called by 0BD2:1001, 0DE8:0D5E and
   1547:07E7. Placeholder: what the values mean is not established. */
/*@ 0D1C:07DB */ int CanUseHand(int who, int flag)
{
    int slot0;
    int result0;
    int slot1;
    int result1;
    int m0;
    int m1;

    slot0 = gChars[who].items[0];
    result0 = ItemClassAllowed(who, slot0);
    m0 = gItems[slot0].type;
    if (flag == 0)
        return slot0 == 0 || result0 != 0;
    slot1 = gChars[who].items[1];
    result1 = ItemClassAllowed(who, slot1);
    m1 = gItems[slot1].type;
    if (slot0 != 0 && gItemTypesClasses[m0].m01 == 2)
        return 0;
    if (slot1 == 0)
        return 1;
    if ((gItemTypesClasses[m1].m09 & 0x7f) > 0 && (gItemTypesClasses[m1].m09 & 0x7f) < 4) {
        if (gItemTypesClasses[m1].m01 == 0)
            return result1;
        return 0;
    }
    return result1;
}
/* 0D1C:08B1: Tests whether a character holds an item of kind 6 in either hand, the item turning
   undead requires. Returns 1 when one of the first two carried items of the character
   record `rec` (items[0] or items[1]) holds a nonzero item whose item record's type indexes a
   ds:A9F5 record with m09 == 6, otherwise 0. The pointer is a character record (the slots are at
   +77). Called by 0D1C:0903. */
/*@ 0D1C:08B1 */ int HoldsHolySymbol(struct CharRec far *rec)
{
    int item;
    int i;

    for (i = 0; i <= 1; i++) {
        if (rec->items[i]) {
            item = gItems[rec->items[i]].type;
            if (gItemTypesClasses[item].m09 == 6)
                return 1;
        }
    }
    return 0;
}
/* 0D1C:0903: Picks the party member with the best turning level (class slot 2 level, or slot 4
   level minus 2) and returns its index and level packed. Scans the six party members and returns a
   packed word. A member
   qualifies when MemberIs(who, 0xD) and HoldsHolySymbol(rec) hold; for it the code takes
   FindClassSlot(2, classIndex) and, failing that, FindClassSlot(4, classIndex), and compares the member's
   levels[slot] (minus 2 in the second case) with the best so far. The result is (best member << 8)
   + best value, or 0 when none qualifies. Called by 0D1C:0A43. */
/*@ 0D1C:0903 */ int FindBestUndeadTurner(void)
{
    struct CharRec far *rec;
    int bestVal;
    int best;
    int who;
    int slot;

    bestVal = best = 0;
    rec = gChars;
    for (who = 0; who < 6; who++, rec++) {
        if (!MemberIs(who, 0xD))
            continue;
        if (!HoldsHolySymbol(rec))
            continue;
        slot = FindClassSlot(2, rec->classIndex);
        if (slot != -1) {
            if (rec->levels[slot] > bestVal) {
                bestVal = rec->levels[slot];
                best = who;
            }
        } else {
            slot = FindClassSlot(4, rec->classIndex);
            if (slot == -1)
                continue;
            if (rec->levels[slot] - 2 > bestVal) {
                bestVal = rec->levels[slot] - 2;
                best = who;
            }
        }
    }
    return (best << 8) + bestVal;
}
/* 0D1C:09CB: Applies one turn-undead roll to a monster: destroy, turn away or no effect. A
   per-monster check. It takes the monster's ds:1A3B record's m19 as
   a row and GetTurnUndeadTarget(column, m19) as a value. A value of -1 calls DamageMonster(obj, 100)
   and returns 1. Otherwise, when `threshold` is below the value it returns 0; else it sets bit 3
   of obj->flags, stores (DirectionFromPoints(obj->cell, gPartyCell) ^ 4) >> 1 in obj->orient and returns 1.
   Called by 0D1C:0A43 and 17A6. */
/*@ 0D1C:09CB */ int TryTurnMonster(struct Monster far *obj, int threshold, int column)
{
    int row;
    int value;

    row = gMonsterTypes[obj->typeIndex].m19;
    value = GetTurnUndeadTarget(column, row);
    if (value != -1) {
        if (threshold >= value) {
            obj->flags |= 8;
            obj->orient = (DirectionFromPoints(obj->cell, gPartyCell) ^ 4) >> 1;
            return 1;
        }
    } else {
        DamageMonster(obj, 100);
        return 1;
    }
    return 0;
}
/* 0D1C:0A43: the "turn undead" action. When the dungeon level gCurLevel is 2 or 7 it derives a ten-bit key from
   gPartyCell and gPartyDir (StepCell) and requires gMaze[key * 9 + 4] & 7 to be nonzero, then
   asks FindBestUndeadTurner for the best qualifying party member (its low byte the member's value, its
   high byte the member's index) and rolls 1d20. It lists the gMonsters indices whose ds:8A82 monster
   record's cell equals key (08E6:17FD) and, for each monster whose ds:1A3B record's m0F has bit 2
   set and whose flags has bit 4 clear, sets gDeferMonsterHits, marks the monster (flags |= 0x10) and runs
   TryTurnMonster(obj, roll, member value), ORing the results. When any monster turned it formats "%s
   uses the power to turn undead!" (ds:550E) with the best member's name, draws it, plays sound
   0x5F and runs 0EF3:06E7 and 08E6:0E48. gDeferMonsterHits is cleared at the end on the normal path. Called
   by 0ACD:0E8A. */
/*@ 0D1C:0A43 */ void TurnUndead(void)
{
    struct Monster far *obj;
    int key;
    int value;
    int list[5];
    int roll;
    int turned;
    int memberVal;
    int member;
    unsigned packed;
    int i;

    if (gCurLevel != 2 && gCurLevel != 7)
        return;
    turned = 0;
    key = StepCell(gPartyCell, gPartyDir);
    value = (unsigned char)gMaze[key * 9 + 4] & 7;
    if (value == 0)
        return;
    packed = FindBestUndeadTurner();
    if (packed == 0)
        return;
    memberVal = packed & 0xff;
    member = (packed & 0xff00) >> 8;
    ListMonstersAtCell(list, key, 4, 4, 1, 1);
    roll = RollDice(1, 20, 0);
    for (i = 0; i < 5; i++) {
        if (list[i] == -1)
            break;
        obj = &gMonsters[list[i]];
        if ((gMonsterTypes[obj->typeIndex].m0F & 4) && !(obj->flags & 0x10)) {
            gDeferMonsterHits = 1;
            obj->flags |= 0x10;
            turned |= TryTurnMonster(obj, roll, memberVal);
        }
    }
    if (turned) {
        sprintf(gFileNameBuf, "\x06\x06%s uses the power to turn undead!\x06\x0f\r",
                gChars[member].name);
        DrawText(gFileNameBuf);
        PlaySound(0x5F);
        FlashViewDamage();
        FinishDeferredHits();
    }
    gDeferMonsterHits = 0;
}
/* 0D1C:0BAB: Rolls percentile dice against the monster type's resistance byte (+0x1A) and returns 1
   when the spell or spell item is resisted. A percentage check on the monster `objIndex` (its
   ds:8A82 record,
   not a pointer like 0D1C:09CB's `obj`): through gMonsters[objIndex].typeIndex it reads gMonsterTypes[typeIndex].m1A
   and returns 1 when RollDice(1, 100, 0) is below it, otherwise 0. Called by 0DE8:0359. */
/*@ 0D1C:0BAB */ int MonsterResistsMagic(int objIndex)
{
    int chance;

    chance = gMonsterTypes[gMonsters[objIndex].typeIndex].m1A;
    return RollDice(1, 100, 0) < chance;
}
/* 0D1C:0BEB: Tests whether a monster type takes half damage from the given weapon item type.
   Returns 1 when gMonsterTypes[gMonsters[obj].typeIndex].m0E has bit 2 set and
   gItemTypesClasses[item].m00 has bit 2 clear, otherwise 0. `item` indexes the ItemInfo records. Called by
   0DE8:0359. */
/*@ 0D1C:0BEB */ int IsWeaponDamageHalved(int obj, int item)
{
    return (gMonsterTypes[gMonsters[obj].typeIndex].m0E & 4) && (gItemTypesClasses[item].m00 & 4) == 0;
}
/* 0D1C:0C21: Draws a double-bevelled frame (x, y, width, height) for a menu box or button, flat on
   the outside when `first` is set and without interior fill when `second` is set. Draws a framed
   rectangle at (x, y) `width` by `height` through
   082C:09D5. It hides the pointer, draws the whole rectangle in the colours picked by `first`
   (gPanelFillColour, gPanelFillColour when `first`, else gPanelHighlight and gPanelShadow) with -1 as the third colour, then shrinks the
   rectangle by one on each side and draws the inner border in gPanelHighlight/gPanelShadow with a third colour of
   -1 when `second`, else gPanelFillColour, and shows the pointer. Called by overlay 1 and overlay 6. */
/*@ 0D1C:0C21 */ void DrawFramedPanel(int x, int y, int width, int height, int first, int second)
{
    int x1;
    int y1;
    int x2;
    int y2;
    int c1;
    int c2;

    LockHidePointer();
    x1 = x;
    y1 = y;
    x2 = x + width - 1;
    y2 = y + height - 1;
    if (first) {
        c2 = gPanelFillColour;
        c1 = gPanelFillColour;
    } else {
        c1 = gPanelHighlight;
        c2 = gPanelShadow;
    }
    DrawFilledPanel(x1, y1, x2, y2, c1, c2, -1);
    x1++;
    y1++;
    x2--;
    y2--;
    DrawFilledPanel(x1, y1, x2, y2, gPanelHighlight, gPanelShadow, second ? -1 : gPanelFillColour);
    LockShowPointer();
}
