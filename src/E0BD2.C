/* Unit at EOB.EXE 0BD2:000E (4681 bytes, 22 functions): the inventory and item-use module. Its
   functions walk the item records at ds:8DD0 (struct ItemRecord, the same table src/E0D1C.C calls
   Entry14 at ds:8DD2) through the circular lists whose head is a word, and the character records at
   ds:F580 (struct CharRec, src/E0A72.C). It draws the item rows, uses and wears items, applies a
   status effect, and is reached from the action handlers of 0ACD and the object/effect units.
   Placeholder names remain where the evidence names the behaviour but not the game concept. */

/*@flags -ml -O -Z */

/* A 14-byte item record. src/E0D1C.C declares the same layout with its base at ds:8DD2 (Entry14);
   the code here computes the base at ds:8DD0 and uses the fields below. */
struct ItemRecord {
    char pad00[2];
    unsigned char flags;    /* +2 (ds:8DD2): bit 0x20 gates 0BD2:0AF2/0C8E, 0x40 is set by
                               0BD2:040E and tested by 0BD2:05B5; src/E07A9.C calls it flags */
    signed char icon;       /* +3 (ds:8DD3): the icon number (src/E07A9.C Entry14.icon) */
    signed char m02;        /* +4 (ds:8DD4): the item type (src/E0D1C.C Entry14.m02) */
    signed char m03;        /* +5 (ds:8DD5): compared with 01D0's key and 02B9's key */
    int m04;                /* +6 (ds:8DD6): the key; -1 when the record is free (Entry14.m04) */
    int m06;                /* +8 (ds:8DD8): one link of the circular list */
    int m08;                /* +0xa (ds:8DDA): the other link */
    signed char m0A;        /* +0xc (ds:8DDC): gFBC8 or -1, set by 01D0 */
    signed char m0B;        /* +0xd (ds:8DDD): the value 01D0/02B9 return (Entry14.m0B) */
};
extern struct ItemRecord gItems[]; /*@ ds:8DD0 */

/* A 16-byte item-type record at ds:A9F0; src/E0D1C.C views the same table as gA9F5, five bytes in. */
union ItemMask {
    unsigned word;          /* +0: the 16-bit class mask 0BD2:0AF2 tests */
    unsigned char byte;     /* +0: its low byte, which 0BD2:0C8E tests */
};
struct ItemType {
    union ItemMask m00;     /* +0 (ds:A9F0) */
    char pad02[3];
    signed char m05;        /* +5 (ds:A9F5): ANDed with g1231 by 0BD2:11FD */
    signed char m06;        /* +6 (ds:A9F6): src/E0D1C.C's gA9F5[].m01 */
    char pad07[7];
    union ItemMask m0E;     /* +0xE (ds:A9FE): bit 0x80 and the low 7 bits, read by 0BD2:00 and 05B5 */
};
extern struct ItemType gItemTypes[]; /*@ ds:A9F0 */

/* A 243-byte character record at ds:F580; only the fields this unit uses (src/E0A72.C). */
struct CharRec {
    char pad00[2];
    char name[0x0B];        /* +2: the character's name, NUL padded */
    char pad0D[0x1E - 0x0D];
    signed char m1E;        /* +1E */
    signed char m1F;        /* +1F */
    signed char m20;        /* +20: indexes g1231 in 0BD2:11FD */
    char pad21[0x77 - 0x21];
    int m77[27];            /* +77: 27 carried-item slots; slot 16 (+97) is also a list head */
    char padAD[0x97 - 0xAD];
    char m97[0xF3 - 0x97];  /* +97: the sub-table slot 16 heads */
};
extern struct CharRec gChars[]; /*@ ds:F580 */

/* An 18-byte effect record at ds:DD2C, 10 of them (src/E1414.C). */
struct DDRec {
    char active;            /* +0 */
    char pad01;
    int m02;                /* +2 */
    int m04;                /* +4 */
    char m06;               /* +6 */
    signed char m07;        /* +7 */
    int m08;                /* +8 */
    char m0A;               /* +0xa */
    char m0B;               /* +0xb */
    char m0C;               /* +0xc */
    unsigned char m0D;      /* +0xd */
    int (far *fn)(struct DDRec far *rec); /* +0xe */
};
extern struct DDRec gDD2C[]; /*@ ds:DD2C */

/* The 28-byte object records at ds:8A82 (src/E08E6.C). */
struct Obj28 {
    char pad00[2];
    int m02;                /* +2 */
    char pad04[0x16 - 0x04];
    char m16;               /* +0x16 */
};
extern struct Obj28 gObjs[]; /*@ ds:8A82 */

extern signed char gA99A; /*@ ds:A99A */    /* the selected character index (src/E0ACD.C) */
extern char gF51A; /*@ ds:F51A */           /* the selected row (src/E0D1C.C) */
extern char gFB44; /*@ ds:FB44 */           /* a display-mode byte (src/E0D1C.C) */
extern int gFBC8; /*@ ds:FBC8 */            /* the current action (src/E0D1C.C) */
extern int gFBCC; /*@ ds:FBCC */            /* a page number 0..3 (src/E0D1C.C) */
extern int gSoundVolume; /*@ ds:FBCA */     /* the global sound volume (src/E177D.C) */
extern int gMouseDisabled; /*@ ds:814A */   /* nonzero disables the mouse (src/E07A9.C) */
extern int g8115; /*@ ds:8115 */            /* nonzero while a status line is up (src/E082C.C) */
extern int g8102; /*@ ds:8102 */            /* nonzero: a colour row exists (src/E07A9.C) */
extern signed char g194B[]; /*@ ds:194B */  /* byte table (src/E0D1C.C) */
extern signed char g1231[]; /*@ ds:1231 */  /* byte table indexed by gChars[].m20 (src/E0DE8.C) */
extern int g1C8D[]; /*@ ds:1C8D */          /* per-row class mask (0BD2:0AF2/0C8E) */
extern signed char gF57E; /*@ ds:F57E */    /* a colour read by 0BD2:0917 (src/E0D1C.C) */
extern signed char gF53A; /*@ ds:F53A */    /* a colour read by 0BD2:0917 (src/E0D1C.C) */
extern signed char gFB4C; /*@ ds:FB4C */    /* a colour read by 0BD2:0917 (src/E0D1C.C) */
extern char gFileNameBuf[]; /*@ ds:FE6E */  /* the text buffer sprintf formats into (src/EOV2.C) */
extern void far *gSegDF4E; /*@ ds:DF4E */   /* slot 5's buffer segment (src/E07A9.C) */
extern char gDF4C; /*@ ds:DF4C */           /* set to 1 when an action took effect (src/E0ACD.C) */
extern char gB899[]; /*@ ds:B899 */         /* the nine-byte record list head (src/E0D1C.C) */
extern char gB894[]; /*@ ds:B894 */         /* the nine-byte records (src/E0D1C.C) */
extern void (far *gSaveRect)(int x, int y, int w, int h, void far *buf); /*@ ds:80AE */ /* E1BBE.ASM */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */ /* src/E082C.C */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int c1, int c2, int c3); /*@ 082C:09D5 */
extern void DrawRect(int x1, int y1, int x2, int y2, int colour); /*@ 082C:04C4 */
extern void PrintInt(int value, int a, int b, int c, int d); /*@ 1AA6:000F */
extern int g17CE[]; /*@ ds:17CE */          /* a per-character word (the item row's x) */
extern unsigned char g1804[]; /*@ ds:1804 */ /* a per-character byte (the item row's y) */
extern signed char g3875[]; /*@ ds:3875 */  /* a per-character byte (0, 1 or 2) */
extern signed char g3890[]; /*@ ds:3890 */  /* a three-byte table read by 0BD2:0917 */
extern signed char g38FA[]; /*@ ds:38FA */  /* a byte table indexed by an item's m0B in 0BD2:0EE8 */
extern void f0EF3_28ED(int a, int b, int c); /*@ 0EF3:28ED */
extern void f0EF3_06E7(void); /*@ 0EF3:06E7 */
extern void f08E6_0297(struct Obj28 far *obj, int a, int b); /*@ 08E6:0297 */

extern int f0D1C_02DC(void); /*@ 0D1C:02DC */
extern void SetMouseItem(int n); /*@ 07A9:010C */
extern void f1547_0991(int who); /*@ 1547:0991 */
extern void f0D1C_070F(int mode, int n, int x, int y); /*@ 0D1C:070F */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void ComputeArmourClass(int who); /*@ 0DE8:0D5E */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */

/* functions of this unit, forward (address order) */
void InsertListItem(void far *head, int key, int item, int sound);
int RemoveListItem(void far *head, int key, int target);
void DrawItemRow(int who, int slot, int one);
int f0BD2_0AF2(int item, int who, int row);
void UseSelectedItem(int slot);
void SwapSelectedItem(int who, int row);
int ItemClassAllowed(int who, int item);

/* functions of this unit, in address order (stubs not yet written) */
/*@ 0BD2:000E stub */ void f0BD2_000E(void) {}

/* 0BD2:0106: Purpose unknown: clears item slot `row` of character `who` (or the mouse's slot when
   `row` is -1), marks the item free (m04 = -1), redraws, and returns the item's m0B. */
/*@ 0BD2:0106 */ int f0BD2_0106(int who, int row)
{
    int item;

    if (row == -1)
        item = f0D1C_02DC();
    else
        item = gChars[who].m77[row];
    gItems[item].m04 = -1;
    if (row == -1) {
        SetMouseItem(0);
    } else {
        gChars[who].m77[row] = 0;
        if (gFB44 == 1)
            DrawItemRow(row, 1, 0);
        else if (gFB44 == 0)
            f1547_0991(who);
    }
    return gItems[item].m0B;
}

/* 0BD2:019F: refreshes the current row: in mode 1 through 0BD2:0BA0 with the selected row, in any
   mode other than 2 through 0BD2:0C8E with the selected character and row. Purpose unknown. */
/*@ 0BD2:019F */ void RedrawSelectedRow(void)
{
    if (gFB44 == 1)
        UseSelectedItem(gF51A);
    else if (gFB44 != 2)
        SwapSelectedItem(gA99A, gF51A);
}

/* 0BD2:01D0: inserts the item `item` at the head of the circular list `head`, stamping it with the
   key `key` and the sound `sound` (m0A becomes gFBC8 or -1 by the key's sign). DRAFT: our build
   preloads key into DI (`mov di,[bp+0xa]`) where the original loads item into SI first; the rest of
   the body follows, so it is a register-allocation difference, not a source one. */
/*@ 0BD2:01D0 draft */ void InsertListItem(void far *head, int key, int item, int sound)
{
    struct ItemRecord *rec;
    struct ItemRecord *cur;
    struct ItemRecord *next;

    if (item == 0)
        return;
    rec = &gItems[item];
    if (*(int far *)head == 0) {
        *(int far *)head = item;
        rec->m06 = item;
        rec->m08 = item;
        rec->m03 = sound;
        rec->m04 = key;
        rec->m0A = key >= 0 ? gFBC8 : -1;
    } else {
        cur = &gItems[*(int far *)head];
        next = &gItems[cur->m06];
        rec->m04 = key;
        rec->m03 = sound;
        rec->m0A = key >= 0 ? gFBC8 : -1;
        rec->m08 = next->m08;
        rec->m06 = cur->m06;
        next->m08 = item;
        cur->m06 = item;
        *(int far *)head = item;
    }
}

/* 0BD2:02B9: removes from the circular list `head` the first item whose m03 equals `key` (when
   `target` is -1) or that equals `target`, unlinks it, and returns the item; 0 when none matches.
   DRAFT: our build keeps the loop counter in CX and `next` in DI, the original the counter in DI and
   `next` in CX; same instructions otherwise. */
/*@ 0BD2:02B9 draft */ int RemoveListItem(void far *head, int key, int target)
{
    int start;
    int item;
    int i;
    int link;
    int next;
    struct ItemRecord *rec;

    item = *(int far *)head;
    if (item == 0)
        return 0;
    i = 0;
    start = item;
    while (item != start || i == 0) {
        rec = &gItems[item];
        if (target == -1) {
            if (rec->m03 == key)
                goto found;
        }
        if (target != item)
            goto next;
found:
        link = rec->m06;
        next = rec->m08;
        gItems[link].m08 = next;
        gItems[next].m06 = link;
        rec->m08 = 0;
        rec->m06 = 0;
        rec->m04 = 0;
        rec->m0A = 0;
        if (*(int far *)head == item) {
            *(int far *)head = next;
            if (*(int far *)head == item)
                *(int far *)head = 0;
        }
        return item;
next:
        i++;
        item = rec->m08;
    }
    return 0;
}

/* 0BD2:0385: walks the circular list `head`. With `target` -1 it matches an item whose m02 is
   `type` (or all when `type` is -1), otherwise the item `target` itself; returns the first match's
   index when `count` is 0, otherwise the number of matches. Purpose unknown. DRAFT: the original's
   first `return 0` is a `jmp` to the shared `xor ax,ax` tail (3 bytes); ours materialises it (5). */
/*@ 0BD2:0385 draft */ int ScanListItem(void far *head, int target, int type, int count)
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
        if (target == -1) {
            if (type == -1 || rec->m02 == type) {
                if (count)
                    n++;
                else
                    return cur;
            }
        } else if (type == -1 && cur == target) {
            if (count)
                n++;
            else
                return cur;
        }
        first = 0;
        cur = rec->m08;
    }
    if (count == 0)
        return 0;
    return n;
}

/* 0BD2:040E: sets bit 0x40 of the flags of every item in the circular list `head`. */
/*@ 0BD2:040E */ void MarkListItems(void far *head)
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
        item = rec->m08;
    }
}

/* 0BD2:0458: queues item `item` at head gB899 + `vol` * 9 through 0BD2:01D0, then, while the item
   still links onward, makes that head the item's next. Purpose unknown. */
/*@ 0BD2:0458 */ void PushListItem(int vol, int item, int value)
{
    InsertListItem((void far *)(gB899 + vol * 9), vol, item, value);
    if (gItems[item].m08 != item)
        *(int *)(gB899 + vol * 9) = gItems[item].m08;
}

/* 0BD2:04AA: drains the list at gB899 + `vol` * 9. Items whose m02 is `type` (or all when `type` is
   -1) are freed (m04 = -1, m0A = 0); the others are re-queued through 0BD2:01D0 in their original
   order. Purpose unknown. DRAFT: our build keeps `count` in DI, the original on the stack with DI as
   a scratch (`mov di,si`); two bytes apart. */
/*@ 0BD2:04AA draft */ void DrainListItem(int vol, int type)
{
    int start;
    int item;
    int next;
    int count;

    item = *(int *)(gB899 + vol * 9);
    if (item == 0)
        return;
    *(int *)(gB899 + vol * 9) = 0;
    count = 0;
    start = item;
    while (item != start || count == 0) {
        if (type == -1 || gItems[item].m02 == type) {
            gItems[item].m04 = -1;
            gItems[item].m0A = 0;
            next = gItems[item].m08;
            gItems[item].m08 = 0;
            gItems[item].m06 = 0;
        } else {
            next = gItems[item].m08;
            gItems[item].m08 = 0;
            gItems[item].m06 = 0;
            InsertListItem((void far *)(gB899 + vol * 9), vol, item, gItems[item].m03);
        }
        item = next;
        count++;
    }
}

/*@ 0BD2:05B5 stub */ void f0BD2_05B5(void) {}

/* 0BD2:07DE: reserves one of the ten effect records at ds:DD2C (returning 0 when all are live),
   queues `item` through 0BD2:01D0 with sound `kind | 4`, fills the record from the arguments and
   the kind-7 sound, and returns 1. */
/*@ 0BD2:07DE */ int AddDDEffect(int who, int item, int volume, int kind, int page, int type)
{
    struct DDRec *rec;
    int i;

    rec = gDD2C;
    for (i = 0; i < 10; i++, rec++) {
        if (rec->active == 0)
            break;
    }
    if (i == 10)
        return 0;
    InsertListItem((void far *)(gB899 + volume * 9), volume, item, kind | 4);
    rec->active = 1;
    rec->m02 = 1;
    rec->m0C = 0;
    rec->m07 = page;
    rec->m0D = 0xC;
    rec->m04 = volume;
    rec->m06 = kind;
    rec->m08 = item;
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

/* 0BD2:089E: when item record `item` is live (m04 != -1), finds the first free record from index 1
   on, copies `item`'s 14 bytes into it with Unpack0, and returns the new index; 0 when none is
   free. Purpose unknown. */
/*@ 0BD2:089E */ int CopyItemToFreeSlot(int item)
{
    struct ItemRecord *cand;
    struct ItemRecord *src;
    int slot;
    int found;

    src = &gItems[item];
    if (src->m04 == -1)
        return 0;
    slot = 1;
    found = 0;
    cand = &gItems[1];
    while (slot < 500) {
        if (cand->m04 == -1) {
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

/* 0BD2:0917: draws the item row of character `who`: a filled panel and rectangle at x g17CE[who]
   and y g1804[who] sized by g3890[g3875[who]], and, when `slot` is nonzero and the row is the
   party's, the item icon. `one` selects the input mode and is stored in g8115 for the duration.
   Called by 0D1C and 0ACD. Purpose unknown. */
/*@ 0BD2:0917 */ void DrawItemRow(int who, int slot, int one)
{
    int y;
    int item;
    int count;
    int saved;
    int x;
    int kind;

    kind = slot;
    saved = g8115;
    g8115 = one;
    x = g17CE[who];
    y = g1804[who];
    item = gChars[gA99A].m77[who];
    if (kind != 0) {
        kind = g3875[who];
        if (g8102 == 0)
            DrawFilledPanel(x - 1, y - 1, x + g3890[kind], y + g3890[kind], 3, 1,
                    kind != 1 ? gF57E : -1);
        else
            DrawFilledPanel(x - 1, y - 1, x + g3890[kind], y + g3890[kind], gFB4C, gF53A,
                    kind != 1 ? gF57E : -1);
        if (kind == 1) {
            gFillRect(0xE3, 0x41, 0xEE, 0x45, 0xC);
            count = ScanListItem(&gChars[gA99A].m77[who], -1, -1, 1);
            if (count < 10)
                x = 0xE9;
            else
                x = 0xE3;
            PrintInt(count, x, 0x41, 0xF, 0);
            x = g17CE[who];
        }
        if (gMouseDisabled && gF51A == who)
            DrawRect(x - 1, y - 1, x + g3890[kind], y + g3890[kind], 0xF);
    }
    if (item != 0 && g3875[who] != 1) {
        if (g3875[who] != 2)
            f0D1C_070F(one, item, x, y);
        else
            f0D1C_070F(one, item, x - 4, y - 4);
    }
    g8115 = saved;
}

/* 0BD2:0ABE: when `one` is set, saves the 18x168 item sheet rectangle through the save-rect hook,
   then redraws the selected character's row. */
/*@ 0BD2:0ABE */ void RedrawRowArea(int one)
{
    if (one)
        gSaveRect(0x16, 0, 0x12, 0xA8, gSegDF4E);
    f1547_0991(gA99A);
}

/* 0BD2:0AF2: Purpose unknown: when `row` is 0x11 and `item` is carried, checks that character `who`
   may wear the item type (0BD2:11FD); on failure prints why. Returns 0 when the worn item's flags
   have 0x20, 1 otherwise, printing a message when the row's class mask does not cover the item.
   DRAFT: TCC tail-merges the two DrawText calls in opposite directions (ours jumps the first
   message forward, the original jumps the second back to the first). */
/*@ 0BD2:0AF2 draft */ int f0BD2_0AF2(int item, int who, int row)
{
    int mask;

    if (row == 0x11 && item != 0) {
        if (!ItemClassAllowed(who, item)) {
            sprintf(gFileNameBuf, "%s can't wear that type of armor.\r", gChars[who].name);
            DrawText(gFileNameBuf);
            return 0;
        }
    }
    if (gItems[gChars[who].m77[row]].flags & 0x20)
        return 0;
    if (item != 0)
        mask = gItemTypes[gItems[item].m02].m00.word;
    else
        mask = -1;
    if (g1C8D[row] & mask)
        return 1;
    DrawText("You cant put that item there.\r");
    return 0;
}
/* 0BD2:0BA0: uses the item in slot `slot` of the selected character through 0BD2:0AF2. On success
   it either installs the item (slot 0x10) or swaps the mouse's item, redraws the row and recomputes
   the armour class. Purpose unknown. */
/*@ 0BD2:0BA0 */ void UseSelectedItem(int slot)
{
    int saved;
    int item;

    saved = gChars[gA99A].m77[slot];
    item = f0D1C_02DC();
    if (!f0BD2_0AF2(item, gA99A, slot))
        return;
    if (slot != 0x10) {
        SetMouseItem(saved);
        gChars[gA99A].m77[slot] = item;
        DrawItemRow(slot, 1, 0);
        ComputeArmourClass(gA99A);
    } else if (item == 0) {
        saved = RemoveListItem(&gChars[gA99A].m97[0], 0, -1);
        DrawItemRow(slot, 1, 0);
        SetMouseItem(saved);
    } else {
        InsertListItem((void far *)&gChars[gA99A].m97[0], 0xFFFE, item, 0);
        DrawItemRow(slot, 1, 0);
        SetMouseItem(0);
    }
}

/* 0BD2:0C8E: swaps the item in slot `row` of character `who` with the mouse's item when the row's
   class mask covers it, redrawing and recomputing the armour class. Purpose unknown. */
/*@ 0BD2:0C8E */ void SwapSelectedItem(int who, int row)
{
    int item;
    int slot;
    int type;
    unsigned char mask;

    item = gChars[who].m77[row];
    if (gItems[item].flags & 0x20)
        return;
    slot = f0D1C_02DC();
    type = gItems[slot].m02;
    if (slot != 0)
        mask = gItemTypes[type].m00.byte;
    else
        mask = 0xFF;
    if (g1C8D[row] & mask) {
        SetMouseItem(item);
        gChars[who].m77[row] = slot;
        f1547_0991(who);
    }
    ComputeArmourClass(who);
}

/*@ 0BD2:0D2D stub */ void f0BD2_0D2D(void) {}

/* 0BD2:0EE8: uses the wand in slot `slot` of character `who`. When the item's m0B is 0 it reports
   no effect; when it is not 5 it dispatches through 0EF3:28ED with g38FA[m0B]; for 5 it plays the
   wand sound, and when the surroundings pass the g194B/gB894 tests, activates every gObjs record
   whose m02 matches the page index, otherwise reports no effect. DRAFT: our build keeps the loop
   counter in a register (0xa-byte frame), the original on the stack (0xc), and uses the same
   instruction sequence otherwise. */
/*@ 0BD2:0EE8 draft */ void UseWandItem(int who, int slot)
{
    int effect;
    int x;
    int y;
    int i;
    struct Obj28 *obj;
    int type;

    type = gItems[gChars[who].m77[slot]].m0B;
    if (type == 0) {
        DrawText("The wand has no apparent magical effect\r");
        return;
    }
    if (type != 5) {
        effect = g38FA[type];
        f0EF3_28ED(who, effect, slot);
        return;
    }
    x = f0D1C_00DA(gSoundVolume, gFBCC);
    y = f0D1C_00DA(x, gFBCC);
    PlaySound(0x62);
    f0EF3_06E7();
    if ((g194B[gB894[y * 9 + (gFBCC ^ 2)]] & 4) && !(gB894[y * 9 + 4] & 7)
            && (gB894[x * 9 + 4] & 7)) {
        obj = gObjs;
        for (i = 0; i < 30; i++, obj++) {
            if (obj->m02 == x) {
                f08E6_0297(obj, y, -1);
                gDF4C = 1;
            }
        }
    } else {
        DrawText("no effect.\r");
    }
}

/*@ 0BD2:1001 stub */ void f0BD2_1001(void) {}

/* 0BD2:11FD: returns 1 when `item` is empty, otherwise the item-type byte gItemTypes[m02].m05 ANDed
   with g1231[gChars[who].m20]. Purpose unknown. */
/*@ 0BD2:11FD */ int ItemClassAllowed(int who, int item)
{
    int result;

    if (item != 0) {
        result = (signed char)(gItemTypes[gItems[item].m02].m05 & g1231[gChars[who].m20]);
    } else {
        result = 1;
    }
    return result;
}

/* 0BD2:1244: returns `slot` when it is 0 or 1, otherwise -1. Purpose unknown. */
/*@ 0BD2:1244 */ int FirstTwoSlots(int slot)
{
    if (slot > 1)
        return -1;
    return slot;
}
