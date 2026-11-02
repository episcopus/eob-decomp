/* Unit at EOB.EXE 1547:0009 (4362 bytes, 8 functions): the character information screen. It draws
   one party member's block: the status bar (0009), the HP line and ratio bar (033F), the portrait
   (0544), the carried item slots (07E7), the action label (0642) and the title and stat lines
   (0991, which calls the others). It reads the 243-byte character records at ds:F580 (struct
   CharRec, src/E0A72.C), the stat/class/race/alignment and per-class-slot name tables at ds:1195,
   ds:11AD, ds:11DD, ds:1219 and ds:1240, and the per-member row layout byte tables at
   ds:5D1C..ds:5D4F. All 8 functions are at L2. The record's mE3 field is a long, so its bit tests
   take the `and ax,K; or ax,0` form (the long bit test at 0D1C:0B17, re/symbols.toml e0d1c_0a43,
   is the evidence for the long type). */

/*@flags -ml -O -Z */

struct CharRec {            /* the 243-byte (0xF3) records at ds:F580 (src/E0A72.C, src/E0D1C.C) */
    char pad00;
    unsigned char m01;      /* +01: status bits: bit 0 alive, bit 1 poisoned, bit 2 paralyzed
                               (src/E0DE8.C:558 keeps only bit 0 at death; src/E1414.C:476-477
                               clears bit 1 at the end of poison and bit 2 at the end of paralysis) */
    char name[0x0B];        /* +02: the NUL-padded name (src/EOV6.C) */
    signed char m0D;        /* +0D: the strength score, drawn by FormatStrengthPercent (src/E0A72.C) */
    signed char m0E;        /* +0E (src/E1414.C) */
    signed char m0F;        /* +0F: the exceptional strength percentile (src/E0A72.C) */
    signed char m10;        /* +10 (src/E1414.C) */
    signed char m11;        /* +11: INTELLIGENCE (ds:1195[1]), the first of the five stat bytes
                               after strength (src/EOV0.C:442-444 formats m11, m13, m15, m17, m19) */
    signed char m12;        /* +12 */
    signed char m13;        /* +13: WISDOM (ds:1195[2]) */
    signed char m14;        /* +14 */
    signed char m15;        /* +15: DEXTERITY (ds:1195[3]); indexes the armour-class base table
                               (GetArmourClassBase, 0A72:019D) */
    signed char m16;        /* +16 */
    signed char m17;        /* +17: CONSTITUTION (ds:1195[4]) */
    signed char m18;        /* +18 */
    signed char m19;        /* +19: CHARISMA (ds:1195[5]) */
    signed char m1A;        /* +1A */
    signed char hp;         /* +1B: current hit points (src/E1414.C) */
    signed char hpMax;      /* +1C: maximum hit points, the second %3d of the HP line */
    signed char m1D;        /* +1D: the armour-class byte, drawn on the ARMOR CLASS row
                               (src/E08E6.C:60 and src/E0EF3.C:25 name it the armour-class byte) */
    signed char m1E;        /* +1E: a per-slot bit toggled by UseItem's `m1E ^= 1 << slot`
                               (0DE8:0763); DrawMemberSlot tests bit `slot` (src/E1414.C) */
    signed char race;       /* +1F: index into the race-name table ds:11AD */
    signed char class;      /* +20: index into the class-name table ds:11DD (src/E0A72.C) */
    signed char align;      /* +21: index into the alignment-name table ds:1240 */
    signed char m22;        /* +22 */
    unsigned char m23;      /* +23: the hunger/food value, clamped to 100 by EatFood (0ACD:07E0)
                               and decremented each tick for a living member by DecayMemberState
                               (1414:0C82); unsigned here (the code tests < 0x21, < 0x14), while
                               src/E0BD2.C:56, src/E0EF3.C:30 and src/EOV0.C:82 declare it signed */
    signed char m24[3];     /* +24: three per-class-slot bytes (src/E0D1C.C) */
    long exp[3];            /* +27: three per-class-slot experience totals (src/E0DE8.C:33) */
    char far *portrait;     /* +33: the far pointer 0544 draws for the member (src/EOV3.C:120 calls
                               it `sprite`, src/EOV5.C:36 `m33`: a cross-unit name to settle) */
    char pad37[0x77 - 0x37];
    int m77[27];            /* +77: 27 word slots, the carried items (src/E0D1C.C) */
    char padAD[0xDF - 0xAD];
    signed char mDF;        /* +0DF: a descending penalty counter; when it runs out the armour class
                               is recomputed (src/E0DE8.C:563-566, src/E0EF3.C:1179 "penalty byte") */
    signed char mE0;        /* +0E0: a countdown (src/E0EF3.C:39) */
    char padE1[0xE3 - 0xE1];
    long mE3;               /* +0E3: status/effect flags: a long, so the bit tests compile to the
                               `mov ax,..; and ax,K; or ax,0` form (src/E1414.C, 0D1C:0B17) */
    signed char mE7;        /* +0E7: the damage amount last stored by DamageCharacter
                               (src/E0DE8.C:553), drawn as a number while above zero */
    signed char mE8[0xF3 - 0xE8]; /* +0E8: per-slot item-use results: UseItem stores its `result`
                               there (0DE8:0763), -1 for a forced miss and -2..-4 for failures */
};

extern int gGraphicsChoice; /*@ ds:8102 */         /* the video mode the game initialised (src/E082C.C) */
extern int gStatusLine; /*@ ds:8115 */         /* nonzero while a status line is up (src/E082C.C) */
extern unsigned char gFBD2; /*@ ds:FBD2 */ /* bit 0 gates the icon colour row (src/E0D1C.C); the 0x30
                                              bits DrawMemberBar tests (:163) are not understood */
extern signed char gFB4C; /*@ ds:FB4C */ /* a colour (src/E0D1C.C) */
extern signed char gF53A; /*@ ds:F53A */ /* a colour (src/E0D1C.C) */
extern struct CharRec gChars[]; /*@ ds:F580 */ /* the six character records */
extern unsigned char g5D1F[]; /*@ ds:5D1F */ /* member bar x by (who & 1): 8, 80 */
extern signed char g5D21[]; /*@ ds:5D21 */   /* member bar y by (who >> 1): 2, 54, 106 */
extern unsigned char g5D26[]; /*@ ds:5D26 */ /* HP line y by (who >> 1): 46, 98, 150 */
extern signed char g5D24[]; /*@ ds:5D24 */   /* HP line x by (who & 1): 23, 95 */
extern signed char g5D1C[]; /*@ ds:5D1C */   /* HP ratio bar colour by state (0, 1, 2) */
extern signed char g5D29[]; /*@ ds:5D29 */   /* food bar colour by state (0, 1, 2) */
extern char gFB44; /*@ ds:FB44 */            /* a screen mode: 0 the plain character screen, 1 the
                                                inventory up, 2 the character information panel; the
                                                three pick RedrawMember's branches and gate the other
                                                redraws (src/E0ACD.C:20) */
extern signed char gA99A; /*@ ds:A99A */     /* the selected member (src/E0ACD.C) */
extern signed char gF57E; /*@ ds:F57E */     /* a colour (src/EOV2.C) */
extern char gFB7A; /*@ ds:FB7A */            /* a colour: the empty part of DrawRatioBar (:279), set
                                                by SetUiPalette (src/EOV2.C:134) */
extern char gBarGraphsOn; /*@ ds:F43C */            /* nonzero after the startup allocations (src/E07A9.C);
                                                picks how DrawMemberHp shows a member: set draws the
                                                "HP" label and the ratio bar, clear prints
                                                "%3d of %-3d" (:269-286) */
extern char gA95A; /*@ ds:A95A */            /* a colour (src/EOV2.C) */
extern char g8A6C; /*@ ds:8A6C */            /* a colour (src/EOV2.C) */
extern char g8A7D; /*@ ds:8A7D */            /* a colour (src/EOV2.C) */
extern void far *gThrownIcon4; /*@ ds:EFBE */ /* the fourth thrown-item icon (src/EOV2.C) */
extern signed char g5D31[]; /*@ ds:5D31 */ /* item slot x by (who & 1): 40, 112 */
extern unsigned char g5D33[]; /*@ ds:5D33 */ /* item slot y by ((who & 6) + slot) */
extern int gMouseDisabled; /*@ ds:814A */    /* nonzero disables the mouse (src/E07A9.C) */
extern char gF51A; /*@ ds:F51A */            /* the selected row (src/E0ACD.C) */
extern void far *gIcon8A7E; /*@ ds:8A7E */   /* the empty-slot highlight icon (src/EOV2.C) */
extern void far *gIconF2F4; /*@ ds:F2F4 */   /* the dead-member portrait icon (src/EOV2.C) */
extern void far *gIconDC94; /*@ ds:DC94 */   /* the wounded-member overlay icon (src/EOV2.C) */
extern signed char g5D2C[]; /*@ ds:5D2C */   /* portrait x by (who & 1): 8, 80 */
extern signed char g5D2E[]; /*@ ds:5D2E */   /* portrait y by (who >> 1): 11, 63, 115 */
extern void far *gIconsDDE4[]; /*@ ds:DDE4 */ /* the item icon far pointers (src/E0D1C.C) */
extern void far *gEmptySlotIcons[]; /*@ ds:DF38 */ /* the two no-item slot icons, gIconsDDE4[0x55]
                                                  and [0x56] (0xDDE4 + 0x55 * 4) */

extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int colour); /*@ ds:8062 */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void (far *gDrawText)(char far *text, int x, int y, int colour, int extra); /*@ ds:80CA */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern char *strcpy(char far *dest, char far *src); /*@ 0000:67CD */
extern void DrawRect(int x1, int y1, int x2, int y2, int colour); /*@ 082C:04C4 */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int colourA, int colourB, int fill); /*@ 082C:09D5 */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern void f1A29_000F(int statusLine, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void DrawItemIcon(int mode, int n, int x, int y); /*@ 0D1C:070F */
extern int f0D1C_07DB(int who, int flag); /*@ 0D1C:07DB */

extern char far *gFont1A47; /*@ ds:FE4E */  /* the current font (src/E1A47.C) */
extern char far *gA9C0; /*@ ds:A9C0 */      /* the 6x6 font loaded from FONT6.FNT (src/E07A9.C) */
extern char gIconTable; /*@ ds:FB34 */          /* the party member being moved, -1 when none
                                                (src/E0ACD.C:42); src/EOV2.C:142 records the
                                                conflict with src/E0EF3.C:227, which reads ds:FB34 as
                                                an array of far sprite pointers */
extern char gFB7B; /*@ ds:FB7B */          /* a colour (src/EOV2.C) */
extern void far *gThrownIcon3; /*@ ds:EFB4 */ /* the third thrown-item icon (src/EOV2.C) */

/* the stat panel's per-member row layout: x and y of the name and the stat block (0991 only) */
extern signed char g5D4B[]; /*@ ds:5D4B */ /* name/bar x by (who & 1): 1, 10 */
extern signed char g5D4D[]; /*@ ds:5D4D */ /* stat block x by (who & 1): 23, 32 */
extern signed char g5D4F[]; /*@ ds:5D4F */ /* y by (who >> 1): 2, 54, 106 */
/* the three item columns of the gFB44 == 2 screen, x1/y1/x2/y2 per column (0991 only) */
extern unsigned g5D39[]; /*@ ds:5D39 */      /* column x1: 179, 272, 301 */
extern unsigned char g5D3F[]; /*@ ds:5D3F */ /* column y1: 36, 51, 51 */
extern unsigned g5D42[]; /*@ ds:5D42 */      /* column x2: 271, 300, 318 */
extern unsigned char g5D48[]; /*@ ds:5D48 */ /* column y2: 165, 165, 147 */
/* the far string tables: stat, race, class and alignment names and the per-class-slot names */
extern char far *g1195[]; /*@ ds:1195 */     /* the six stat names */
extern char far *g11AD[]; /*@ ds:11AD */     /* the race names */
extern char far *g11DD[]; /*@ ds:11DD */     /* the class names */
extern char far *g1219[]; /*@ ds:1219 */     /* the per-class-slot names (GetClassSlotByte index) */
extern char far *g1240[]; /*@ ds:1240 */     /* the alignment names */

extern char *itoa(int value, char *string, int radix); /*@ 0000:62AA */
extern void f1A47_0169(char far *font); /*@ 1A47:0169 */
extern void DrawShadowText(char far *text, int x, int y, int color, int extra); /*@ 082C:0898 */
extern void PrintInt(int value, int a, int b, int c, int d); /*@ 1AA6:000F */
extern void f1BA4_000C(long value, int a, int b, int c, int d); /*@ 1BA4:000C */
extern int NumberWidthLong(long value); /*@ 082C:0974 */
extern int NumberWidthInt(int value); /*@ 082C:09A6 */
extern char *FormatStrengthPercent(char score, char percent); /*@ 0A72:0468 */
extern char GetClassSlotByte(char row, char slot); /*@ 0A72:0509 */
extern void DrawItemRow(int who, int slot, int one); /*@ 0BD2:0917 */
/* 1547:0009: draws the status bar at the member's block corner (g5D1F[who & 1], g5D21[who >> 1],
   the x shifted by 0xB0 while no status line is up). Two conditions chosen from the record decide
   the shape: `outline` is set by mE3 bits 0x302, the descending penalty counter mDF or the
   countdown mE0; `hatch` by mE3 bits 0x4c8 or the gFBD2 bits 0x30. When neither holds the bar is
   only a frame of four lines (colours gFB4C, gF53A, 0xC); when only `outline` holds it is a
   DrawRect in `colour` (6 or 3 by video mode); when only `hatch` holds a DrawRect in colour 5;
   when both hold the border alternates, each row in `colour` for `outline` and 5 for `hatch`. What
   the individual mE3 bits mean is not established (mDF is the penalty counter, mE0 a countdown:
   src/E0DE8.C:563-566, src/E0EF3.C:39). Called by RedrawMember (0991) only. The flag tests read
   mE3 as a long, so they take the `mov ax,es:[bx+0xE3]; and ax,K; or ax,0` form (the long bit test
   at 0D1C:0B17). */
/*@ 1547:0009 */ void DrawMemberBar(int who)
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
    x = g5D1F[who & 1];
    y = g5D21[who >> 1];
    if (!gStatusLine)
        x += 0xb0;
    rec = &gChars[who];
    outline = (rec->mE3 & 0x302) || rec->mDF || rec->mE0;
    hatch = (rec->mE3 & 0x4c8) || (gFBD2 & 0x30);
    if (!outline && !hatch) {
        gDrawLine(x, y, x + 0x3e, y, gFB4C);
        gDrawLine(x, y + 0x31, x + 0x3e, y + 0x31, gF53A);
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
   y + height) in the panel colours gF53A/gFB4C, then the bar itself from x to x + width, height
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
    DrawFilledPanel(x - 1, y - 1, x + width + 1, y + height + 1, gF53A, gFB4C, -1);
    if (part > 0)
        gFillRect(x, y, x + part, y + height, filledColour);
    if (part < width)
        gFillRect(x + part, y, x + width, y + height, emptyColour);
}
/* 1547:033F: draws one member's HP line. It returns unless the member passes f0D1C_0670(member,
   1) and, while gFB44 is nonzero, is also the selected member gA99A. The line is at (g5D24[member &
   1], g5D26[member >> 1]), shifted right by 0xB0 while no status line is up, or fixed at (0xfa, 0x10)
   while gFB44 is nonzero. When gBarGraphsOn is set it draws the "HP" label (only while gFB44 is 0) and a
   ratio bar (DrawRatioBar) of hp + 10 against hpMax + 10 (the +10 keeps the bar visible at negative
   hit points), the colour picked by a wound state: 1 when (hpMax + 10) / 3 exceeds hp + 10, 2 when
   hp + 10 is 10 or less (hp at or below 0), else 0; when gBarGraphsOn is 0 it prints "%3d of %-3d" (hp,
   hpMax) instead. Called by RedrawMember (0991) and HealCharacter (0DE8:0ADB, src/E0DE8.C:592). */
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

    if (!f0D1C_0670(member, 1))
        return;
    rec = &gChars[member];
    if (gFB44 != 0 && gA99A != member)
        return;
    if (gFB44 == 0) {
        x = g5D24[member & 1];
        y = g5D26[member >> 1];
        width = 0x26;
        height = 3;
        if (!gStatusLine)
            x += 0xb0;
    } else {
        x = 0xfa;
        y = 0x10;
        width = 0x33;
        height = 5;
    }
    if (gBarGraphsOn) {
        value = rec->hp + 10;
        max = rec->hpMax + 10;
        state = 0;
        if (max / 3 > value)
            state = 1;
        if (value <= 10)
            state = 2;
        if (gFB44 == 0)
            gDrawText("HP", x - 0xd, y - 1, 0xc, gF57E);
        DrawRatioBar(x, y, value, max, width, height, g5D1C[state], gFB7A);
    } else {
        sprintf(buf, "%3d of %-3d", rec->hp, rec->hpMax);
        if (gFB44 == 0) {
            x -= 0xd;
            y -= 1;
        }
        gDrawText(buf, x, y, 0xc, gF57E);
    }
}
/* 1547:04A0: draws one member's food/hunger bar (the small bar under the HP line while the
   inventory is up). It returns unless gFB44 is nonzero, the member passes f0D1C_0670(member, 1),
   m01 bit 0 (alive) is set and the member is the selected gA99A. The bar is at (0xfa, 0x19): value
   m23 (the food value) against 100, 51 wide, 5 tall, the colour picked by a hunger state (the lower
   the food value the hungrier: 1 below 0x21, 2 below 0x14, else 0). Called from RedrawMember (0991),
   EatFood (0ACD:07E0, src/E0ACD.C:577) and DecayMemberState (1414:0C82, src/E1414.C:657). */
/*@ 1547:04A0 */ void DrawMemberFood(int member)
{
    struct CharRec far *rec;
    int state;

    if (gFB44 == 0)
        return;
    if (!f0D1C_0670(member, 1))
        return;
    rec = &gChars[member];
    if (!(rec->m01 & 1))
        return;
    if (gFB44 && gA99A != member)
        return;
    state = 0;
    if (rec->m23 < 0x21)
        state = 1;
    if (rec->m23 < 0x14)
        state = 2;
    DrawRatioBar(0xfa, 0x19, rec->m23, 0x64, 0x33, 5, g5D29[state], gFB7A);
}
/* 1547:0544: draws one member's portrait on the character screen. It returns unless the member
   passes f0D1C_0670(who, 1). The portrait corner is (g5D2C[who & 1], g5D2E[who >> 1]), the x
   shifted by 0xB0 while no status line is up, or the fixed (0xb5, 3) while gFB44 is nonzero (and
   only for the selected member gA99A). At hp == -10 it draws the dead portrait gIconF2F4 and
   returns; when mE3 bit 2 is set and gFB44 is 0 it fills the portrait in colour 0xC and returns;
   otherwise it draws the member's own portrait pointer (rec + 0x33) and, at hp below 1, the
   wounded overlay gIconDC94. Called by RedrawMember (0991) and DamageCharacter (0DE8:09B9,
   src/E0DE8.C:573). The mE3 bit test reads it as a long, so it takes the `and ax,4; or ax,0` form
   (the long bit test at 0D1C:0B17). */
/*@ 1547:0544 */ void DrawMemberPortrait(int who)
{
    struct CharRec far *rec;
    int x;
    int y;

    rec = &gChars[who];
    if (!f0D1C_0670(who, 1))
        return;
    if (gFB44 == 0) {
        x = g5D2C[who & 1];
        y = g5D2E[who >> 1];
        if (!gStatusLine)
            x += 0xb0;
    } else {
        if (gA99A != who)
            return;
        x = 0xb5;
        y = 3;
    }
    if (rec->hp == -10) {
        f1A29_000F(gStatusLine, gIconF2F4, x, y, 0);
        return;
    }
    if (rec->mE3 & 4) {
        if (gFB44 == 0) {
            gFillRect(x, y, x + 0x1f, y + 0x1f, 0xc);
            return;
        }
    }
    f1A29_000F(gStatusLine, rec->portrait, x, y, 0);
    if (rec->hp < 1)
        f1A29_000F(gStatusLine, gIconDC94, x, y, 0);
}
/* 1547:0642: draws the label of an item-use result at (x, y). It clears the second line buffer,
   sets the colour (0xF, or 2 while the video mode gGraphicsChoice is 0), and then either draws the
   thrown-item icon gThrownIcon4 at (x - 1, y) through f1A29_000F when `result` is above -3, or
   fills a panel (x, y)-(x + 0x1E, y + 0xF) in colours gA95A/g8A6C/g8A7D. The switch on `result`
   fills `name` (6 bytes) and the optional `suffix` (6 bytes, empty for the default): -1 "MISS" (and
   y + 1), -2 "HACK", -3 "CAN'T"+"REACH", -4 "NO"+"AMMO", anything else the decimal `result`. It
   draws `name` centred (indent 0x10 - strlen*6/2) at (x + indent, y + 5), or `name` at (x + indent,
   y + 2) and `suffix` at (x + indent, y + 9) when the suffix is set. Called by DrawMemberSlot (07E7)
   only. `result` is what UseItem (0DE8:0763, src/E0DE8.C:499) stored in the record's mE8[slot]. */
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
        f1A29_000F(gStatusLine, gThrownIcon4, x - 1, y, 0);
    } else {
        DrawFilledPanel(x, y, x + 0x1e, y + 0xf, gA95A, g8A6C, g8A7D);
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
   slot corner is (g5D31[who & 1], g5D33[(who & 6) + slot]), the x shifted by 0xB0 while no status
   line is up. It hides the pointer, fills the panel in gFB4C/gF53A/gF57E, and outlines it in colour
   0xF when the mouse is DISABLED and the slot is the selected one (gMouseDisabled, gA99A, gF51A).
   When the record's mE8[slot] is set it draws the item-use label DrawActionLabel; else it draws the
   item icon through DrawItemIcon, or the empty-slot icon gEmptySlotIcons[slot] through f1A29_000F,
   and then, in that else branch only, draws the highlight icon gIcon8A7E when the slot's m1E bit is
   set, or f0D1C_07DB(who, slot) returns 0, or hp is 0 or below, or m01 bit 2 (paralyzed) is set.
   Called by RedrawMember (0991) and RemoveCarriedItems (0D1C:01CA, src/E0D1C.C:294). */
/*@ 1547:07E7 */ void DrawMemberSlot(char who, char slot)
{
    int item;
    int x;
    int y;

    x = g5D31[who & 1];
    y = g5D33[(who & 6) + slot];
    if (!gStatusLine)
        x += 0xb0;
    item = gChars[who].m77[slot];
    LockHidePointer();
    DrawFilledPanel(x, y, x + 0x1e, y + 0xf, gFB4C, gF53A, gF57E);
    if (gMouseDisabled && who == gA99A && slot == gF51A)
        DrawRect(x, y, x + 0x1e, y + 0xf, 0xf);
    if (gChars[who].mE8[slot] != 0) {
        DrawActionLabel(x, y, gChars[who].mE8[slot]);
    } else {
        if (item != 0)
            DrawItemIcon(gStatusLine, item, x + 8, y);
        else
            f1A29_000F(gStatusLine, gEmptySlotIcons[slot], x + 8, y, 0);
        if ((gChars[who].m1E & (1 << slot)) || f0D1C_07DB(who, slot) == 0
                || gChars[who].hp <= 0 || (gChars[who].m01 & 4))
            f1A29_000F(gStatusLine, gIcon8A7E, x, y, 0);
    }
    LockShowPointer();
}
/* 1547:0991: redraws one member's block on the character screen. It returns unless the member
   passes f0D1C_0670(who, 1). The block layout depends on the screen mode gFB44: mode 0 (the
   plain character screen) draws the name (or "Swapping" while this member is the one being moved,
   gIconTable), the portrait, the two carried-item slots, the HP line, the status bar and, while
   mE7 (the last damage amount) is above zero, the thrown-damage icon gThrownIcon3 and its number;
   mode 1 (the inventory up) draws the HP line, the food bar, one status line (DEAD, UNCONSCIOUS,
   POISON (SLOW), POISONED or PARALYZED) and the item rows; mode 2 (the character information
   panel) draws the stat panel with the class, race and alignment names, the six stat lines, armor
   class, EXP/LVL and the per-class slot lines. The name colour and text attribute come from m01
   bits 1 and 2 (8/6), otherwise 0xc/0xf; m01 bit 0 is the alive bit, bit 1 poisoned and bit 2
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

    if (!f0D1C_0670(who, 1))
        return;
    rec = &gChars[who];
    if (rec->m01 & 6) {
        textColour = 8;
        textAttr = 6;
    } else {
        textColour = 0xc;
        textAttr = 0xf;
    }
    switch (gFB44) {
    case 0:
        LockHidePointer();
        savedFont = gFont1A47;
        f1A47_0169(gA9C0);
        x = g5D4B[who & 1];
        y = g5D4F[who >> 1];
        ClipRect(0x16, 0xa8, x, y, 8, 0x18, 2, 2);
        ClipRect(0x1e, 0xa8, x, y + 0x18, 8, 0x1a, 2, 2);
        savedStatusLine = gStatusLine;
        gStatusLine = 2;
        x <<= 3;    /* the layout tables hold 8-pixel columns; the text routines take pixels */
        if (gIconTable == who)
            gDrawText("Swapping", x + 2, y + 2, 8, gF57E);
        else
            gDrawText(rec->name, x + 2, y + 2, textColour, gF57E);
        DrawMemberPortrait(who);
        DrawMemberSlot(who, 0);
        DrawMemberSlot(who, 1);
        DrawMemberHp(who);
        DrawMemberBar(who);
        if (rec->mE7 > 0) {
            itoa(rec->mE7, buf, 10);
            f1A29_000F(2, gThrownIcon3, x + 0xd, y + 0x1e, 0);
            gDrawText(buf, x + 0x22 - strlen(buf) * 3, y + 0x2a,
                    gGraphicsChoice == 0 ? 0xc : 0xf, 0);
        }
        gStatusLine = savedStatusLine;
        if (!gStatusLine)
            ClipRect(x >> 3, y, g5D4D[who & 1], y, 8, 0x32, 2, 0);
        f1A47_0169(savedFont);
        LockShowPointer();
        return;
    case 1:
    case 2:
        if (gA99A != who)
            return;
        ClipRect(0x16, 0, 0, 0, 0x12, 0xa8, 2, 2);
        gStatusLine = 2;
        DrawMemberPortrait(who);
        DrawShadowText(rec->name, 0xdb, 6, textAttr, gF57E);
        DrawMemberHp(who);
        DrawMemberFood(who);
        if (gFB44 == 1) {
            /* one status line is drawn at y 0x9e, a different x per label (0xf7 DEAD, 0xe2
               UNCONSCIOUS, 0xdc POISON (SLOW), 0xeb POISONED, 0xe8 PARALYZED); the hp + 10 and the
               unsigned hp + 9 range test are how the original compiles hp == -10 (DEAD) and
               -9 <= hp <= 0 (UNCONSCIOUS) */
            if (rec->hp + 10 == 0)
                DrawShadowText("DEAD", 0xf7, 0x9e, 6, gFB7B);
            else if ((unsigned)(rec->hp + 9) < 10)
                DrawShadowText("UNCONSCIOUS", 0xe2, 0x9e, 6, gFB7B);
            else if ((rec->m01 & 2) && (rec->mE3 & 0x80))
                DrawShadowText("POISON (SLOW)", 0xdc, 0x9e, 6, gFB7B);
            else if (rec->m01 & 2)
                DrawShadowText("POISONED", 0xeb, 0x9e, 6, gFB7B);
            else if (rec->m01 & 4)
                DrawShadowText("PARALYZED", 0xe8, 0x9e, 6, gFB7B);
            for (n = 0; n < 0x1b; n++)
                DrawItemRow(n, 0, 2);
            DrawItemRow(0x10, 1, 2);
            DrawItemRow(gF51A, 1, 2);
        } else {
            for (n = 0; n < 3; n++)
                gFillRect(g5D39[n], g5D3F[n], g5D42[n], g5D48[n], gFB7B);
            DrawShadowText("CHARACTER INFO", 0xb7, 0x2a, 0xf, gFB7B);
            gDrawText(g11DD[rec->class], 0xb7, 0x37, 0xc, gFB7B);
            gDrawText(g1240[rec->align], 0xb7, 0x3e, 0xc, gFB7B);
            gDrawText(g11AD[rec->race], 0xb7, 0x45, 0xc, gFB7B);
            for (n = 0; n < 6; n++)
                gDrawText(g1195[n], 0xb7, n * 7 + 0x52, 0xc, gFB7B);
            gDrawText("ARMOR CLASS", 0xb7, 0x7c, 0xc, gFB7B);
            gDrawText("EXP", 0xef, 0x8a, 0xc, gFB7B);
            gDrawText("LVL", 0x116, 0x8a, 0xc, gFB7B);
            gDrawText(FormatStrengthPercent(rec->m0D, rec->m0F), 0x113, 0x52, 0xf, gFB7B);
            PrintInt(rec->m11, 0x113, 0x59, 0xf, gFB7B);   /* INTELLIGENCE */
            PrintInt(rec->m13, 0x113, 0x60, 0xf, gFB7B);   /* WISDOM */
            PrintInt(rec->m15, 0x113, 0x67, 0xf, gFB7B);   /* DEXTERITY */
            PrintInt(rec->m17, 0x113, 0x6e, 0xf, gFB7B);   /* CONSTITUTION */
            PrintInt(rec->m19, 0x113, 0x75, 0xf, gFB7B);   /* CHARISMA */
            PrintInt(rec->m1D, 0x113, 0x7c, 0xf, gFB7B);   /* ARMOR CLASS */
            for (n = 0; n < 3; n++) {
                /* the three per-class-slot lines: the slot's name, its experience total (as a
                   long, right-aligned at x 0xf7) and its level m24 (right-aligned at 0x11e, the
                   value LevelUpSlot increments); GetClassSlotByte returns -1 for a class with no
                   such slot */
                classSlot = GetClassSlotByte(rec->class, n);
                if (classSlot == -1)
                    continue;
                gDrawText(g1219[classSlot], 0xb7, n * 7 + 0x91, 0xc, gFB7B);
                expValue = rec->exp[n];
                slotValue = rec->m24[n];
                f1BA4_000C(expValue, 0xf7 - NumberWidthLong(expValue), n * 7 + 0x91,
                        0xf, gFB7B);
                PrintInt(slotValue, 0x11e - NumberWidthInt(slotValue), n * 7 + 0x91,
                        0xf, gFB7B);
            }
        }
        gStatusLine = 0;
        LockHidePointer();
        ClipRect(0x16, 0, 0x16, 0, 0x12, 0xa8, 2, 0);
        LockShowPointer();
        ClipRect(0, 0, 0x16, 0, 0x12, 0xa8, 2, 2);
        break;
    }
}
