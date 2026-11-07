/* Unit at EOB.EXE 0BD2:000E (4681 bytes, 22 functions): the inventory and item-use module. Its
   functions walk the item records at ds:8DD0 (struct ItemRecord, gItems) through the circular
   lists whose head is a word, and the character records at
   ds:F580 (struct CharRec, src/E0A72.C). It draws the item rows, uses and wears items, applies a
   status effect, and is reached from the action handlers of 0ACD and the object/effect units.
   Placeholder names remain where the evidence names the behaviour but not the game concept. */

/*@flags -ml -O -Z -v */

#include "EOB.H"
extern struct ItemRecord gItems[]; /*@ ds:8DD0 */

extern struct ItemType gItemTypes[]; /*@ ds:A9F0 */

extern struct CharRec gChars[]; /*@ ds:F580 */

extern struct Projectile gProjectiles[]; /*@ ds:DD2C */

extern struct Monster gMonsters[]; /*@ ds:8A82 */

extern signed char gInventoryMember; /*@ ds:A99A */    /* the selected character index (src/E0ACD.C) */
extern char gCursorSlot; /*@ ds:F51A */           /* the selected row (src/E0D1C.C) */
extern char gPanelMode; /*@ ds:FB44 */           /* a display-mode byte (src/E0D1C.C) */
extern int gCurLevel; /*@ ds:FBC8 */            /* the dungeon level, 1..12 (src/E0D1C.C) */
extern int gPartyDir; /*@ ds:FBCC */            /* the party's facing, 0..3 north, east, south, west */
extern int gPartyCell; /*@ ds:FBCA */     /* the party's map cell (src/E177D.C:92): this unit passes
                                            it to StepCell (UseWandItem) as the cell; E0ACD.C
                                            indexes the nine-byte records with it */
extern int gMouseDisabled; /*@ ds:814A */   /* nonzero disables the mouse (src/E07A9.C) */
extern int gDrawPage; /*@ ds:8115 */            /* the graphics driver's draw page (src/E082C.C) */
extern int gGraphicsChoice; /*@ ds:8102 */            /* nonzero: a colour row exists (src/E07A9.C) */
extern signed char gWallTypeFlags[]; /*@ ds:194B */  /* byte table (src/E0D1C.C) */
extern signed char gClassBaseBits[]; /*@ ds:1231 */  /* byte table indexed by gChars[].classIndex (src/E0DE8.C) */
extern int gSlotItemMask[]; /*@ ds:1C8D */          /* per-row class mask (0BD2:0AF2/0C8E) */
extern signed char gPanelFillColour; /*@ ds:F57E */    /* a colour read by 0BD2:0917 (src/E0D1C.C) */
extern signed char gPanelShadow; /*@ ds:F53A */    /* a colour read by 0BD2:0917 (src/E0D1C.C) */
extern signed char gPanelHighlight; /*@ ds:FB4C */    /* a colour read by 0BD2:0917 (src/E0D1C.C) */
extern char gFileNameBuf[]; /*@ ds:FE6E */  /* the text buffer sprintf formats into (src/EOV2.C) */
extern void far *gStagingPage; /*@ ds:DF4E */   /* slot 5's buffer segment (src/E07A9.C) */
extern char gViewDirty; /*@ ds:DF4C */           /* set to 1 when an action took effect (src/E0ACD.C) */
extern char gMazeItems[]; /*@ ds:B899 */         /* the nine-byte record list head (src/E0D1C.C) */
extern char gMaze[]; /*@ ds:B894 */         /* the nine-byte records (src/E0D1C.C) */
extern void (far *gSaveRect)(int x, int y, int w, int h, void far *buf); /*@ ds:80AE */ /* E1BBE.ASM */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */ /* src/E082C.C */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int c1, int c2, int c3); /*@ 082C:09D5 */
extern void DrawRect(int x1, int y1, int x2, int y2, int colour); /*@ 082C:04C4 */
extern void PrintInt(int value, int a, int b, int c, int d); /*@ 1AA6:000F */
extern int gInvSlotX[]; /*@ ds:17CE */          /* a per-row word (the item row's x) */
extern unsigned char gInvSlotY[]; /*@ ds:1804 */ /* a per-row byte (the item row's y) */
extern signed char gInvSlotKind[]; /*@ ds:3875 */  /* a per-row byte (0, 1 or 2): the row's kind, 1 is
                                              the count row, 2 the item's icon is drawn offset */
extern signed char gInvSlotBoxSize[]; /*@ ds:3890 */  /* a three-byte table read by 0BD2:0917 */
extern signed char gWandSpells[]; /*@ ds:38FA */  /* a byte table indexed by an item's value in 0BD2:0EE8 */
extern void StartItemCast(int who, int action, int kind); /*@ 0EF3:28ED */
extern void FlashViewDamage(void); /*@ 0EF3:06E7 */
extern void FlashPartyMembers(int who); /*@ 0EF3:0665 */
extern void ClearPoisonEffect(int who); /*@ 0EF3:1CE8 */
extern void MoveMonsterToCell(struct Monster far *obj, int id, int orient); /*@ 08E6:0297 */
extern void ApplyCharEffect(char who, long interval, char value, char firstEmpty); /*@ 1414:05ED */
extern void HealCharacter(int who, int amount); /*@ 0DE8:0ADB */
extern void ApplyStatusEffect(int who, int mask, char far *text, int attack, int count, int value, int flag); /*@ 0DE8:101A */
extern char far *SkipStrings(char far *text, int count); /*@ 0D1C:07A7 */
extern char gPotionFeelings[]; /*@ ds:3893 */        /* the "feels ..." string table (0BD2:0D2D) */
extern char far *gMageSpellNames[]; /*@ ds:1278 */   /* far item-name pointers indexed by m0B (0BD2:05B5) */
extern char far *gClericSpellNames[]; /*@ ds:12E0 */   /* far item-name pointers indexed by m0B (0BD2:05B5) */
extern char gRingNames[]; /*@ ds:37B4 */        /* a string table SkipStrings walks (0BD2:05B5) */
extern char gPotionNames[]; /*@ ds:37E1 */        /* a string table SkipStrings walks (0BD2:05B5) */
extern char gWandNames[]; /*@ ds:3837 */        /* a string table SkipStrings walks (0BD2:05B5) */

extern int StepCell(int cell, int dir); /*@ 0D1C:00DA */
extern int GetCurrentItem(void); /*@ 0D1C:02DC */
extern int MemberIs(int who, int mask); /*@ 0D1C:0670 */
extern int CanUseHand(int who, int flag); /*@ 0D1C:07DB */
extern void UseItem(int who, int slot, int field); /*@ 0DE8:0763 */
extern void EatFood(int index); /*@ 0ACD:07E0 */
extern void OpenSpellPanel(int who, int flag); /*@ 0EF3:21E0 */
extern void UseItemPanel(int msg); /*@ 211E:0093 */  /* overlay 5 entry, ovl5:1CDD */
extern void SetMouseItem(int n); /*@ 07A9:010C */
extern void RedrawMember(int who); /*@ 1547:0991 */
extern void DrawItemIcon(int mode, int n, int x, int y); /*@ 0D1C:070F */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void ComputeArmourClass(int who); /*@ 0DE8:0D5E */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern char *strcpy(char far *dest, char far *src); /*@ 0000:67CD */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */

/* functions of this unit, forward (address order) */
void InsertListItem(void far *head, int key, int item, int tag);
int RemoveListItem(void far *head, int key, int target);
void DrawItemRow(int row, int slot, int drawMode);
int CanPlaceItemInSlot(int item, int who, int row);
void UseSelectedItem(int slot);
void SwapSelectedItem(int who, int row);
int ItemClassAllowed(int who, int item);
void DrawItemName(int item);

/*@data */
/* the data of e0bd2, ds:37B4 to ds:3A62 (686 bytes, 0 relocations): drafted by make data-skeleton from the original's bytes; each item at its annotation, in address order. */
char gRingNames[45] = { /*@ ds:37B4 */
    65, 100, 111, 114, 110, 109, 101, 110, 116, 0, 87, 105, 122, 97, 114, 100, 114, 121, 0, 83, 117,
    115, 116, 101, 110, 97, 110, 99, 101, 0, 70, 101, 97, 116, 104, 101, 114, 32, 70, 97, 108, 108,
    0, 0, 0
};
char gPotionNames[86] = { /*@ ds:37E1 */
    0, 71, 105, 97, 110, 116, 32, 83, 116, 114, 101, 110, 103, 116, 104, 0, 72, 101, 97, 108, 105,
    110, 103, 0, 69, 120, 116, 114, 97, 32, 72, 101, 97, 108, 105, 110, 103, 0, 80, 111, 105, 115,
    111, 110, 0, 86, 105, 116, 97, 108, 105, 116, 121, 0, 83, 112, 101, 101, 100, 0, 73, 110, 118,
    105, 115, 105, 98, 105, 108, 105, 116, 121, 0, 67, 117, 114, 101, 32, 80, 111, 105, 115, 111,
    110, 0, 0
};
char gWandNames[62] = { /*@ ds:3837 */
    83, 116, 105, 99, 107, 0, 76, 105, 103, 104, 116, 110, 105, 110, 103, 0, 70, 114, 111, 115, 116,
    0, 67, 117, 114, 105, 110, 103, 0, 70, 105, 114, 101, 98, 97, 108, 108, 0, 83, 105, 108, 118,
    105, 97, 115, 0, 77, 97, 103, 105, 99, 32, 77, 105, 115, 115, 105, 108, 101, 0, 0, 0
};
signed char gInvSlotKind[27] = { /*@ ds:3875 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2
};
signed char gInvSlotBoxSize[3] = { 16, 16, 8 }; /*@ ds:3890 */
char gPotionFeelings[103] = { /*@ ds:3893 */
    0, 109, 117, 99, 104, 32, 115, 116, 114, 111, 110, 103, 101, 114, 0, 98, 101, 116, 116, 101, 114,
    0, 109, 117, 99, 104, 32, 98, 101, 116, 116, 101, 114, 0, 105, 108, 108, 32, 102, 111, 114, 32,
    97, 32, 109, 111, 109, 101, 110, 116, 0, 110, 111, 32, 108, 111, 110, 103, 101, 114, 32, 104,
    117, 110, 103, 114, 121, 0, 102, 97, 115, 116, 32, 97, 110, 100, 32, 97, 103, 105, 108, 101, 0,
    116, 114, 97, 110, 115, 112, 97, 114, 101, 110, 116, 0, 98, 101, 116, 116, 101, 114, 0, 0
};
signed char gWandSpells[7] = { 0, 18, 24, 40, 13, -1, 4 }; /*@ ds:38FA */
/* ds:3901-3912: the code's (a literal or template, '%s has lost her '): written by its function */
/* ds:3912-3923: the code's (a literal or template, '%s has lost his '): written by its function */
/* ds:3923-3926: the code's (a literal or template, '.\r'): written by its function */
/* ds:3926-3933: the code's (a literal or template, 'Cursed %s %d'): written by its function */
/* ds:3933-393A: the code's (a literal or template, '%s +%d'): written by its function */
/* ds:393A-3946: the code's (a literal or template, 'Mage Scroll'): written by its function */
/* ds:3946-3954: the code's (a literal or template, 'Cleric Scroll'): written by its function */
/* ds:3954-3959: the code's (a literal or template, 'Ring'): written by its function */
/* ds:3959-3960: the code's (a literal or template, 'Potion'): written by its function */
/* ds:3960-3965: the code's (a literal or template, 'Wand'): written by its function */
/* ds:3965-396E: the code's (a literal or template, '%s of %s'): written by its function */
/* ds:396E-3991: the code's (a literal or template, "%s can't wear that type of armor.\r"): written by its function */
/* ds:3991-39B0: the code's (a literal or template, 'You cant put that item there.\r'): written by its function */
/* ds:39B0-39B9: the code's (a literal or template, 'poisoned'): written by its function */
/* ds:39B9-39C7: the code's (a literal or template, '%s feels %s!\r'): written by its function */
/* ds:39C7-39F0: the code's (a literal or template, 'The wand has no apparent magical effect\r'): written by its function */
/* ds:39F0-39FC: the code's (a literal or template, 'no effect.\r'): written by its function */
/* ds:39FC-3A15: the code's (a literal or template, ' can not use this item.\r'): written by its function */
/* ds:3A15-3A3E: the code's (a literal or template, 'This item automatically used when worn.\r'): written by its function */
/* ds:3A3E-3A62: the code's (a literal or template, 'This item is not used in this way.\r'): written by its function */

/* functions of this unit, in address order */
/* 0BD2:000E: the "lost item" message. Rolls 1d27 for a starting slot, then scans the 27 item slots
   of character `who` from there, wrapping, for the first item whose type flags have 0x80. It clears
   the slot, prints "<name> has lost her/his " (the possessive by race bit 0), draws the item's name
   through 0BD2:05B5 and ".\r", and redraws the character's row. */
/*@ 0BD2:000E */ void ReportLostItem(int who)
{
    int i;
    int item;
    int roll;

    roll = RollDice(1, 0x1b, -1);
    for (i = 0; i < 0x1b; i++) {
        item = gChars[who].items[roll];
        if (item != 0) {
            if (gItemTypes[gItems[item].type].m0E.word & 0x80) {
                gChars[who].items[roll] = 0;
                if (gChars[who].race & 1)
                    sprintf(gFileNameBuf, "%s has lost her ", gChars[who].name);
                else
                    sprintf(gFileNameBuf, "%s has lost his ", gChars[who].name);
                DrawText(gFileNameBuf);
                DrawItemName(item);
                DrawText(".\r");
                break;
            }
        }
        if (++roll >= 0x1b)
            roll = 0;
    }
    RedrawMember(who);
}

/* 0BD2:0106: takes the item out of slot `row` of character `who` (or the mouse's slot when `row` is
   -1): clears the slot, marks the record free (key = -1), redraws, and returns the item's value. */
/*@ 0BD2:0106 */ int TakeItemFromSlot(int who, int row)
{
    int item;

    if (row == -1)
        item = GetCurrentItem();
    else
        item = gChars[who].items[row];
    gItems[item].key = -1;
    if (row == -1) {
        SetMouseItem(0);
    } else {
        gChars[who].items[row] = 0;
        if (gPanelMode == 1)
            DrawItemRow(row, 1, 0);
        else if (gPanelMode == 0)
            RedrawMember(who);
    }
    return gItems[item].value;
}

/* 0BD2:019F: refreshes the current row: in mode 1 through 0BD2:0BA0 with the selected row, in any
   mode other than 2 through 0BD2:0C8E with the selected character and row. Purpose unknown. */
/*@ 0BD2:019F */ void RedrawSelectedRow(void)
{
    if (gPanelMode == 1)
        UseSelectedItem(gCursorSlot);
    else if (gPanelMode != 2)
        SwapSelectedItem(gInventoryMember, gCursorSlot);
}

/* 0BD2:01D0: inserts the item `item` into the circular list whose head word is `head`, stamping it
   with the list's owner key `key` and the tag `tag` (+5; `level` becomes gCurLevel, or -1 when the
   key is negative). The key is a list owner: a map cell (E0ACD.C:596, E0D1C.C:386, 0BD2:07DE), an
   object's id (E08E6.C:848) or 0xFFFE for a character's own pack (E0ACD.C:808, :851, 0BD2:0BA0).
   An empty list becomes `item` alone, linked to itself. Otherwise `item` is spliced in between the
   old head and its predecessor, the tail of the item walk: the new item's nextLink becomes the old head
   (the tail's forward link) and its prevLink becomes the tail, so only the old head's prevLink and the
   tail's nextLink change. `item` becomes the new head, so the lists walked by nextLink (0BD2:0385, 040E, 04AA) meet
   the newest item first. */
/*@ 0BD2:01D0 */ void InsertListItem(void far *head, int key, int item, int tag)
{
    struct ItemRecord *rec;
    struct ItemRecord *tailRec;
    struct ItemRecord *cur;
    int oldHead;
    int tail;

    if (item == 0)
        return;
    rec = &gItems[item];
    if (*(int far *)head == 0) {
        *(int far *)head = item;
        rec->nextLink = rec->prevLink = item;
        rec->cellPos = tag;
        rec->key = key;
        if (key >= 0)
            rec->level = gCurLevel;
        else
            rec->level = -1;
    } else {
        oldHead = *(int far *)head;
        cur = &gItems[oldHead];
        tail = gItems[oldHead].prevLink;
        tailRec = &gItems[tail];
        rec->key = key;
        rec->cellPos = tag;
        if (key >= 0)
            rec->level = gCurLevel;
        else
            rec->level = -1;
        rec->nextLink = tailRec->nextLink;
        rec->prevLink = tail;
        tailRec->nextLink = item;
        cur->prevLink = item;
        *(int far *)head = item;
    }
}

/* 0BD2:02B9: removes from the circular list `head` the first item whose tag equals `key` (when
   `target` is -1) or whose index equals `target`, unlinks it, and returns the item; 0 when none
   matches. Called with (head, 0, -1) to take the head item off a character's own pack. */
/*@ 0BD2:02B9 */ int RemoveListItem(void far *head, int key, int target)
{
    int start;
    int item;
    int link;
    int next;
    int i;
    struct ItemRecord *rec;

    item = *(int far *)head;
    if (item == 0)
        return 0;
    i = 0;
    start = item;
    while (item != start || i == 0) {
        rec = &gItems[item];
        if (target == -1) {
            if (rec->cellPos == key)
                goto found;
        }
        if (target != item)
            goto next;
found:
        link = rec->prevLink;
        next = rec->nextLink;
        gItems[link].nextLink = next;
        gItems[next].prevLink = link;
        rec->nextLink = 0;
        rec->prevLink = 0;
        rec->key = 0;
        rec->level = 0;
        if (*(int far *)head == item)
            *(int far *)head = next;
        if (*(int far *)head == item)
            *(int far *)head = 0;
        return item;
next:
        i++;
        item = rec->nextLink;
    }
    return 0;
}

/* 0BD2:0385: walks the circular list `head` and returns the first matching item's index when
   `count` is 0, otherwise the number of matches. A match is every item when `target` and `type` are
   both -1, the item whose m02 is `type` when `target` is -1, or the item `target` when `type` is
   -1; with both `target` and `type` set, nothing matches. Purpose unknown. */
/*@ 0BD2:0385 */ int ScanListItem(void far *head, int target, int type, int count)
{
    int start;
    int cur;
    int n;
    int first;
    struct ItemRecord *rec;

    cur = *(int far *)head;
    if (cur == 0)
        return 0;
    first = 1;
    n = 0;
    start = cur;
    while (cur != start || first) {
        rec = &gItems[cur];
        if ((target == -1 && type == -1) || (target == -1 && rec->type == type)
                || (type == -1 && cur == target)) {
            if (count)
                n++;
            else
                return cur;
        }
        first = 0;
        cur = rec->nextLink;
    }
    if (count != 0)
        return n;
    return 0;
}

/* 0BD2:040E: sets bit 0x40 of the flags of every item in the circular list `head`. 0BD2:05B5 reads
   that bit to show the item's specific name (or its second name) instead of the generic first name,
   so 0x40 is the identified flag. */
/*@ 0BD2:040E */ void IdentifyListItems(void far *head)
{
    int start;
    int item;
    int first;
    struct ItemRecord *rec;

    item = *(int far *)head;
    if (item == 0)
        return;
    first = 1;
    start = item;
    while (item != start || first) {
        rec = &gItems[item];
        rec->flags |= 0x40;
        first = 0;
        item = rec->nextLink;
    }
}

/* 0BD2:0458: queues item `item` at the nine-byte record gMazeItems + `volume` * 9 through 0BD2:01D0
   with the tag `tag`. When the queued item still links onward (its nextLink is not itself), the record's
   head word moves to that nextLink. Purpose unknown. */
/*@ 0BD2:0458 */ void PushListItem(int volume, int item, int tag)
{
    InsertListItem((void far *)(gMazeItems + volume * 9), volume, item, tag);
    if (gItems[item].nextLink != item)
        *(int *)(gMazeItems + volume * 9) = gItems[item].nextLink;
}

/* 0BD2:04AA: drains the list at gMazeItems + `volume` * 9. Items whose type is `type` (or all when
   `type` is -1) are freed (key = -1, level = 0); the others are re-queued through 0BD2:01D0 in their
   original order, keeping each item's cellPos. Purpose unknown. */
/*@ 0BD2:04AA */ void DrainListItem(int volume, int type)
{
    int start;
    int item;
    int count;
    int saved;

    item = *(int *)(gMazeItems + volume * 9);
    if (item == 0)
        return;
    *(int *)(gMazeItems + volume * 9) = 0;
    count = 0;
    start = item;
    while (item != start || count == 0) {
        if (gItems[item].type == type || type == -1) {
            gItems[item].key = -1;
            gItems[item].level = 0;
            saved = item;
            item = gItems[item].nextLink;
            gItems[saved].nextLink = 0;
            gItems[saved].prevLink = 0;
        } else {
            saved = item;
            item = gItems[item].nextLink;
            gItems[saved].nextLink = 0;
            gItems[saved].prevLink = 0;
            InsertListItem((void far *)(gMazeItems + volume * 9), volume, saved, gItems[saved].cellPos);
        }
        count++;
    }
}

/* 0BD2:05B5: draws item `item`'s name into the text buffer. The record's first two bytes index one
   table of 0x23-byte entries at 1F28:0000: both nameIndex and identifiedNameIndex offset that base by * 0x23. When the
   record's flags have 0x40 and the second name is empty, the item type's m0E low 7 bits select the
   name: the plain types use the first name with "%s +%d" or "Cursed %s %d", and types 9, 10, 16,
   14 and 18 use "Mage Scroll", "Cleric Scroll", "Ring", "Potion" or "Wand" joined with " of " to a
   per-m0B far pointer (ds:1278, ds:12E0) or a SkipStrings walk (ds:37B4, ds:37E1, ds:3837).
   Otherwise the second (or the first) name is copied whole. The buffer is drawn with DrawText.
   Called by 0BD2:000E and SetMouseItem (07A9:010C). The case order is the jump table at 0BD2:07B8
   (0..3, 9, 10, 14, 16, 18; all the rest fall to the default). */
/*@ 0BD2:05B5 */ void DrawItemName(int item)
{
    signed char m0B;
    char cls;
    struct ItemRecord *rec;
    char far *firstName;
    char far *secondName;
    char far *str1;
    char far *str2;
    char buf[0x3C];

    rec = &gItems[item];
    firstName = (char far *)0x1F280000L + rec->nameIndex * 0x23;
    secondName = (char far *)0x1F280000L + rec->identifiedNameIndex * 0x23;
    cls = gItemTypes[rec->type].m0E.byte & 0x7F;
    m0B = rec->value;
    str1 = 0;
    str2 = 0;
    if ((rec->flags & 0x40) && strlen(secondName) == 0) {
        switch (cls & 0x7F) {
        case 0:
        case 1:
        case 2:
        case 3:
            if (m0B != 0) {
                if (m0B < 0)
                    sprintf(buf, "Cursed %s %d", firstName, m0B);
                else
                    sprintf(buf, "%s +%d", firstName, m0B);
            } else
                strcpy(buf, firstName);
            break;
        case 9:
            str1 = "Mage Scroll";
            str2 = gMageSpellNames[m0B];
            break;
        case 10:
            str1 = "Cleric Scroll";
            str2 = gClericSpellNames[m0B];
            break;
        case 16:
            str1 = "Ring";
            str2 = SkipStrings(gRingNames, m0B);
            break;
        case 14:
            str1 = "Potion";
            str2 = SkipStrings(gPotionNames, m0B);
            break;
        case 18:
            str1 = "Wand";
            str2 = SkipStrings(gWandNames, m0B);
            break;
        default:
            strcpy(buf, firstName);
            break;
        }
        if (str2 != 0)
            sprintf(buf, "%s of %s", str1, str2);
    } else if (rec->flags & 0x40) {
        strcpy(buf, secondName);
    } else {
        strcpy(buf, firstName);
    }
    DrawText(buf);
}

/* 0BD2:07DE: reserves one of the ten effect records at ds:DD2C (returning 0 when all are live),
   queues `item` at the map record gMazeItems + `volume` * 9 through 0BD2:01D0 with the tag `kind | 4`,
   fills the record from the arguments, plays sound 0x0B (0x1A when `type` is 7), and returns 1.
   `volume` is the item's map cell (most callers pass gPartyCell, E0ACD.C:446, :769, E0DE8.C:415;
   E122E.C:752 passes the script's id), `page` is gPartyDir, `kind` is the per-page action kind and
   `type` is the item's m02. QUIRK 0BD2:07DE: rec->m02 is stored 1 twice (the second store is in the original). */
/*@ 0BD2:07DE */ int AddDDEffect(int who, int item, int volume, int kind, int page, int type)
{
    struct Projectile *rec;
    int i;

    rec = gProjectiles;
    for (i = 0; i < 10; i++, rec++) {
        if (rec->active == 0)
            break;
    }
    if (i == 10)
        return 0;
    InsertListItem((void far *)(gMazeItems + volume * 9), volume, item, kind | 4);
    rec->active = 1;
    rec->m02 = 1;
    rec->m0C = 0;
    rec->dir = page;
    rec->m0D = 0xC;
    rec->cell = volume;
    rec->m06 = kind;
    rec->item = item;
    rec->m0A = type;
    rec->m0B = who;
    rec->m02 = 1;
    rec->fn = 0;
    if (type != 7)
        PlaySound(0xB);
    else
        PlaySound(0x1A);
    return 1;
}

/* 0BD2:089E: when item record `item` is live (key != -1), finds the first free record from index 1
   on, copies `item`'s 14 bytes into it with Unpack0, and returns the new index; 0 when none is
   free. Purpose unknown. */
/*@ 0BD2:089E */ int CopyItemToFreeSlot(int item)
{
    struct ItemRecord *cand;
    struct ItemRecord *src;
    int slot;
    int found;

    src = &gItems[item];
    if (src->key == -1)
        return 0;
    slot = 1;
    found = 0;
    cand = &gItems[1];
    while (slot < 500) {
        if (cand->key == -1) {
            found = 1;
            break;
        }
        slot++;
        cand++;
    }
    if (!found)
        return 0;
    Unpack0((char far *)src, (void far *)cand, 0xEL);
    return slot;
}

/* 0BD2:0917: draws the selected character's (gInventoryMember) item row `row`. When `slot` is
   nonzero it draws
   the row's frame: a filled panel and rectangle at x gInvSlotX[row], y gInvSlotY[row] sized by
   gInvSlotBoxSize[gInvSlotKind[row]] (a kind-1 row is the count row, which also prints the row's item count, and a
   row that is the selected row gCursorSlot is highlighted when gMouseDisabled). When the row holds an
   item and its kind gInvSlotKind[row] is not 1 it draws the item icon, offset by -4 when the kind is 2.
   `drawMode` is stored in gDrawPage for the duration. Called by 0ACD:09EF, 0BD2:0106, 0BD2:0BA0,
   0D1C:01CA, 0D1C:0304, 122E:06D0 and 1547:0991. */
/*@ 0BD2:0917 */ void DrawItemRow(int row, int slot, int drawMode)
{
    int y;
    int item;
    int count;
    int saved;
    int x;
    int kind;

    kind = slot;
    saved = gDrawPage;
    gDrawPage = drawMode;
    x = gInvSlotX[row];
    y = gInvSlotY[row];
    item = gChars[gInventoryMember].items[row];
    if (kind != 0) {
        kind = gInvSlotKind[row];
        if (gGraphicsChoice == 0)
            DrawFilledPanel(x - 1, y - 1, x + gInvSlotBoxSize[kind], y + gInvSlotBoxSize[kind], 3, 1,
                    kind != 1 ? gPanelFillColour : -1);
        else
            DrawFilledPanel(x - 1, y - 1, x + gInvSlotBoxSize[kind], y + gInvSlotBoxSize[kind], gPanelHighlight, gPanelShadow,
                    kind != 1 ? gPanelFillColour : -1);
        if (kind == 1) {
            gFillRect(0xE3, 0x41, 0xEE, 0x45, 0xC);
            count = ScanListItem(&gChars[gInventoryMember].items[row], -1, -1, 1);
            if (count < 10)
                x = 0xE9;
            else
                x = 0xE3;
            PrintInt(count, x, 0x41, 0xF, 0);
            x = gInvSlotX[row];
        }
        if (gMouseDisabled && gCursorSlot == row)
            DrawRect(x - 1, y - 1, x + gInvSlotBoxSize[kind], y + gInvSlotBoxSize[kind], 0xF);
    }
    if (item != 0 && gInvSlotKind[row] != 1) {
        if (gInvSlotKind[row] != 2)
            DrawItemIcon(drawMode, item, x, y);
        else
            DrawItemIcon(drawMode, item, x - 4, y - 4);
    }
    gDrawPage = saved;
}

/* 0BD2:0ABE: when `drawMode` is set, saves the 18x168 item sheet rectangle through the save-rect
   hook, then redraws the selected character's row. */
/*@ 0BD2:0ABE */ void RedrawRowArea(int drawMode)
{
    if (drawMode)
        gSaveRect(0x16, 0, 0x12, 0xA8, gStagingPage);
    RedrawMember(gInventoryMember);
}

/* 0BD2:0AF2: when `row` is 0x11 and `item` is carried, checks that character `who` may wear the
   item type (0BD2:11FD) and prints why on failure. Returns 0 when the worn item's flags have 0x20
   or the row's class mask does not cover the item type mask (printing a message), 1 when it fits. */
/*@ 0BD2:0AF2 */ int CanPlaceItemInSlot(int item, int who, int row)
{
    int mask;

    if (row == 0x11 && item != 0) {
        if (!ItemClassAllowed(who, item)) {
            sprintf(gFileNameBuf, "%s can't wear that type of armor.\r", gChars[who].name);
            DrawText(gFileNameBuf);
            return 0;
        }
    }
    if (gItems[gChars[who].items[row]].flags & 0x20)
        return 0;
    if (item != 0)
        mask = gItemTypes[gItems[item].type].slotMask.word;
    else
        mask = -1;
    if (!(gSlotItemMask[row] & mask)) {
        DrawText("You cant put that item there.\r");
        return 0;
    }
    return 1;
}
/* 0BD2:0BA0: uses the item in slot `slot` of the selected character gInventoryMember through
   0BD2:0AF2. For a
   slot other than 0x10 it exchanges the held item with the slot: the slot's item goes into the hand
   (SetMouseItem) and the held item goes into the slot, then the row is redrawn and the armour class
   recomputed. For slot 0x10 (the pack list) with an empty hand it takes the first item out of the
   list at items[16] into the hand; with an item in hand it puts the held item into the list with the
   key 0xFFFE and empties the hand. */
/*@ 0BD2:0BA0 */ void UseSelectedItem(int slot)
{
    int saved;
    int item;

    saved = gChars[gInventoryMember].items[slot];
    item = GetCurrentItem();
    if (!CanPlaceItemInSlot(item, gInventoryMember, slot))
        return;
    if (slot != 0x10) {
        SetMouseItem(saved);
        gChars[gInventoryMember].items[slot] = item;
        DrawItemRow(slot, 1, 0);
        ComputeArmourClass(gInventoryMember);
    } else if (item == 0) {
        saved = RemoveListItem(&gChars[gInventoryMember].items[16], 0, -1);
        DrawItemRow(slot, 1, 0);
        SetMouseItem(saved);
    } else {
        InsertListItem((void far *)&gChars[gInventoryMember].items[16], 0xFFFE, item, 0);
        DrawItemRow(slot, 1, 0);
        SetMouseItem(0);
    }
}

/* 0BD2:0C8E: swaps the item in slot `row` of character `who` with the mouse's item when the row's
   class mask covers it, redrawing and recomputing the armour class. Purpose unknown. */
/*@ 0BD2:0C8E */ void SwapSelectedItem(int who, int row)
{
    int item;
    int heldItem;
    int type;
    unsigned char mask;

    item = gChars[who].items[row];
    if (gItems[item].flags & 0x20)
        return;
    heldItem = GetCurrentItem();
    type = gItems[heldItem].type;
    if (heldItem != 0)
        mask = gItemTypes[type].slotMask.byte;
    else
        mask = 0xFF;
    if (gSlotItemMask[row] & mask) {
        SetMouseItem(item);
        gChars[who].items[row] = heldItem;
        RedrawMember(who);
    }
    ComputeArmourClass(who);
}

/* 0BD2:0D2D: applies the effect of the item just taken from slot `slot` of character `who`. It
   clears the slot through 0BD2:0106 (the item's m0B selects the effect), plays sound 0x0A and
   dispatches on m0B: 1 sets the strength score m0D to 0x16 and arms a timed giant-strength effect
   (ApplyCharEffect(who, ..., 7, 1); 7 is the giant-strength effect type, E1414.C:5ED stores it as
   the type byte and the 07CA runner prints "%s no longer has giant strength" and restores m0D/strengthPct
   from m0E/m10 when it runs out), 2 and 3 heal by a dice roll, 4 poisons through 0DE8:101A, 5 sets
   food to 100, 6 and 8 run the 0EF3 helpers, 7 sets effect flag 4. After the switch it announces
   "%s feels <effect>!\r" with the effect name from the ds:3893 table, except for effect 4 when the character's status
   bit 1 is set. Called by 0BD2:1001. */
/*@ 0BD2:0D2D */ void ApplyItemEffect(int who, int slot)
{
    struct CharRec *rec;
    int kind;

    rec = &gChars[who];
    kind = TakeItemFromSlot(who, slot);
    PlaySound(0xA);
    switch (kind) {
    case 1:
        FlashPartyMembers(who);
        rec->strength = 0x16;
        rec->strengthPct = 0;
        ApplyCharEffect(who, (RollDice(1, 4, 0) + 4) * 0x222, 7, 1);
        break;
    case 2:
        FlashPartyMembers(who);
        HealCharacter(who, RollDice(2, 4, 2));
        break;
    case 3:
        FlashPartyMembers(who);
        HealCharacter(who, RollDice(3, 8, 3));
        break;
    case 4:
        ApplyStatusEffect(who, 2, "poisoned", 0, 1, 8, 1);
        rec->effectFlags &= ~0x80;
        break;
    case 5:
        FlashPartyMembers(who);
        rec->food = 0x64;
        if (gPanelMode != 0)
            RedrawMember(who);
        break;
    case 6:
        FlashPartyMembers(who);
        break;
    case 7:
        FlashPartyMembers(who);
        rec->effectFlags |= 4;
        RedrawMember(who);
        break;
    case 8:
        FlashPartyMembers(who);
        ClearPoisonEffect(who);
        break;
    }
    if (kind - 1 != 3 || (rec->status & 2) == 0) {
        sprintf(gFileNameBuf, "%s feels %s!\r", rec->name, SkipStrings(gPotionFeelings, kind));
        DrawText(gFileNameBuf);
    }
}

/* 0BD2:0EE8: uses the wand in slot `slot` of character `who`. When the item's m0B is 0 it reports
   no effect; when it is not 5 it dispatches through 0EF3:28ED with the action gWandSpells[value]; for 5 it
   plays the wand sound, and when the surroundings pass the gWallTypeFlags/gMaze tests, moves every gMonsters
   record whose cell equals the cell x (from StepCell(gPartyCell, page)) to the cell y
   (MoveMonsterToCell(obj, y, -1)) and sets gViewDirty, otherwise reports no effect. */
/*@ 0BD2:0EE8 */ void UseWandItem(int who, int slot)
{
    int effect;
    int x;
    int y;
    int i;
    struct Monster *obj;
    int type;
    int page;

    type = gItems[gChars[who].items[slot]].value;
    if (type == 0) {
        DrawText("The wand has no apparent magical effect\r");
        return;
    }
    if (type != 5) {
        effect = gWandSpells[type];
        StartItemCast(who, effect, slot);
        return;
    }
    page = gPartyDir;
    x = StepCell(gPartyCell, page);
    y = StepCell(x, page);
    PlaySound(0x62);
    FlashViewDamage();
    if ((gWallTypeFlags[gMaze[y * 9 + (page ^ 2)]] & 4) && (gMaze[y * 9 + 4] & 7) == 0
            && (gMaze[x * 9 + 4] & 7) != 0) {
        obj = gMonsters;
        for (i = 0; i < 30; i++, obj++) {
            if (obj->cell == x) {
                MoveMonsterToCell(obj, y, -1);
                gViewDirty = 1;
            }
        }
    } else {
        DrawText("no effect.\r");
    }
}

/* 0BD2:1001: the "use the item in slot `slot`" action for character `who`. It returns early when
   MemberIs(who, 0x0D) refuses, and, when the slot maps to one of the first two carried slots
   (0BD2:1244), when CanUseHand refuses or the slot's m1E bit is set in mode 0. When the item type
   may not be worn it draws the character's name and " can not use this item.\r". Otherwise it
   switches on the item type's m0E low 7 bits and either draws a message, eats, uses the item
   through 0DE8:0763, arms 0EF3:21E0, dispatches through 0EF3:28ED or the overlay 5 entry
   UseItemPanel (ovl5:1CDD), applies the item effect (0BD2:0D2D) or uses it as a wand (0BD2:0EE8).
   Called by 0ACD:0415 and 0ACD:0D9D. */
/*@ 0BD2:1001 */ void UseItemSlot(int who, int slot)
{
    int item;
    int type;
    int firstTwo;
    int cls;
    int value;
    struct CharRec *rec;

    rec = &gChars[who];
    if (!MemberIs(who, 0x0D))
        return;
    item = rec->items[slot];
    firstTwo = GetHandOfSlot(slot);
    if (firstTwo != -1) {
        if (!CanUseHand(who, firstTwo))
            return;
        if (gPanelMode == 0 && (gChars[who].m1E & (1 << slot)))
            return;
    }
    if (!ItemClassAllowed(who, item)) {
        DrawText(rec->name);
        DrawText(" can not use this item.\r");
        return;
    }
    if (item == 0 && firstTwo == -1)
        return;
    type = gItems[item].type;
    cls = gItemTypes[type].m0E.word & 0x7F;
    switch (cls) {
    case 0:
    case 16:
        DrawText("This item automatically used when worn.\r");
        break;
    case 1:
    case 2:
    case 3:
        if (gPanelMode != 0)
            break;
        UseItem(who, slot, item);
        break;
    case 5:
    case 6:
        if (gPanelMode == 0)
            OpenSpellPanel(who, cls == 6);
        break;
    case 7:
        if (gMouseDisabled != 0)
            EatFood(who);
        break;
    case 4:
    case 8:
    case 12:
    case 13:
    case 15:
        DrawText("This item is not used in this way.\r");
        break;
    case 9:
        if (gPanelMode == 0) {
            value = gItems[item].value;
            StartItemCast(who, value, firstTwo);
        }
        break;
    case 10:
        if (gPanelMode == 0) {
            value = gItems[item].value + 0x19;
            StartItemCast(who, value, firstTwo);
        }
        break;
    case 11:
        UseItemPanel(gItems[item].value);
        break;
    case 14:
        ApplyItemEffect(who, slot);
        break;
    case 18:
        if (gPanelMode == 0)
            UseWandItem(who, slot);
        break;
    }
}

/* 0BD2:11FD: returns 1 when `item` is empty, otherwise the item-type byte gItemTypes[m02].m05 ANDed
   with gClassBaseBits[gChars[who].classIndex], cast to signed char. The result is nonzero when the character's
   class may use the item, so the callers use it as yes/no. Purpose unknown beyond that. */
/*@ 0BD2:11FD */ int ItemClassAllowed(int who, int item)
{
    int result;

    if (item != 0) {
        result = (signed char)(gItemTypes[gItems[item].type].m05 & gClassBaseBits[gChars[who].classIndex]);
    } else {
        result = 1;
    }
    return result;
}

/* 0BD2:1244: returns `slot` when it is 0 or 1, one of the two hands, otherwise -1. */
/*@ 0BD2:1244 */ int GetHandOfSlot(int slot)
{
    if (slot > 1)
        return -1;
    return slot;
}
