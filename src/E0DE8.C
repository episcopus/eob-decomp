/* Unit at EOB.EXE 0DE8:0007 (4279 bytes, 20 functions): the combat, damage and item-effect
   module. It rolls to-hit and damage for a character against a creature or an item (0007, 011A,
   01FF, 02D4), applies and records the resulting hit points (09B9, 0ADB), awards experience and
   levels a character up (0B45, 0C2B, 0C82), computes a character's armour class (0D5E), saves
   against an effect (0F98, 0FCA, 101A) and runs the party-wipe ending (093B). It walks the
   243-byte character records at ds:F580 (struct CharRec, src/E0A72.C), the 28-byte object records
   at ds:8A82 (struct Obj28, src/E08E6.C), the 14-byte records at ds:8DD2 (struct Entry14,
   src/E0D1C.C), the 27-byte records at ds:1A3B (struct Entry27, src/E0D1C.C) and the 16-byte item
   types at ds:A9F0 (struct ItemType, src/EOV2.C). Most meanings are not established, so most
   function names say what a function does rather than what the game calls it. */

/*@flags -ml -O -Z */

struct CharRec {            /* the 243-byte (0xF3) character records at ds:F580; see src/E0A72.C */
    char pad00;
    unsigned char m01;      /* +01: status bits (0D1C:0670, 0DE8:09B9) */
    char name[0x0B];        /* +02: the character's name, NUL padded (0DE8:0C82) */
    char pad0D[0x15 - 0x0D];
    signed char m15;        /* +15: an index into a class table (0A72:0181) */
    char pad16;
    signed char m17;        /* +17: an index into g54F6 (0D1C:059B) */
    char pad18[0x1B - 0x18];
    signed char hp;         /* +1B: current hit points */
    char m1C;               /* +1C: a copy of the max hit points */
    signed char m1D;        /* +1D: the armour-class byte 0D5E builds */
    signed char m1E;        /* +1E: a bit set/cleared by the per-slot helper */
    signed char m1F;        /* +1F */
    signed char m20;        /* +20: an index into a class table */
    char pad21[3];
    signed char m24[3];     /* +24: three per-class-slot counters */
    char pad27[0x77 - 0x27];
    int m77[27];            /* +77: 27 carried-item slots (0D1C:00F1) */
    char padAD[0xDF - 0xAD];
    signed char mDF;        /* +DF */
    char padE0[0xE2 - 0xE0];
    signed char mE2;        /* +E2 */
    long mE3;               /* +E3: effect flags (a 32-bit mask; high word folded at 09B9) */
    unsigned char mE7;      /* +E7 */
    unsigned char mE8[0xF3 - 0xE8];
};

struct Obj28 {              /* the 28-byte object records at ds:8A82; see src/E08E6.C */
    char b0;                /* +0: an index into the ds:1A3B table */
    char pad01;
    int m02;                /* +2 */
    char pad04;
    char m05;               /* +5 */
    char pad06[0x16 - 0x06];
    char m16;               /* +16: bit 0 set by 0007 */
    char pad17[0x1C - 0x17];
};

struct Entry27 {            /* the 27-byte records at ds:1A3B; see src/E0D1C.C */
    signed char m00;        /* +0: a class byte for 0A72:0522 */
    char m01;               /* +1 */
    signed char m02;        /* +2: a class byte read by 0FCA */
    char pad03[0x0E - 0x03];
    signed char m0E;        /* +0E: bit 0 tested by 011A */
    int m0F;                /* +0F */
    char pad11[0x19 - 0x11];
    signed char m19;        /* +19 */
    signed char m1A;        /* +1A */
};

struct Entry14 {            /* the 14-byte records at ds:8DD2; see src/E0D1C.C */
    unsigned char flags;    /* +0 */
    signed char icon;       /* +1 */
    signed char m02;        /* +2 */
    signed char m03;        /* +3: low two bits read by 01FF */
    int m04;                /* +4 */
    char pad06[0x0B - 0x06];
    signed char m0B;        /* +0B */
    char pad0C[0x0E - 0x0C];
};

struct DDRec {              /* the records at ds:DD2C that 1414:0172 walks; see src/E1414.C */
    char active;            /* +0 */
    char pad01;
    int m02;                /* +2 */
    int m04;                /* +4 */
    char m06;               /* +6 */
    signed char m07;        /* +7 */
    int m08;                /* +8: an item slot, indexed into gTable8DD2 (01FF, 02D4) */
    char m0A;               /* +0A: an item type, indexed into gItemTypes (01FF, 02D4) */
    char m0B;               /* +0B */
    char m0C;               /* +0C */
    unsigned char m0D;      /* +0D */
    int (far *fn)(struct DDRec far *rec); /* +0E */
};

struct ItemType {           /* the 16-byte item types at ds:A9F0; see src/EOV2.C */
    signed char m00;        /* +0: bit 4 tested by 0D1C:0BEB */
    signed char m01;        /* +1 */
    char pad02[2];
    signed char m04;        /* +4 */
    signed char m05;        /* +5 */
    char pad06;
    signed char m07;        /* +7 */
    signed char m08;        /* +8 */
    signed char m09;        /* +9 */
    signed char m0A;        /* +0A */
    signed char m0B;        /* +0B */
    char pad0C[2];
    short m0E;              /* +0E: low 7 bits tested by 01D7 */
};

extern struct CharRec gChars[]; /*@ ds:F580 */
extern struct Obj28 gObjs[]; /*@ ds:8A82 */
extern struct Entry27 gTable1A3B[]; /*@ ds:1A3B */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */
extern struct ItemType gItemTypes[]; /*@ ds:A9F0 */
extern signed char g1A1B[]; /*@ ds:1A1B */   /* per-page sound table (src/E0D1C.C) */
extern signed char g1A2B[]; /*@ ds:1A2B */   /* per-page byte table used by 01FF */
extern signed char g1A3D[]; /*@ ds:1A3D */   /* a class-byte table (0x1B per row) used by 0FCA */
extern signed char g115F[]; /*@ ds:115F */   /* a byte table indexed by a class, used by 0B45 */
extern signed char g1231[]; /*@ ds:1231 */   /* a byte table used by 0D5E */
extern signed char g552F[]; /*@ ds:552F */   /* a byte table indexed by a class, used by 05BB */
extern signed char g553E[]; /*@ ds:553E */   /* a byte table used by 0D5E */
extern signed char g54F6[]; /*@ ds:54F6 */   /* byte table indexed by m17 (src/E0D1C.C) */
extern unsigned char gFBD2; /*@ ds:FBD2 */   /* bit 0 gates the icon colour row (src/E07A9.C) */
extern int gFBCC; /*@ ds:FBCC */             /* a page number 0..3 (src/E0D1C.C) */
extern int gSoundVolume; /*@ ds:FBCA */      /* the global sound volume (src/E177D.C) */
extern signed char gF57E; /*@ ds:F57E */     /* a colour read by 0D1C:0C21 */
extern signed char gFB4C; /*@ ds:FB4C */     /* a colour read by 0D1C:0C21 */
extern signed char gF53A; /*@ ds:F53A */     /* a colour read by 0D1C:0C21 */
extern char gFB44; /*@ ds:FB44 */            /* a mode byte (src/E0D1C.C) */
extern signed char gA9C6; /*@ ds:A9C6 */     /* an offset added into gEF6A's targets (src/E1414.C) */
extern char gDF4C; /*@ ds:DF4C */            /* set to 1 when a hit lands (src/E0D1C.C) */
extern char gDCA4; /*@ ds:DCA4 */            /* the object acting this step, set by 0359 */
extern unsigned char g1905[]; /*@ ds:1905 */   /* a 256-byte table gB894 indexes into (0359) */
extern char gB894[]; /*@ ds:B894 */          /* nine-byte records; +0x19C bytes read via B894 (0359) */
extern char gFileNameBuf[]; /*@ ds:FE6E */   /* the text buffer sprintf formats into (src/EOV2.C) */

extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int GetDerivedStatA(int who); /*@ 0A72:00CF */
extern int GetDerivedStatB(int who); /*@ 0A72:0128 */
extern int GetCharM15Value(int who); /*@ 0A72:0181 */
extern char GetScaledValue(char who, char x); /*@ 0A72:0522 */
extern char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */
extern long GetClassSlotLong(char indexA, char indexB, char countC); /*@ 0A72:04B7 */
extern int GetClassWord(char classIndex); /*@ 0A72:019D */
extern int GetIndexedByte(char index, char limit, char offset); /*@ 0A72:0006 */
extern int RollDamage(char who, char level, char countA, char countB, char countC); /*@ 0A72:01AE */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern int f0D1C_0BAB(int obj); /*@ 0D1C:0BAB */
extern int f0D1C_0BEB(int obj, int flag); /*@ 0D1C:0BEB */
extern int f0D1C_059B(int who, int attack); /*@ 0D1C:059B */
extern int f0D1C_07DB(int who, int flag); /*@ 0D1C:07DB */
extern int f0BD2_02B9(void far *head, int key, int item); /*@ 0BD2:02B9 */
extern int f0BD2_07DE(int who, int field, int volume, int tableByte, int page, int m02); /*@ 0BD2:07DE */
extern int f0BD2_07DE(int who, int slot, int a, int b, int c, int d); /*@ 0BD2:07DE */
extern int f08E6_15B4(char a, int b); /*@ 08E6:15B4 */
extern int f0EF3_04F7(int who); /*@ 0EF3:04F7 */
extern void f08E6_0FBE(struct Obj28 far *obj, int n); /*@ 08E6:0FBE */
extern void f0EF3_12A1(void far *arg, int n); /*@ 0EF3:12A1 */
extern void f0EF3_0F86(void far *arg); /*@ 0EF3:0F86 */
extern void ApplyCharEffect(char who, long interval, char value, char firstEmpty); /*@ 1414:05ED */
extern void f1547_0991(int dir); /*@ 1547:0991 */
extern void f1547_033F(int who); /*@ 1547:033F */
extern void f1547_0544(int who); /*@ 1547:0544 */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int c1, int c2, int c3); /*@ 082C:09D5 */
extern void DelaySeconds(int n); /*@ 082C:0B43 */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */

void f0DE8_09B9(int who, int what);  /* forward: 01FF reaches it with a far call */

/* 0DE8:0007: the to-hit roll of character `who` against object `obj`. It derives an attack bonus
   from the item `slot` (the m02 kind selects the m0B weapon value when it is 1..3), adds either
   the derived stat A or the m15 char value depending on `flag`, marks gObjs[obj].m16 bit 0 as
   hit, and compares a d20 roll (raised by gFBD2 bits 4 and 5 and by gChars[who].mE3 bit 6, clamped
   to 1..20) against GetScaledValue(who, gTable1A3B[gObjs[obj].b0].m00) minus the bonus. Returns 1
   when the roll reaches that armour class. Called by 0DE8:02D4 and 0DE8:0359. */
/*@ 0DE8:0007 */ int f0DE8_0007(int who, int obj, int slot, int flag)
{
    int b0;
    int ac;
    int kind;
    int bonus;
    int roll;

    if (slot != 0)
        kind = gTable8DD2[slot].m02;
    else
        kind = 0;
    if (kind >= 1 && kind <= 3)
        bonus = gTable8DD2[slot].m0B;
    else
        bonus = 0;
    if (flag != 0)
        bonus += GetDerivedStatA(who);
    else
        bonus += GetCharM15Value(who);
    b0 = gObjs[obj].b0;
    gObjs[obj].m16 |= 1;
    ac = GetScaledValue(who, gTable1A3B[b0].m00) - bonus;
    roll = RollDice(1, 20, 0);
    if (gFBD2 & 0x30)
        roll++;
    if ((gChars[who].mE3 & 0x40) != 0)
        roll++;
    roll = roll < 20 ? roll : 20;
    roll = roll > 1 ? roll : 1;
    return roll >= ac;
}

/* 0DE8:011A: the damage a hit deals. It starts from GetDerivedStatB(who) when `flag` is set and
   `who` is valid, then adds a damage roll from the item type `itemType` (dice at m0A/m0B or
   m07/m08 with the m09 bonus, the choice depending on whether `obj`'s gTable1A3B entry has m0E
   bit 0 set) plus the item slot's m0B, or the bare RollDice(1, 2, 0) when `slot` is 0, and clamps
   at 0. Called by 0DE8:01FF, 0DE8:02D4 and 0DE8:0359. */
/*@ 0DE8:011A */ int f0DE8_011A(int who, int obj, int slot, int itemType, char flag)
{
    struct ItemType far *item;
    int total;

    if (flag != 0 && who != -1)
        total = GetDerivedStatB(who);
    else
        total = 0;
    if (slot != 0) {
        item = &gItemTypes[itemType];
        if (obj != -1 && (gTable1A3B[gObjs[obj].b0].m0E & 1))
            total += RollDice(item->m0A, item->m0B, item->m09);
        else
            total += RollDice(item->m07, item->m08, item->m09);
        total += gTable8DD2[slot].m0B;
    } else {
        total += RollDice(1, 2, 0);
    }
    return total < 0 ? 0 : total;
}

/* 0DE8:01D7: returns 1 when the item type `index` has a nonzero weapon kind whose low seven bits
   are 2 or 3, otherwise 0. The kind is the word at item+0x0E. Called by 0DE8:01FF and 0DE8:02D4
   to decide whether an item attacks. */
/*@ 0DE8:01D7 */ int f0DE8_01D7(int index)
{
    int kind;

    kind = gItemTypes[index].m0E & 0x7f;
    return kind == 2 || kind == 3;
}

/*@ 0DE8:01FF */ int f0DE8_01FF(struct DDRec far *rec)
{
    int slot;
    int itemType;
    int damage;
    int roll;
    int result;

    slot = rec->m08;
    itemType = rec->m0A;
    roll = gTable8DD2[slot].m03 & 3;
    roll = g1A2B[gFBCC * 4 + roll];
    if (f0DE8_01D7(itemType))
        damage = f0DE8_011A(-1, -1, slot, itemType, 0);
    else
        damage = 1;
    if ((f0D1C_0670(5, 1) || f0D1C_0670(6, 1)) && RollDice(1, 2, -1))
        result = roll + 2;
    else
        result = roll;
    if (f0D1C_0670(result, 1)) {
        f0DE8_09B9(result, damage);
        return 1;
    }
    return 0;
}
/* 0DE8:02D4: tests and runs the attack of a ds:DD2C record against object `idx`. It takes the
   record's m08 slot, m0A item type and m0B attacker; when m0B is valid it first demands the
   0007 to-hit roll, then rolls 011A damage for the item type (or 1 when the type does not attack)
   and hands the object to f08E6_0FBE. Returns 0 when the to-hit fails, else 1. Called by
   1414:0172 via TestDDEffect. */
/*@ 0DE8:02D4 */ int f0DE8_02D4(struct DDRec far *rec, int idx)
{
    int slot;
    int itemType;
    int who;
    int damage;

    slot = rec->m08;
    itemType = rec->m0A;
    who = rec->m0B;
    if (who != -1 && !f0DE8_0007(who, idx, slot, 0))
        return 0;
    if (f0DE8_01D7(itemType))
        damage = f0DE8_011A(who, idx, slot, itemType, 0);
    else
        damage = 1;
    f08E6_0FBE(&gObjs[idx], damage);
    return 1;
}
/* 0DE8:0359: one item/effect attack by character `who` on the item slot `slot`. It rejects a
   `who` above 1 with -3, then looks the action up through f0D1C_00DA(gSoundVolume, gFBCC) and
   f08E6_15B4. A -1 target with g1905[gB894[value * 9 + gA9C6]] at 0xFF bumps two gB894 cells and
   returns -2, otherwise -1. A real target demands the 0007 to-hit roll (returns -1 on a miss),
   and when the slot's m02 is 0x34..0x38 f0D1C_0BAB can also fail it; the surviving hit is rolled
   through 011A and halved when f0D1C_0BEB says so. Called by 0DE8:0763. The gDCA4 byte records
   the target for the effect dispatcher. */
/*@ 0DE8:0359 */ int f0DE8_0359(int who, int slot)
{
    int target;
    int m02;
    int value;
    int damage;

    if (who > 1)
        return -3;
    value = f0D1C_00DA(gSoundVolume, gFBCC);
    target = f08E6_15B4(who, value);
    if (target == -1) {
        if (g1905[gB894[value * 9 + gA9C6]] == 0xff) {
            gB894[value * 9 + gA9C6]++;
            gB894[value * 9 + (gA9C6 ^ 2)]++;
            gDF4C = 1;
            return -2;
        }
        return -1;
    }
    if (f0DE8_0007(who, target, slot, 1) == 0)
        goto fail;
    m02 = gTable8DD2[slot].m02;
    if (m02 > 0x33 && m02 < 0x39 && f0D1C_0BAB(target))
        return -1;
    gDCA4 = target;
    damage = f0DE8_011A(who, target, slot, gTable8DD2[slot].m02, 1);
    if (f0D1C_0BEB(target, m02))
        damage >>= 1;
    return damage;
fail:
    return -1;
}

/* 0DE8:0474: moves a carried item into gChars[who].m77[slot]. With `flag` set and the special
   slot m77[16] occupied, it takes that slot's item through f0BD2_02B9 (key 0, item -1). Otherwise
   it scans slots 24 down to 22 for the first occupied item whose gTable8DD2 m02 is not `type` (or
   `type` is -1) and moves that item to `slot`, clearing the source. Called by 0DE8:0537.
   DRAFT: matches except one reload of `item` before the m02 expression (ours mov ax,[bp-2],
   original keeps the array load in AX). */
/*@ 0DE8:0474 draft */ void f0DE8_0474(int who, int slot, int type, int flag)
{
    int i;
    int item;

    if (flag != 0 && gChars[who].m77[16] != 0) {
        gChars[who].m77[slot] = f0BD2_02B9(&gChars[who].m77[16], 0, -1);
    } else {
        for (i = 0x18; i >= 0x16; i--) {
            item = gChars[who].m77[i];
            if (item == 0)
                continue;
            if (gTable8DD2[item].m02 == type && type != -1)
                continue;
            gChars[who].m77[slot] = item;
            gChars[who].m77[i] = 0;
            break;
        }
    }
}
/* 0DE8:0537: tries to use the item in the carried slot `slot` of character `who` against the
   item type `field`. It dispatches through f0BD2_07DE (the item's m02, gFBCC, the g1A1B page
   byte, gSoundVolume); on success it plays sound 0x0B, clears gChars[who].m77[slot] and refills
   it through f0DE8_0474(who, slot, -1, 0), setting gDF4C. Returns 0. Called by 0DE8:0763. */
/*@ 0DE8:0537 */ int f0DE8_0537(int who, int slot, int field)
{
    int idx;

    idx = who;
    if (idx > 3)
        idx -= 2;
    if (f0BD2_07DE(who, field, gSoundVolume, g1A1B[gFBCC * 4 + idx], gFBCC,
                   gTable8DD2[field].m02)) {
        PlaySound(0xb);
        gChars[who].m77[slot] = 0;
        f0DE8_0474(who, slot, -1, 0);
        gDF4C = 1;
    }
    return 0;
}

/* 0DE8:05BB: picks the item for the action `slot` and runs it. The item's m02 indexes g552F to a
   `kind`; kind 0x10 takes the first m77[0], m77[1] or the special m77[16] slot whose own m02 is
   0x10 (clearing the slot, pulling m77[16] through f0BD2_02B9), otherwise it scans all 27 slots
   for the first whose item m02 equals `kind` (clearing it and redrawing with f1547_0991 when the
   slot is below 2). A found item is dispatched through f0BD2_07DE (sound 0x1A when m02 is 7, else
   0x0B, setting gDF4C), returning 0, or -4 when no item was found. Called by 0DE8:0763.
   DRAFT: the kind 0x10 slot-clearing branch lays its blocks out differently (original tests
   `m02 == 0x10` with je clear, ours with jne item0). */
/*@ 0DE8:05BB draft */ int f0DE8_05BB(int who, int slot)
{
    int m02;
    int kind;
    int item;
    int i;
    int idx;

    m02 = gTable8DD2[slot].m02;
    kind = g552F[m02];
    item = 0;
    if (kind == 0x10) {
        item = gChars[who].m77[0];
        if (item == 0) {
            item = 0;
        } else if (gTable8DD2[item].m02 == 0x10) {
            gChars[who].m77[0] = 0;
        } else {
            item = 0;
        }
        if (item == 0) {
            item = gChars[who].m77[1];
            if (item == 0) {
                item = 0;
            } else if (gTable8DD2[item].m02 == 0x10) {
                gChars[who].m77[1] = 0;
            } else {
                item = 0;
            }
        }
        if (item == 0 && gChars[who].m77[16] != 0)
            item = f0BD2_02B9(&gChars[who].m77[16], 0, -1);
    } else {
        for (i = 0; i < 0x1b; i++) {
            if (gTable8DD2[gChars[who].m77[i]].m02 == kind) {
                item = gChars[who].m77[i];
                gChars[who].m77[i] = 0;
                if (i < 2)
                    f1547_0991(who);
                break;
            }
        }
    }
    if (item == 0)
        return -4;
    idx = who;
    if (idx > 3)
        idx -= 2;
    if (f0BD2_07DE(who, item, gSoundVolume, g1A1B[gFBCC * 4 + idx], gFBCC, m02)) {
        if (m02 == 7)
            PlaySound(0x1a);
        else
            PlaySound(0xb);
        gDF4C = 1;
    }
    return 0;
}
/*@ 0DE8:0763 stub */ void f0DE8_0763(void) {}
/* 0DE8:093B: the party-wipe ending. It counts how many of the six characters survive the
   f0D1C_0670(n, 5) test (creatures with hit points); when none does it hides the pointer, fills
   the panel (0, 0x79)-(0x13F, 0xC8), writes "All of your party has been defeated. ..." in text
   window 9 and shuts the game down. Called by 0DE8:09B9. */
/*@ 0DE8:093B */ void f0DE8_093B(void)
{
    int n;
    int alive;

    alive = n = 0;
    for (; n < 6; n++)
        alive += f0D1C_0670(n, 5);
    if (alive != 0)
        return;
    LockHidePointer();
    DrawFilledPanel(0, 0x79, 0x13f, 0xc8, gFB4C, gF53A, gF57E);
    SetTextWindow(9);
    DrawText("All of your party has been defeated.  The minions of evil will be able to carry out "
             "their plans unhindered!\r");
    LockShowPointer();
    DelaySeconds(1);
    ShutdownGame();
}

/* 0DE8:09B9: applies `amount` points of damage to character `who`. When f0D1C_0670(who, 3) holds
   it derates mE2 (clamped at 0), subtracts from hp, stores the amount in mE7, and at or below -10
   sets hp to -10, clears mE2, clears mE3 bit 2 and keeps only bit 0 of m01; it plays sound 0x16
   for the death and 0x15 otherwise, runs the mDF descending effect through f0EF3_0F86 when it runs
   out, redraws through f1547_0991 (or f1547_0544 when gFB44 is set), triggers the party-wipe
   ending when hp is at or below -10 and arms ApplyCharEffect(who, 0x12, 6, 1). Called by 0EF3,
   0E08E6, 122E, 1414 and overlay 6.
   DRAFT: the mE2 clamp differs. The original tests the record offset, not the byte: it emits
   `mov ax,[bp-4]; add ax,0xdf; or ax,ax; jae` (rec + mDF, always nonzero) before the mE2 store,
   where ours emits `cmp byte es:[bp-4+0xe2],0; jge`. */
/*@ 0DE8:09B9 draft */ void f0DE8_09B9(int who, int amount)
{
    struct CharRec far *rec;

    if (!f0D1C_0670(who, 3))
        return;
    rec = &gChars[who];
    if (rec->mE2 != 0) {
        rec->mE2 -= amount;
        if (rec->mE2 < 0)
            rec->mE2 = 0;
    }
    rec->hp -= amount;
    rec->mE7 = amount;
    if (rec->hp <= -10) {
        rec->hp = -10;
        rec->mE2 = 0;
        rec->mE3 &= ~4;
        rec->m01 &= 1;
        PlaySound(0x16);
    } else {
        PlaySound(0x15);
    }
    if (rec->mDF > 0) {
        rec->mDF -= amount;
        if (rec->mDF <= 0) {
            rec->mDF = 0;
            f0EF3_0F86(rec);
        }
    }
    if (gFB44 == 0)
        f1547_0991(who);
    else
        f1547_0544(who);
    if (rec->hp <= -10)
        f0DE8_093B();
    ApplyCharEffect(who, 0x12, 6, 1);
}
/* 0DE8:0ADB: heals character `who` by `amount` hit points when the f0D1C_0670(who, 3) condition
   holds, clamping at the record's m1C maximum, then redraws the character through f1547_033F and
   f1547_0991. Called by 0BD2, 0EF3 and 122E. */
/*@ 0DE8:0ADB */ void f0DE8_0ADB(int who, int amount)
{
    struct CharRec far *rec;

    if (!f0D1C_0670(who, 3))
        return;
    rec = &gChars[who];
    if (rec->hp + amount > rec->m1C)
        rec->hp = rec->m1C;
    else
        rec->hp = rec->hp + amount;
    f1547_033F(who);
    f1547_0991(who);
}

/*@ 0DE8:0B45 stub */ void f0DE8_0B45(int who, int amount) {}
/* 0DE8:0C2B: gives `amount` experience to the whole party. It counts the members that pass
   f0D1C_0670(n, 3), divides `amount` by that count and calls f0DE8_0B45(n, amount) for each of
   them. Called by 08E6:0EC3 and 122E:1B5A. */
/*@ 0DE8:0C2B */ void f0DE8_0C2B(int amount)
{
    int n;
    int count;

    count = 0;
    n = 0;
    for (; n < 6; n++) {
        if (f0D1C_0670(n, 3))
            count++;
    }
    if (count <= 0)
        return;
    amount /= count;
    for (n = 0; n < 6; n++) {
        if (f0D1C_0670(n, 3))
            f0DE8_0B45(n, amount);
    }
}

/* 0DE8:0C82: levels character `who` up in class slot `slot`. It bumps that m24 slot and, for the
   three slots 0/1/2, rolls one die of the matching damage group through RollDamage(attr, m17),
   adding the result to both hp and m1C, then formats "%s has gained a level of experience." into
   gFileNameBuf for the character's name, redraws through f1547_0991, draws the text and plays
   sound 0x17. Called by 0DE8:0B45. */
/*@ 0DE8:0C82 */ void f0DE8_0C82(int who, int attr, int slot)
{
    int damage;

    gChars[who].m24[slot]++;
    if (slot == 0)
        damage = RollDamage(attr, gChars[who].m17, 1, 0, 0);
    else if (slot == 1)
        damage = RollDamage(attr, gChars[who].m17, 0, 1, 0);
    else if (slot == 2)
        damage = RollDamage(attr, gChars[who].m17, 0, 0, 1);
    gChars[who].hp += damage;
    gChars[who].m1C += damage;
    sprintf(gFileNameBuf, "\x06\x01%s has gained a level of experience.\x06\x0f\r",
            gChars[who].name);
    f1547_0991(who);
    DrawText(gFileNameBuf);
    PlaySound(0x17);
}
/*@ 0DE8:0D5E stub */ void f0DE8_0D5E(void) {}
/* 0DE8:0F98: a saving-throw style keeper: halves `value` when the f0D1C_059B test for character
   `who` succeeds (a d20 roll at or above the character's class value). Called by 0EF3:0A3F,
   0EF3:20A8 and 0DE8:101A. */
/*@ 0DE8:0F98 */ int f0DE8_0F98(int who, int value)
{
    int chance;

    chance = f0D1C_059B(who, 4);
    if (RollDice(1, 20, 0) >= chance)
        value >>= 1;
    return value;
}

/* 0DE8:0FCA: the same half-on-success keeper for the type byte at `type`: it derives a class
   value through the byte at gTable1A3B[*type].m02 and GetIndexedByte, then halves `value` on a
   d20 roll at or above it. Called by 0EF3:0A3F. */
/*@ 0DE8:0FCA */ int f0DE8_0FCA(char far *type, int value)
{
    int chance;
    int classByte;

    classByte = gTable1A3B[*type].m02;
    chance = GetIndexedByte(0, classByte, 4);
    if (RollDice(1, 20, 0) >= chance)
        value >>= 1;
    return value;
}

/*@ 0DE8:101A stub */ void f0DE8_101A(void) {}
