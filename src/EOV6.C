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
    signed char maxHp;         /* +1C: maximum hit points; HealCharacter (0DE8:0ADB) clamps
                                  hp at it */
    char pad1D[0x20 - 0x1D];
    signed char classIndex;    /* +20: an index into a class table (0A72:007B's classIndex) */
    char pad21[0x23 - 0x21];
    unsigned char food;        /* +23: the food value (EatFood, 0ACD:07E0): a member with none
                                  regains neither spells nor hit points while resting, and rest
                                  without a ration costs 5 (ovl6:0000) */
    char pad24[0x37 - 0x24];
    signed char mageSpells[30];     /* +37: spell slots, negative: a spell to be memorised */
    signed char clericSpells[30]; /* +55: spell slots, positive: a spell ready to cast; negative:
                                  spent or not yet regained (CampLoop turns it positive as
                                  "gained") */
    char pad73[0xF3 - 0x55 - 30];
};

/* a 19-byte spell record at ds:1344 and ds:151F: a far pointer to its name, then spell data this
   unit does not read (src/E0EF3.C:155-161 names flags +04, apply +05 and ddFn +09) */
struct Spell {
    char *name;                /* +00: a far pointer to the spell's name */
    char rest[0x13 - 4];
};

/* the 28-byte map-object records at ds:8A82 (struct Monster in src/E08E6.C); ovl6:09D2 reads each
   record's id at +2 for its Distance test */
struct Monster {
    char typeIndex;                   /* +00: an index into the ds:1A3B table (src/E08E6.C) */
    signed char kind;          /* +01 */
    int cell;                    /* +02: the object's map cell, Distance-tested */
    char rest[0x1C - 4];
};

extern struct CharRec gChars[]; /*@ ds:F580 */  /* the 243-byte character records */
extern struct Spell gSpells[]; /*@ ds:1344 */ /* far name pointers, indexed by a spell id */
extern struct Spell gClericSpells[]; /*@ ds:151F */
extern struct Monster gMonsters[]; /*@ ds:8A82 */       /* the 28-byte map-object records, 30 of them */
extern char far gCampStrings; /*@ 2003:0000 */    /* the message string table */
extern int gTextRegion; /*@ ds:811F */
extern int gDrawPage; /*@ ds:8115 */
extern int g159; /*@ ds:0159 */                 /* written from gPanelFillColour by ovl6:0B9B and ovl1:13F7
                                                   (ShowMessage); no reader found in src/ */
extern int gTextHeight; /*@ ds:FE60 */
extern int gTextWidth; /*@ ds:FE62 */
extern int gTextLeft; /*@ ds:FE64 */
extern int gTextTop; /*@ ds:FE66 */
extern char gFileNameBuf[]; /*@ ds:FE6E */              /* the text buffer ovl6:0858 formats into */
extern void *gCurFont; /*@ ds:FE4E */          /* the current font (1A47:0169 records it) */
extern void *gFont8x8; /*@ ds:FE5A */              /* the 8x8 font loaded from Font8.fnt */
extern void *gFont6x6; /*@ ds:A9C0 */              /* the 6x6 font loaded from FONT6.FNT */
extern int gPartyCell; /*@ ds:FBCA */
extern signed char gPanelFillColour; /*@ ds:F57E */        /* the colour2 argument ovl6:0858 passes; 082C:0898
                                                   forwards it to its first shadow draw only */
extern char gQuietObjectUpdate; /*@ ds:F442 */               /* 1 while ovl6:09D2 updates the objects */
extern unsigned long gGameTicks; /*@ ds:8492 */      /* the 32-bit counter the 1Ch handler bumps */

extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void SetTextWindowAndClear(int n); /*@ 1DE7:00B6 */
extern void ClearTextRegion(void); /*@ 1DE7:0344 */
extern void DrawText(char *text); /*@ 1DE7:060A */
extern void SetFont(void *font); /*@ 1A47:0169 */
extern void WaitFrames(int n); /*@ 1A16:000B */
extern int MemberIs(int member, int what); /*@ 0D1C:0670 */
extern int FindCarriedSlot(int who, int field2, int fieldB); /*@ 0D1C:00F1 */
extern int f0D1C_0163(int member, int what); /*@ 0D1C:0163 */
extern void DrawFramedPanel(int x, int y, int width, int height, int c, int d); /*@ 0D1C:0C21 */
extern char *SkipStrings(char *s, int n); /*@ 0D1C:07A7 */
extern void DrawShadowText(char *text, int x, int y, int colour, int colour2); /*@ 082C:0898 */
extern void WaitTicks(int n); /*@ 082C:0425 */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int Distance(int x, int y); /*@ 17A6:0F61 */
extern void RedrawMember(int member); /*@ 1547:0991 */
extern void DamageCharacter(int who, int amount); /*@ 0DE8:09B9 */
extern void UpdateObjects(int kind); /*@ 08E6:0CC8 */
extern void CopyRegion(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern int ReadKey(void); /*@ 1AAD:038E */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern int AskYesNo(int what); /*@ 2102:0039 */
extern int FindClassSlot(int slotId, int classIndex); /*@ 0A72:007B */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern void *memchr(void *s, int c, unsigned n); /*@ 0000:6319 */

int FindMostWoundedMember(void);
void ShowHoursRested(int hours, int draw);
int AreMonstersNear(void);
int HasSpellsToRegain(void);
void ShowCampMessage(int message);

/* ovl6:0000: the rest/camp loop, the overlay's entry. In six sub-steps per hour (the local tick)
   it increments each member's spell group counters, memorises a spell when one of the 30 memorized
   slots at +0x55 or known slots at +0x37 is negative ("%s gained %s." / "%s memorized %s."), and
   spends a healer's memorized slots ("%s casts healing on %s.") to top up the most wounded member.
   Each hour it prints "Hours rested: %-4d" (ShowHoursRested) and damages by 10 hit points every member with
   flag 1 and without flag 0x10 (DamageCharacter(member, 10)). Every eight hours it puts the class-2
   spell 0x18 into a memorized slot of a member whose classIndex is 2, and, for a member with flag 3
   above level 0 (`if (!MemberIs(member, 3)) continue;`), regains one hit point of a member below
   his maximum and drops the level of a member whose 0D1C:0163(member, 2) is negative (by 5, or to 0
   when at 5 or less); every 24 hours it damages a member with flag 3 at level 0 above -10 hit points
   (hp > (signed char)0xf6; hp is a signed char). It never
   updates the object list: only AreMonstersNear does (UpdateObjects), at the start and on each key test. On
   the done path, when the last AreMonstersNear result was 0 and no member is wounded, it shows the warning box
   ShowCampMessage(0x2b), "All characters are fully rested." (gCampStrings message 0x2b). It returns 1 when the
   opening AreMonstersNear found an object near and 0 when the "Poisoned party members will die!" picker
   answers 0; every other exit returns `result`, which is assigned only by the AreMonstersNear call in the key
   test, so the two gotos and the ReadKey branch return whatever the frame held (the caller tests AX,
   src/EOV1.ASM:3692-3694).
   QUIRK ovl6:07C9, ovl6:07D9: the done path reads the local before any assignment on the paths that
   reach it without going through the key test (the two gotos at ovl6:0718 and ovl6:0765 and the
   ReadKey exits): the warning-box test `!foundWounded && !result` (ovl6:07C9, `cmp word ptr
   [bp-6],0`) and the final `return result` (ovl6:07D9) both read it, so the box may show and the
   value returned is whatever the frame held.
   Called from ovl1:1ED2 through the overlay's stub entry 212A:0039.

   The six zero bytes at +0008 are a local array initialiser, `unsigned char counters[6]`,
   which the original copies from its compiler template at ds:7F40 (F_SCOPY@ 0000:78C9). The
   calls to its five siblings are forward same-file calls; the original reaches each with
   `nop; push cs; call near` (TLINK's overlay rewrite) where TCC emits a far call of the same
   length. */
/*@ ovl6:0000 */ int CampLoop(void)
{
    int flag2;                 /* AskYesNo(0x28), the healer picker's answer (meaning unknown) */
    int flag4;
    int result;                /* the last AreMonstersNear answer; unassigned on the goto paths (QUIRK 07C9, 07D9) */
    int flag8;
    int changed;               /* set when a member drops to level 0 or takes the 24-hour damage */
    char *slots;
    unsigned char counters[6] = { 0, 0, 0, 0, 0, 0 };  /* the spell level reached this rest, as '0'+n */
    int healAmount;
    int foundWounded;          /* 1-based most wounded member (FindMostWoundedMember); reused 0-based as the target */
    int hours;
    int tick;
    int member;
    int i;

    hours = 0;
    if (AreMonstersNear())
        return 1;
    DrawFramedPanel(gTextLeft << 3, gTextTop, gTextWidth << 3, gTextHeight, 0, 0);
    flag8 = 0;
    for (member = 0; member < 6; member++) {
        if (MemberIs(member, 1))
            flag8 |= !MemberIs(member, 0x10);
    }
    if (flag8) {
        if (AskYesNo(0x3b) == 0)
            return 0;
    }
    flag4 = 0;
    flag2 = 0;
    foundWounded = FindMostWoundedMember();
    if (foundWounded > 0) {
        for (member = 0; member < 6; member++) {
            if (FindClassSlot(2, gChars[member].classIndex) == -1)
                continue;
            if (gChars[member].hp <= 0)
                continue;
            if (gChars[member].name[0] == 0)
                continue;
            if (gChars[member].food <= 0)
                continue;
            flag2 = AskYesNo(0x28);
            break;
        }
    }
    SetTextWindowAndClear(7);
    foundWounded = FindMostWoundedMember();
    tick = 0;
    ShowHoursRested(hours, 1);
    SetTextWindow(7);
    SetFont(gFont6x6);
    for (;;) {
        tick++;
        for (i = 0; i < 6; i++) {
            counters[i]++;
            if (gChars[i].food == 0)
                continue;
            if (!MemberIs(i, 5))
                continue;
            slots = &gChars[i].clericSpells[0];
            for (member = 0; member < 0x1e; member++) {
                if (member / 6 + '0' >= counters[i])
                    break;
                if (*slots < 0) {
                    *slots *= -1;
                    counters[i] = 0x30;
                    sprintf(gFileNameBuf, "%s gained %s.\r", &gChars[i].name[0],
                            gClericSpells[*slots].name);
                    LockHidePointer();
                    DrawText(gFileNameBuf);
                    LockShowPointer();
                    WaitFrames(5);
                    break;
                }
                slots++;
            }
            if (FindCarriedSlot(i, 0x1d, -1) == -1)
                continue;
            slots = &gChars[i].mageSpells[0];
            for (member = 0; member < 0x1e; member++) {
                if (member / 6 + '0' >= counters[i])
                    break;
                if (*slots < 0) {
                    *slots *= -1;
                    counters[i] = 0x30;
                    sprintf(gFileNameBuf, "%s memorized %s.\r", &gChars[i].name[0],
                            gSpells[*slots].name);
                    LockHidePointer();
                    DrawText(gFileNameBuf);
                    LockShowPointer();
                    WaitFrames(5);
                    break;
                }
                slots++;
            }
        }
        if (flag2 && foundWounded) {
            for (i = 0; i < 6; i++) {
                if (FindClassSlot(2, gChars[i].classIndex) != -1) {
                    if (counters[i] >= '0'
                        && (memchr(&gChars[i].clericSpells[0], -2, 0x1e) != 0
                            || memchr(&gChars[i].clericSpells[0], -0xf, 0x1e) != 0
                            || memchr(&gChars[i].clericSpells[0], -0x14, 0x1e) != 0))
                        break;
                    slots = &gChars[i].clericSpells[0];
                    for (member = 0; member < 0x1e; member++) {
                        healAmount = 0;
                        switch (*slots) {
                        case 2:
                            healAmount = RollDice(1, 8, 0);
                            *slots *= -1;
                            break;
                        case 0xf:
                            healAmount = RollDice(2, 8, 1);
                            *slots *= -1;
                            break;
                        case 0x14:
                            healAmount = RollDice(3, 8, 3);
                            *slots *= -1;
                            break;
                        }
                        slots++;
                        if (healAmount) {
                            counters[i] = 0;
                            foundWounded--;
                            sprintf(gFileNameBuf, "%s casts healing on %s.\r", &gChars[i].name[0],
                                    &gChars[foundWounded].name[0]);
                            LockHidePointer();
                            DrawText(gFileNameBuf);
                            LockShowPointer();
                            WaitFrames(5);
                            if (gChars[foundWounded].hp + healAmount > gChars[foundWounded].maxHp)
                                gChars[foundWounded].hp = gChars[foundWounded].maxHp;
                            else
                                gChars[foundWounded].hp += healAmount;
                            foundWounded++;
                        }
                        if (foundWounded == 0)
                            break;
                    }
                }
                if (foundWounded == 0)
                    break;
            }
        }
        if (tick == 6) {
            tick = 0;
            hours++;
            gGameTicks += 0x7ff8L;
            ShowHoursRested(hours, 0);
            for (member = 0; member < 6; member++) {
                if (!MemberIs(member, 1))
                    continue;
                if (MemberIs(member, 0x10))
                    continue;
                DamageCharacter(member, 0xa);
                WaitTicks(5);
            }
            if (hours % 8 == 0) {
                changed = 0;
                for (member = 0; member < 6; member++) {
                    if (gChars[member].classIndex == 2) {
                        if ((slots = memchr(&gChars[member].clericSpells[0], 0xffe8, 6)) != 0)
                            *slots = 0x18;
                        else if (!memchr(&gChars[member].clericSpells[0], 0x18, 6))
                            *(char *)memchr(&gChars[member].clericSpells[0], 0, 6) = 0x18;
                    }
                    if (!MemberIs(member, 3))
                        continue;
                    if (gChars[member].food != 0) {
                        if (gChars[member].hp < gChars[member].maxHp) {
                            gChars[member].hp++;
                            SetFont(gFont6x6);
                            RedrawMember(member);
                        }
                        if (f0D1C_0163(member, 2) < 0) {
                            if (gChars[member].food <= 5) {
                                gChars[member].food = 0;
                                changed = 1;
                            } else {
                                gChars[member].food -= 5;
                            }
                        }
                    } else if (hours % 0x18 == 0 && gChars[member].hp > (signed char)0xf6) {
                        DamageCharacter(member, 1);
                        changed = 1;
                        SetFont(gFont6x6);
                        RedrawMember(member);
                    }
                }
                if (changed) {
                    if (AskYesNo(0x2f) == 0) {
                        ShowHoursRested(hours, 1);
                        flag4 = 0;
                        foundWounded = 0;
                        goto done;
                    }
                    ShowHoursRested(hours, 1);
                }
                foundWounded = FindMostWoundedMember();
            }
        }
        if (HasSpellsToRegain() == 0 && !flag4 && foundWounded) {
            flag4 = AskYesNo(0x29);
            ShowHoursRested(hours, 1);
            if (!flag4)
                goto done;
        }
        if (!ReadKey()) {
            result = AreMonstersNear();
            if ((HasSpellsToRegain() || ((flag4 || flag2) && foundWounded))
                && !ReadKey() && result == 0)
                continue;
        }
done:
        FlushKeyBuffer();
        SetTextWindow(4);
        SetFont(gFont8x8);
        if (!foundWounded && !result)
            ShowCampMessage(0x2b);
        return result;
    }
}

/* ovl6:07E2: the 0-based index plus one of the party member with the lowest current hit points,
   among members with flag 3 whose hit points are below their maximum and below 0x384 (the start
   value of the running minimum; hp is a signed char), or zero when no member needs healing. Called
   by the camp loop. */
/*@ ovl6:07E2 */ int FindMostWoundedMember(void)
{
    int found;
    register int member;
    register int lowest;

    found = -1;
    lowest = 0x384;
    for (member = 0; member < 6; member++) {
        if (MemberIs(member, 3) && gChars[member].hp < gChars[member].maxHp
            && gChars[member].hp < lowest) {
            found = member;
            lowest = gChars[member].hp;
        }
    }
    return found + 1;
}

/* ovl6:0858: shows the camp panel and the "Hours rested" line. It selects the 8x8 font and the
   region 0xa, and when `draw` is nonzero clears gDrawPage, hides the pointer, fills the panel
   rectangle (DrawFramedPanel) inside its clip (CopyRegion), draws the title through DrawShadowText and shows
   the pointer again. Then it formats "Hours rested: %-4d" with gFileNameBuf and, with gDrawPage set to 2,
   draws the line in colour 0xf with gPanelFillColour as the colour2 (fifth) argument of the first shadow draw
   only (DrawShadowText 082C:0898) inside a second clip. Finally it waits ten frames and restores the
   font and the region. Called by the camp loop. */
/*@ ovl6:0858 */ void ShowHoursRested(int hours, int draw)
{
    void *savedFont;
    register int region;

    region = gTextRegion;
    savedFont = gCurFont;
    SetFont(gFont8x8);
    SetTextWindow(0xa);
    if (draw) {
        gDrawPage = 0;
        LockHidePointer();
        ClearTextRegion();
        DrawFramedPanel(gTextLeft << 3, gTextTop, gTextWidth << 3, gTextHeight, 0, 0);
        CopyRegion(gTextLeft, gTextTop, gTextLeft, gTextTop, gTextWidth, gTextHeight, 0, 2);
        DrawShadowText(SkipStrings(&gCampStrings, 0x2a), (gTextLeft + 1) << 3, gTextTop + 5, 9, 0);
        LockShowPointer();
    }
    sprintf(gFileNameBuf, "Hours rested: %-4d", hours);
    gDrawPage = 2;
    LockHidePointer();
    DrawShadowText(gFileNameBuf, (gTextLeft + 1) << 3, gTextTop + 0x14, 0xf, gPanelFillColour);
    gDrawPage = 0;
    CopyRegion(gTextLeft + 1, gTextTop + 0x14, gTextLeft + 1, gTextTop + 0x14, 0x12, 8, 2, 0);
    LockShowPointer();
    WaitFrames(0xa);
    SetFont(savedFont);
    SetTextWindow(region);
}
/* ovl6:09D2: the "objects are near" test. It selects the 6x6 font, marks the object update
   (gQuietObjectUpdate) and updates objects 0 and 1, then scans the 30 28-byte map-object records gMonsters
   (ds:8A82): when Distance(gPartyCell, obj.id) is below two an object is close, so it switches to
   region 7 (saving the previous region), draws "\rYou can't rest here, monsters are near." and
   returns 1. Restores the caller's font and returns 0 when nothing is close. Called at the start
   of the camp loop and on each key test. */
/*@ ovl6:09D2 */ int AreMonstersNear(void)
{
    void *savedFont;
    register int i;
    register int savedRegion;

    savedFont = gCurFont;
    SetFont(gFont6x6);
    gQuietObjectUpdate = 1;
    UpdateObjects(0);
    UpdateObjects(1);
    gQuietObjectUpdate = 0;
    for (i = 0; i < 0x1e; i++) {
        if (Distance(gPartyCell, gMonsters[i].cell) < 2) {
            savedRegion = gTextRegion;
            SetTextWindow(7);
            gDrawPage = 0;
            LockHidePointer();
            DrawText("\rYou can't rest here, monsters are near.");
            LockShowPointer();
            SetTextWindow(savedRegion);
            SetFont(savedFont);
            return 1;
        }
    }
    SetFont(savedFont);
    return 0;
}
/* ovl6:0A92: one when some party member (with hit points and a level) still has a spell to spend:
   a negative "memorized" slot (a spell spent or not yet regained; a positive one is ready to cast)
   for a class whose slot 2 matches (FindClassSlot(2, classIndex) != -1) or whose slot 4 test does
   not return 0 (FindClassSlot(4, classIndex) != 0), or a negative "known" slot (a spell still to be
   memorised) for a class whose slot 1 applies and 0D1C:00F1(member, 0x1d, -1) does not return -1.
   Returns zero when the party can rest. Called by the camp loop. */
/*@ ovl6:0A92 */ int HasSpellsToRegain(void)
{
    register int member;
    char *slots;
    register int i;

    for (member = 0; member < 6; member++) {
        if (gChars[member].hp <= 0)
            continue;
        if (gChars[member].food <= 0)
            continue;
        if (FindClassSlot(2, gChars[member].classIndex) != -1
            || FindClassSlot(4, gChars[member].classIndex) != 0) {
            slots = &gChars[member].clericSpells[0];
            for (i = 0; i < 0x1e; i++)
                if (*slots++ < 0)
                    return 1;
        }
        if (FindClassSlot(1, gChars[member].classIndex) == -1)
            continue;
        if (FindCarriedSlot(member, 0x1d, -1) == -1)
            continue;
        slots = &gChars[member].mageSpells[0];
        for (i = 0; i < 0x1e; i++)
            if (*slots++ < 0)
                return 1;
    }
    return 0;
}
/* ovl6:0B9B: the copy in overlay 6 of ShowMessage (ovl1:13F7): shows the modal warning box for
   message number `message` in the far string table at 2003:0000. It saves gDrawPage, the
   current font and gTextRegion, selects the 8x8 font and text region 0xb, stores gPanelFillColour in ds:0159
   (the value here, gDrawPage = 2 at ds:8115), fills the panel (DrawFramedPanel) and draws the message
   string through DrawShadowText in color 0xf inside a clip (CopyRegion). Then it waits up to 0x78
   frames for a key (1AAD:038E), clears the pending key (1AAD:0480) and restores the font, region
   and gDrawPage. Its only caller's message is 0x2b, "All characters are fully rested." */
/*@ ovl6:0B9B */ void ShowCampMessage(int message)
{
    register int i;
    register int savedRegion;
    int savedSelect;
    void *savedFont;

    savedRegion = gDrawPage;
    savedFont = gCurFont;
    savedSelect = gTextRegion;
    SetFont(gFont8x8);
    LockHidePointer();
    g159 = gPanelFillColour;
    SetTextWindowAndClear(0xb);
    gDrawPage = 2;
    DrawFramedPanel(gTextLeft << 3, gTextTop, gTextWidth << 3, gTextHeight, 0, 0);
    DrawShadowText(SkipStrings(&gCampStrings, message), (gTextLeft << 3) + 5, gTextTop + 5, 0xf, 0);
    CopyRegion(gTextLeft, gTextTop, gTextLeft, gTextTop, gTextWidth, gTextHeight, 2, 0);
    LockShowPointer();
    for (i = 0; i < 0x78; i++) {
        if (ReadKey()) {
            FlushKeyBuffer();
            break;
        }
        WaitFrames(1);
    }
    gDrawPage = savedRegion;
    SetFont(savedFont);
    SetTextWindow(savedSelect);
    FlushKeyBuffer();
}
