/* Unit at overlay 6 of EOB.EXE (3258 bytes, 6 functions): the rest/camp screen. The overlay's
   entry (ovl6:0000) is the "make camp" loop: it advances the clock a step at a time, wounds
   heal, casters memorise their spells ("%s gained %s.", "%s memorized %s."), a healer spends
   healing spells ("%s casts healing on %s.") and it prints "Hours rested: %-4d" and
   "\rYou can't rest here, monsters are near." when a monster is close. The other functions are
   the helpers it uses: the most wounded member (ovl6:07E2), the hours panel (ovl6:0858), the
   monster-near test and its message (ovl6:09D2), the "ready to rest" test (ovl6:0A92) and the
   modal warning box (ovl6:0B9B). Character records are the 243-byte (0xF3) records at ds:F580
   (0A72.C); their spells are the 30-byte arrays at +0x37 and +0x55. Names are placeholders. */

/* A character record: the 243-byte records at ds:F580 (6 of them). Only the fields this unit
   touches are named. */
struct CharRec {
    char pad00[2];
    char name[11];             /* +02: the character's name, NUL padded */
    char pad0D[0x1B - 0x0D];
    signed char hp;            /* +1B: current hit points (ovl6:07E2 picks the lowest) */
    signed char maxhp;         /* +1C: maximum hit points (healing clamps to it) */
    char pad1D[0x20 - 0x1D];
    signed char classIndex;    /* +20: an index into a class table (0A72:007B's classIndex) */
    char pad21[0x23 - 0x21];
    unsigned char level;       /* +23: the character's level (0: not in the party) */
    char pad24[0x37 - 0x24];
    signed char known[30];     /* +37: spell slots, negative: a spell to be memorised */
    signed char memorized[30]; /* +55: spell slots, negative: a spell ready to cast */
    char pad73[0xF3 - 0x55 - 30];
};

/* a 19-byte spell record: a far pointer to its name, then spell data this unit does not use */
struct Spell {
    char *name;                /* +00: a far pointer to the spell's name */
    char rest[0x13 - 4];
};

/* an opaque 28-byte record whose first word ovl6:09D2 reads (the monster-near test) */
struct Entry28 {
    int value;                 /* +00 */
    char rest[0x1C - 2];
};

/* a 6-byte struct, the template ovl6:0000 copies into its per-member counter array */
struct Six {
    char b[6];
};

extern struct CharRec gChars[]; /*@ ds:F580 */  /* the 243-byte character records */
extern struct Spell gSpells1344[]; /*@ ds:1344 */ /* far name pointers, indexed by a spell id */
extern struct Spell gSpells151F[]; /*@ ds:151F */
extern struct Entry28 gTable8A84[]; /*@ ds:8A84 */ /* 28-byte records, 30 of them */
extern char far gStrings2003; /*@ 2003:0000 */    /* the message string table */
extern struct Six g7F40; /*@ ds:7F40 */         /* six zero bytes, the counter template */
extern int g811F; /*@ ds:811F */
extern int g8115; /*@ ds:8115 */
extern int gFE60; /*@ ds:FE60 */
extern int gFE62; /*@ ds:FE62 */
extern int gFE64; /*@ ds:FE64 */
extern int gFE66; /*@ ds:FE66 */
extern char gFE6E[]; /*@ ds:FE6E */              /* the text buffer ovl6:0858 formats into */
extern void *gFont1A47; /*@ ds:FE4E */          /* the current font (1A47:0169 records it) */
extern void *gFE5A; /*@ ds:FE5A */              /* the 8x8 font loaded from Font8.fnt */
extern void *gA9C0; /*@ ds:A9C0 */              /* the 6x6 font loaded from FONT6.FNT */
extern int gSoundVolume; /*@ ds:FBCA */
extern signed char gF57E; /*@ ds:F57E */        /* the colour 082C:0898 draws with (ovl6:0858) */
extern signed char gF442; /*@ ds:F442 */        /* 1 while ovl6:09D2 updates the objects */
extern unsigned long g8492; /*@ ds:8492 */      /* the 32-bit counter the 1Ch handler bumps */

extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void f1DE7_0009(int n); /*@ 1DE7:0009 */
extern void f1DE7_00B6(int n); /*@ 1DE7:00B6 */
extern void f1DE7_0344(void); /*@ 1DE7:0344 */
extern void f1DE7_060A(char *text); /*@ 1DE7:060A */
extern void f1A47_0169(void *font); /*@ 1A47:0169 */
extern void WaitFrames(int n); /*@ 1A16:000B */
extern int f0D1C_0670(int member, int what); /*@ 0D1C:0670 */
extern int f0D1C_00F1(int member, int what, int which); /*@ 0D1C:00F1 */
extern int f0D1C_0163(int member, int what); /*@ 0D1C:0163 */
extern void f0D1C_0C21(int x, int y, int width, int height, int c, int d); /*@ 0D1C:0C21 */
extern char *f0D1C_07A7(char *s, int n); /*@ 0D1C:07A7 */
extern void f082C_0898(char *text, int x, int y, int colour, int flag); /*@ 082C:0898 */
extern void f082C_0425(int n); /*@ 082C:0425 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int f17A6_0F61(int x, int y); /*@ 17A6:0F61 */
extern void f1547_0991(int member); /*@ 1547:0991 */
extern void f0DE8_09B9(int member, int what); /*@ 0DE8:09B9 */
extern void UpdateObjects(int kind); /*@ 08E6:0CC8 */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern int f1AAD_038E(void); /*@ 1AAD:038E */
extern void f1AAD_0480(void); /*@ 1AAD:0480 */
extern int fOvl1_243A(int what); /*@ 2102:0039 */
extern int f0A72_007B(int value, int classIndex); /*@ 0A72:007B */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern void *memchr(void *s, int c, unsigned n); /*@ 0000:6319 */

/*@ ovl6:0000 stub */ int Ovl6_0000(void) { return 0; }

/* ovl6:07E2: the 0-based index plus one of the party member with the lowest current hit points and
   hit points below his maximum (at most 0x384), or zero when no member needs healing. Called by
   the camp loop. */
/*@ ovl6:07E2 */ int f07E2(void)
{
    int found;
    register int member;
    register int lowest;

    found = -1;
    lowest = 0x384;
    for (member = 0; member < 6; member++) {
        if (f0D1C_0670(member, 3) && gChars[member].hp < gChars[member].maxhp
            && gChars[member].hp < lowest) {
            found = member;
            lowest = gChars[member].hp;
        }
    }
    return found + 1;
}

/* ovl6:0858: shows the camp panel and the "Hours rested" line. It selects the 8x8 font and the
   region 0xa, and when `draw` is nonzero clears g8115, hides the pointer, fills the panel
   rectangle (f0D1C_0C21) inside its clip (ClipRect), draws the title through f082C_0898 and shows
   the pointer again. Then it formats "Hours rested: %-4d" with gFE6E and, with g8115 set to 2,
   draws the line in the colour gF57E inside a second clip. Finally it waits ten frames and
   restores the font and the region. Called by the camp loop. */
/*@ ovl6:0858 */ void f0858(int hours, int draw)
{
    void *savedFont;
    register int region;

    region = g811F;
    savedFont = gFont1A47;
    f1A47_0169(gFE5A);
    f1DE7_0009(0xa);
    if (draw) {
        g8115 = 0;
        LockHidePointer();
        f1DE7_0344();
        f0D1C_0C21(gFE64 << 3, gFE66, gFE62 << 3, gFE60, 0, 0);
        ClipRect(gFE64, gFE66, gFE64, gFE66, gFE62, gFE60, 0, 2);
        f082C_0898(f0D1C_07A7(&gStrings2003, 0x2a), (gFE64 + 1) << 3, gFE66 + 5, 9, 0);
        LockShowPointer();
    }
    sprintf(gFE6E, "Hours rested: %-4d", hours);
    g8115 = 2;
    LockHidePointer();
    f082C_0898(gFE6E, (gFE64 + 1) << 3, gFE66 + 0x14, 0xf, gF57E);
    g8115 = 0;
    ClipRect(gFE64 + 1, gFE66 + 0x14, gFE64 + 1, gFE66 + 0x14, 0x12, 8, 2, 0);
    LockShowPointer();
    WaitFrames(0xa);
    f1A47_0169(savedFont);
    f1DE7_0009(region);
}
/* ovl6:09D2: the "monsters are near" test. It selects the 6x6 font, marks the object update
   (gF442) and updates objects 0 and 1, then scans the 30 28-byte records at ds:8A84: when
   f17A6_0F61(gSoundVolume, record.value) is below two a monster is close, so it switches back to
   the previous region (saved in g811F), draws "\rYou can't rest here, monsters are near." and
   returns 1. Restores the caller's font and returns 0 when nothing is close. Called at the start
   of the camp loop. */
/*@ ovl6:09D2 */ int f09D2(void)
{
    void *savedFont;
    register int i;
    register int member;

    savedFont = gFont1A47;
    f1A47_0169(gA9C0);
    gF442 = 1;
    UpdateObjects(0);
    UpdateObjects(1);
    gF442 = 0;
    for (i = 0; i < 0x1e; i++) {
        if (f17A6_0F61(gSoundVolume, gTable8A84[i].value) < 2) {
            member = g811F;
            f1DE7_0009(7);
            g8115 = 0;
            LockHidePointer();
            f1DE7_060A("\rYou can't rest here, monsters are near.");
            LockShowPointer();
            f1DE7_0009(member);
            f1A47_0169(savedFont);
            return 1;
        }
    }
    f1A47_0169(savedFont);
    return 0;
}
/* ovl6:0A92: one when some party member (with hit points and a level) still holds a negative
   (not yet memorised) spell: the "memorized" slots for a class whose slots 2 or 4 apply, or the
   "known" slots for a class whose slot 1 applies and 0D1C:00F1(member, 0x1d, -1) does not return
   -1. Returns zero when the party can rest. Called by the camp loop. */
/*@ ovl6:0A92 */ int f0A92(void)
{
    register int member;
    char *slots;
    register int i;

    for (member = 0; member < 6; member++) {
        if (gChars[member].hp <= 0)
            continue;
        if (gChars[member].level <= 0)
            continue;
        if (f0A72_007B(2, gChars[member].classIndex) != -1
            || f0A72_007B(4, gChars[member].classIndex) != 0) {
            slots = &gChars[member].memorized[0];
            for (i = 0; i < 0x1e; i++)
                if (*slots++ < 0)
                    return 1;
        }
        if (f0A72_007B(1, gChars[member].classIndex) == -1)
            continue;
        if (f0D1C_00F1(member, 0x1d, -1) == -1)
            continue;
        slots = &gChars[member].known[0];
        for (i = 0; i < 0x1e; i++)
            if (*slots++ < 0)
                return 1;
    }
    return 0;
}
/*@ ovl6:0B9B stub */ void Ovl6_0B9B(void) {}
