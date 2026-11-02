/* Unit at EOB.EXE 1547:0009 (4362 bytes, 8 functions): the character information screen. It draws
   one party member's block (the bar at 0009, the HP label and ratio bar at 033F, the two carried
   item slots at 07E7, the title and the stat lines at 0991), and the 0991 dispatcher redraws a
   single member on the six-member screen. It reads the 243-byte character records at ds:F580
   (struct CharRec, src/E0A72.C), the stat/class/race/alignment name tables at ds:1195, ds:11AD,
   ds:11DD and ds:1240, and the row layout byte tables at ds:5D1C..ds:5D4E. Placeholder names until
   the callers and the tables are understood. */

/*@flags -ml -O -Z */

struct CharRec {            /* the 243-byte (0xF3) records at ds:F580 (src/E0A72.C, src/E0D1C.C) */
    char pad00;
    unsigned char m01;      /* +01: status bits (src/E0D1C.C) */
    char name[0x0B];        /* +02: the NUL-padded name (src/EOV6.C) */
    signed char m0D;        /* +0D (src/E0A72.C) */
    signed char m0E;        /* +0E (src/E1414.C) */
    signed char m0F;        /* +0F (src/E0A72.C) */
    signed char m10;        /* +10 (src/E1414.C) */
    signed char str;        /* +11: STRENGTH, the first of the six stat values (name ds:1195) */
    signed char m12;        /* +12 */
    signed char intel;      /* +13: INTELLIGENCE */
    signed char m14;        /* +14 */
    signed char wis;        /* +15: WISDOM */
    signed char m16;        /* +16 */
    signed char dex;        /* +17: DEXTERITY */
    signed char m18;        /* +18 */
    signed char con;        /* +19: CONSTITUTION */
    signed char m1A;        /* +1A */
    signed char hp;         /* +1B: current hit points (src/E1414.C) */
    signed char hpMax;      /* +1C: maximum hit points, the second %3d of the HP line */
    signed char cha;        /* +1D: CHARISMA, the sixth stat */
    signed char m1E;        /* +1E: a bit set/cleared by the per-slot helper (src/E1414.C) */
    signed char race;       /* +1F: index into the race-name table ds:11AD */
    signed char class;      /* +20: index into the class-name table ds:11DD (src/E0A72.C) */
    signed char align;      /* +21: index into the alignment-name table ds:1240 */
    signed char m22;        /* +22 */
    unsigned char m23;      /* +23: the level-like byte (src/E1414.C) */
    signed char m24[3];     /* +24: three per-class-slot bytes (src/E0D1C.C) */
    long exp[3];            /* +27: three 32-bit values drawn after EXP */
    char far *portrait;     /* +33: the far pointer 0544 draws for the member */
    char pad37[0x77 - 0x37];
    int m77[27];            /* +77: 27 word slots, the carried items (src/E0D1C.C) */
    char padAD[0xDF - 0xAD];
    signed char mDF;        /* +0DF: a status byte read by 0009 */
    signed char mE0;        /* +0E0: a status byte read by 0009 */
    char padE1[0xE3 - 0xE1];
    int mE3;                /* +0E3: flags tested by 0009, 0544 and 0991 (src/E1414.C) */
    char padE5[0xE7 - 0xE5];
    unsigned char mE7;      /* +0E7: a counter (src/E1414.C) */
    signed char mE8[0xF3 - 0xE8]; /* +0E8: per-slot bytes: an action label (src/E1414.C) */
};

extern int g8102; /*@ ds:8102 */         /* the video mode the game initialised (src/E082C.C) */
extern int g8115; /*@ ds:8115 */         /* nonzero while a status line is up (src/E082C.C) */
extern unsigned char gFBD2; /*@ ds:FBD2 */ /* bit 0 gates the icon colour row (src/E0D1C.C) */
extern signed char gFB4C; /*@ ds:FB4C */ /* a colour (src/E0D1C.C) */
extern signed char gF53A; /*@ ds:F53A */ /* a colour (src/E0D1C.C) */
extern struct CharRec gChars[]; /*@ ds:F580 */ /* the six character records */
extern unsigned char g5D1F[]; /*@ ds:5D1F */ /* member bar x by (who & 1): 8, 80 */
extern signed char g5D21[]; /*@ ds:5D21 */   /* member bar y by (who >> 1): 2, 54, 106 */
extern unsigned char g5D26[]; /*@ ds:5D26 */ /* HP line y by (who >> 1): 46, 98, 150 */
extern signed char g5D24[]; /*@ ds:5D24 */   /* HP line x by (who & 1): 23, 95 */
extern signed char g5D1C[]; /*@ ds:5D1C */   /* HP ratio bar colour by state (0, 1, 2) */
extern signed char g5D29[]; /*@ ds:5D29 */   /* level ratio bar colour by state (0, 1, 2) */
extern char gFB44; /*@ ds:FB44 */            /* a mode byte: 0 and 1 gate the redraws (src/E0D1C.C) */
extern signed char gA99A; /*@ ds:A99A */     /* the selected member (src/E0ACD.C) */
extern signed char gF57E; /*@ ds:F57E */     /* a colour (src/EOV2.C) */
extern char gFB7A; /*@ ds:FB7A */            /* a colour (src/EOV2.C) */
extern char gF43C; /*@ ds:F43C */            /* nonzero after the startup allocations (src/E07A9.C) */
extern char gA95A; /*@ ds:A95A */            /* a colour (src/EOV2.C) */
extern char g8A6C; /*@ ds:8A6C */            /* a colour (src/EOV2.C) */
extern char g8A7D; /*@ ds:8A7D */            /* a colour (src/EOV2.C) */
extern void far *gThrownIcon4; /*@ ds:EFBE */ /* the fourth thrown-item icon (src/EOV2.C) */
extern signed char g5D31[]; /*@ ds:5D31 */ /* item slot x by (who & 1): 40, 112 */
extern unsigned char g5D33[]; /*@ ds:5D33 */ /* item slot y by ((who & 6) + slot) */
extern int gMouseDisabled; /*@ ds:814A */    /* nonzero disables the mouse (src/E07A9.C) */
extern char gF51A; /*@ ds:F51A */            /* the selected row (src/E0ACD.C) */
extern void far *gIcon8A7E; /*@ ds:8A7E */   /* the empty-slot highlight icon (src/EOV2.C) */
extern void far *gIconsDDE4[]; /*@ ds:DDE4 */ /* the item icon far pointers (src/E0D1C.C) */
extern void far *gEmptySlotIcons[]; /*@ ds:DF38 */ /* the two no-item slot icons, gIconsDDE4[0x55]
                                                  and [0x56] (0xDDE4 + 0x55 * 4) */

extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int colour); /*@ ds:8062 */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void (far *gDrawText)(char far *text, int a, int b, int c, int d); /*@ ds:80CA */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern unsigned strlen(char far *s); /*@ 0000:67F6 */
extern char *strcpy(char far *dest, char far *src); /*@ 0000:67CD */
extern void DrawRect(int x1, int y1, int x2, int y2, int colour); /*@ 082C:04C4 */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int colourA, int colourB, int fill); /*@ 082C:09D5 */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void f0D1C_070F(int mode, int n, int x, int y); /*@ 0D1C:070F */
extern int f0D1C_07DB(int who, int flag); /*@ 0D1C:07DB */
/* 1547:0009: draws the bar at the member's block corner (g5D1F[who & 1], g5D21[who >> 1], the x
   shifted by 0xB0 while no status line is up). Two conditions chosen from the record decide the
   shape: when neither holds the bar is only a frame of four lines (colours gFB4C, gF53A, 0xC);
   when exactly one holds it is a DrawRect in `colour` (6 or 3 by video mode) for the "in play"
   one, or colour 5 otherwise; when both hold the interior is hatched, each row drawn in `colour`
   for the first condition and 5 for the second. Purpose unknown: what mE3's bits, mDF and mE0
   mean is not established. Called by 0991 only. */
/*@ 1547:0009 */ void DrawMemberBar(int who)
{
    int colour;
    struct CharRec far *rec;
    int frame;
    int lit;
    int i;
    int base;
    int x;
    int y;

    colour = g8102 == 0 ? 3 : 6;
    x = g5D1F[who & 1];
    y = g5D21[who >> 1];
    if (!g8115)
        x += 0xb0;
    rec = &gChars[who];
    frame = (unsigned)(rec->mE3 & 0x302) || rec->mDF || rec->mE0;
    lit = (unsigned)(rec->mE3 & 0x4c8) || (gFBD2 & 0x30);
    if (!frame && !lit) {
        gDrawLine(x, y, x + 0x3e, y, gFB4C);
        gDrawLine(x, y + 0x31, x + 0x3e, y + 0x31, gF53A);
        gDrawLine(x, y, x, y + 0x33, 0xc);
        gDrawLine(x + 0x3f, y, x + 0x3f, y + 0x33, 0xc);
    } else {
        if (frame && !lit) {
            DrawRect(x, y, x + 0x3f, y + 0x31, colour);
        } else if (!frame && lit) {
            DrawRect(x, y, x + 0x3f, y + 0x31, 5);
        } else {
            base = x;
            for (i = 0; i < 0x40; i += 0x10) {
                x = base + i;
                if (frame) {
                    gDrawLine(x, y, x + 7, y, colour);
                    gDrawLine(x + 8, y + 0x31, x + 0xf, y + 0x31, colour);
                }
                if (lit) {
                    gDrawLine(x + 8, y, x + 0xf, y, 5);
                    gDrawLine(x, y + 0x31, x + 7, y + 0x31, 5);
                }
            }
            x = base;
            base = y;
            for (i = 1; i < 0x30; i += 0xc) {
                y = base + i - 1;
                if (lit) {
                    gDrawLine(x, y + 1, x, y + 6, 5);
                    gDrawLine(x + 0x3f, y + 7, x + 0x3f, y + 0xc, 5);
                }
                if (frame) {
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
   immediately when max < 1. The 033F caller passes the member's HP as value and hp max as max. */
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
   while gFB44 is nonzero. When gF43C is set it draws the "HP" label (only while gFB44 is 0) and a
   ratio bar (DrawRatioBar) of the member's hp (m1B) to its hp max (m1C), both shifted by 10 and the
   bar colour picked by a wound state (1 when hp max/3 exceeds hp, 2 when hp is 0 or less); when
   gF43C is 0 it prints "%3d of %-3d" (hp, hp max) instead. Called by 0991 only. */
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
        if (!g8115)
            x += 0xb0;
    } else {
        x = 0xfa;
        y = 0x10;
        width = 0x33;
        height = 5;
    }
    if (gF43C) {
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
/* 1547:04A0: draws one member's level ratio bar (the small bar under the HP line while the
   inventory is up). It returns unless gFB44 is nonzero, the member passes f0D1C_0670(member, 1),
   m01 bit 0 is set and (while gFB44 is nonzero) the member is the selected gA99A. The bar is at
   (0xfa, 0x19): value m23 against 100, 51 wide, 5 tall, the colour picked by a level state (1
   below 0x21, 2 below 0x14, 0 above). Called by 1414:0C82 (DecayMemberState, src/E1414.C) only.
   DRAFT: matches every byte except the second early return's branch form: the original ends it with
   a short `je 009e` (1547:0023), our `-ml -O -Z` build expands it to `jne @@; jmp end; nop`
   (3 bytes longer). Tried two ifs, `||`, `== 0`, `!`, `< 1`, `else`, nested ifs and a `goto`; all
   expand. Same TCC branch-layout class as 0ACD:091C and 0D1C:0903 (playbook 30). */
/*@ 1547:04A0 draft */ void DrawMemberLevel(int member)
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
/*@ 1547:0544 stub */ void f1547_0544(void) {}
/* 1547:0642: draws the label of an item action at (x, y). It clears the second line buffer, sets
   the colour (0xF, or 2 while the video mode g8102 is 0), and then either draws the thrown-item
   icon gThrownIcon4 at (x - 1, y) through f1A29_000F when `action` is above -3, or fills a panel
   (x, y)-(x + 0x1E, y + 0xF) in colours gA95A/g8A6C/g8A7D. The switch on `action` fills `name`
   (6 bytes) and the optional `suffix` (6 bytes, empty for the default): -1 "MISS" (and y + 1), -2
   "HACK", -3 "CAN'T"+"REACH", -4 "NO"+"AMMO", anything else the decimal `action`. Then it draws
   `name` centred (indent 0x10 - strlen*6/2) at (x, y + 5), or `name` at (x, y + 2) and `suffix` at
   (x, y + 9) when the suffix is set. Called by 07E7 only. */
/*@ 1547:0642 */ void DrawActionLabel(int x, int y, int action)
{
    char name[6];
    char suffix[6];
    char indent;
    char colour;

    suffix[0] = 0;
    colour = 0xf;
    if (g8102 == 0)
        colour = 2;
    if (action > -3) {
        f1A29_000F(g8115, gThrownIcon4, x - 1, y, 0);
    } else {
        DrawFilledPanel(x, y, x + 0x1e, y + 0xf, gA95A, g8A6C, g8A7D);
    }
    switch (action) {
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
        sprintf(name, "%d", action);
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
   line is up. It hides the pointer, fills the panel in gFB4C/gF53A/gF57E, outlines it in colour
   0xF when the mouse is enabled and the slot is the selected one (gA99A/gF51A), and then draws the
   slot: the action label DrawActionLabel when the record's mE8[slot] is set, else the item icon
   through f0D1C_070F, else the empty-slot icon gIconsDDE4[0x55 + slot] through f1A29_000F. Last it
   draws the highlight icon gIcon8A7E unless the slot's m1E bit is set, f0D1C_07DB accepted it, hp
   is above 0 and m01 bit 2 is clear. Called by 0991 only. */
/*@ 1547:07E7 */ void DrawMemberSlot(char who, char slot)
{
    int item;
    int x;
    int y;

    x = g5D31[who & 1];
    y = g5D33[(who & 6) + slot];
    if (!g8115)
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
            f0D1C_070F(g8115, item, x + 8, y);
        else
            f1A29_000F(g8115, gEmptySlotIcons[slot], x + 8, y, 0);
        if ((gChars[who].m1E & (1 << slot)) || f0D1C_07DB(who, slot) == 0
                || gChars[who].hp <= 0 || (gChars[who].m01 & 4))
            f1A29_000F(g8115, gIcon8A7E, x, y, 0);
    }
    LockShowPointer();
}
/*@ 1547:0991 stub */ void f1547_0991(void) {}
