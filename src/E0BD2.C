/* Unit at EOB.EXE 0BD2:000E (4681 bytes, 22 functions): the inventory and item-use module. Its
   functions walk the item records at ds:8DD0 (struct ItemRecord, the same table src/E0D1C.C calls
   Entry14 at ds:8DD2) through the circular lists whose head is a word, and the character records at
   ds:F580 (struct CharRec, src/E0A72.C). It draws the item rows, uses and wears items, applies a
   status effect, and is reached from the action handlers of 0ACD and the object/effect units.
   Placeholder names remain where the evidence names the behaviour but not the game concept. */

/*@flags -ml -O -Z -v */

/* A 14-byte item record. src/E0D1C.C declares the same layout with its base at ds:8DD2 (Entry14);
   the code here computes the base at ds:8DD0 and uses the fields below. */
struct ItemRecord {
    signed char m00;        /* +0 (ds:8DD0): indexes the item-name table at segment 0x1F28 */
    signed char m01;        /* +1 (ds:8DD1): the second name index */
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

/* A 243-byte character record at ds:F580; only the fields this unit uses (src/E0A72.C, src/E0DE8.C). */
struct CharRec {
    char pad00;
    unsigned char m01;      /* +01: status bits, tested by 0BD2:0D2D */
    char name[0x0B];        /* +02: the character's name, NUL padded */
    signed char m0D;        /* +0D: the strength score, set to 0x16 by 0BD2:0D2D (src/E0A72.C) */
    char pad0E;
    signed char m0F;        /* +0F: the exceptional strength percentile, set to 0 by 0BD2:0D2D */
    char pad10[0x1E - 0x10];
    signed char m1E;        /* +1E */
    signed char m1F;        /* +1F */
    signed char m20;        /* +20: indexes g1231 in 0BD2:11FD */
    char pad21[0x23 - 0x21];
    signed char m23;        /* +23: set to 100 by 0BD2:0D2D */
    char pad24[0x77 - 0x24];
    int m77[27];            /* +77: 27 carried-item slots; slot 16 (+97) is also a list head */
    char padAD[0xE3 - 0xAD];
    long mE3;               /* +E3: effect flags, changed by 0BD2:0D2D */
    char padE7[0xF3 - 0xE7];
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
    char pad17[0x1C - 0x17];
};
extern struct Obj28 gObjs[]; /*@ ds:8A82 */

extern signed char gA99A; /*@ ds:A99A */    /* the selected character index (src/E0ACD.C) */
extern char gF51A; /*@ ds:F51A */           /* the selected row (src/E0D1C.C) */
extern char gFB44; /*@ ds:FB44 */           /* a display-mode byte (src/E0D1C.C) */
extern int gFBC8; /*@ ds:FBC8 */            /* the current action (src/E0D1C.C) */
extern int gFBCC; /*@ ds:FBCC */            /* a page number 0..3 (src/E0D1C.C) */
extern int gPartyCell; /*@ ds:FBCA */     /* the global sound volume (src/E177D.C) */
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
extern void StartPanelAction(int a, int b, int c); /*@ 0EF3:28ED */
extern void f0EF3_06E7(void); /*@ 0EF3:06E7 */
extern void FlashPartyMembers(int who); /*@ 0EF3:0665 */
extern void ClearPoisonEffect(int who); /*@ 0EF3:1CE8 */
extern void MoveObjectToId(struct Obj28 far *obj, int a, int b); /*@ 08E6:0297 */
extern void ApplyCharEffect(char who, long interval, char value, char firstEmpty); /*@ 1414:05ED */
extern void HealCharacter(int who, int amount); /*@ 0DE8:0ADB */
extern void ApplyStatusEffect(int who, int mask, char far *text, int attack, int count, int value, int flag); /*@ 0DE8:101A */
extern char far *SkipStrings(char far *text, int count); /*@ 0D1C:07A7 */
extern char g3893[]; /*@ ds:3893 */        /* the "feels ..." string table (0BD2:0D2D) */
extern char far *g1278[]; /*@ ds:1278 */   /* far item-name pointers indexed by m0B (0BD2:05B5) */
extern char far *g12E0[]; /*@ ds:12E0 */   /* far item-name pointers indexed by m0B (0BD2:05B5) */
extern char g37B4[]; /*@ ds:37B4 */        /* a string table SkipStrings walks (0BD2:05B5) */
extern char g37E1[]; /*@ ds:37E1 */        /* a string table SkipStrings walks (0BD2:05B5) */
extern char g3837[]; /*@ ds:3837 */        /* a string table SkipStrings walks (0BD2:05B5) */

extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern int f0D1C_02DC(void); /*@ 0D1C:02DC */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern int f0D1C_07DB(int who, int flag); /*@ 0D1C:07DB */
extern void UseItem(int who, int slot, int field); /*@ 0DE8:0763 */
extern void EatFood(int index); /*@ 0ACD:07E0 */
extern void f0EF3_21E0(int who, int flag); /*@ 0EF3:21E0 */
extern void fOvl5_1CDD(int x); /*@ 211E:0093 */  /* overlay 5 entry, ovl5:1CDD */
extern void SetMouseItem(int n); /*@ 07A9:010C */
extern void RedrawMember(int who); /*@ 1547:0991 */
extern void f0D1C_070F(int mode, int n, int x, int y); /*@ 0D1C:070F */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void ComputeArmourClass(int who); /*@ 0DE8:0D5E */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern char *strcpy(char far *dest, char far *src); /*@ 0000:67CD */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */

/* functions of this unit, forward (address order) */
void InsertListItem(void far *head, int key, int item, int sound);
int RemoveListItem(void far *head, int key, int target);
void DrawItemRow(int who, int slot, int one);
int f0BD2_0AF2(int item, int who, int row);
void UseSelectedItem(int slot);
void SwapSelectedItem(int who, int row);
int ItemClassAllowed(int who, int item);
void DrawItemName(int item);

/* functions of this unit, in address order (stubs not yet written) */
/* 0BD2:000E: the "lost item" message. Rolls 1d27 for a starting slot, then scans the 27 item slots
   of character `who` from there, wrapping, for the first item whose type flags have 0x80. It clears
   the slot, prints "<name> has lost her/his " (the possessive by m1F bit 0), draws the item's name
   through 0BD2:05B5 and ".\r", and redraws the character's row. */
/*@ 0BD2:000E */ void ReportLostItem(int who)
{
    int i;
    int item;
    int roll;

    roll = RollDice(1, 0x1b, -1);
    for (i = 0; i < 0x1b; i++) {
        item = gChars[who].m77[roll];
        if (item != 0) {
            if (gItemTypes[gItems[item].m02].m0E.word & 0x80) {
                gChars[who].m77[roll] = 0;
                if (gChars[who].m1F & 1)
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
            RedrawMember(who);
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
   key `key` and the sound `sound` (m0A becomes gFBC8 or -1 by the key's sign). BLOCKED: compiler
   register allocation. The original's frame is 0xc bytes (rec, next, cur): `item` is the SI
   register variable (0BD2:01D8 `mov si,[bp+0xc]`), DI is a temporary holding cur->m06 (0BD2:0251
   to 029B), and `key` stays in memory and is reloaded in each branch (0BD2:0212, 0268, 0276). Our
   build promotes `key`: bare it takes DI and the cur->m06 temporary falls to AX and reloads; with a
   `link` local taking DI it caches `key` in CX. The branch-1 `mov ax,si` (0BD2:0201) is the chained
   assignment `rec->m08 = rec->m06 = item;`. Many source forms tried (see re/symbols.toml) still
   promote or cache key. */
/*@ 0BD2:01D0 draft */ void InsertListItem(void far *head, int key, int item, int sound)
{
    struct ItemRecord *rec;
    struct ItemRecord *next;
    struct ItemRecord *cur;

    if (item == 0)
        return;
    rec = &gItems[item];
    if (*(int far *)head == 0) {
        *(int far *)head = item;
        rec->m08 = rec->m06 = item;
        rec->m03 = sound;
        rec->m04 = key;
        if (key >= 0)
            rec->m0A = gFBC8;
        else
            rec->m0A = -1;
    } else {
        cur = &gItems[*(int far *)head];
        next = &gItems[cur->m06];
        rec->m04 = key;
        rec->m03 = sound;
        if (key >= 0)
            rec->m0A = gFBC8;
        else
            rec->m0A = -1;
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
        if (*(int far *)head == item)
            *(int far *)head = next;
        if (*(int far *)head == item)
            *(int far *)head = 0;
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
        if ((target == -1 && type == -1) || (target == -1 && rec->m02 == type)
                || (type == -1 && cur == target)) {
            if (count)
                n++;
            else
                return cur;
        }
        first = 0;
        cur = rec->m08;
    }
    if (count != 0)
        return n;
    return 0;
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
/*@ 0BD2:04AA */ void DrainListItem(int vol, int type)
{
    int start;
    int item;
    int count;
    int saved;

    item = *(int *)(gB899 + vol * 9);
    if (item == 0)
        return;
    *(int *)(gB899 + vol * 9) = 0;
    count = 0;
    start = item;
    while (item != start || count == 0) {
        if (gItems[item].m02 == type || type == -1) {
            gItems[item].m04 = -1;
            gItems[item].m0A = 0;
            saved = item;
            item = gItems[item].m08;
            gItems[saved].m08 = 0;
            gItems[saved].m06 = 0;
        } else {
            saved = item;
            item = gItems[item].m08;
            gItems[saved].m08 = 0;
            gItems[saved].m06 = 0;
            InsertListItem((void far *)(gB899 + vol * 9), vol, saved, gItems[saved].m03);
        }
        count++;
    }
}

/* 0BD2:05B5: draws item `item`'s name into the text buffer. The record's first two bytes index two
   name tables at segment 0x1F28, 0x23 bytes apart. When the record's flags have 0x40 and the second
   name is empty, the item type's m0E low 7 bits select the name: the plain types use the first name
   with "%s +%d" or "Cursed %s %d", and types 9, 10, 16, 14 and 18 use "Mage Scroll", "Cleric
   Scroll", "Ring", "Potion" or "Wand" joined with " of " to a per-m0B far pointer (ds:1278,
   ds:12E0) or a SkipStrings walk (ds:37B4, ds:37E1, ds:3837). Otherwise the second (or the first)
   name is copied whole. The buffer is drawn with DrawText. Called by 0BD2:000E. The names chosen by
   the item type's m0E low 7 bits are the ones the jump table at 0BD2:07B8 shows (0..3, 9, 10, 14,
   16, 18; all the rest fall to the default). Case 0..3's name selection must be written as an
   `if (m0B != 0) { if (m0B < 0) ... else ... } else strcpy(...)`, or the compiler folds its
   `strcpy(buf, firstName)` into the default case's copy. */
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
    firstName = (char far *)0x1F280000L + rec->m00 * 0x23;
    secondName = (char far *)0x1F280000L + rec->m01 * 0x23;
    cls = gItemTypes[rec->m02].m0E.byte & 0x7F;
    m0B = rec->m0B;
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
            str2 = g1278[m0B];
            break;
        case 10:
            str1 = "Cleric Scroll";
            str2 = g12E0[m0B];
            break;
        case 16:
            str1 = "Ring";
            str2 = SkipStrings(g37B4, m0B);
            break;
        case 14:
            str1 = "Potion";
            str2 = SkipStrings(g37E1, m0B);
            break;
        case 18:
            str1 = "Wand";
            str2 = SkipStrings(g3837, m0B);
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
    RedrawMember(gA99A);
}

/* 0BD2:0AF2: Purpose unknown: when `row` is 0x11 and `item` is carried, checks that character `who`
   may wear the item type (0BD2:11FD); on failure prints why. Returns 0 when the worn item's flags
   have 0x20, 1 otherwise, printing a message when the row's class mask does not cover the item.
   DRAFT: TCC tail-merges the two DrawText calls in opposite directions (ours jumps the first
   message forward, the original jumps the second back to the first). */
/*@ 0BD2:0AF2 */ int f0BD2_0AF2(int item, int who, int row)
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
    if (!(g1C8D[row] & mask)) {
        DrawText("You cant put that item there.\r");
        return 0;
    }
    return 1;
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
        saved = RemoveListItem(&gChars[gA99A].m77[16], 0, -1);
        DrawItemRow(slot, 1, 0);
        SetMouseItem(saved);
    } else {
        InsertListItem((void far *)&gChars[gA99A].m77[16], 0xFFFE, item, 0);
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
        RedrawMember(who);
    }
    ComputeArmourClass(who);
}

/* 0BD2:0D2D: applies the effect of the item just taken from slot `slot` of character `who`. It
   clears the slot through 0BD2:0106 (the item's m0B selects the effect), plays sound 0x0A and
   dispatches on m0B: 1 restores the strength score to 22 and heals, 2 and 3 heal by a dice roll,
   4 poisons through 0DE8:101A, 5 sets m23 to 100, 6 and 8 run the 0EF3 helpers, 7 sets effect
   flag 4. After the switch it announces "%s feels <effect>!\r" with the effect name from the
   ds:3893 table, except for effect 4 when m01 bit 1 is set. Called by 0BD2:1001. This unit is
   compiled with `-v` (source-level debugging), which keeps `case 5` and `case 7`'s
   `RedrawMember(who)` calls
   apart where `-Z` alone cross-jumps them. */
/*@ 0BD2:0D2D */ void ApplyItemEffect(int who, int slot)
{
    struct CharRec *rec;
    int kind;

    rec = &gChars[who];
    kind = f0BD2_0106(who, slot);
    PlaySound(0xA);
    switch (kind) {
    case 1:
        FlashPartyMembers(who);
        rec->m0D = 0x16;
        rec->m0F = 0;
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
        rec->mE3 &= ~0x80;
        break;
    case 5:
        FlashPartyMembers(who);
        rec->m23 = 0x64;
        if (gFB44 != 0)
            RedrawMember(who);
        break;
    case 6:
        FlashPartyMembers(who);
        break;
    case 7:
        FlashPartyMembers(who);
        rec->mE3 |= 4;
        RedrawMember(who);
        break;
    case 8:
        FlashPartyMembers(who);
        ClearPoisonEffect(who);
        break;
    }
    if (kind - 1 != 3 || (rec->m01 & 2) == 0) {
        sprintf(gFileNameBuf, "%s feels %s!\r", rec->name, SkipStrings(g3893, kind));
        DrawText(gFileNameBuf);
    }
}

/* 0BD2:0EE8: uses the wand in slot `slot` of character `who`. When the item's m0B is 0 it reports
   no effect; when it is not 5 it dispatches through 0EF3:28ED with g38FA[m0B]; for 5 it plays the
   wand sound, and when the surroundings pass the g194B/gB894 tests, activates every gObjs record
   whose m02 matches the page index, otherwise reports no effect. */
/*@ 0BD2:0EE8 */ void UseWandItem(int who, int slot)
{
    int effect;
    int x;
    int y;
    int i;
    struct Obj28 *obj;
    int type;
    int page;

    type = gItems[gChars[who].m77[slot]].m0B;
    if (type == 0) {
        DrawText("The wand has no apparent magical effect\r");
        return;
    }
    if (type != 5) {
        effect = g38FA[type];
        StartPanelAction(who, effect, slot);
        return;
    }
    page = gFBCC;
    x = f0D1C_00DA(gPartyCell, page);
    y = f0D1C_00DA(x, page);
    PlaySound(0x62);
    f0EF3_06E7();
    if ((g194B[gB894[y * 9 + (page ^ 2)]] & 4) && (gB894[y * 9 + 4] & 7) == 0
            && (gB894[x * 9 + 4] & 7) != 0) {
        obj = gObjs;
        for (i = 0; i < 30; i++, obj++) {
            if (obj->m02 == x) {
                MoveObjectToId(obj, y, -1);
                gDF4C = 1;
            }
        }
    } else {
        DrawText("no effect.\r");
    }
}

/* 0BD2:1001: the "use the item in slot `slot`" action for character `who`. It returns early when
   f0D1C_0670(who, 0x0D) refuses, and, when the slot maps to one of the first two carried slots
   (0BD2:1244), when f0D1C_07DB refuses or the slot's m1E bit is set in mode 0. When the item type
   may not be worn it draws the character's name and " can not use this item.\r". Otherwise it
   switches on the item type's m0E low 7 bits and either draws a message, eats, uses the item
   through 0DE8:0763, arms 0EF3:21E0, dispatches through 0EF3:28ED or the overlay 5 entry, applies
   the item effect (0BD2:0D2D) or uses it as a wand (0BD2:0EE8). Called by 0ACD:0415 and 0ACD:0D9D.
   The `case` bodies are in the source order the jump table shows (0/16, 1/2/3, 5/6, 7,
   4/8/12/13/15, 9, 10, 11, 14, 18); compiling the unit through the `-v` source-level-debug
   switch keeps the per-case `pop cx; pop cx` call cleanups apart the way the original does. */
/*@ 0BD2:1001 */ void UseItemSlot(int who, int slot)
{
    int item;
    int type;
    int firstTwo;
    int cls;
    int value;
    struct CharRec *rec;

    rec = &gChars[who];
    if (!f0D1C_0670(who, 0x0D))
        return;
    item = rec->m77[slot];
    firstTwo = FirstTwoSlots(slot);
    if (firstTwo != -1) {
        if (!f0D1C_07DB(who, firstTwo))
            return;
        if (gFB44 == 0 && (gChars[who].m1E & (1 << slot)))
            return;
    }
    if (!ItemClassAllowed(who, item)) {
        DrawText(rec->name);
        DrawText(" can not use this item.\r");
        return;
    }
    if (item == 0 && firstTwo == -1)
        return;
    type = gItems[item].m02;
    cls = gItemTypes[type].m0E.word & 0x7F;
    switch (cls) {
    case 0:
    case 16:
        DrawText("This item automatically used when worn.\r");
        break;
    case 1:
    case 2:
    case 3:
        if (gFB44 != 0)
            break;
        UseItem(who, slot, item);
        break;
    case 5:
    case 6:
        if (gFB44 == 0)
            f0EF3_21E0(who, cls == 6);
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
        if (gFB44 == 0) {
            value = gItems[item].m0B;
            StartPanelAction(who, value, firstTwo);
        }
        break;
    case 10:
        if (gFB44 == 0) {
            value = gItems[item].m0B + 0x19;
            StartPanelAction(who, value, firstTwo);
        }
        break;
    case 11:
        fOvl5_1CDD(gItems[item].m0B);
        break;
    case 14:
        ApplyItemEffect(who, slot);
        break;
    case 18:
        if (gFB44 == 0)
            UseWandItem(who, slot);
        break;
    }
}

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
