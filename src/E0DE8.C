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
    char pad00[0x0D];
    signed char m0D;        /* +0D */
    char pad0E;
    signed char m0F;        /* +0F */
    char pad10[0x15 - 0x10];
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
    unsigned char m03;      /* +3: low two bits read by 01FF */
    int m04;                /* +4 */
    char pad06[0x0B - 0x06];
    signed char m0B;        /* +0B */
    char pad0C[0x0E - 0x0C];
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
extern int f0BD2_07DE(int who, int slot, int a, int b, int c, int d); /*@ 0BD2:07DE */
extern int f08E6_15B4(int a, int b); /*@ 08E6:15B4 */
extern int f0EF3_04F7(int who); /*@ 0EF3:04F7 */
extern void f08E6_0FBE(struct Obj28 far *obj, int n); /*@ 08E6:0FBE */
extern void f0EF3_12A1(void far *arg, int n); /*@ 0EF3:12A1 */
extern void f0EF3_0F86(void far *arg); /*@ 0EF3:0F86 */
extern void ApplyCharEffect(char who, int a, long b, char c, char d); /*@ 1414:05ED */
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

/*@ 0DE8:011A stub */ void f0DE8_011A(void) {}

/* 0DE8:01D7: returns 1 when the item type `index` has a nonzero weapon kind whose low seven bits
   are 2 or 3, otherwise 0. The kind is the word at item+0x0E. Called by 0DE8:01FF and 0DE8:02D4
   to decide whether an item attacks. */
/*@ 0DE8:01D7 */ int f0DE8_01D7(int index)
{
    int kind;

    kind = gItemTypes[index].m0E & 0x7f;
    return kind == 2 || kind == 3;
}

/*@ 0DE8:01FF stub */ void f0DE8_01FF(void) {}
/*@ 0DE8:02D4 stub */ void f0DE8_02D4(void) {}
/*@ 0DE8:0359 stub */ void f0DE8_0359(void) {}
/*@ 0DE8:0474 stub */ void f0DE8_0474(void) {}
/*@ 0DE8:0537 stub */ void f0DE8_0537(void) {}
/*@ 0DE8:05BB stub */ void f0DE8_05BB(void) {}
/*@ 0DE8:0763 stub */ void f0DE8_0763(void) {}
/*@ 0DE8:093B stub */ void f0DE8_093B(void) {}
/*@ 0DE8:09B9 stub */ void f0DE8_09B9(void) {}
/*@ 0DE8:0ADB stub */ void f0DE8_0ADB(void) {}
/*@ 0DE8:0B45 stub */ void f0DE8_0B45(void) {}
/*@ 0DE8:0C2B stub */ void f0DE8_0C2B(void) {}
/*@ 0DE8:0C82 stub */ void f0DE8_0C82(void) {}
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
