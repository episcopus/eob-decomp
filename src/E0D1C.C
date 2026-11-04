/* Unit at EOB.EXE 0D1C:0006 (3265 bytes, 26 functions): the party-status and character-effect
   module. It walks the six 243-byte character records at ds:F580 (struct CharRec, src/E0A72.C),
   the 28-byte object records at ds:8A82 (struct Monster, src/E08E6.C) and a 15-word table at
   ds:A9C8, and it draws the party rows through the rectangle and region front ends. Several
   helpers are pulled in from the character unit (0A72), the object unit (08E6) and the resident
   drawing code. Most meanings are not established, so most function names say what a function
   does rather than what the game calls it. The unit is compiled with -v, the debug information
   that keeps identical code tails apart while TCC still sizes the jumps (whitepaper 2.1.1). */

/*@flags -ml -O -Z -v */

struct CharRec {            /* 243-byte (0xF3) character records at ds:F580; see src/E0A72.C */
    char pad00;
    unsigned char status;      /* +01 */
    char name[0x0B];           /* +02: the character's name, NUL padded (src/EOV6.C) */
    char pad0D[0x17 - 0x0D];
    signed char constitution;  /* +17 */
    char pad18[0x1B - 0x18];
    signed char hp;            /* +1B */
    char pad1C[0x1F - 0x1C];
    signed char race;          /* +1F */
    signed char classIndex;    /* +20 */
    char pad21[3];
    signed char levels[3];     /* +24: three per-class-slot bytes (0D1C:0903 indexes it, 0D1C:059B reads [0]) */
    char pad27[0x77 - 0x27];
    int items[27];             /* +77: 27 word slots; slot 16 (+97) is also read on its own */
    char padAD[0xF3 - 0xAD];
};

extern int gA9C8[]; /*@ ds:A9C8 */       /* a 15-word table searched by 0D1C:0006 */
extern char gDF4C; /*@ ds:DF4C */        /* set to 1 when 0D1C:0006 finds the value */
extern signed char gInventoryMember; /*@ ds:A99A */ /* the selected character index (src/E0ACD.C) */
extern char gF51A; /*@ ds:F51A */        /* an index into the 27 word slots of gChars[gInventoryMember].items
                                            (0D1C:02DC, 0D1C:0304); slot 16 is the own-list head
                                            0D1C:035A drains. E0ACD disagrees: it reads it as an
                                            action-menu row 0..2 with 0x10 for the own list */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern signed char gDirCellOffset[]; /*@ ds:1790 */  /* byte table indexed by the second argument of 0D1C:00DA */
extern signed char gOppositeSide[]; /*@ ds:547E */  /* byte table indexed by the second argument of 0D1C:0080 */
extern char gMaze[]; /*@ ds:B894 */         /* nine-byte records; +4 (ds:B898) is tested by 0D1C:0080 and +5 (ds:B899) is a list head used by 0D1C:035A */
extern signed char gWallTypeFlags[]; /*@ ds:194B */  /* byte table read by 0D1C:0080 */
extern signed char g1A1B[]; /*@ ds:1A1B */  /* per-page sound table read by 0D1C:035A */
extern int gPartyCell; /*@ ds:FBCA */     /* the party's map cell: 0D1C:035A indexes the nine-byte
                                            records (gMaze + gPartyCell * 9 + 5) and passes it as the
                                            InsertListItem key, and 0D1C:09CB passes it as the second
                                            point of DirectionFromPoints. MoveParty (122E:0001)
                                            sets it (settled 2026-11-03, playbook 65) */
extern int gPartyDir; /*@ ds:FBCC */            /* the party's facing, 0..3 (TurnLeft, TurnRight) */
extern int g54F2; /*@ ds:54F2 */            /* the index drawn by 0D1C:0454, -1 when none */
extern int g54F4; /*@ ds:54F4 */            /* the row of the animated list 0D1C:0454 draws */
extern unsigned long gTicks; /*@ ds:848A */  /* a tick counter compared with gFC5E by 0D1C:0454 */
extern unsigned long gFC5E; /*@ ds:FC5E */  /* the next tick 0D1C:0454 may draw at */
extern signed char g1CE7[]; /*@ ds:1CE7 */  /* per-row colours for the list 0D1C:0454 draws */
extern signed char gF57E; /*@ ds:F57E */    /* a colour read by 0D1C:0C21 */
extern signed char gFB4C; /*@ ds:FB4C */    /* a colour read by 0D1C:0C21 */
extern signed char gF53A; /*@ ds:F53A */    /* a colour read by 0D1C:0C21 */
extern signed char g54F6[]; /*@ ds:54F6 */  /* byte table indexed by a character's constitution in 0D1C:059B */
extern unsigned char gFBD2; /*@ ds:FBD2 */  /* bit 0 gates the icon colour row in 0D1C:070F (src/E07A9.C) */
extern int gGraphicsChoice; /*@ ds:8102 */            /* nonzero: a colour row exists (src/E07A9.C) */
extern char far *gBufF534; /*@ ds:F534 */   /* the 1600-byte ITEMRMP buffer (src/E07A9.C) */
extern void (far *gFn80D2)(void far *map); /*@ ds:80D2 */ /* the far colour-table callback (src/E07A9.C) */
extern void far *gIconsDDE4[]; /*@ ds:DDE4 */ /* the item icon far pointers (src/E07A9.C) */

struct ItemInfo {           /* 16-byte records at ds:A9F5; read by 0D1C:07DB, 0D1C:08B1 and 0D1C:0BEB */
    signed char m00;        /* +0 (ds:A9F5), bit 2 tested by 0D1C:0BEB */
    signed char m01;        /* +1 (ds:A9F6), compared by 0D1C:07DB */
    char pad02[7];
    int m09;                /* +9 (ds:A9FE), tested by 0D1C:07DB */
    char pad0B[5];
};
extern struct ItemInfo gA9F5[]; /*@ ds:A9F5 */

struct Monster {              /* 28-byte object records at ds:8A82 (see src/E08E6.C) */
    char typeIndex;                /* +0: an index into the ds:1A3B table */
    char pad01;
    int cell;                 /* +2: the object's map cell, read by 0D1C:09CB */
    char pad04;
    char orient;            /* +5: the orientation quarter (times 2 is a direction), written by 0D1C:09CB */
    char pad06[0x16 - 0x06];
    char m16;               /* +0x16, bit 3 set by 0D1C:09CB */
    char pad17[0x1C - 0x17];
};
extern struct Monster gMonsters[]; /*@ ds:8A82 */

struct MonsterType {            /* 27-byte records at ds:1A3B, indexed by gMonsters[obj].typeIndex */
    char pad00[0x0E];
    signed char m0E;        /* +0xE (ds:1A49), bit 2 tested by 0D1C:0BEB */
    long m0F;               /* +0xF (ds:1A4A), tested by 0D1C:0A43; a `long` because the original
                               emits the and/or bit-test form at 0D1C:0B17 */
    char pad13[0x19 - 0x13];
    signed char m19;        /* +0x19 (ds:1A54), read by 0D1C:09CB */
    signed char m1A;        /* +0x1A (ds:1A55), read by 0D1C:0BAB */
};
extern struct MonsterType gMonsterTypes[]; /*@ ds:1A3B */

struct Row8 {               /* eight-byte rows at ds:5482, four words: a DrawBox rectangle */
    int x1;
    int y1;
    int x2;
    int y2;
};
extern struct Row8 gRows5482[]; /*@ ds:5482 */

struct Entry14 {            /* the 14-byte records E07A9 calls Entry14; view based at ds:8DD2 (E07A9) */
    unsigned char flags;    /* +0 (ds:8DD2), read by 07A9:015C */
    signed char icon;       /* +1 (ds:8DD3), read by 07A9:0141 and 0D1C:070F */
    signed char type;        /* +2 (ds:8DD4), compared by 0D1C:00F1 and 0D1C:01CA */
    char pad03;
    int key;                /* +4 (ds:8DD6), set to -1 by 0D1C:01CA */
    char pad06[0x0B - 0x06];
    signed char value;        /* +0xB (ds:8DDD), compared by 0D1C:00F1 */
    char pad0C[0x0E - 0x0C];
};
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */

extern char gPanelMode; /*@ ds:FB44 */            /* a mode byte: 0 and 1 gate the redraws in 0D1C:01CA and 0D1C:0304 */
extern int gFBCE; /*@ ds:FBCE */             /* the item index shown by the mouse (src/E07A9.C) */
extern int gMouseDisabled; /*@ ds:814A */    /* nonzero disables the mouse (src/E07A9.C) */
extern int gFBC8; /*@ ds:FBC8 */             /* the current action: 2 or 7 gates the turn-undead
                                                effect (0D1C:0A43); other units set it too */
extern int gF518; /*@ ds:F518 */             /* set to 1 while an object is being turned (0D1C:0A43) */
extern char gFileNameBuf[]; /*@ ds:FE6E */   /* the text buffer sprintf formats into (src/EOV2.C) */

extern void f08E6_17FD(int far *list, int key, int field, char flags, char scanAll, char scanFiltered); /*@ 08E6:17FD */
/* fills `list` with the gMonsters indices whose id equals key, terminated by -1; flags, scanAll
   and scanFiltered select the scan (Purpose unknown) */
extern void FlashViewDamage(void); /*@ 0EF3:06E7 */
extern void f08E6_0E48(void); /*@ 08E6:0E48 */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */

extern void RedrawMember(int who); /*@ 1547:0991 */
extern void DrawMemberSlot(char who, char slot); /*@ 1547:07E7 */
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
extern int GetIndexedByte(char index, char limit, char offset); /*@ 0A72:0006 */
extern char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */
extern int FindClassSlot(int slotId, int classIndex); /*@ 0A72:007B */
/* the original sign-extends the classIndex argument at 0D1C:0942 and 0D1C:0977 (`mov al,es:[bx+0x20];
   cwde; push ax`), so the second parameter is int, not char */
extern int GetClampedTableByte(int col, int row); /*@ 0A72:0597 */
extern void f1A29_008B(int mode, void far *src, int x, int y, int region); /*@ 1A29:008B */
extern void DrawShape(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int c1, int c2, int c3); /*@ 082C:09D5 */
extern int DirectionFromPoints(int fromCell, int toCell); /*@ 17A6:0314 */
extern void DamageObject(struct Monster far *obj, int n); /*@ 08E6:0FBE */

int MemberIs(int who, int mask);
int GetCurrentItem(void);

/* 0D1C:0006: Purpose unknown: the value `value` is looked for in the 15 words at ds:A9C8; when
   it is found, ds:DF4C is set to 1 and the function returns 1, otherwise it returns 0. Called
   from the object unit (08E6) and from 1414. */
/*@ 0D1C:0006 */ int f0D1C_0006(int value)
{
    int far *slot;
    int i;

    slot = gA9C8;
    for (i = 0; i < 15; i++, slot++) {
        if (*slot == value) {
            gDF4C = 1;
            return 1;
        }
    }
    return 0;
}

/* 0D1C:003F: selects the party member at index `who` (the signed char at ds:A99A). When
   `who` differs from the current one, the old index and the new index are passed to RedrawMember
   (which redraws that member's row), and nothing is done when they are equal. gInventoryMember indexes the
   character records read by 0D1C:0670 and is toggled 0..5 by Turn091C (src/E0ACD.C). Called by
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

/* 0D1C:006D: Purpose unknown: increments its two word arguments (which the caller passes by
   value, so the increment has no effect). Called by 0ACD as (gF51A, 9). */
/*@ 0D1C:006D */ void f0D1C_006D(int first, int second)
{
    first++;
    second++;
}

/* 0D1C:0078: Purpose unknown: increments its word argument (passed by value, so the increment
   has no effect). Called by 0ACD as (gF51A). */
/*@ 0D1C:0078 */ void f0D1C_0078(int value)
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
   nonzero index into the 14-byte records at ds:8DD2) whose type equals `field2` and whose value
   equals `fieldB`. A `field2` or `fieldB` of -1 is a wildcard. Returns the slot number, or -1.
   Called by 0D1C:01CA and from overlays 1 and 6. */
/*@ 0D1C:00F1 */ int FindCarriedSlot(int who, int field2, int fieldB)
{
    int far *slot;
    unsigned i;

    slot = gChars[who].items;
    for (i = 0; i < 27; slot++, i++) {
        if (*slot != 0) {
            if (gTable8DD2[*slot].type == field2 || field2 == -1) {
                if (gTable8DD2[*slot].value == fieldB || fieldB == -1)
                    return i;
            }
        }
    }
    return -1;
}
/* 0D1C:0163: Purpose unknown: over the last two of the 27 word slots of gChars[who].items (25
   and 26), returns 1 when one of them holds an item (an index into the ds:8DD2 records) whose
   type is 0x2F and whose value equals `fieldB`, otherwise 0. Slots 25 and 26 are the only two
   inspected. Called by 122E, 1414 and overlays 1 and 6. The empty-slot test is an early
   `continue`, not an `if` around the body: that is the source form the original has. The
   original reloads `item` into AX before the first multiply (0D1C:0194 `mov ax, di`), and the
   reload survives -O only when the body follows an early `continue` (the same redundant-load
   quirk the e08E6 note in `re/symbols.toml` records); with `if (item = rec->items[i]) { ... }`
   TCC elides the load (2 bytes short). */
/*@ 0D1C:0163 */ int f0D1C_0163(int who, int fieldB)
{
    struct CharRec far *rec;
    int item;
    int i;

    rec = &gChars[who];
    for (i = 25; i <= 26; i++) {
        if (!(item = rec->items[i]))
            continue;
        if (gTable8DD2[item].type == 0x2F && gTable8DD2[item].value == fieldB)
            return 1;
    }
    return 0;
}
/* 0D1C:01CA: Purpose unknown: for every character `who` that passes MemberIs(who, 1), removes
   all carried items whose ds:8DD2 record has type == `field2` and value == `fieldB`
   (FindCarriedSlot's match): clears the item's key (sets it to -1), clears the character's slot,
   and redraws. Returns 1 when at least one item was removed, otherwise 0. The redraw paths: when
   gPanelMode is 0 and the slot is one of the first two of a character before the sixth, DrawMemberSlot;
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
            gTable8DD2[item].key = -1;
            gChars[who].items[slot] = 0;
            found = 1;
            if (gPanelMode == 0) {
                if (slot < 2 && who < 5)
                    DrawMemberSlot(who, slot);
            }
            if (gPanelMode == 1 && gInventoryMember == who)
                DrawItemRow(who, slot, 1);
            if (!gMouseDisabled) {
                item = GetCurrentItem();
                if (gTable8DD2[item].type == field2) {
                    gTable8DD2[item].key = -1;
                    SetMouseItem(0);
                }
            }
        }
    }
    return found;
}
/* 0D1C:02DC: returns the item index of the currently displayed slot: gFBCE when the mouse is
   enabled, otherwise gChars[gInventoryMember].items[gF51A]. The getter of the SetCurrentItem pair (0D1C:0304).
   Called by 0D1C:01CA and from many other units. */
/*@ 0D1C:02DC */ int GetCurrentItem(void)
{
    if (!gMouseDisabled)
        return gFBCE;
    return gChars[gInventoryMember].items[gF51A];
}
/* 0D1C:0304: stores `n` as the item index in the current slot: gFBCE when the mouse is enabled,
   or gChars[gInventoryMember].items[gF51A] when it is disabled. When the mouse is disabled it also redraws
   the slot, through RedrawMember(gInventoryMember) when gPanelMode is 0 and DrawItemRow(gF51A, 1, 0) otherwise.
   Called by SetMouseItem to set the icon shown for a selected item. */
/*@ 0D1C:0304 */ void SetCurrentItem(int n)
{
    if (!gMouseDisabled) {
        gFBCE = n;
    } else {
        gChars[gInventoryMember].items[gF51A] = n;
        if (gPanelMode == 0)
            RedrawMember(gInventoryMember);
        else
            DrawItemRow(gF51A, 1, 0);
    }
}
/* 0D1C:035A: Purpose unknown: clears gChars[who].status and then, for each of the 27 word slots of
   gChars[who].items except slot 16 (which is a list head at +97), queues the item in the slot
   through InsertListItem at the head gPartyCell * 9 + 5 (the nine-byte records at ds:B894) with a
   random sound g1A1B[gPartyDir * 4 + RollDice(1, 2, -1)]. Then it drains the slot-16 list through
   RemoveListItem and queues each returned item the same way. Finally, when `who` is the selected
   character, gInventoryMember is cleared to 0, and RearmCharEffects runs. Called by overlays 1 and 5. */
/*@ 0D1C:035A */ void f0D1C_035A(int who)
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
                    g1A1B[gPartyDir * 4 + RollDice(1, 2, -1)]);
        }
    }
    while (rec->items[16]) {
        item = RemoveListItem(&rec->items[16], 0, -1);
        InsertListItem((int far *)(gMaze + gPartyCell * 9 + 5), gPartyCell, item,
                g1A1B[gPartyDir * 4 + RollDice(1, 2, -1)]);
    }
    if (gInventoryMember == who)
        gInventoryMember = 0;
    RearmCharEffects();
}
/* 0D1C:0454: draws the animated row list at ds:5482. When `index` differs from the stored g54F2
   it hides the pointer, erases the old row (DrawBox over gRows5482[g54F2] in colour 0xC), shows
   the pointer, resets the row counter g54F4 to 0, stores `index` in g54F2 and records the
   current tick (gFC5E = gTicks). Otherwise, when a row is selected and one tick has passed, it
   draws the next row g1CE7[g54F4] in the row's colour, advances g54F4 (wrapping to 0 at a zero
   colour) and sets gFC5E = gTicks + 1. Called by overlays 0 and 1. */
/*@ 0D1C:0454 */ void DrawRowList(int index)
{
    int x1;
    int y1;
    int x2;
    int y2;

    if (g54F2 != index) {
        if (g54F2 != -1) {
            LockHidePointer();
            DrawBox(gRows5482[g54F2].x1, gRows5482[g54F2].y1, gRows5482[g54F2].x2,
                    gRows5482[g54F2].y2, 0xC);
            LockShowPointer();
        }
        g54F4 = 0;
        g54F2 = index;
        gFC5E = gTicks;
    } else {
        if (g54F2 != -1) {
            if (gTicks > gFC5E) {
                if (g1CE7[g54F4] == 0)
                    g54F4 = 0;
                x1 = gRows5482[g54F2].x1;
                y1 = gRows5482[g54F2].y1;
                x2 = gRows5482[g54F2].x2;
                y2 = gRows5482[g54F2].y2;
                LockHidePointer();
                DrawBox(x1, y1, x2, y2, g1CE7[g54F4]);
                LockShowPointer();
                g54F4++;
                gFC5E = gTicks + 1;
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
/* 0D1C:059B: Purpose unknown: a per-character check against a 1d20 roll. It derives a class
   slot from gChars[who].classIndex (GetClassSlotByte, with 4 and 5 folded to 0), a value from
   GetIndexedByte(classSlot, levels, `attack`), then, depending on a level-like value race * 2
   (3 or 5, and separately 4), for `attack` 4, 0 or 1 in the 3-or-5 case and for `attack` 4 or 1
   in the level-4 case, subtracts g54F6[constitution]; finally it returns 1 when RollDice(1, 20, 0) >=
   value. QUIRK 0D1C:0609/0604: race * 2 is even, so the tests for 3
   and 5 can never hold; the original still has them. Called by 0DE8:0F98 and 0DE8:101A. */
/*@ 0D1C:059B */ int f0D1C_059B(int who, int attack)
{
    struct CharRec far *rec;
    int classSlot;
    int value;
    int level;

    rec = &gChars[who];
    classSlot = GetClassSlotByte(rec->classIndex, 0);
    if (classSlot == 4 || classSlot == 5)
        classSlot = 0;
    value = GetIndexedByte(classSlot, rec->levels[0], attack);
    level = rec->race * 2;
    if (level == 3 || level == 5) {
        if (attack == 4 || attack == 0 || attack == 1)
            value -= g54F6[rec->constitution];
    }
    if (level == 4) {
        if (attack == 4 || attack == 1)
            value -= g54F6[rec->constitution];
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
/* 0D1C:070F: draws the item icon of the ds:8DD2 record `n` at (x, y), the far pointer
   gIconsDDE4[icon]. Colour-row path, Purpose unknown: when gFBD2 bit 0 is set and the record's
   flags have 0x80, it first selects the icon's 16-byte colour row (gBufF534 + (icon << 4), or the
   empty string at ds:550A when gGraphicsChoice is 0) through gFn80D2 and draws through f1A29_008B,
   otherwise through DrawShape. Called by 0BD2 (three sites) and 1547:07E7. */
/*@ 0D1C:070F */ void DrawItemIcon(int mode, int n, int x, int y)
{
    int icon;

    icon = gTable8DD2[n].icon;
    if ((gFBD2 & 1) && (gTable8DD2[n].flags & 0x80)) {
        if (gGraphicsChoice)
            gFn80D2(gBufF534 + (icon << 4));
        else
            gFn80D2("");
        f1A29_008B(mode, gIconsDDE4[icon], x, y, 0);
    } else {
        DrawShape(mode, gIconsDDE4[icon], x, y, 0);
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
/* 0D1C:07DB: Purpose unknown: combines the first two carried items of gChars[who] (slots 0 and
   1). Each slot's item is passed to ItemClassAllowed and its ds:8DD2 record's type indexes the
   16-byte records at ds:A9F5. With `flag` 0 it returns 1 when slot 0 is empty or ItemClassAllowed
   accepted its item. Otherwise it returns 0 when slot 0 holds an item whose gA9F5[type].m01 is 2, then
   1 when slot 1 is empty, and for a slot-1 item whose gA9F5[m1].m09 (low 7 bits) is 1, 2 or 3,
   0 when gA9F5[m1].m01 is nonzero, else the slot-1 result. Called by 0BD2:1001, 0DE8:0D5E and
   1547:07E7. Placeholder: what the values mean is not established. */
/*@ 0D1C:07DB */ int f0D1C_07DB(int who, int flag)
{
    int slot0;
    int result0;
    int slot1;
    int result1;
    int m0;
    int m1;

    slot0 = gChars[who].items[0];
    result0 = ItemClassAllowed(who, slot0);
    m0 = gTable8DD2[slot0].type;
    if (flag == 0)
        return slot0 == 0 || result0 != 0;
    slot1 = gChars[who].items[1];
    result1 = ItemClassAllowed(who, slot1);
    m1 = gTable8DD2[slot1].type;
    if (slot0 != 0 && gA9F5[m0].m01 == 2)
        return 0;
    if (slot1 == 0)
        return 1;
    if ((gA9F5[m1].m09 & 0x7f) > 0 && (gA9F5[m1].m09 & 0x7f) < 4) {
        if (gA9F5[m1].m01 == 0)
            return result1;
        return 0;
    }
    return result1;
}
/* 0D1C:08B1: Purpose unknown: returns 1 when one of the first two carried items of the character
   record `rec` (items[0] or items[1]) holds a nonzero item whose ds:8DD2 record's type indexes a
   ds:A9F5 record with m09 == 6, otherwise 0. The pointer is a character record (the slots are at
   +77). Called by 0D1C:0903. */
/*@ 0D1C:08B1 */ int f0D1C_08B1(struct CharRec far *rec)
{
    int item;
    int i;

    for (i = 0; i <= 1; i++) {
        if (rec->items[i]) {
            item = gTable8DD2[rec->items[i]].type;
            if (gA9F5[item].m09 == 6)
                return 1;
        }
    }
    return 0;
}
/* 0D1C:0903: Purpose unknown: scans the six party members and returns a packed word. A member
   qualifies when MemberIs(who, 0xD) and f0D1C_08B1(rec) hold; for it the code takes
   FindClassSlot(2, classIndex) and, failing that, FindClassSlot(4, classIndex), and compares the member's
   levels[slot] (minus 2 in the second case) with the best so far. The result is (best member << 8)
   + best value, or 0 when none qualifies. Called by 0D1C:0A43. */
/*@ 0D1C:0903 */ int f0D1C_0903(void)
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
        if (!f0D1C_08B1(rec))
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
/* 0D1C:09CB: Purpose unknown: a per-object check. It takes the object's ds:1A3B record's m19 as
   a row and GetClampedTableByte(column, m19) as a value. A value of -1 calls DamageObject(obj, 100)
   and returns 1. Otherwise, when `threshold` is below the value it returns 0; else it sets bit 3
   of obj->m16, stores (DirectionFromPoints(obj->id, gPartyCell) ^ 4) >> 1 in obj->orient and returns 1.
   Called by 0D1C:0A43 and 17A6. */
/*@ 0D1C:09CB */ int f0D1C_09CB(struct Monster far *obj, int threshold, int column)
{
    int row;
    int value;

    row = gMonsterTypes[obj->typeIndex].m19;
    value = GetClampedTableByte(column, row);
    if (value != -1) {
        if (threshold >= value) {
            obj->m16 |= 8;
            obj->orient = (DirectionFromPoints(obj->cell, gPartyCell) ^ 4) >> 1;
            return 1;
        }
    } else {
        DamageObject(obj, 100);
        return 1;
    }
    return 0;
}
/* 0D1C:0A43: the "turn undead" action. When gFBC8 is 2 or 7 it derives a ten-bit key from
   gPartyCell and gPartyDir (StepCell) and requires gMaze[key * 9 + 4] & 7 to be nonzero, then
   asks f0D1C_0903 for the best qualifying party member (its low byte the member's value, its
   high byte the member's index) and rolls 1d20. It lists the gMonsters indices whose ds:8A82 object
   record's m02 equals key (f08E6_17FD) and, for each object whose ds:1A3B record's m0F has bit 2
   set and whose m16 has bit 4 clear, sets gF518, marks the object (m16 |= 0x10) and runs
   f0D1C_09CB(obj, roll, member value), ORing the results. When any object turned it formats "%s
   uses the power to turn undead!" (ds:550E) with the best member's name, draws it, plays sound
   0x5F and runs 0EF3:06E7 and 08E6:0E48. gF518 is cleared at the end on the normal path. Called
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

    if (gFBC8 != 2 && gFBC8 != 7)
        return;
    turned = 0;
    key = StepCell(gPartyCell, gPartyDir);
    value = (unsigned char)gMaze[key * 9 + 4] & 7;
    if (value == 0)
        return;
    packed = f0D1C_0903();
    if (packed == 0)
        return;
    memberVal = packed & 0xff;
    member = (packed & 0xff00) >> 8;
    f08E6_17FD(list, key, 4, 4, 1, 1);
    roll = RollDice(1, 20, 0);
    for (i = 0; i < 5; i++) {
        if (list[i] == -1)
            break;
        obj = &gMonsters[list[i]];
        if ((gMonsterTypes[obj->typeIndex].m0F & 4) && !(obj->m16 & 0x10)) {
            gF518 = 1;
            obj->m16 |= 0x10;
            turned |= f0D1C_09CB(obj, roll, memberVal);
        }
    }
    if (turned) {
        sprintf(gFileNameBuf, "\x06\x06%s uses the power to turn undead!\x06\x0f\r",
                gChars[member].name);
        DrawText(gFileNameBuf);
        PlaySound(0x5F);
        FlashViewDamage();
        f08E6_0E48();
    }
    gF518 = 0;
}
/* 0D1C:0BAB: Purpose unknown: a percentage check on the object `objIndex` (its ds:8A82 record,
   not a pointer like 0D1C:09CB's `obj`): through gMonsters[objIndex].typeIndex it reads gMonsterTypes[typeIndex].m1A
   and returns 1 when RollDice(1, 100, 0) is below it, otherwise 0. Called by 0DE8:0359. */
/*@ 0D1C:0BAB */ int f0D1C_0BAB(int objIndex)
{
    int chance;

    chance = gMonsterTypes[gMonsters[objIndex].typeIndex].m1A;
    return RollDice(1, 100, 0) < chance;
}
/* 0D1C:0BEB: Purpose unknown: returns 1 when gMonsterTypes[gMonsters[obj].typeIndex].m0E has bit 2 set and
   gA9F5[item].m00 has bit 2 clear, otherwise 0. `item` indexes the ItemInfo records. Called by
   0DE8:0359. */
/*@ 0D1C:0BEB */ int f0D1C_0BEB(int obj, int item)
{
    return (gMonsterTypes[gMonsters[obj].typeIndex].m0E & 4) && (gA9F5[item].m00 & 4) == 0;
}
/* 0D1C:0C21: Purpose unknown: draws a framed rectangle at (x, y) `width` by `height` through
   082C:09D5. It hides the pointer, draws the whole rectangle in the colours picked by `first`
   (gF57E, gF57E when `first`, else gFB4C and gF53A) with -1 as the third colour, then shrinks the
   rectangle by one on each side and draws the inner border in gFB4C/gF53A with a third colour of
   -1 when `second`, else gF57E, and shows the pointer. Called by overlay 1 and overlay 6. */
/*@ 0D1C:0C21 */ void f0D1C_0C21(int x, int y, int width, int height, int first, int second)
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
        c2 = gF57E;
        c1 = gF57E;
    } else {
        c1 = gFB4C;
        c2 = gF53A;
    }
    DrawFilledPanel(x1, y1, x2, y2, c1, c2, -1);
    x1++;
    y1++;
    x2--;
    y2--;
    DrawFilledPanel(x1, y1, x2, y2, gFB4C, gF53A, second ? -1 : gF57E);
    LockShowPointer();
}
