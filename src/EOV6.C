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

extern struct CharRec gChars[]; /*@ ds:F580 */  /* the 243-byte character records */
extern struct Spell gSpells1344[]; /*@ ds:1344 */ /* far name pointers, indexed by a spell id */
extern struct Spell gSpells151F[]; /*@ ds:151F */
extern struct Entry28 gTable8A84[]; /*@ ds:8A84 */ /* 28-byte records, 30 of them */
extern char far gStrings2003; /*@ 2003:0000 */    /* the message string table */
extern int g811F; /*@ ds:811F */
extern int g8115; /*@ ds:8115 */
extern int g159; /*@ ds:0159 */                 /* holds gF57E while ovl6:0B9B is on screen */
extern int gTextHeight; /*@ ds:FE60 */
extern int gTextWidth; /*@ ds:FE62 */
extern int gTextLeft; /*@ ds:FE64 */
extern int gTextTop; /*@ ds:FE66 */
extern char gFileNameBuf[]; /*@ ds:FE6E */              /* the text buffer ovl6:0858 formats into */
extern void *gFont1A47; /*@ ds:FE4E */          /* the current font (1A47:0169 records it) */
extern void *gFE5A; /*@ ds:FE5A */              /* the 8x8 font loaded from Font8.fnt */
extern void *gA9C0; /*@ ds:A9C0 */              /* the 6x6 font loaded from FONT6.FNT */
extern int gSoundVolume; /*@ ds:FBCA */
extern signed char gF57E; /*@ ds:F57E */        /* the colour 082C:0898 draws with (ovl6:0858) */
extern signed char gF442; /*@ ds:F442 */        /* 1 while ovl6:09D2 updates the objects */
extern unsigned long g8492; /*@ ds:8492 */      /* the 32-bit counter the 1Ch handler bumps */

extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void SetTextWindowAndClear(int n); /*@ 1DE7:00B6 */
extern void ClearTextRegion(void); /*@ 1DE7:0344 */
extern void DrawText(char *text); /*@ 1DE7:060A */
extern void f1A47_0169(void *font); /*@ 1A47:0169 */
extern void WaitFrames(int n); /*@ 1A16:000B */
extern int f0D1C_0670(int member, int what); /*@ 0D1C:0670 */
extern int FindCarriedSlot(int member, int what, int which); /*@ 0D1C:00F1 */
extern int f0D1C_0163(int member, int what); /*@ 0D1C:0163 */
extern void f0D1C_0C21(int x, int y, int width, int height, int c, int d); /*@ 0D1C:0C21 */
extern char *SkipStrings(char *s, int n); /*@ 0D1C:07A7 */
extern void DrawShadowText(char *text, int x, int y, int colour, int flag); /*@ 082C:0898 */
extern void WaitTicks(int n); /*@ 082C:0425 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int f17A6_0F61(int x, int y); /*@ 17A6:0F61 */
extern void RedrawMember(int member); /*@ 1547:0991 */
extern void DamageCharacter(int member, int what); /*@ 0DE8:09B9 */
extern void UpdateObjects(int kind); /*@ 08E6:0CC8 */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern int ReadKey(void); /*@ 1AAD:038E */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern int fOvl1_243A(int what); /*@ 2102:0039 */
extern int FindClassSlot(int value, int classIndex); /*@ 0A72:007B */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern void *memchr(void *s, int c, unsigned n); /*@ 0000:6319 */

int f07E2(void);
void f0858(int hours, int draw);
int f09D2(void);
int f0A92(void);
void f0B9B(int message);

/* ovl6:0000: the rest/camp loop, the overlay's entry. In six sub-steps (ds:tick) per hour it
   increments each member's spell group counters, memorises a spell when one of the 30 memorized
   slots at +0x55 or known slots at +0x37 is negative ("%s gained %s." / "%s memorized %s."),
   spends a healer's memorized slots ("%s casts healing on %s.") to top up the most wounded member
   and, every eight hours, grants the class-2 spell, levels members up or down and updates the
   object list. Each hour it prints "Hours rested: %-4d" (f0858), every read of a key or the end
   of the rest shows the warning box (f0B9B). Returns 1 or 0 when the camp could not start, or the
   09D2 result otherwise. Called from ovl1:1ED2 through the overlay's stub entry 212A:0039.

   The six zero bytes at +0008 are a local array initialiser, `char counters[6] = { 0, ... }`,
   which the original copies from its compiler template at ds:7F40 (F_SCOPY@ 0000:78C9).

   Blocked on the calls to its five siblings: every one is defined later in the file and the
   original reaches it with `nop; push cs; call near` (5 bytes, 90 0E E8 at ovl6:001F, 00AC,
   012A, 013E, 04FD, 0707, 0722, 0729, 0731, 075A, 0770, 0778, 07D3). TCC 1.00 emits a far call
   (lcall, 9A) for a forward same-file call and `push cs; call near` (4 bytes, no nop) for a
   backward one, so no source form gives those five bytes. See the notes in re/symbols.toml. */
/*@ ovl6:0000 draft */ int CampLoop(void)
{
    int healerFound;
    int flag4;
    int result;
    int flag8;
    int gained;
    char *slots;
    char counters[6] = { 0, 0, 0, 0, 0, 0 };
    int healAmount;
    int foundWounded;
    int hours;
    int tick;
    int member;
    int i;

    hours = 0;
    if (f09D2())
        return 1;
    f0D1C_0C21(gTextLeft << 3, gTextTop, gTextWidth << 3, gTextHeight, 0, 0);
    flag8 = 0;
    for (member = 0; member < 6; member++) {
        if (f0D1C_0670(member, 1))
            flag8 |= !f0D1C_0670(member, 0x10);
    }
    if (flag8) {
        if (fOvl1_243A(0x3b) == 0)
            return 0;
    }
    flag4 = 0;
    healerFound = 0;
    foundWounded = f07E2();
    if (foundWounded > 0) {
        for (member = 0; member < 6; member++) {
            if (FindClassSlot(2, gChars[member].classIndex) == -1)
                continue;
            if (gChars[member].hp <= 0)
                continue;
            if (gChars[member].name[0] == 0)
                continue;
            if (gChars[member].level <= 0)
                continue;
            healerFound = fOvl1_243A(0x28);
            break;
        }
    }
    SetTextWindowAndClear(7);
    foundWounded = f07E2();
    tick = 0;
    f0858(hours, 1);
    SetTextWindow(7);
    f1A47_0169(gA9C0);
    for (;;) {
        tick++;
        for (i = 0; i < 6; i++) {
            counters[i]++;
            if (gChars[i].level == 0)
                continue;
            if (!f0D1C_0670(i, 5))
                continue;
            slots = &gChars[i].memorized[0];
            for (member = 0; member < 0x1e; member++) {
                if (member / 6 + '0' >= counters[i])
                    break;
                if (*slots >= 0) {
                    slots++;
                    continue;
                }
                *slots = -*slots;
                counters[i] = 0x30;
                sprintf(gFileNameBuf, "%s gained %s.\r", &gChars[i].name[0],
                        gSpells151F[*slots].name);
                LockHidePointer();
                DrawText(gFileNameBuf);
                LockShowPointer();
                WaitFrames(5);
                break;
            }
            if (FindCarriedSlot(i, 0x1d, -1) == -1)
                continue;
            slots = &gChars[i].known[0];
            for (member = 0; member < 0x1e; member++) {
                if (member / 6 + '0' >= counters[i])
                    break;
                if (*slots >= 0) {
                    slots++;
                    continue;
                }
                *slots = -*slots;
                counters[i] = 0x30;
                sprintf(gFileNameBuf, "%s memorized %s.\r", &gChars[i].name[0],
                        gSpells1344[*slots].name);
                LockHidePointer();
                DrawText(gFileNameBuf);
                LockShowPointer();
                WaitFrames(5);
                break;
            }
        }
        if (healerFound && foundWounded) {
            for (i = 0; i < 6; i++) {
                if (FindClassSlot(2, gChars[i].classIndex) == -1)
                    continue;
                if (memchr(&gChars[i].memorized[0], -2, 0x1e) != 0
                    || memchr(&gChars[i].memorized[0], -0xf, 0x1e) != 0
                    || memchr(&gChars[i].memorized[0], -0x14, 0x1e) != 0)
                    break;
                slots = &gChars[i].memorized[0];
                for (member = 0; member < 0x1e; member++) {
                    healAmount = 0;
                    switch (*slots) {
                    case 2:
                        healAmount = RollDice(1, 8, 0);
                        break;
                    case 0xf:
                        healAmount = RollDice(2, 8, 1);
                        break;
                    case 0x14:
                        healAmount = RollDice(3, 8, 3);
                        break;
                    }
                    *slots = -*slots;
                    slots++;
                    if (!healAmount)
                        break;
                    counters[i] = 0;
                    foundWounded--;
                    sprintf(gFileNameBuf, "%s casts healing on %s.\r", &gChars[i].name[0],
                            &gChars[foundWounded].name[0]);
                    LockHidePointer();
                    DrawText(gFileNameBuf);
                    LockShowPointer();
                    WaitFrames(5);
                    if (gChars[foundWounded].hp + healAmount > gChars[foundWounded].maxhp)
                        gChars[foundWounded].hp = gChars[foundWounded].maxhp;
                    else
                        gChars[foundWounded].hp += healAmount;
                    foundWounded++;
                }
                if (foundWounded == 0)
                    break;
            }
        }
        if (tick == 6) {
            tick = 0;
            hours++;
            g8492 += 0x7ff8L;
            f0858(hours, 0);
            for (member = 0; member < 6; member++) {
                if (!f0D1C_0670(member, 1))
                    continue;
                if (f0D1C_0670(member, 0x10))
                    continue;
                DamageCharacter(member, 0xa);
                WaitTicks(5);
            }
            if (hours % 8 == 0) {
                gained = 0;
                for (member = 0; member < 6; member++) {
                    if (gChars[member].classIndex == 2) {
                        slots = memchr(&gChars[member].memorized[0], 0xffe8, 6);
                        if (slots)
                            *slots = 0x18;
                        else if (!memchr(&gChars[member].memorized[0], 0x18, 6))
                            *(char *)memchr(&gChars[member].memorized[0], 0, 6) = 0x18;
                    }
                    if (!f0D1C_0670(member, 3))
                        continue;
                    if (gChars[member].level == 0) {
                        if (hours % 0x18 == 0 && gChars[member].hp > 0xf6) {
                            DamageCharacter(member, 1);
                            gained = 1;
                            f1A47_0169(gA9C0);
                            RedrawMember(member);
                        }
                        continue;
                    }
                    if (gChars[member].hp < gChars[member].maxhp) {
                        gChars[member].hp++;
                        f1A47_0169(gA9C0);
                        RedrawMember(member);
                    }
                    if (f0D1C_0163(member, 2) < 0) {
                        if (gChars[member].level > 5) {
                            gChars[member].level -= 5;
                        } else {
                            gChars[member].level = 0;
                            gained = 1;
                        }
                    }
                }
                if (gained) {
                    if (fOvl1_243A(0x2f) == 0) {
                        f0858(hours, 1);
                        flag4 = 0;
                        foundWounded = 0;
                        goto done;
                    }
                    f0858(hours, 1);
                    foundWounded = f07E2();
                }
            }
        }
        if (f0A92() == 0 && !flag4 && foundWounded) {
            flag4 = fOvl1_243A(0x29);
            f0858(hours, 1);
            if (!flag4)
                goto done;
        }
        if (!ReadKey()) {
            result = f09D2();
            if ((f0A92() || (foundWounded && (flag4 || healerFound)))
                && !ReadKey() && result == 0)
                continue;
        }
done:
        FlushKeyBuffer();
        SetTextWindow(4);
        f1A47_0169(gFE5A);
        if (!foundWounded && !result)
            f0B9B(0x2b);
        return result;
    }
}

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
   rectangle (f0D1C_0C21) inside its clip (ClipRect), draws the title through DrawShadowText and shows
   the pointer again. Then it formats "Hours rested: %-4d" with gFileNameBuf and, with g8115 set to 2,
   draws the line in the colour gF57E inside a second clip. Finally it waits ten frames and
   restores the font and the region. Called by the camp loop. */
/*@ ovl6:0858 */ void f0858(int hours, int draw)
{
    void *savedFont;
    register int region;

    region = g811F;
    savedFont = gFont1A47;
    f1A47_0169(gFE5A);
    SetTextWindow(0xa);
    if (draw) {
        g8115 = 0;
        LockHidePointer();
        ClearTextRegion();
        f0D1C_0C21(gTextLeft << 3, gTextTop, gTextWidth << 3, gTextHeight, 0, 0);
        ClipRect(gTextLeft, gTextTop, gTextLeft, gTextTop, gTextWidth, gTextHeight, 0, 2);
        DrawShadowText(SkipStrings(&gStrings2003, 0x2a), (gTextLeft + 1) << 3, gTextTop + 5, 9, 0);
        LockShowPointer();
    }
    sprintf(gFileNameBuf, "Hours rested: %-4d", hours);
    g8115 = 2;
    LockHidePointer();
    DrawShadowText(gFileNameBuf, (gTextLeft + 1) << 3, gTextTop + 0x14, 0xf, gF57E);
    g8115 = 0;
    ClipRect(gTextLeft + 1, gTextTop + 0x14, gTextLeft + 1, gTextTop + 0x14, 0x12, 8, 2, 0);
    LockShowPointer();
    WaitFrames(0xa);
    f1A47_0169(savedFont);
    SetTextWindow(region);
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
            SetTextWindow(7);
            g8115 = 0;
            LockHidePointer();
            DrawText("\rYou can't rest here, monsters are near.");
            LockShowPointer();
            SetTextWindow(member);
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
        if (FindClassSlot(2, gChars[member].classIndex) != -1
            || FindClassSlot(4, gChars[member].classIndex) != 0) {
            slots = &gChars[member].memorized[0];
            for (i = 0; i < 0x1e; i++)
                if (*slots++ < 0)
                    return 1;
        }
        if (FindClassSlot(1, gChars[member].classIndex) == -1)
            continue;
        if (FindCarriedSlot(member, 0x1d, -1) == -1)
            continue;
        slots = &gChars[member].known[0];
        for (i = 0; i < 0x1e; i++)
            if (*slots++ < 0)
                return 1;
    }
    return 0;
}
/* ovl6:0B9B: shows the modal warning box for message number `message` in the far string table at
   2003:0000. It saves the region (g811F), g8115 and the font, selects the 8x8 font and region 2,
   stores gF57E in ds:0159, fills the panel (f0D1C_0C21) and draws the message string through
   DrawShadowText inside a clip (ClipRect). Then it waits up to 0x78 frames for a key (1AAD:038E),
   clears the pending key (1AAD:0480) and restores the font, region and g8115. Called by the camp
   loop for "You can't rest here, monsters are near." */
/*@ ovl6:0B9B */ void f0B9B(int message)
{
    register int i;
    register int savedRegion;
    int savedSelect;
    void *savedFont;

    savedRegion = g8115;
    savedFont = gFont1A47;
    savedSelect = g811F;
    f1A47_0169(gFE5A);
    LockHidePointer();
    g159 = gF57E;
    SetTextWindowAndClear(0xb);
    g8115 = 2;
    f0D1C_0C21(gTextLeft << 3, gTextTop, gTextWidth << 3, gTextHeight, 0, 0);
    DrawShadowText(SkipStrings(&gStrings2003, message), (gTextLeft << 3) + 5, gTextTop + 5, 0xf, 0);
    ClipRect(gTextLeft, gTextTop, gTextLeft, gTextTop, gTextWidth, gTextHeight, 2, 0);
    LockShowPointer();
    for (i = 0; i < 0x78; i++) {
        if (ReadKey()) {
            FlushKeyBuffer();
            break;
        }
        WaitFrames(1);
    }
    g8115 = savedRegion;
    f1A47_0169(savedFont);
    SetTextWindow(savedSelect);
    FlushKeyBuffer();
}
