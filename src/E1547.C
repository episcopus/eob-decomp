/* Unit at EOB.EXE 1547:0009 (4362 bytes, 8 functions): the character information screen. It draws
   one party member's block: the status bar (0009), the HP line and ratio bar (033F), the portrait
   (0544), the carried item slots (07E7), the action label (0642) and the title and stat lines
   (0991, which calls the others). It reads the 243-byte character records at ds:F580 (struct
   CharRec, src/E0A72.C), the stat/class/race/alignment and per-class-slot name tables at ds:1195,
   ds:11AD, ds:11DD, ds:1219 and ds:1240, and the per-member row layout byte tables at
   ds:5D1C..ds:5D4F. All 8 functions are at L2. The record's effectFlags field is a long, so its bit tests
   take the `and ax,K; or ax,0` form (the long bit test at 0D1C:0B17, re/symbols.toml e0d1c_0a43,
   is the evidence for the long type). */

/*@flags -ml -O -Z */

struct CharRec {            /* the 243-byte (0xF3) records at ds:F580 (src/E0A72.C, src/E0D1C.C) */
    char pad00;
    unsigned char status;   /* +01: status bits: bit 0 alive, bit 1 poisoned, bit 2 paralyzed
                               (src/E0DE8.C:558 keeps only bit 0 at death; src/E1414.C:476-477
                               clears bit 1 at the end of poison and bit 2 at the end of paralysis) */
    char name[0x0B];        /* +02: the NUL-padded name (src/EOV6.C) */
    signed char strength;   /* +0D: the strength score, drawn by FormatStrengthPercent (src/E0A72.C) */
    signed char baseStrength;        /* +0E (src/E1414.C) */
    signed char strengthPct;        /* +0F: the exceptional strength percentile (src/E0A72.C) */
    signed char baseStrengthPct;        /* +10 (src/E1414.C) */
    signed char intelligence;        /* +11: INTELLIGENCE (ds:1195[1]), the first of the five stat bytes
                               after strength (src/EOV0.C:442-444 formats intelligence, wisdom, dexterity, constitution, charisma) */
    signed char m12;        /* +12 */
    signed char wisdom;     /* +13: WISDOM (ds:1195[2]) */
    signed char m14;        /* +14 */
    signed char dexterity;  /* +15: DEXTERITY (ds:1195[3]); indexes the armour-class base table
                               (GetArmourClassBase, 0A72:019D) */
    signed char m16;        /* +16 */
    signed char constitution;        /* +17: CONSTITUTION (ds:1195[4]) */
    signed char m18;        /* +18 */
    signed char charisma;   /* +19: CHARISMA (ds:1195[5]) */
    signed char m1A;        /* +1A */
    signed char hp;         /* +1B: current hit points (src/E1414.C) */
    signed char maxHp;      /* +1C: maximum hit points, the second %3d of the HP line */
    signed char armorClass; /* +1D: the armour-class byte, drawn on the ARMOR CLASS row
                               (src/E08E6.C:60 and src/E0EF3.C:25 name it the armour-class byte) */
    signed char m1E;        /* +1E: a per-slot bit toggled by UseItem's `m1E ^= 1 << slot`
                               (0DE8:0763); DrawMemberSlot tests bit `slot` (src/E1414.C) */
    signed char race;       /* +1F: index into the race-name table ds:11AD */
    signed char classIndex; /* +20: index into the class-name table ds:11DD (src/E0A72.C) */
    signed char alignment;  /* +21: index into the alignment-name table ds:1240 */
    signed char portraitIndex;        /* +22 */
    unsigned char food;     /* +23: the hunger/food value, clamped to 100 by EatFood (0ACD:07E0)
                               and decremented each tick for a living member by DecayMemberState
                               (1414:0C82); unsigned here (the code tests < 0x21, < 0x14), while
                               src/E0BD2.C:56, src/E0EF3.C:30 and src/EOV0.C:82 declare it signed */
    signed char levels[3];  /* +24: three per-class-slot bytes (src/E0D1C.C) */
    long experience[3];     /* +27: three per-class-slot experience totals (src/E0DE8.C:33) */
    char far *portrait;     /* +33: the member's picture, which 0544 draws as the portrait; ovl3
                               unpacks the sprite into it (src/EOV3.C) */
    char pad37[0x77 - 0x37];
    int items[27];          /* +77: 27 word slots, the carried items (src/E0D1C.C) */
    char padAD[0xDF - 0xAD];
    signed char mDF;        /* +0DF: a descending penalty counter; when it runs out the armour class
                               is recomputed (src/E0DE8.C:563-566, src/E0EF3.C:1179 "penalty byte") */
    signed char mE0;        /* +0E0: a countdown (src/E0EF3.C:39) */
    char padE1[0xE3 - 0xE1];
    long effectFlags;       /* +0E3: status/effect flags: a long, so the bit tests compile to the
                               `mov ax,..; and ax,K; or ax,0` form (src/E1414.C, 0D1C:0B17) */
    signed char lastDamage; /* +0E7: the damage amount last stored by DamageCharacter
                               (src/E0DE8.C:553), drawn as a number while above zero */
    signed char lastAction[0xF3 - 0xE8]; /* +0E8: per-slot item-use results: UseItem stores its `result`
                               there (0DE8:0763), -1 for a forced miss and -2..-4 for failures */
};

extern int gGraphicsChoice; /*@ ds:8102 */         /* the video mode the game initialised (src/E082C.C) */
extern int gDrawPage; /*@ ds:8115 */         /* the graphics driver's draw page (src/E082C.C) */
extern unsigned char gPartySpellFlags; /*@ ds:FBD2 */ /* bit 0 gates the icon colour row (src/E0D1C.C); the 0x30
                                              bits DrawMemberFrame tests (:163) are not understood */
extern signed char gPanelHighlight; /*@ ds:FB4C */ /* a colour (src/E0D1C.C) */
extern signed char gPanelShadow; /*@ ds:F53A */ /* a colour (src/E0D1C.C) */
extern struct CharRec gChars[]; /*@ ds:F580 */ /* the six character records */
extern unsigned char gMemberFrameX[]; /*@ ds:5D1F */ /* member bar x by (who & 1): 8, 80 */
extern signed char gMemberFrameY[]; /*@ ds:5D21 */   /* member bar y by (who >> 1): 2, 54, 106 */
extern unsigned char gHpLineY[]; /*@ ds:5D26 */ /* HP line y by (who >> 1): 46, 98, 150 */
extern signed char gHpLineX[]; /*@ ds:5D24 */   /* HP line x by (who & 1): 23, 95 */
extern signed char gHpBarColours[]; /*@ ds:5D1C */   /* HP ratio bar colour by state (0, 1, 2) */
extern signed char gFoodBarColours[]; /*@ ds:5D29 */   /* food bar colour by state (0, 1, 2) */
extern char gPanelMode; /*@ ds:FB44 */            /* a screen mode: 0 the plain character screen, 1 the
                                                inventory up, 2 the character information panel; the
                                                three pick RedrawMember's branches and gate the other
                                                redraws (src/E0ACD.C:20) */
extern signed char gInventoryMember; /*@ ds:A99A */     /* the selected member (src/E0ACD.C) */
extern signed char gPanelFillColour; /*@ ds:F57E */     /* a colour (src/EOV2.C) */
extern char gBarEmptyColour; /*@ ds:FB7A */            /* a colour: the empty part of DrawRatioBar (:279), set
                                                by SetUiPalette (src/EOV2.C:134) */
extern char gBarGraphsOn; /*@ ds:F43C */            /* nonzero after the startup allocations (src/E07A9.C);
                                                picks how DrawMemberHp shows a member: set draws the
                                                "HP" label and the ratio bar, clear prints
                                                "%3d of %-3d" (:269-286) */
extern char gFailLabelLight; /*@ ds:A95A */            /* a colour (src/EOV2.C) */
extern char gFailLabelShadow; /*@ ds:8A6C */            /* a colour (src/EOV2.C) */
extern char gFailLabelFill; /*@ ds:8A7D */            /* a colour (src/EOV2.C) */
extern void far *gThrownIcon4; /*@ ds:EFBE */ /* the fourth thrown-item icon (src/EOV2.C) */
extern signed char gHandSlotX[]; /*@ ds:5D31 */ /* item slot x by (who & 1): 40, 112 */
extern unsigned char gHandSlotY[]; /*@ ds:5D33 */ /* item slot y by ((who & 6) + slot) */
extern int gMouseDisabled; /*@ ds:814A */    /* nonzero disables the mouse (src/E07A9.C) */
extern char gCursorSlot; /*@ ds:F51A */            /* the selected row (src/E0ACD.C) */
extern void far *gHandShadeIcon; /*@ ds:8A7E */   /* the empty-slot highlight icon (src/EOV2.C) */
extern void far *gDeadPortraitIcon; /*@ ds:F2F4 */   /* the dead-member portrait icon (src/EOV2.C) */
extern void far *gUnconsciousIcon; /*@ ds:DC94 */   /* the wounded-member overlay icon (src/EOV2.C) */
extern signed char gPortraitX[]; /*@ ds:5D2C */   /* portrait x by (who & 1): 8, 80 */
extern signed char gPortraitY[]; /*@ ds:5D2E */   /* portrait y by (who >> 1): 11, 63, 115 */
extern void far *gItemIcons[]; /*@ ds:DDE4 */ /* the item icon far pointers (src/E0D1C.C) */
extern void far *gEmptySlotIcons[]; /*@ ds:DF38 */ /* the two no-item slot icons, gItemIcons[0x55]
                                                  and [0x56] (0xDDE4 + 0x55 * 4) */

extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int colour); /*@ ds:8062 */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void (far *gDrawText)(char far *text, int x, int y, int colour, int extra); /*@ ds:80CA */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern char *strcpy(char far *dest, char far *src); /*@ 0000:67CD */
extern void DrawRect(int x1, int y1, int x2, int y2, int colour); /*@ 082C:04C4 */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int colourA, int colourB, int fill); /*@ 082C:09D5 */
extern int MemberIs(int who, int mask); /*@ 0D1C:0670 */
extern void DrawShape(int page, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void CopyRegion(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void DrawItemIcon(int mode, int n, int x, int y); /*@ 0D1C:070F */
extern int CanUseHand(int who, int flag); /*@ 0D1C:07DB */

extern char far *gCurFont; /*@ ds:FE4E */  /* the current font (src/E1A47.C) */
extern char far *gFont6x6; /*@ ds:A9C0 */      /* the 6x6 font loaded from FONT6.FNT (src/E07A9.C) */
extern char gIconTable; /*@ ds:FB34 */          /* the party member being moved, -1 when none
                                                (src/E0ACD.C:42); src/EOV2.C:142 records the
                                                conflict with src/E0EF3.C:227, which reads ds:FB34 as
                                                an array of far sprite pointers */
extern char gAltPanelFill; /*@ ds:FB7B */          /* a colour (src/EOV2.C) */
extern void far *gThrownIcon3; /*@ ds:EFB4 */ /* the third thrown-item icon (src/EOV2.C) */

/* the stat panel's per-member row layout: x and y of the name and the stat block (0991 only) */
extern signed char gMemberBlockCol[]; /*@ ds:5D4B */ /* name/bar x by (who & 1): 1, 10 */
extern signed char gMemberScreenCol[]; /*@ ds:5D4D */ /* stat block x by (who & 1): 23, 32 */
extern signed char gMemberBlockY[]; /*@ ds:5D4F */ /* y by (who >> 1): 2, 54, 106 */
/* the three item columns of the gPanelMode == 2 screen, x1/y1/x2/y2 per column (0991 only) */
extern unsigned gInfoBoxLeft[]; /*@ ds:5D39 */      /* column x1: 179, 272, 301 */
extern unsigned char gInfoBoxTop[]; /*@ ds:5D3F */ /* column y1: 36, 51, 51 */
extern unsigned gInfoBoxRight[]; /*@ ds:5D42 */      /* column x2: 271, 300, 318 */
extern unsigned char gInfoBoxBottom[]; /*@ ds:5D48 */ /* column y2: 165, 165, 147 */
/* the far string tables: stat, race, class and alignment names and the per-class-slot names */
extern char far *gStatNames[]; /*@ ds:1195 */     /* the six stat names */
extern char far *gRaceNames[]; /*@ ds:11AD */     /* the race names */
extern char far *gClassNames[]; /*@ ds:11DD */     /* the class names */
extern char far *gBaseClassNames[]; /*@ ds:1219 */     /* the per-class-slot names (GetClassSlotByte index) */
extern char far *gAlignmentNames[]; /*@ ds:1240 */     /* the alignment names */

extern char *itoa(int value, char *string, int radix); /*@ 0000:62AA */
extern void SetFont(char far *font); /*@ 1A47:0169 */
extern void DrawShadowText(char far *text, int x, int y, int color, int extra); /*@ 082C:0898 */
extern void PrintInt(int value, int a, int b, int c, int d); /*@ 1AA6:000F */
extern void PrintLong(long value, int a, int b, int c, int d); /*@ 1BA4:000C */
extern int NumberWidthLong(long value); /*@ 082C:0974 */
extern int NumberWidthInt(int value); /*@ 082C:09A6 */
extern char *FormatStrengthPercent(char score, char percent); /*@ 0A72:0468 */
extern char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */
extern void DrawItemRow(int who, int slot, int one); /*@ 0BD2:0917 */
/* 1547:0009: draws the status bar at the member's block corner (gMemberFrameX[who & 1], gMemberFrameY[who >> 1],
   the x shifted by 0xB0 while gDrawPage is 0). Two conditions chosen from the record decide
   the shape: `outline` is set by effectFlags bits 0x302, the descending penalty counter mDF or the
   countdown mE0; `hatch` by effectFlags bits 0x4c8 or the gPartySpellFlags bits 0x30. When neither holds the bar is
   only a frame of four lines (colours gPanelHighlight, gPanelShadow, 0xC); when only `outline` holds it is a
   DrawRect in `colour` (6 or 3 by video mode); when only `hatch` holds a DrawRect in colour 5;
   when both hold the border alternates, each row in `colour` for `outline` and 5 for `hatch`. What
   the individual effectFlags bits mean is not established (mDF is the penalty counter, mE0 a countdown:
   src/E0DE8.C:563-566, src/E0EF3.C:39). Called by RedrawMember (0991) only. The flag tests read
   effectFlags as a long, so they take the `mov ax,es:[bx+0xE3]; and ax,K; or ax,0` form (the long bit test
   at 0D1C:0B17). */
/*@ 1547:0009 */ void DrawMemberFrame(int who)
{
    int colour;
    struct CharRec far *rec;
    int outline;
    int hatch;
    int i;
    int base;
    int x;
    int y;

    colour = gGraphicsChoice == 0 ? 3 : 6;
    x = gMemberFrameX[who & 1];
    y = gMemberFrameY[who >> 1];
    if (!gDrawPage)
        x += 0xb0;
    rec = &gChars[who];
    outline = (rec->effectFlags & 0x302) || rec->mDF || rec->mE0;
    hatch = (rec->effectFlags & 0x4c8) || (gPartySpellFlags & 0x30);
    if (!outline && !hatch) {
        gDrawLine(x, y, x + 0x3e, y, gPanelHighlight);
        gDrawLine(x, y + 0x31, x + 0x3e, y + 0x31, gPanelShadow);
        gDrawLine(x, y, x, y + 0x33, 0xc);
        gDrawLine(x + 0x3f, y, x + 0x3f, y + 0x33, 0xc);
    } else {
        if (outline && !hatch) {
            DrawRect(x, y, x + 0x3f, y + 0x31, colour);
        } else if (hatch && !outline) {
            DrawRect(x, y, x + 0x3f, y + 0x31, 5);
        } else {
            base = x;
            for (i = 0; i < 0x40; i += 0x10) {
                x = base + i;
                if (outline) {
                    gDrawLine(x, y, x + 7, y, colour);
                    gDrawLine(x + 8, y + 0x31, x + 0xf, y + 0x31, colour);
                }
                if (hatch) {
                    gDrawLine(x + 8, y, x + 0xf, y, 5);
                    gDrawLine(x, y + 0x31, x + 7, y + 0x31, 5);
                }
            }
            x = base;
            base = y;
            for (i = 1; i < 0x30; i += 0xc) {
                y = base + i - 1;
                if (hatch) {
                    gDrawLine(x, y + 1, x, y + 6, 5);
                    gDrawLine(x + 0x3f, y + 7, x + 0x3f, y + 0xc, 5);
                }
                if (outline) {
                    gDrawLine(x, y + 7, x, y + 0xc, colour);
                    gDrawLine(x + 0x3f, y + 1, x + 0x3f, y + 6, colour);
                }
            }
        }
    }
}

/* 1547:028C: draws a horizontal ratio bar: a filled panel at (x - 1, y - 1) to (x + width + 1,
   y + height) in the panel colours gPanelShadow/gPanelHighlight, then the bar itself from x to x + width, height
   `height` lines, the filled part (width * value / max, clamped to [0, max] first and forced to
   at least 1 when value is positive) in `filledColour` and the rest in `emptyColour`. Returns
   immediately when max < 1. The 033F caller shifts both the member's hp and its hp max up by 10
   before passing them. */
/*@ 1547:028C */ void DrawRatioBar(int x, int y, int value, int max, int width, int height,
        int filledColour, int emptyColour)
{
    int amount;
    int part;

    amount = value;
    if (max < 1)
        return;
    if (amount < 0)
        amount = 0;
    if (amount > max)
        amount = max;
    part = width * amount / max;
    if (part <= 0 && amount > 0)
        part = 1;
    height--;
    DrawFilledPanel(x - 1, y - 1, x + width + 1, y + height + 1, gPanelShadow, gPanelHighlight, -1);
    if (part > 0)
        gFillRect(x, y, x + part, y + height, filledColour);
    if (part < width)
        gFillRect(x + part, y, x + width, y + height, emptyColour);
}
/* 1547:033F: draws one member's HP line. It returns unless the member passes MemberIs(member,
   1) and, while gPanelMode is nonzero, is also the selected member gInventoryMember. The line is at (gHpLineX[member &
   1], gHpLineY[member >> 1]), shifted right by 0xB0 while gDrawPage is 0, or fixed at (0xfa, 0x10)
   while gPanelMode is nonzero. When gBarGraphsOn is set it draws the "HP" label (only while gPanelMode is 0) and a
   ratio bar (DrawRatioBar) of hp + 10 against maxHp + 10 (the +10 keeps the bar visible at negative
   hit points), the colour picked by a wound state: 1 when (maxHp + 10) / 3 exceeds hp + 10, 2 when
   hp + 10 is 10 or less (hp at or below 0), else 0; when gBarGraphsOn is 0 it prints "%3d of %-3d" (hp,
   maxHp) instead. Called by RedrawMember (0991) and HealCharacter (0DE8:0ADB, src/E0DE8.C:592). */
/*@ 1547:033F */ void DrawMemberHp(int member)
{
    int state;
    int value;
    int max;
    int width;
    int height;
    struct CharRec far *rec;
    char buf[20];
    int x;
    int y;

    if (!MemberIs(member, 1))
        return;
    rec = &gChars[member];
    if (gPanelMode != 0 && gInventoryMember != member)
        return;
    if (gPanelMode == 0) {
        x = gHpLineX[member & 1];
        y = gHpLineY[member >> 1];
        width = 0x26;
        height = 3;
        if (!gDrawPage)
            x += 0xb0;
    } else {
        x = 0xfa;
        y = 0x10;
        width = 0x33;
        height = 5;
    }
    if (gBarGraphsOn) {
        value = rec->hp + 10;
        max = rec->maxHp + 10;
        state = 0;
        if (max / 3 > value)
            state = 1;
        if (value <= 10)
            state = 2;
        if (gPanelMode == 0)
            gDrawText("HP", x - 0xd, y - 1, 0xc, gPanelFillColour);
        DrawRatioBar(x, y, value, max, width, height, gHpBarColours[state], gBarEmptyColour);
    } else {
        sprintf(buf, "%3d of %-3d", rec->hp, rec->maxHp);
        if (gPanelMode == 0) {
            x -= 0xd;
            y -= 1;
        }
        gDrawText(buf, x, y, 0xc, gPanelFillColour);
    }
}
/* 1547:04A0: draws one member's food/hunger bar (the small bar under the HP line while the
   inventory is up). It returns unless gPanelMode is nonzero, the member passes MemberIs(member, 1),
   status bit 0 (alive) is set and the member is the selected gInventoryMember. The bar is at (0xfa, 0x19): value
   food (the food value) against 100, 51 wide, 5 tall, the colour picked by a hunger state (the lower
   the food value the hungrier: 1 below 0x21, 2 below 0x14, else 0). Called from RedrawMember (0991),
   EatFood (0ACD:07E0, src/E0ACD.C:577) and DecayMemberState (1414:0C82, src/E1414.C:657). */
/*@ 1547:04A0 */ void DrawMemberFood(int member)
{
    struct CharRec far *rec;
    int state;

    if (gPanelMode == 0)
        return;
    if (!MemberIs(member, 1))
        return;
    rec = &gChars[member];
    if (!(rec->status & 1))
        return;
    if (gPanelMode && gInventoryMember != member)
        return;
    state = 0;
    if (rec->food < 0x21)
        state = 1;
    if (rec->food < 0x14)
        state = 2;
    DrawRatioBar(0xfa, 0x19, rec->food, 0x64, 0x33, 5, gFoodBarColours[state], gBarEmptyColour);
}
/* 1547:0544: draws one member's portrait on the character screen. It returns unless the member
   passes MemberIs(who, 1). The portrait corner is (gPortraitX[who & 1], gPortraitY[who >> 1]), the x
   shifted by 0xB0 while gDrawPage is 0, or the fixed (0xb5, 3) while gPanelMode is nonzero (and
   only for the selected member gInventoryMember). At hp == -10 it draws the dead portrait gDeadPortraitIcon and
   returns; when effectFlags bit 2 is set and gPanelMode is 0 it fills the portrait in colour 0xC and returns;
   otherwise it draws the member's own portrait pointer (rec + 0x33) and, at hp below 1, the
   wounded overlay gUnconsciousIcon. Called by RedrawMember (0991) and DamageCharacter (0DE8:09B9,
   src/E0DE8.C:573). The effectFlags bit test reads it as a long, so it takes the `and ax,4; or ax,0` form
   (the long bit test at 0D1C:0B17). */
/*@ 1547:0544 */ void DrawMemberPortrait(int who)
{
    struct CharRec far *rec;
    int x;
    int y;

    rec = &gChars[who];
    if (!MemberIs(who, 1))
        return;
    if (gPanelMode == 0) {
        x = gPortraitX[who & 1];
        y = gPortraitY[who >> 1];
        if (!gDrawPage)
            x += 0xb0;
    } else {
        if (gInventoryMember != who)
            return;
        x = 0xb5;
        y = 3;
    }
    if (rec->hp == -10) {
        DrawShape(gDrawPage, gDeadPortraitIcon, x, y, 0);
        return;
    }
    if (rec->effectFlags & 4) {
        if (gPanelMode == 0) {
            gFillRect(x, y, x + 0x1f, y + 0x1f, 0xc);
            return;
        }
    }
    DrawShape(gDrawPage, rec->portrait, x, y, 0);
    if (rec->hp < 1)
        DrawShape(gDrawPage, gUnconsciousIcon, x, y, 0);
}
/* 1547:0642: draws the label of an item-use result at (x, y). It clears the second line buffer,
   sets the colour (0xF, or 2 while the video mode gGraphicsChoice is 0), and then either draws the
   thrown-item icon gThrownIcon4 at (x - 1, y) through DrawShape when `result` is above -3, or
   fills a panel (x, y)-(x + 0x1E, y + 0xF) in colours gFailLabelLight/gFailLabelShadow/gFailLabelFill. The switch on `result`
   fills `name` (6 bytes) and the optional `suffix` (6 bytes, empty for the default): -1 "MISS" (and
   y + 1), -2 "HACK", -3 "CAN'T"+"REACH", -4 "NO"+"AMMO", anything else the decimal `result`. It
   draws `name` centred (indent 0x10 - strlen*6/2) at (x + indent, y + 5), or `name` at (x + indent,
   y + 2) and `suffix` at (x + indent, y + 9) when the suffix is set. Called by DrawMemberSlot (07E7)
   only. `result` is what UseItem (0DE8:0763, src/E0DE8.C:499) stored in the record's lastAction[slot]. */
/*@ 1547:0642 */ void DrawActionLabel(int x, int y, int result)
{
    char name[6];
    char suffix[6];
    char indent;
    char colour;

    suffix[0] = 0;
    colour = 0xf;
    if (gGraphicsChoice == 0)
        colour = 2;
    if (result > -3) {
        DrawShape(gDrawPage, gThrownIcon4, x - 1, y, 0);
    } else {
        DrawFilledPanel(x, y, x + 0x1e, y + 0xf, gFailLabelLight, gFailLabelShadow, gFailLabelFill);
    }
    switch (result) {
    case -1:
        y++;
        strcpy(name, "MISS");
        break;
    case -2:
        strcpy(name, "HACK");
        break;
    case -3:
        strcpy(name, "CAN'T");
        strcpy(suffix, "REACH");
        break;
    case -4:
        strcpy(name, "NO");
        strcpy(suffix, "AMMO");
        break;
    default:
        sprintf(name, "%d", result);
        break;
    }
    if (suffix[0] == 0) {
        indent = 0x10 - strlen(name) * 6 / 2;
        gDrawText(name, x + indent, y + 5, colour, 0);
    } else {
        indent = 0x10 - strlen(name) * 6 / 2;
        gDrawText(name, x + indent, y + 2, colour, 0);
        indent = 0x10 - strlen(suffix) * 6 / 2;
        gDrawText(suffix, x + indent, y + 9, colour, 0);
    }
}
/* 1547:07E7: draws one carried item slot of member `who` (0 or 1) on the character screen. The
   slot corner is (gHandSlotX[who & 1], gHandSlotY[(who & 6) + slot]), the x shifted by 0xB0 while no status
   line is up. It hides the pointer, fills the panel in gPanelHighlight/gPanelShadow/gPanelFillColour, and outlines it in colour
   0xF when the mouse is DISABLED and the slot is the selected one (gMouseDisabled, gInventoryMember, gCursorSlot).
   When the record's lastAction[slot] is set it draws the item-use label DrawActionLabel; else it draws the
   item icon through DrawItemIcon, or the empty-slot icon gEmptySlotIcons[slot] through DrawShape,
   and then, in that else branch only, draws the highlight icon gHandShadeIcon when the slot's m1E bit is
   set, or CanUseHand(who, slot) returns 0, or hp is 0 or below, or status bit 2 (paralyzed) is set.
   Called by RedrawMember (0991) and RemoveCarriedItems (0D1C:01CA, src/E0D1C.C:294). */
/*@ 1547:07E7 */ void DrawMemberSlot(char who, char slot)
{
    int item;
    int x;
    int y;

    x = gHandSlotX[who & 1];
    y = gHandSlotY[(who & 6) + slot];
    if (!gDrawPage)
        x += 0xb0;
    item = gChars[who].items[slot];
    LockHidePointer();
    DrawFilledPanel(x, y, x + 0x1e, y + 0xf, gPanelHighlight, gPanelShadow, gPanelFillColour);
    if (gMouseDisabled && who == gInventoryMember && slot == gCursorSlot)
        DrawRect(x, y, x + 0x1e, y + 0xf, 0xf);
    if (gChars[who].lastAction[slot] != 0) {
        DrawActionLabel(x, y, gChars[who].lastAction[slot]);
    } else {
        if (item != 0)
            DrawItemIcon(gDrawPage, item, x + 8, y);
        else
            DrawShape(gDrawPage, gEmptySlotIcons[slot], x + 8, y, 0);
        if ((gChars[who].m1E & (1 << slot)) || CanUseHand(who, slot) == 0
                || gChars[who].hp <= 0 || (gChars[who].status & 4))
            DrawShape(gDrawPage, gHandShadeIcon, x, y, 0);
    }
    LockShowPointer();
}
/* 1547:0991: redraws one member's block on the character screen. It returns unless the member
   passes MemberIs(who, 1). The block layout depends on the screen mode gPanelMode: mode 0 (the
   plain character screen) draws the name (or "Swapping" while this member is the one being moved,
   gIconTable), the portrait, the two carried-item slots, the HP line, the status bar and, while
   lastDamage (the last damage amount) is above zero, the thrown-damage icon gThrownIcon3 and its number;
   mode 1 (the inventory up) draws the HP line, the food bar, one status line (DEAD, UNCONSCIOUS,
   POISON (SLOW), POISONED or PARALYZED) and the item rows; mode 2 (the character information
   panel) draws the stat panel with the class, race and alignment names, the six stat lines, armor
   class, EXP/LVL and the per-class slot lines. The name colour and text attribute come from status
   bits 1 and 2 (8/6), otherwise 0xc/0xf; status bit 0 is the alive bit, bit 1 poisoned and bit 2
   paralyzed (E1414.C:476-477 clears them at the end of the effect; E0DE8.C:558 keeps only bit 0
   at death). Called by several screens with the member index (for example Action0264, 0ACD:0264,
   while moving a member, src/E0ACD.C:302). */
/*@ 1547:0991 */ void RedrawMember(int who)
{
    struct CharRec far *rec;
    char far *savedFont;
    int n;
    int classSlot;
    int slotValue;
    int savedStatusLine;
    long expValue;
    char buf[6];
    int textColour;
    int textAttr;
    int x;
    int y;

    if (!MemberIs(who, 1))
        return;
    rec = &gChars[who];
    if (rec->status & 6) {
        textColour = 8;
        textAttr = 6;
    } else {
        textColour = 0xc;
        textAttr = 0xf;
    }
    switch (gPanelMode) {
    case 0:
        LockHidePointer();
        savedFont = gCurFont;
        SetFont(gFont6x6);
        x = gMemberBlockCol[who & 1];
        y = gMemberBlockY[who >> 1];
        CopyRegion(0x16, 0xa8, x, y, 8, 0x18, 2, 2);
        CopyRegion(0x1e, 0xa8, x, y + 0x18, 8, 0x1a, 2, 2);
        savedStatusLine = gDrawPage;
        gDrawPage = 2;
        x <<= 3;    /* the layout tables hold 8-pixel columns; the text routines take pixels */
        if (gIconTable == who)
            gDrawText("Swapping", x + 2, y + 2, 8, gPanelFillColour);
        else
            gDrawText(rec->name, x + 2, y + 2, textColour, gPanelFillColour);
        DrawMemberPortrait(who);
        DrawMemberSlot(who, 0);
        DrawMemberSlot(who, 1);
        DrawMemberHp(who);
        DrawMemberFrame(who);
        if (rec->lastDamage > 0) {
            itoa(rec->lastDamage, buf, 10);
            DrawShape(2, gThrownIcon3, x + 0xd, y + 0x1e, 0);
            gDrawText(buf, x + 0x22 - strlen(buf) * 3, y + 0x2a,
                    gGraphicsChoice == 0 ? 0xc : 0xf, 0);
        }
        gDrawPage = savedStatusLine;
        if (!gDrawPage)
            CopyRegion(x >> 3, y, gMemberScreenCol[who & 1], y, 8, 0x32, 2, 0);
        SetFont(savedFont);
        LockShowPointer();
        return;
    case 1:
    case 2:
        if (gInventoryMember != who)
            return;
        CopyRegion(0x16, 0, 0, 0, 0x12, 0xa8, 2, 2);
        gDrawPage = 2;
        DrawMemberPortrait(who);
        DrawShadowText(rec->name, 0xdb, 6, textAttr, gPanelFillColour);
        DrawMemberHp(who);
        DrawMemberFood(who);
        if (gPanelMode == 1) {
            /* one status line is drawn at y 0x9e, a different x per label (0xf7 DEAD, 0xe2
               UNCONSCIOUS, 0xdc POISON (SLOW), 0xeb POISONED, 0xe8 PARALYZED); the hp + 10 and the
               unsigned hp + 9 range test are how the original compiles hp == -10 (DEAD) and
               -9 <= hp <= 0 (UNCONSCIOUS) */
            if (rec->hp + 10 == 0)
                DrawShadowText("DEAD", 0xf7, 0x9e, 6, gAltPanelFill);
            else if ((unsigned)(rec->hp + 9) < 10)
                DrawShadowText("UNCONSCIOUS", 0xe2, 0x9e, 6, gAltPanelFill);
            else if ((rec->status & 2) && (rec->effectFlags & 0x80))
                DrawShadowText("POISON (SLOW)", 0xdc, 0x9e, 6, gAltPanelFill);
            else if (rec->status & 2)
                DrawShadowText("POISONED", 0xeb, 0x9e, 6, gAltPanelFill);
            else if (rec->status & 4)
                DrawShadowText("PARALYZED", 0xe8, 0x9e, 6, gAltPanelFill);
            for (n = 0; n < 0x1b; n++)
                DrawItemRow(n, 0, 2);
            DrawItemRow(0x10, 1, 2);
            DrawItemRow(gCursorSlot, 1, 2);
        } else {
            for (n = 0; n < 3; n++)
                gFillRect(gInfoBoxLeft[n], gInfoBoxTop[n], gInfoBoxRight[n], gInfoBoxBottom[n], gAltPanelFill);
            DrawShadowText("CHARACTER INFO", 0xb7, 0x2a, 0xf, gAltPanelFill);
            gDrawText(gClassNames[rec->classIndex], 0xb7, 0x37, 0xc, gAltPanelFill);
            gDrawText(gAlignmentNames[rec->alignment], 0xb7, 0x3e, 0xc, gAltPanelFill);
            gDrawText(gRaceNames[rec->race], 0xb7, 0x45, 0xc, gAltPanelFill);
            for (n = 0; n < 6; n++)
                gDrawText(gStatNames[n], 0xb7, n * 7 + 0x52, 0xc, gAltPanelFill);
            gDrawText("ARMOR CLASS", 0xb7, 0x7c, 0xc, gAltPanelFill);
            gDrawText("EXP", 0xef, 0x8a, 0xc, gAltPanelFill);
            gDrawText("LVL", 0x116, 0x8a, 0xc, gAltPanelFill);
            gDrawText(FormatStrengthPercent(rec->strength, rec->strengthPct), 0x113, 0x52, 0xf, gAltPanelFill);
            PrintInt(rec->intelligence, 0x113, 0x59, 0xf, gAltPanelFill);   /* INTELLIGENCE */
            PrintInt(rec->wisdom, 0x113, 0x60, 0xf, gAltPanelFill);   /* WISDOM */
            PrintInt(rec->dexterity, 0x113, 0x67, 0xf, gAltPanelFill);   /* DEXTERITY */
            PrintInt(rec->constitution, 0x113, 0x6e, 0xf, gAltPanelFill);   /* CONSTITUTION */
            PrintInt(rec->charisma, 0x113, 0x75, 0xf, gAltPanelFill);   /* CHARISMA */
            PrintInt(rec->armorClass, 0x113, 0x7c, 0xf, gAltPanelFill);   /* ARMOR CLASS */
            for (n = 0; n < 3; n++) {
                /* the three per-class-slot lines: the slot's name, its experience total (as a
                   long, right-aligned at x 0xf7) and its level levels (right-aligned at 0x11e, the
                   value LevelUpSlot increments); GetClassSlotByte returns -1 for a class with no
                   such slot */
                classSlot = GetClassSlotByte(rec->classIndex, n);
                if (classSlot == -1)
                    continue;
                gDrawText(gBaseClassNames[classSlot], 0xb7, n * 7 + 0x91, 0xc, gAltPanelFill);
                expValue = rec->experience[n];
                slotValue = rec->levels[n];
                PrintLong(expValue, 0xf7 - NumberWidthLong(expValue), n * 7 + 0x91,
                        0xf, gAltPanelFill);
                PrintInt(slotValue, 0x11e - NumberWidthInt(slotValue), n * 7 + 0x91,
                        0xf, gAltPanelFill);
            }
        }
        gDrawPage = 0;
        LockHidePointer();
        CopyRegion(0x16, 0, 0x16, 0, 0x12, 0xa8, 2, 0);
        LockShowPointer();
        CopyRegion(0, 0, 0x16, 0, 0x12, 0xa8, 2, 2);
        break;
    }
}
