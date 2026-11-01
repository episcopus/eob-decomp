/* Unit at EOB.EXE 0D1C:0006 (3265 bytes, 26 functions): the party-status and character-effect
   module. It walks the six 243-byte character records at ds:F580 (struct CharRec, src/E0A72.C),
   the 28-byte object records at ds:8A82 (struct Obj28, src/E08E6.C) and a 15-word table at
   ds:A9C8, and it draws the party rows through the rectangle and region front ends. Several
   helpers are pulled in from the character unit (0A72), the object unit (08E6) and the resident
   drawing code. Most meanings are not established, so most function names say what a function
   does rather than what the game calls it. */

/*@flags -ml -O -Z */

struct CharRec {            /* 243-byte (0xF3) character records at ds:F580; see src/E0A72.C */
    char pad00;
    signed char m01;        /* +01 */
    char pad02[0x17 - 0x02];
    signed char m17;        /* +17 */
    char pad18[0x1B - 0x18];
    signed char m1B;        /* +1B */
    char pad1C[0x1F - 0x1C];
    signed char m1F;        /* +1F */
    signed char m20;        /* +20 */
    char pad21[3];
    signed char m24;        /* +24 */
    char pad25[0x77 - 0x25];
    int m77[27];            /* +77: 27 word slots; slot 16 (+97) is also read on its own */
    char padAD[0xF3 - 0xAD];
};

extern int gA9C8[]; /*@ ds:A9C8 */       /* a 15-word table searched by 0D1C:0006 */
extern char gDF4C; /*@ ds:DF4C */        /* set to 1 when 0D1C:0006 finds the value */
extern signed char gA99A; /*@ ds:A99A */ /* the selected character index (src/E0ACD.C) */
extern char gF51A; /*@ ds:F51A */        /* the selected row (src/E0ACD.C) */
extern struct CharRec gChars[]; /*@ ds:F580 */
extern signed char g1790[]; /*@ ds:1790 */  /* byte table indexed by the second argument of 0D1C:00DA */
extern signed char g547E[]; /*@ ds:547E */  /* byte table indexed by the second argument of 0D1C:0080 */
extern char gB894[]; /*@ ds:B894 */         /* nine-byte records; +4 (ds:B898) is tested by 0D1C:0080 and +5 (ds:B899) is a list head used by 0D1C:035A */
extern signed char g194B[]; /*@ ds:194B */  /* byte table read by 0D1C:0080 */
extern signed char g1A1B[]; /*@ ds:1A1B */  /* per-page sound table read by 0D1C:035A */
extern int gSoundVolume; /*@ ds:FBCA */     /* the global sound volume (src/E177D.C) */
extern int gFBCC; /*@ ds:FBCC */            /* a page number 0..3 (kept modulo 4 by 0ACD) */
extern int g54F2; /*@ ds:54F2 */            /* the index drawn by 0D1C:0454, -1 when none */
extern int g54F4; /*@ ds:54F4 */            /* the row of the animated list 0D1C:0454 draws */
extern unsigned long g848A; /*@ ds:848A */  /* a tick counter compared with gFC5E by 0D1C:0454 */
extern unsigned long gFC5E; /*@ ds:FC5E */  /* the next tick 0D1C:0454 may draw at */
extern signed char g1CE7[]; /*@ ds:1CE7 */  /* per-row colours for the list 0D1C:0454 draws */
extern signed char g54F6[]; /*@ ds:54F6 */  /* byte table indexed by a character's m17 in 0D1C:059B */

struct Row8 {               /* eight-byte rows at ds:5482, four words: a DrawBox rectangle */
    int m0;
    int m2;
    int m4;
    int m6;
};
extern struct Row8 gRows5482[]; /*@ ds:5482 */

struct Entry14 {            /* the 14-byte records E07A9 calls Entry14; view based at ds:8DD2 (E07A9) */
    unsigned char flags;    /* +0 (ds:8DD2), read by 07A9:015C */
    signed char icon;       /* +1 (ds:8DD3), read by 07A9:0141 */
    signed char m02;        /* +2 (ds:8DD4), compared by 0D1C:00F1 and 0D1C:01CA */
    char pad03;
    int m04;                /* +4 (ds:8DD6), set to -1 by 0D1C:01CA */
    char pad06[0x0B - 0x06];
    signed char m0B;        /* +0xB (ds:8DDD), compared by 0D1C:00F1 */
    char pad0C[0x0E - 0x0C];
};
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */

extern char gFB44; /*@ ds:FB44 */            /* a mode byte: 0 and 1 gate 0D1C:01CA's redraws */
extern int gFBCE; /*@ ds:FBCE */             /* the item index shown by the mouse (src/E07A9.C) */
extern int gMouseDisabled; /*@ ds:814A */    /* nonzero disables the mouse (src/E07A9.C) */

extern void f1547_0991(int dir); /*@ 1547:0991 */
extern void f1547_07E7(char who, char slot); /*@ 1547:07E7 */
extern void f0BD2_0917(int who, int slot, int one); /*@ 0BD2:0917 */
extern void f0BD2_01D0(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int f0BD2_02B9(void far *head, int key, int item); /*@ 0BD2:02B9 */
extern void f07A9_010C(int n); /*@ 07A9:010C */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void f1414_0B2E(void); /*@ 1414:0B2E */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void DrawBox(int x1, int y1, int x2, int y2, int colour); /*@ 1A1F:000C */
extern int GetIndexedByte(char index, char limit, char offset); /*@ 0A72:0006 */
extern char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */

int f0D1C_0670(int who, int mask);
int f0D1C_02DC(void);

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

/* 0D1C:003F: selects the party member at index `value` (the signed char at ds:A99A). When
   `value` differs from the current one, the old index and the new index are passed to f1547_0991
   (which redraws that member's row), and nothing is done when they are equal. gA99A indexes the
   character records read by 0D1C:0670 and is toggled 0..5 by Turn091C (src/E0ACD.C). Called by
   0ACD, 0DE8, 0EF3, 1414 and overlays 1, 5 and 6. */
/*@ 0D1C:003F */ void SelectCharacter(int value)
{
    int old;

    if (gA99A != value) {
        old = gA99A;
        gA99A = value;
        f1547_0991(old);
        f1547_0991(gA99A);
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
/* 0D1C:0080: Purpose unknown: computes a ten-bit value x through 0D1C:00DA, then looks up
   gB894[x * 9 + g547E[page]] in g194B; returns -1 unless that byte is odd and gB894[x * 9 + 4] has
   no bits 0..2 set, otherwise returns x. The callers (0ACD key handlers) pass gFBCC as b, so
   g547E and g1790 are indexed by a page number 0..3. */
/*@ 0D1C:0080 */ int f0D1C_0080(int value, signed char page)
{
    int x;
    int flag;

    x = f0D1C_00DA(value, page);
    flag = g194B[gB894[x * 9 + g547E[page]]];
    if (!(flag & 1))
        return -1;
    if (gB894[x * 9 + 4] & 7)
        return -1;
    return x;
}

/* 0D1C:00DA: Purpose unknown: returns (value + g1790[page]) & 0x3FF, a ten-bit value. g1790 is a
   byte table indexed by `page` (gFBCC at the call sites). Called by 0D1C:0080 and 0D1C:0A43. */
/*@ 0D1C:00DA */ int f0D1C_00DA(int value, int page)
{
    return (value + g1790[page]) & 0x3FF;
}
/* 0D1C:00F1: finds the first of the 27 word slots of gChars[who].m77 that holds an item (a
   nonzero index into the 14-byte records at ds:8DD2) whose m02 equals `field2` and whose m0B
   equals `fieldB`. A `field2` or `fieldB` of -1 is a wildcard. Returns the slot number, or -1.
   Called by 0D1C:01CA and from overlays 1 and 6. */
/*@ 0D1C:00F1 */ int FindCarriedSlot(int who, int field2, int fieldB)
{
    int far *slot;
    unsigned i;

    slot = gChars[who].m77;
    for (i = 0; i < 27; slot++, i++) {
        if (*slot != 0) {
            if (gTable8DD2[*slot].m02 == field2 || field2 == -1) {
                if (gTable8DD2[*slot].m0B == fieldB || fieldB == -1)
                    return i;
            }
        }
    }
    return -1;
}
/* 0D1C:0163: Purpose unknown: over the last two of the 27 word slots of gChars[who].m77 (25
   and 26), returns 1 when one of them holds an item (an index into the ds:8DD2 records) whose
   m02 is 0x2F and whose m0B equals `fieldB`, otherwise 0. Slots 25 and 26 are the only two
   inspected. Called by 122E, 1414 and overlays 1 and 6. DRAFT: matches every byte except that the
   original reloads `item` into AX before the first multiply (0D1C:018A `mov ax, di`); ours
   forwards the value AX still holds from the test (two bytes shorter). */
/*@ 0D1C:0163 draft */ int f0D1C_0163(int who, int fieldB)
{
    struct CharRec far *rec;
    int item;
    int i;

    rec = &gChars[who];
    for (i = 25; i <= 26; i++) {
        if (item = rec->m77[i]) {
            if (gTable8DD2[item].m02 == 0x2F && gTable8DD2[item].m0B == fieldB)
                return 1;
        }
    }
    return 0;
}
/* 0D1C:01CA: Purpose unknown: for every character `who` that passes f0D1C_0670(who, 1), removes
   all carried items whose ds:8DD2 record has m02 == `field2` and m0B == `fieldB`
   (FindCarriedSlot's match): clears the item's m04 (sets it to -1), clears the character's slot,
   and redraws. Returns 1 when at least one item was removed, otherwise 0. The redraw paths: when
   gFB44 is 0 and the slot is one of the first two of a character before the sixth, f1547_07E7;
   when gFB44 is 1 and the character is the selected one, f0BD2_0917; and when the mouse is
   enabled, the item in the slot f0D1C_02DC names is cleared and redrawn through f07A9_010C.
   Called by overlays 5. What m02, m0B and m04 mean is not established. */
/*@ 0D1C:01CA */ int RemoveCarriedItems(int field2, int fieldB)
{
    int slot;
    int who;
    register int item;
    register int found;

    found = who = 0;
    for (; who < 6; who++) {
        if (!f0D1C_0670(who, 1))
            continue;
        while ((slot = FindCarriedSlot(who, field2, fieldB)) != -1) {
            item = gChars[who].m77[slot];
            gTable8DD2[item].m04 = -1;
            gChars[who].m77[slot] = 0;
            found = 1;
            if (gFB44 == 0) {
                if (slot < 2 && who < 5)
                    f1547_07E7(who, slot);
            }
            if (gFB44 == 1 && gA99A == who)
                f0BD2_0917(who, slot, 1);
            if (!gMouseDisabled) {
                item = f0D1C_02DC();
                if (gTable8DD2[item].m02 == field2) {
                    gTable8DD2[item].m04 = -1;
                    f07A9_010C(0);
                }
            }
        }
    }
    return found;
}
/* 0D1C:02DC: Purpose unknown: returns the item index in the currently displayed slot: gFBCE
   when the mouse is disabled, otherwise gChars[gA99A].m77[gF51A]. Called by 0D1C:01CA and from
   many other units. */
/*@ 0D1C:02DC */ int f0D1C_02DC(void)
{
    if (!gMouseDisabled)
        return gFBCE;
    return gChars[gA99A].m77[gF51A];
}
/* 0D1C:0304: stores `n` as the item index in the current slot: gFBCE when the mouse is enabled,
   or gChars[gA99A].m77[gF51A] when it is disabled. When the mouse is disabled it also redraws
   the slot, through f1547_0991(gA99A) when gFB44 is 0 and f0BD2_0917(gF51A, 1, 0) otherwise.
   Called by f07A9_010C to set the icon shown for a selected item. */
/*@ 0D1C:0304 */ void SetCurrentItem(int n)
{
    if (!gMouseDisabled) {
        gFBCE = n;
    } else {
        gChars[gA99A].m77[gF51A] = n;
        if (gFB44 == 0)
            f1547_0991(gA99A);
        else
            f0BD2_0917(gF51A, 1, 0);
    }
}
/* 0D1C:035A: Purpose unknown: clears gChars[who].m01 and then, for each of the 27 word slots of
   gChars[who].m77 except slot 16 (which is a list head at +97), queues the item in the slot
   through f0BD2_01D0 at the head gSoundVolume * 9 + 5 (the nine-byte records at ds:B894) with a
   random sound g1A1B[gFBCC * 4 + RollDice(1, 2, -1)]. Then it drains the slot-16 list through
   f0BD2_02B9 and queues each returned item the same way. Finally, when `who` is the selected
   character, gA99A is cleared to 0, and f1414_0B2E runs. Called by overlays 1 and 5. */
/*@ 0D1C:035A */ void f0D1C_035A(int who)
{
    struct CharRec far *rec;
    int item;
    int i;

    rec = &gChars[who];
    rec->m01 = 0;
    for (i = 0; i < 27; i++) {
        if (i == 16)
            continue;
        if (item = rec->m77[i]) {
            f0BD2_01D0((int far *)(gB894 + gSoundVolume * 9 + 5), gSoundVolume, item,
                    g1A1B[gFBCC * 4 + RollDice(1, 2, -1)]);
        }
    }
    while (rec->m77[16]) {
        item = f0BD2_02B9(&rec->m77[16], 0, -1);
        f0BD2_01D0((int far *)(gB894 + gSoundVolume * 9 + 5), gSoundVolume, item,
                g1A1B[gFBCC * 4 + RollDice(1, 2, -1)]);
    }
    if (gA99A == who)
        gA99A = 0;
    f1414_0B2E();
}
/* 0D1C:0454: draws the animated row list at ds:5482. When `index` differs from the stored g54F2
   it hides the pointer, erases the old row (DrawBox over gRows5482[g54F2] in colour 0xC), shows
   the pointer, resets the row counter g54F4 to 0, stores `index` in g54F2 and records the
   current tick (gFC5E = g848A). Otherwise, when a row is selected and one tick has passed, it
   draws the next row g1CE7[g54F4] in the row's colour, advances g54F4 (wrapping to 0 at a zero
   colour) and sets gFC5E = g848A + 1. Called by overlays 0 and 1. */
/*@ 0D1C:0454 */ void DrawRowList(int index)
{
    int x1;
    int y1;
    int x2;
    int y2;

    if (g54F2 != index) {
        if (g54F2 != -1) {
            LockHidePointer();
            DrawBox(gRows5482[g54F2].m0, gRows5482[g54F2].m2, gRows5482[g54F2].m4,
                    gRows5482[g54F2].m6, 0xC);
            LockShowPointer();
        }
        g54F4 = 0;
        g54F2 = index;
        gFC5E = g848A;
    } else {
        if (g54F2 != -1) {
            if (g848A > gFC5E) {
                if (g1CE7[g54F4] == 0)
                    g54F4 = 0;
                x1 = gRows5482[g54F2].m0;
                y1 = gRows5482[g54F2].m2;
                x2 = gRows5482[g54F2].m4;
                y2 = gRows5482[g54F2].m6;
                LockHidePointer();
                DrawBox(x1, y1, x2, y2, g1CE7[g54F4]);
                LockShowPointer();
                g54F4++;
                gFC5E = g848A + 1;
            }
        }
    }
}
/* 0D1C:0583: calls f1547_0991 (which redraws one character's row) for each of the six party
   members. Called by 0ACD, 0EF3 and overlays 1, 3 and 5 after a change that affects the whole
   party. */
/*@ 0D1C:0583 */ void RedrawCharacters(void)
{
    int i;

    for (i = 0; i < 6; i++)
        f1547_0991(i);
}
/* 0D1C:059B: Purpose unknown: a per-character check against a 1d20 roll. It derives a class
   slot from gChars[who].m20 (GetClassSlotByte, with 4 and 5 folded to 0), a value from
   GetIndexedByte(classSlot, m24, `attack`), then, depending on a level-like value m1F * 2
   (3 or 5, and separately 4), for `attack` 4, 0 or 1, subtracts g54F6[m17]; finally it returns
   1 when RollDice(1, 20, 0) >= value. QUIRK 0D1C:0609/0604: m1F * 2 is even, so the tests for 3
   and 5 can never hold; the original still has them. Called by 0DE8:0F98 and 0DE8:101A. */
/*@ 0D1C:059B */ int f0D1C_059B(int who, int attack)
{
    struct CharRec far *rec;
    int classSlot;
    int value;
    int level;

    rec = &gChars[who];
    classSlot = GetClassSlotByte(rec->m20, 0);
    if (classSlot == 4 || classSlot == 5)
        classSlot = 0;
    value = GetIndexedByte(classSlot, rec->m24, attack);
    level = rec->m1F * 2;
    if (level == 3 || level == 5) {
        if (attack == 4 || attack == 0 || attack == 1)
            value -= g54F6[rec->m17];
    }
    if (level == 4) {
        if (attack == 4 || attack == 1)
            value -= g54F6[rec->m17];
    }
    return RollDice(1, 20, 0) >= value;
}
/*@ 0D1C:0670 stub */ int f0D1C_0670(int who, int mask) { return 0; }
/*@ 0D1C:070F stub */ void f0D1C_070F(void) {}
/*@ 0D1C:07A7 stub */ void f0D1C_07A7(void) {}
/*@ 0D1C:07DB stub */ void f0D1C_07DB(void) {}
/*@ 0D1C:08B1 stub */ void f0D1C_08B1(void) {}
/*@ 0D1C:0903 stub */ void f0D1C_0903(void) {}
/*@ 0D1C:09CB stub */ void f0D1C_09CB(void) {}
/*@ 0D1C:0A43 stub */ void f0D1C_0A43(void) {}
/*@ 0D1C:0BAB stub */ void f0D1C_0BAB(void) {}
/*@ 0D1C:0BEB stub */ void f0D1C_0BEB(void) {}
/*@ 0D1C:0C21 stub */ void f0D1C_0C21(void) {}
