/* Unit at overlay 0 of EOB.EXE (10873 bytes, 35 functions): the character panel and its drawing
   helpers. It builds and frees the 17-cell icon table (ovl0:0000, 00AF), draws panel icons, rows and
   characters (ovl0:0128, 037A, 0504, 05BA, 065B, 11BA, 1224), builds the panel row lists (ovl0:03B2,
   042F), shows the two report lines (ovl0:1071, 10F6), draws the party-status report (ovl0:0504),
   and runs the character panel loop RunCharacterPanel (ovl0:28F9). Character creation lives here
   too: PickRace (ovl0:1F66) picks the race, PickClass (ovl0:207A) the class, PickAlignment
   (ovl0:2238) the alignment, RollCharacterStats (ovl0:1BF2) rolls the abilities and hit points,
   EditStatField (ovl0:135D) edits one, PickPortrait (ovl0:0A5E) places the member, and
   CreateCharacter (ovl0:23EC) drives the sequence. Placeholder names where no evidence. */

/*@flags -ml -O -Z -v */

#include <dos.h>

extern char gCgaMapDefault[]; /*@ ds:113F */ /* the overlay screen buffer the cell slicer reads */
extern void far * (far *gGetSpriteCellRle)(int x, int y, int w, int h, void far *buf); /*@ ds:8082 */
extern void farfree(void far *block); /*@ 0000:71EE */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern int DispatchKeyWait(void far *list); /*@ 16A5:005A */
extern void WaitFrames(int n); /*@ 1A16:000B */
extern unsigned char far gScriptFlag; /*@ 0000:03A0 */ /* the sound driver's script flag (src/E_TEXT.ASM: script command C7h sets it, 0000:4A23) */

struct IconDef9 {               /* the 17 9-byte icon definitions at 2084 (17 * 9 = 153 bytes) */
    unsigned char x;            /* +0: the cell's left edge */
    unsigned char y;            /* +1: the cell's top edge */
    unsigned char w;            /* +2: the cell's width, also the "defined" flag */
    unsigned char h;            /* +3: the cell's height */
    unsigned char m04;
    unsigned char m05;
    unsigned char m06;
    unsigned char m07;
    unsigned char m08;
};
extern struct IconDef9 far gPanelIconDefs[]; /*@ 2084:0000 */

struct IconDef4 {               /* 4-byte rectangle records at 2080 (the icon cell table) */
    unsigned char left;         /* +0 */
    unsigned char top;          /* +1 */
    unsigned char width;        /* +2 */
    unsigned char height;       /* +3 */
};
extern struct IconDef4 far gPanelIconRects[]; /*@ 2080:0000 */

extern void CopyRegion(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void DrawShape(int page, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void); /*@ 1BBE:02C2 */
extern void movmem(void far *src, void far *dst, unsigned n); /*@ 0000:63A3 */
extern void DrawShadowText(char far *text, int x, int y, int color, int extra); /*@ 082C:0898 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern int gDrawPage; /*@ ds:8115 */ /* the graphics driver's draw page (src/E082C.C) */
extern char gStrPartyComplete[]; /*@ ds:613D */ /* the string the party-status report draws (Purpose unknown) */
extern void DrawRowList(int index); /*@ 0D1C:0454 */

struct CharRec {                /* 243-byte character records at ds:F580 (src/E0A72.C) */
    char index;                   /* +00: the character index ovl0:26C6 writes */
    unsigned char status;         /* +01: status bits */
    char name[0x0b];              /* +02: the character's name, NUL padded */
    char strength;                /* +0D: the source of 284A's byte copy */
    char baseStrength;
    char strengthPct;
    char baseStrengthPct;
    char intelligence;
    char m12;
    char wisdom;
    char m14;
    char dexterity;
    char m16;
    char constitution;
    char m18;
    char charisma;
    char m1A;
    char hp;
    char maxHp;                   /* +1C: the destination of 284A's byte copy */
    char armorClass;
    char m1E;
    signed char race;             /* +1F: the race index, half of it indexes gRaceMinAbilities (src/E0A72.C) */
    signed char classIndex;       /* +20: the class index, indexes gClassBaseBits (src/E0BD2.C) */
    signed char alignment;        /* +21: index into the alignment-name table ds:1240 (src/E1547.C) */
    signed char portraitIndex;              /* +22: the position ovl0:0A5E stores (Purpose unknown) */
    signed char food;             /* +23: the food value, set to 100 (full) by ovl0:26C6 */
    signed char levels[3];        /* +24: the class-slot levels ovl0:1BF2 fills from gClassStartLevels */
    long experience[3];           /* +27: the experience of each class slot, which ovl0:1BF2 sets */
    void far *portrait;           /* +33: the character's packed sprite buffer (src/EOV3.C) */
    char pad37[0x73 - 0x37];
    long spellBook;               /* +73: the mage's spell book (src/EOV1.C); 280D sets 0x26c */
    int items[27];                /* +77: 27 carried-item slots (src/E0DE8.C) */
    char padAD[0xf3 - 0xAD];
};
extern struct CharRec gChars[]; /*@ ds:F580 */

struct ItemSlot {               /* 4-byte records at ds:60D5 (ovl0:26C6) */
    int item;                   /* +0: an item type, or -1 for "no item" */
    int slot;                   /* +2: the carrying slot it prefers */
};
extern struct ItemSlot gStartItems[]; /*@ ds:60D5 */ /* the per-class item/slot table ovl0:26C6 walks (Purpose unknown) */
extern char gClassStartItems[]; /*@ ds:6101 */ /* the per-class four-entry starting item table
                                                   ovl0:26C6 walks. QUIRK ds:6101: an entry 255
                                                   (signed -1) indexes ds:60D5 at -1, the last word of
                                                   ds:6099; harmless because slot 0 is filled first */
extern int GetArmourClassBase(char m15); /*@ 0A72:019D */
extern int RollHitPoints(char classIndex, char m17, char countA, char countB, char countC); /*@ 0A72:01AE */
extern int MaxHitPoints(char classIndex, char m17, char countA, char countB, char countC); /*@ 0A72:02B3 */
extern int MinHitPoints(char classIndex, char m17, char countA, char countB, char countC); /*@ 0A72:03A4 */
extern int gHpMaxOrAttackTarget; /*@ ds:DCA4 */ /* the max hit points ovl0:135D recomputes through MaxHitPoints (0A72:02B3); src/E0DE8.C:132 gives the same byte a char meaning (a conflict left to E0DE8.C) */
extern int gHpMinOrHitFlag; /*@ ds:DD10 */ /* the min hit points ovl0:135D recomputes through MinHitPoints (0A72:03A4); src/E08E6.C also uses the byte as a 0/1 damage flag (08E6:0E48 clears it, 08E6:0FBE sets it to 1) */
extern struct Node far *FindNode(struct Node far *head, unsigned key); /*@ 1658:01A2 */
extern char gFmtStrengthOnEntry[]; /*@ ds:6227 */ /* a value format ovl0:135D uses (Purpose unknown) */
extern char gFmtValueOnEntry[]; /*@ ds:622A */ /* a value format ovl0:135D uses (Purpose unknown) */
extern char gFmtStrengthChanged[]; /*@ ds:622D */ /* a value format ovl0:135D uses (Purpose unknown) */
extern char gFmtValueChanged[]; /*@ ds:6230 */ /* a value format ovl0:135D uses (Purpose unknown) */
extern char gFmtHpChanged[]; /*@ ds:6233 */ /* a value format ovl0:135D uses (Purpose unknown) */
extern char gFmtArmorClassChanged[]; /*@ ds:6236 */ /* a value format ovl0:135D uses (Purpose unknown) */
extern char gFmtStrengthOnExit[]; /*@ ds:6239 */ /* a value format ovl0:135D uses (Purpose unknown) */
extern char gFmtValueOnExit[]; /*@ ds:623C */ /* a value format ovl0:135D uses (Purpose unknown) */
extern int CopyItemToFreeSlot(int n); /*@ 0BD2:089E */
extern void ComputeArmourClass(int who); /*@ 0DE8:0D5E */
extern signed char gClassBaseBits[]; /*@ ds:1231 */ /* byte table indexed by gChars[].classIndex (src/E0BD2.C) */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void far *gPortraitCells[]; /*@ ds:FC8C */ /* the overlay screen buffers, ds:FC8C up to ds:FD98 (0x43 far pointers); ovl0:28B0 frees all of them, InitPanelScreens sets only the first 0x2c (ovl0:2547) and entries 0x32..0x3b alias the ten gBoxAnimCells row cells */
extern int gChargenMember; /*@ ds:DCB0 */ /* the current character index for the panel: RunCharacterPanel toggles it (ovl0:28F9) and the panel routines index gChars[gChargenMember]. Purpose unknown beyond "current character" */
extern int gPanelX5E54[]; /*@ ds:5E54 */ /* per-character x read by ovl0:11BA (Purpose unknown) */
extern int gPanelY5E5C[]; /*@ ds:5E5C */ /* per-character y read by ovl0:11BA (Purpose unknown) */
extern char gStrSelectBoxPrompt[]; /*@ ds:61CB */ /* a fixed report label (Purpose unknown) */
extern char gStrEnteringGame[]; /*@ ds:6207 */ /* a fixed report label (Purpose unknown) */
extern int gClassMinAbilities[][6]; /*@ ds:5FC5 */ /* per-class rows read by ovl0:1EA9 (Purpose unknown) */
extern int gRaceMinAbilities[][6]; /*@ ds:5F35 */ /* per-class rows read by ovl0:1EA9 (Purpose unknown) */
extern int gRaceMaxAbilities[][6]; /*@ ds:5F7D */ /* per-class rows read by ovl0:1EA9 (Purpose unknown) */
extern int gStatMin[6]; /*@ ds:DD04 */ /* six words ovl0:1EA9 writes (Purpose unknown) */
extern int gStatMax[6]; /*@ ds:DC98 */ /* six words ovl0:1EA9 writes (Purpose unknown) */
extern signed char gClassCombatGroup[]; /*@ ds:116E */ /* per-class byte (src/E0A72.C) */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void *LoadOverlayScreen(char far *name, int srcKind, int dstKind); /*@ 082C:05EC */
extern void *LoadScreenImage(char far *name, char far *extra, int srcKind, int dstKind, int mode); /*@ 082C:051B */
extern void CopyPage(int a, int b, int c, int d); /*@ 19EC:002A */
extern int gGraphicsChoice; /*@ ds:8102 */ /* the video mode the game initialised (src/E082C.C) */
extern void far *gBoxAnimCells[]; /*@ ds:FD54 */ /* the ten overlay buffers ovl0:2547 slices (Purpose unknown) */
extern char gFileNameChargenA[]; /*@ ds:6272 */ /* a screen name (Purpose unknown) */
extern char gFileNameChargen[]; /*@ ds:627B */ /* a screen name (Purpose unknown) */
extern char gFileNameChargenB[]; /*@ ds:6283 */ /* a screen name (Purpose unknown) */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void DrawRect(int x1, int y1, int x2, int y2, int colour); /*@ 082C:04C4 */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern void DrawOptionList(int panel, char far * far *names, long mask, int shift, int spacing); /*@ 14EF:0154 */
extern int UpdateOptionList(int panel, char far * far *names, long counter, long mask, int shift); /*@ 14EF:0274 */
extern char far *gRaceNames[]; /*@ ds:11AD */ /* the race names (src/E1547.C) */
extern char far *gClassNames[]; /*@ ds:11DD */ /* the class names (src/E1547.C) */
extern char gFileNameBuf[]; /*@ ds:FE6E */ /* the buffer sprintf formats into (src/E082C.C) */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern int strlen(char far *s); /*@ 0000:67F6 */
extern char *FormatStrengthPercent(char score, char percent); /*@ 0A72:0468 */
extern char gStrAcHpLvlLabels[]; /*@ ds:6197 */ /* a fixed panel label (Purpose unknown) */
extern char gFmtAbilityScores[]; /*@ ds:61A1 */ /* a format string ovl0:065B uses (Purpose unknown) */
extern char gFmtAcHp[]; /*@ ds:61B3 */ /* a format string ovl0:065B uses (Purpose unknown) */
extern char gFmtOneLevel[]; /*@ ds:61B9 */ /* a format string ovl0:065B selects (Purpose unknown) */
extern char gFmtTwoLevels[]; /*@ ds:61BC */ /* a format string ovl0:065B selects (Purpose unknown) */
extern char gFmtThreeLevels[]; /*@ ds:61C2 */ /* a format string ovl0:065B selects (Purpose unknown) */
extern char far *gAbilityAbbrevs[]; /*@ ds:115D */ /* six far string pointers ovl0:065B draws in a column (Purpose unknown) */
extern int gNewMemberFemale; /*@ ds:FC8A */ /* the parity of the race index PickRace (ovl0:1F66) returned (Purpose unknown) */
extern unsigned long gTicks; /*@ ds:848A */ /* the 32-bit tick counter (src/E082C.C) */
extern signed char gBoxAnimMember; /*@ ds:5EB0 */ /* the character ovl0:1224 last drew (Purpose unknown) */
extern signed char gBoxAnimFrame; /*@ ds:FC84 */ /* the character panel's animation frame 0..9 (Purpose unknown) */
extern signed char gBoxAnimDrawnFrame; /*@ ds:FC85 */ /* the frame ovl0:1224 last drew (Purpose unknown) */
extern unsigned long gBoxAnimNextTick; /*@ ds:FC86 */ /* the tick ovl0:1224 next advances the frame at (Purpose unknown) */
extern unsigned char gMemberBoxX[]; /*@ ds:6079 */ /* per-character x read by ovl0:1F66 (Purpose unknown) */
extern unsigned char gMemberBoxY[]; /*@ ds:607D */ /* per-character y read by ovl0:1F66 (Purpose unknown) */
extern char gStrSelectRace[]; /*@ ds:623F */ /* a fixed panel label (Purpose unknown) */
extern void (far *gDrawText)(char far *text, int x, int y, int colour, int extra); /*@ ds:80CA */
extern char far *gCurFont; /*@ ds:FE4E */ /* the current font (src/E1547.C) */
extern void far *gFont6x6; /*@ ds:A9C0 */ /* the 6x6 font loaded from FONT6.FNT (src/E07A9.C) */
extern void SetFont(char far *font); /*@ 1A47:0169 */
extern char gFontWidth; /*@ ds:8100 */ /* the current font's character width (src/E14EF.C) */
extern int gNamePosX[]; /*@ ds:5E44 */ /* the x of each panel name slot (Purpose unknown) */
extern int gNamePosY[]; /*@ ds:5E4C */ /* the y of each panel name slot (Purpose unknown) */
extern char gStrBlankName[]; /*@ ds:618C */ /* a fixed panel label (Purpose unknown) */
extern struct Node far *ListAdd(struct Node far *head, struct Node far *item); /*@ 1658:001E */
extern char gNodeListChanged; /*@ ds:5E20 */ /* nonzero forces the row-list walk to rebuild and redraw the whole list (16A5:00BD); BuildRowNodeList sets it (ovl0:042F), not the mouse */

struct Panel {                  /* the 16-byte option-list records at ds:017F (src/E14EF.C) */
    int x;                      /* +0 */
    int y;                      /* +2 */
    int width;                  /* +4 */
    int height;                 /* +6: the number of rows the list draws */
    int sel;                    /* +8: the selected entry */
    int colorA;                 /* +a */
    int colorB;                 /* +c */
    int pad;                    /* +e */
};
extern struct Panel gPanels[]; /*@ ds:017F */ /* the shared option-list panels (src/E14EF.C) */
extern char far *gAlignmentNames[]; /*@ ds:1240 */ /* the alignment names (src/E1547.C) */
extern long gRaceClassMasks[]; /*@ ds:6081 */ /* the class-availability mask per race pair ovl0:207A picks */
extern long gClassAlignmentMasks[]; /*@ ds:6099 */ /* the alignment-availability mask per class ovl0:2238 picks */
extern int gRaceAbilityBonus[][6]; /*@ ds:5EB1 */ /* the six ability bonuses per race pair ovl0:1BF2 adds */
extern unsigned char gClassStartLevels[]; /*@ ds:5EF9 */ /* the four bytes per class ovl0:1BF2 copies to levels */
extern unsigned char gClassStartLevelsNumClasses[]; /*@ ds:5EFC */ /* the divisor per class ovl0:1BF2 divides 5000 by */
extern char gStrSelectClass[]; /*@ ds:624C */ /* the caption ovl0:207A draws (Purpose unknown) */
extern char gStrSelectAlignment[]; /*@ ds:625A */ /* the caption ovl0:2238 draws (Purpose unknown) */
extern char gStrNamePrompt[]; /*@ ds:626C */ /* the caption ovl0:23EC draws (Purpose unknown) */
extern int gUnwrittenPanelRow; /*@ ds:DD12 */ /* the panel row ovl0:23EC redraws. QUIRK ds:DD12:
                                                   nothing writes it (its address occurs once in the
                                                   code, at that read), so it is always 0 */
extern int gUnhandledOptionKey; /*@ ds:FC7A */ /* the key UpdateOptionList did not handle (src/E14EF.C) */
extern int gClickX; /*@ ds:8140 */ /* x of the last click (src/E082C.C) */
extern int gClickY; /*@ ds:8142 */ /* y of the last click (src/E082C.C) */
extern int gMouseDisabled; /*@ ds:814A */ /* nonzero disables the mouse (src/E07A9.C) */
extern char gMemberPortraits[]; /*@ ds:5EAC */ /* the four slot indices the party members hold */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern int CountMaskBits(int n, long mask, char shift); /*@ 14EF:0107 */
extern int IsPointInRect(int x, int y, int left, int top, int right, int bottom); /*@ 14EF:00E1 */
extern int IsMouseInRect(int left, int top, int right, int bottom); /*@ 1A72:02A6 */
extern int ReadKey(void); /*@ 1AAD:038E */
extern int WaitKeyEvent(void); /*@ 1AAD:05D3 */
extern int EditTextField(char far *text, char x, char y, char maxlen, char color, char attr, char cursorAttr); /*@ 082C:000B */

struct Node {                   /* the 60-byte panel row records (src/E0CF7.C) */
    struct Node far *next;      /* +00: next node, or null at the tail */
    unsigned key;               /* +04: the sort key ListAdd compares */
    unsigned key06;             /* +06: the first key word the dispatcher matches */
    unsigned key08;             /* +08: the second key word the dispatcher matches */
    unsigned char kind;         /* +0A: the drawing kind */
    unsigned char modeB;        /* +0B: the "on" drawing mode */
    unsigned char modeC;        /* +0C: the second drawing mode */
    unsigned char pad0D;        /* +0D: unknown (src/E0CF7.C leaves it unnamed) */
    unsigned flags;             /* +0E: the inactive bit and the layout bits */
    void far *src10;            /* +10: the default art pointer */
    void far *src14;            /* +14: the "on" art pointer */
    void far *src18;            /* +18: the second art pointer */
    int region;                 /* +1C: index into the region table ds:009F */
    int x;                      /* +1E: box left */
    int y;                      /* +20: box top */
    unsigned width;             /* +22: box width */
    unsigned height;            /* +24: box height */
    char pad26[0x3c - 0x26];
};
extern struct Node far *gNodePtrs[]; /*@ ds:FB7C */ /* the panel row nodes this unit lists and draws */
extern struct Node gNodeTemplate; /*@ ds:3A62 */ /* the template InitRowNode copies into each node */

struct RowDef8 {                /* 8-byte row definitions at 206B (41 of them) */
    unsigned char m00;
    unsigned char m01;
    unsigned char m02;
    unsigned char m03;
    unsigned char m04;
    char pad05[3];
};
extern struct RowDef8 far gRowNodeDefs[]; /*@ 206B:0000 */

extern void far *gPanelIconCells[0x11]; /*@ ds:5E64 */ /* the 17 sliced icon pointers this unit builds and draws */

/* ovl0:0000: builds the 17-entry far-pointer table gPanelIconCells. For each of the 17 icon
   definitions in
   gPanelIconDefs whose +2 byte (the width) is nonzero, it slices a sprite cell out of the overlay buffer
   gCgaMapDefault through ds:8082 (the definition's four leading bytes are the cell rectangle) and
   stores the pointer at gPanelIconCells[i]; every slot is cleared first. Called by ovl0:2547. */
/*@ ovl0:0000 */ void BuildIconTable(void)
{
    int i;

    for (i = 0; i < 0x11; i++) {
        gPanelIconCells[i] = 0;
        if (gPanelIconDefs[i].w)
            gPanelIconCells[i] = gGetSpriteCellRle(gPanelIconDefs[i].x, gPanelIconDefs[i].y, gPanelIconDefs[i].w, gPanelIconDefs[i].h,
                gCgaMapDefault);
    }
}
/* ovl0:00AF: frees the 17 far pointers in gPanelIconCells through farfree and clears every slot.
   Called by
   ovl0:28B0. */
/*@ ovl0:00AF */ void FreeIconTable(void)
{
    int i;

    for (i = 0; i < 0x11; i++) {
        if (gPanelIconCells[i]) {
            farfree(gPanelIconCells[i]);
            gPanelIconCells[i] = 0;
        }
    }
}
/* ovl0:00F7: waits for a key through 16A5:005A with the caller's list and returns its code. When the
   resident flag gScriptFlag (0000:03A0) is 1, it clears the flag and plays sound 0x14 first. Called by
   ovl0:0A5E, 0DDC, 0EE5, 135D and 28F9. */
/*@ ovl0:00F7 */ int WaitKeyOrPause(void far *list)
{
    if (gScriptFlag == 1) {
        gScriptFlag = 0;
        PlaySound(0x14);
    }
    return DispatchKeyWait(list);
}
/* ovl0:0128: draws one character-panel icon. It maps slot `index` (0..0x10) to the cell row
   gPanelIconDefs[index].m06 + `row`, clips the panel rectangle gPanelIconRects[row], draws the icon cell
   gPanelIconCells[index] inside it through DrawShape (mode 2), and unless `mode` is 2 hides and shows the
   mouse pointer around the icon's live area. The rectangle's top-left is (0x14, 0) in mode 0,
   else the icon's own origin gPanelIconDefs[index].m07 + 2 / m08 - 0x40. Called by ovl0:037A, 0504,
   0A5E, 11BA, 207A and 2238. */
/*@ ovl0:0128 */ void DrawPanelIcon(unsigned index, int rowBase, int mode)
{
    register int cellRow;
    int x;
    int y;

    if (index >= 0x11)
        return;
    cellRow = gPanelIconDefs[index].m06 + rowBase;
    if (mode == 0) {
        x = 0x14;
        y = 0;
    } else {
        x = gPanelIconDefs[index].m07 + 2;
        y = gPanelIconDefs[index].m08 - 0x40;
    }
    CopyRegion(gPanelIconRects[cellRow].left, gPanelIconRects[cellRow].top, x, y, gPanelIconRects[cellRow].width,
        gPanelIconRects[cellRow].height, 2, 2);
    if (gPanelIconDefs[index].w)
        DrawShape(2, gPanelIconCells[index], (x << 3) + gPanelIconDefs[index].m04, y + gPanelIconDefs[index].m05, 0);
    if (mode == 2)
        return;
    HidePointerZone(gPanelIconDefs[index].m07 << 3, gPanelIconDefs[index].m08,
        (gPanelIconDefs[index].m07 + gPanelIconRects[cellRow].width) << 3,
        gPanelIconDefs[index].m08 + gPanelIconRects[cellRow].height);
    CopyRegion(0x14, 0, gPanelIconDefs[index].m07, gPanelIconDefs[index].m08, gPanelIconRects[cellRow].width,
        gPanelIconRects[cellRow].height, 2, 0);
    ShowPointerZone();
}
/* ovl0:037A: flashes the panel icon `index`: draws it at row + 1 (rowBase 1, the selected cell row,
   mode 0), plays sound 0x4c, waits 5 frames and draws it again at its own row (rowBase 0, mode 0).
   Called by ovl0:0A5E, 0DDC, 0EE5, 135D, 207A, 2238 and 28F9. */
/*@ ovl0:037A */ void FlashPanelIcon(int index)
{
    DrawPanelIcon(index, 1, 0);
    PlaySound(0x4c);
    WaitFrames(5);
    DrawPanelIcon(index, 0, 0);
}
/* ovl0:03B2: initialises one panel row node. It takes the far pointer gNodePtrs[index], copies the
   0x3c-byte template gNodeTemplate over it, then stores the node's key (index + 1), its two word
   keys `key`/`key` + 0x100, the box x (x << 3), y, width, height and the flags 0x1100. Returns the
   node. Called by ovl0:042F. */
/*@ ovl0:03B2 */ struct Node far *InitRowNode(int index, int x, int y, int width, int height, int key)
{
    struct Node far *node;

    node = gNodePtrs[index];
    movmem(&gNodeTemplate, node, 0x3c);
    node->key = index + 1;
    node->key06 = key;
    node->key08 = key + 0x100;
    node->x = x << 3;
    node->y = y;
    node->width = width;
    node->height = height;
    node->flags = 0x1100;
    return node;
}
/* ovl0:042F: builds the panel's linked row list. For i in 0..count-1 it initialises node i from the
   row definition gRowNodeDefs[base + i] (the definition's five bytes are the node's x, y, width,
   height and key), then links nodes 1..count-1 onto node 0 with ListAdd and sets gNodeListChanged. Returns
   the list head (node 0). Called by ovl0:0A5E, 0DDC, 0EE5, 135D and 28F9. */
/*@ ovl0:042F */ struct Node far *BuildRowNodeList(int base, int count)
{
    struct Node far *list;
    int i;
    int idx;

    list = gNodePtrs[0];
    for (i = 0; i < count; i++) {
        idx = base + i;
        InitRowNode(i, gRowNodeDefs[idx].m00, gRowNodeDefs[idx].m01, gRowNodeDefs[idx].m02,
            gRowNodeDefs[idx].m03, gRowNodeDefs[idx].m04);
    }
    for (i = 1; i < count; i++)
        list = ListAdd(list, gNodePtrs[i]);
    gNodeListChanged = 1;
    return list;
}
/* ovl0:0504: draws the party-status panel when all four front party slots hold a named character
   (a nonzero first name byte at gChars[i]+2). It counts them, and when there are four it shows the
   gStrPartyComplete report on page 2 (DrawShadowText, gDrawPage = 2), hides the pointer, clips the
   report box and draws panel icon 0xf; otherwise it draws icon 0xe. Called by ovl0:1071. */
/*@ ovl0:0504 */ void DrawPartyStatus(void)
{
    char i;
    char count;

    count = 0;
    for (i = 0; i < 4; i++)
        if (gChars[i].name[0])
            count++;
    count = (count == 4);
    if (count == 1) {
        gDrawPage = 2;
        DrawShadowText(gStrPartyComplete, 0xa8, 0x3d, 0xf, 0);
        gDrawPage = 0;
        LockHidePointer();
        CopyRegion(0x15, 0x3d, 0x13, 0x7d, 0x11, 0x28, 2, 0);
        LockShowPointer();
        DrawPanelIcon(0xf, 0, 0);
    } else
        DrawPanelIcon(0xe, 0, 0);
}
/* ovl0:05BA: draws one panel name slot: switches the current font to the 6x6 gFont6x6, computes the
   centred x offset (0x3c - len * gFontWidth) >> 1 (pixels) for the name's `len` characters, hides the
   pointer, draws the fixed label gStrBlankName at gNamePosX[index]/gNamePosY[index] and the character's
   name gChars[index].name centred on it in `color`, shows the pointer and restores the font.
   Called by ovl0:0EE5 and 23EC. */
/*@ ovl0:05BA */ void DrawCharName(int index, int len, int color)
{
    char far *savedFont;
    int x;

    savedFont = gCurFont;
    SetFont(gFont6x6);
    x = (0x3c - gFontWidth * len) >> 1;
    LockHidePointer();
    gDrawText(gStrBlankName, gNamePosX[index], gNamePosY[index], 0xc, 0xc);
    gDrawText(gChars[index].name, gNamePosX[index] + x, gNamePosY[index], color, 0);
    LockShowPointer();
    SetFont(savedFont);
}
/* ovl0:065B: draws the current character's panel page for gChars[index]. It clips the page and,
   unless `mode` is 4, draws the character's sprite, then the name, race gRaceNames[race] and class
   gClassNames[classIndex] centred over the page and six labels gAbilityAbbrevs[8..13] down the left column. It formats
   three stat lines into gFileNameBuf (the strength string "score/percentile" from strength and strengthPct with
   intelligence, wisdom, dexterity, constitution, charisma, the armorClass/maxHp pair and the levels[0..2] triple, the third picking one of three
   gStr61xx formats from levels[1] and levels[2]) and draws each; the mode selects which panel icons are
   highlighted: 1 highlights 4, 7, 8 and 6, 2 highlights 0x10 and 9, 3 highlights 0xa, 0xb and 9
   (each drawn through DrawPanelIcon mode 2, at the icon's own origin with the pointer left shown).
   Called by ovl0:0A5E, 0DDC, 0EE5, 135D, 1ADD and 23EC. */
/*@ ovl0:065B */ void DrawCharPanel(int index, char mode)
{
    char far *text;
    int i;

    LockHidePointer();
    CopyRegion(0, 0, 0x14, 0, 0x14, 0x80, 2, 2);
    gDrawPage = 2;
    if (mode != 4)
        DrawShape(2, gChars[index].portrait, 0xe0, 2, 0);
    text = gChars[index].name;
    DrawShadowText(text, 0xa0 + (0x14 - strlen(text)) * 4, 0x23, 0xf, 0);
    text = gRaceNames[gChars[index].race];
    DrawShadowText(text, 0xa0 + (0x14 - strlen(text)) * 4, 0x2d, 0xf, 0);
    text = gClassNames[gChars[index].classIndex];
    DrawShadowText(text, 0xa0 + (0x14 - strlen(text)) * 4, 0x36, 0xf, 0);
    for (i = 8; i < 0xe; i++)
        DrawShadowText(gAbilityAbbrevs[i], 0xa3, i * 8, 0xf, 0);
    DrawShadowText(gStrAcHpLvlLabels, 0xf8, 0x40, 0xf, 0);
    sprintf(gFileNameBuf, gFmtAbilityScores, FormatStrengthPercent(gChars[index].strength, gChars[index].strengthPct),
        gChars[index].intelligence, gChars[index].wisdom, gChars[index].dexterity, gChars[index].constitution,
        gChars[index].charisma);
    DrawShadowText(gFileNameBuf, 0xc0, 0x40, 0xf, 0);
    sprintf(gFileNameBuf, gFmtAcHp, gChars[index].armorClass, gChars[index].maxHp);
    DrawShadowText(gFileNameBuf, 0x118, 0x40, 0xf, 0);
    text = gFmtOneLevel;
    if (gChars[index].levels[1] > 0)
        text = gFmtTwoLevels;
    if (gChars[index].levels[2] > 0)
        text = gFmtThreeLevels;
    sprintf(gFileNameBuf, text, gChars[index].levels[0], gChars[index].levels[1], gChars[index].levels[2]);
    DrawShadowText(gFileNameBuf, 0x118, 0x50, 0xf, 0);
    switch (mode) {
    case 1:
        DrawPanelIcon(4, 0, 2);
        DrawPanelIcon(7, 0, 2);
        DrawPanelIcon(8, 0, 2);
        DrawPanelIcon(6, 0, 2);
        break;
    case 2:
        DrawPanelIcon(0x10, 0, 2);
        DrawPanelIcon(9, 0, 2);
        break;
    case 3:
        DrawPanelIcon(0xa, 0, 2);
        DrawPanelIcon(0xb, 0, 2);
        DrawPanelIcon(9, 0, 2);
        break;
    }
    CopyRegion(0x14, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    LockShowPointer();
    gDrawPage = 0;
}
/* ovl0:0A4F: draws the row list index 0xa. No direct caller found (reached through an overlay entry
   or jump table). */
/*@ ovl0:0A4F */ void DrawRowList10(void)
{
    DrawRowList(10);
}
void AnimateMemberBox(void);              /* ovl0:1224, defined below */
void ModifyCharacterStats(void);              /* ovl0:1ADD, defined below */
void RollCharacterStats(int who);           /* ovl0:1BF2, defined below */
void DrawPanelRow(int index);      /* ovl0:11BA, defined below */
void BuildAbilityLimits(void);      /* ovl0:1EA9, defined below */
int PickClass(int race);           /* ovl0:207A, defined below */
int PickAlignment(int classIndex); /* ovl0:2238, defined below */

/* ovl0:0A5E: the party-position screen. It lays out a six-row list from row 0x15, picks the first
   free slot for the current member gChargenMember (FindFreePortrait from slot 0x1a when gNewMemberFemale, the parity of the race
   choice PickRace returned, is odd, else from 0) and then, until a slot is confirmed (cur), redraws
   the four member cells: cell i gets gPortraitCells[taken[i]] at (0xb0 + i*0x20, 0x42), with the four
   slots walked by FindFreePortrait. It then waits for a key (ovl0:1224 animates first) and turns it into a
   command: 0x2b/0x61 confirm (3), the left keys 0x5c/0x8001 blink icon 0xc and step left (1), the
   right keys 0x66/0x8002 blink icon 0xd and step right (2), the 0x8003..0x8006 keys pick position
   key-3 and confirm, anything else loops.
   A command of 1/2 slides the slot through FindFreePortrait when `pos` sits at an end (or the mouse is
   enabled), else steps `pos`; a command of 3 records `pos`. On exit it stores the chosen slot into
   gMemberPortraits[gChargenMember] and the record's portraitIndex, unpacks the chosen cell into the member's portrait and redraws
   the page in mode 1. Called by ovl0:0DDC. The 44 cells of gPortraitCells are the faces cut from CHARGENA. */
/*@ ovl0:0A5E */ void PickPortrait(void)
{
    int key;
    char slot;
    char i;
    char prev;
    char pos;
    char cur;
    char taken[4] = { 0, 0, 0, 0 };
    struct Node far *list;

    FlushKeyBuffer();
    list = BuildRowNodeList(0x15, 6);
    slot = gNewMemberFemale ? 0x1a : 0;
    DrawCharPanel(gChargenMember, 4);
    DrawPanelIcon(0xc, 0, 0);
    DrawPanelIcon(0xd, 0, 0);
    DrawRowList(-1);
    slot = FindFreePortrait(slot, gNewMemberFemale, 1, gMemberPortraits);
    cur = -1;
    pos = 1;
    while (cur == -1) {
        prev = slot;
        LockHidePointer();
        for (i = 0; i < 4; i++) {
            taken[i] = slot;
            DrawShape(0, gPortraitCells[taken[i]], i * 0x20 + 0xb0, 0x42, 0);
            slot = FindFreePortrait(slot + 1, gNewMemberFemale, 1, gMemberPortraits);
        }
        LockShowPointer();
        slot = prev;
        do {
            AnimateMemberBox();
            key = WaitKeyOrPause(list);
            if (gMouseDisabled)
                DrawRowList(pos + 0xa);
            switch (key) {
            case 0x8003:
            case 0x8004:
            case 0x8005:
            case 0x8006:
                pos = key - 3;
            case 0x2b:
            case 0x61:
                key = 3;
                break;
            case 0x5c:
            case 0x8001:
                FlashPanelIcon(0xc);
                key = 1;
                break;
            case 0x66:
            case 0x8002:
                FlashPanelIcon(0xd);
                key = 2;
                break;
            default:
                if (key < 0)
                    key &= 0xff;
                else
                    key = 0;
                break;
            }
        } while (key == 0);
        if (gMouseDisabled)
            DrawRowList(-1);
        switch (key) {
        case 1:
            if (pos == 0 || !gMouseDisabled)
                slot = FindFreePortrait(slot - 1, gNewMemberFemale, -1, gMemberPortraits);
            else
                pos--;
            break;
        case 2:
            if (pos == 3 || !gMouseDisabled)
                slot = FindFreePortrait(slot + 1, gNewMemberFemale, 1, gMemberPortraits);
            else
                pos++;
            break;
        case 3:
            cur = pos;
            break;
        }
    }
    DrawRowList(-1);
    AnimateMemberBox();
    gMemberPortraits[gChargenMember] = taken[cur];
    gChars[gChargenMember].portraitIndex = taken[cur];
    Unpack0(gPortraitCells[taken[cur]], gChars[gChargenMember].portrait, 0x514);
    DrawCharPanel(gChargenMember, 1);
}
/* ovl0:0D5B: walks the panel slot indices modulo 0x2c (44) to find one that is free. Starting from
   `start` (a negative start is folded to 0x2b), it loops: a slot is rejected when one of the four
   named characters gChars[0..3] has `taken[di]` equal to it, when `flag` is nonzero and the slot is
   below 0x1a, or when `flag` is zero and the slot is above 0x1c; a rejected slot advances by
   `delta` (folded and wrapped) and the search repeats. Returns the free slot. Called by ovl0:0A5E. */
/*@ ovl0:0D5B */ int FindFreePortrait(int start, int flag, int delta, char far *taken)
{
    int slot;
    int found;
    int di;

    slot = start;
    if (slot < 0)
        slot = 0x2b;
    slot %= 0x2c;
    do {
        found = 0;
        di = 0;
        for (; di < 4; di++) {
            if (gChars[di].name[0] && taken[di] == slot)
                found = 1;
        }
        if (flag != 0 && slot < 0x1a)
            found = 1;
        if (flag == 0 && slot > 0x1c)
            found = 1;
        if (found) {
            slot += delta;
            if (slot < 0)
                slot = 0x2b;
            slot %= 0x2c;
        }
    } while (found);
    return slot;
}
/* ovl0:0DDC: the character panel's function-key loop. It picks the party slot (ovl0:0A5E), opens the
   panel page, builds the 0x1b-based four-row list, then until a key ends it redraws the animation and
   waits: keys 0x8001/0x8002/0x8003 flash icons 4/7/8 and roll the member's stats (ovl0:1BF2), edit
   the seven ability/stat fields (ModifyCharacterStats) or pick the party slot (ovl0:0A5E), each redrawing the
   page and list. The enter key 0x61 and key 0x8004 flash icon 6 and set key to 1, which draws the
   current row list and returns 0; any other key sets key to 0, so the loop continues. Called by
   CreateCharacter (ovl0:23EC). Purpose unknown: the 0x8001..0x8004 key codes. */
/*@ ovl0:0DDC */ int ReviewRolledStats(void)
{
    struct Node far *list;
    register int key;

    PickPortrait();
    DrawCharPanel(gChargenMember, 1);
    list = BuildRowNodeList(0x1b, 4);
    FlushKeyBuffer();
    for (;;) {
        AnimateMemberBox();
        key = WaitKeyOrPause(list);
        switch (key) {
        case 0x8001:
            FlashPanelIcon(4);
            AnimateMemberBox();
            RollCharacterStats(gChargenMember);
            key = -1;
            break;
        case 0x8002:
            FlashPanelIcon(7);
            ModifyCharacterStats();
            key = -1;
            break;
        case 0x8003:
            FlashPanelIcon(8);
            PickPortrait();
            key = -1;
            break;
        case 0x61:
        case 0x8004:
            FlashPanelIcon(6);
            key = 1;
            break;
        default:
            key = 0;
            break;
        }
        if (key < 0) {
            DrawCharPanel(gChargenMember, 1);
            list = BuildRowNodeList(0x1b, 4);
            key = 0;
        }
        if (key == 0)
            continue;
        DrawRowList(gChargenMember + 6);
        DrawRowList(-1);
        return 0;
    }
}
/* ovl0:0EE5: Shows a created member's page, letting the player move to another member, delete this
   one, go back, or start the game (returns 1 for PLAY). The character-editor loop. It builds a
   seven-row list, draws the page
   ovl0:065B(mode 2), then until the current character has a name and no exit was asked for it waits
   for a key: the four 0x8001..0x8004 keys pick the character (key & 0xff) - 1, 0x8005 returns 1
   (leaving the editor), 0x8006 and the enter/confirm keys 0x2b/0x3d/0x61/0x6e flash icon 9 and clear
   the result, 0x8007 erases the current character's name (icon 0x10, DrawCharName, DrawPanelRow), and
   0x5c/0x66 and 0x60/0x62 flip the character index's bit 0 and bit 1. Every pass redraws the row list
   and, when the selection moved to a named character, the page. Returns the result. Called by
   ovl0:28F9. */
/*@ ovl0:0EE5 */ int ViewCharacter(void)
{
    int key;
    int result;
    struct Node far *list;
    register int pos;
    register int flag;

    list = BuildRowNodeList(0, 7);
    FlushKeyBuffer();
    DrawRowList(-1);
    result = 0;
    flag = 0;
    DrawCharPanel(gChargenMember, 2);
    while (gChars[gChargenMember].name[0] != 0 && !flag) {
        DrawRowList(gChargenMember + 6);
        key = WaitKeyOrPause(list);
        pos = gChargenMember;
        switch (key) {
        case 0x8001:
        case 0x8002:
        case 0x8003:
        case 0x8004:
            pos = (key & 0xff) - 1;
            break;
        case 0x8005:
            result = 1;
            flag = 1;
            break;
        case 0x2b:
        case 0x3d:
        case 0x61:
        case 0x6e:
        case 0x8006:
            FlashPanelIcon(9);
            result = 0;
            flag = 1;
            break;
        case 0x8007:
            if (gChars[gChargenMember].name) {
                FlashPanelIcon(0x10);
                gChars[gChargenMember].name[0] = 0;
                DrawCharName(gChargenMember, 1, 0xc);
                DrawPanelRow(gChargenMember + 0x32);
            }
            break;
        case 0x5c:
        case 0x66:
            pos ^= 1;
            break;
        case 0x60:
        case 0x62:
            pos ^= 2;
            break;
        }
        if (flag)
            DrawRowList(-1);
        if (gChars[pos].name[0] == 0)
            flag = 1;
        if (pos != gChargenMember) {
            gChargenMember = pos;
            DrawRowList(-1);
            if (!flag)
                DrawCharPanel(gChargenMember, 2);
        }
    }
    return result;
}
/* ovl0:1071: draws the panel report line gStrSelectBoxPrompt at (0xa8, 0x10) over a fresh region
   (gDrawPage = 2
   while it draws), restores gDrawPage, redraws the report box and then calls DrawPartyStatus. Called
   by ovl0:28F9. */
/*@ ovl0:1071 */ void ShowSelectBoxPrompt(void)
{
    int saved;

    CopyRegion(0, 0, 0x14, 0, 0x14, 0x80, 2, 2);
    saved = gDrawPage;
    gDrawPage = 2;
    DrawShadowText(gStrSelectBoxPrompt, 0xa8, 0x10, 0xf, 0);
    gDrawPage = saved;
    LockHidePointer();
    CopyRegion(0x14, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    LockShowPointer();
    DrawPartyStatus();
}

/* ovl0:10F6: the same as ovl0:1071 but draws gStrEnteringGame at (0xa8, 0x20) and does not call
   DrawPartyStatus. Called by ovl0:28F9. */
/*@ ovl0:10F6 */ void ShowEnteringGame(void)
{
    int saved;

    CopyRegion(0, 0, 0x14, 0, 0x14, 0x80, 2, 2);
    saved = gDrawPage;
    gDrawPage = 2;
    DrawShadowText(gStrEnteringGame, 0xa8, 0x20, 0xf, 0);
    gDrawPage = saved;
    LockHidePointer();
    CopyRegion(0x14, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    LockShowPointer();
}
/* ovl0:1177: rolls an ability score: four 1d6 rolls summed with the lowest dropped. Returns the
   sum as a byte. Called by ovl0:1BF2. */
/*@ ovl0:1177 */ char RollAbilityScore(void)
{
    int sum;
    int roll;
    int i;
    int min;

    min = 0xa;
    sum = 0;
    for (i = 0; i < 4; i++) {
        roll = RollDice(1, 6, 0);
        sum += roll;
        if (roll < min)
            min = roll;
    }
    sum -= min;
    return sum;
}
/* ovl0:11BA: draws the panel row the icon index `index` maps to. It clears the row list (-1), hides
   the pointer, then either draws panel icon index - 0x32 when `index` is above 0x30, or slices the
   current character gChargenMember's sprite over the row through DrawShape (mode 0) at gPanelX5E54/gChargenMember,
   gPanelY5E5C/gChargenMember + 1. Shows the pointer afterwards. Called by ovl0:0EE5 and 23EC. */
/*@ ovl0:11BA */ void DrawPanelRow(int index)
{
    int row;

    row = index;
    DrawRowList(-1);
    LockHidePointer();
    if (row > 0x30) {
        row -= 0x32;
        DrawPanelIcon(row, 0, 0);
    } else
        DrawShape(0, gChars[gChargenMember].portrait, gPanelX5E54[gChargenMember], gPanelY5E5C[gChargenMember] + 1, 0);
    LockShowPointer();
}
/* ovl0:1224: advances and draws the current character's animation frame. When the selected character
   changed it resets the frame timer gBoxAnimNextTick; when gBoxAnimNextTick is behind gTicks it steps gBoxAnimFrame through 0..9
   and sets gBoxAnimNextTick two ticks ahead. If the frame did not change it returns, else it clips the character
   cell, blits cell gBoxAnimCells[gBoxAnimFrame] at (0x120, 0x80) through DrawShape, hides the pointer over the
   character's panel box, clips it and shows the pointer. Called by ovl0:0A5E, 0DDC, 135D, 1F66,
   207A and 2238. Purpose unknown: which animation the ten cells are. */
/*@ ovl0:1224 */ void AnimateMemberBox(void)
{
    if (gBoxAnimMember != gChargenMember) {
        gBoxAnimNextTick = 0;
        gBoxAnimMember = gChargenMember;
    }
    if (gBoxAnimNextTick < gTicks) {
        gBoxAnimFrame++;
        if (gBoxAnimFrame > 9)
            gBoxAnimFrame = 0;
        gBoxAnimNextTick = gTicks + 2;
    }
    if (gBoxAnimDrawnFrame == gBoxAnimFrame)
        return;
    CopyRegion(gChargenMember * 4, 0x80, 0x24, 0x80, 4, 0x20, 2, 2);
    DrawShape(2, gBoxAnimCells[gBoxAnimFrame], 0x120, 0x80, 0);
    HidePointerZone(gPanelX5E54[gChargenMember], gPanelY5E5C[gChargenMember] + 1,
        gPanelX5E54[gChargenMember] + 0x20, gPanelY5E5C[gChargenMember] + 0x20);
    CopyRegion(0x24, 0x80, gPanelX5E54[gChargenMember] >> 3, gPanelY5E5C[gChargenMember] + 1,
        4, 0x20, 2, 0);
    ShowPointerZone();
    gBoxAnimDrawnFrame = gBoxAnimFrame;
}
/* ovl0:135D: the field editor of the character editor. `index` is the field (0..6), `value` its
   byte and `percent` the strength percentile, read on every call (ovl0:135D) but used only for
   index 0 (the callers pass a null far pointer for indexes 1..6, ovl0:1ADD). It draws the
   panel page (ovl0:065B mode 3), finds the list node index+1 and its box, writes the field into
   gFileNameBuf and draws it, then loops: each pass reads a key (ovl0:1224 animates first). The
   decrement keys 0xc/0x5c/0x69/0x8009 blink icon 0xb and lower the value, the increment keys
   0xd/0x66/0x6a/0x8008 blink icon 0xa and raise it, 0x60/0x62 step the returned field index (mod 7),
   and 0x61/0x6e/0x800a confirm (icon 9). A changed value is clamped (index 0 between gStatMin[0] and
   gStatMax[0] & 0xff, with a strength above 0x12 moved into the percentile and the percentile reset,
   later indexes between gStatMin[index] and gStatMax[index]), stored, and index 6 copies it to maxHp
   (on entry for index 6 it also recomputes gHpMaxOrAttackTarget/gHpMinOrHitFlag from MaxHitPoints/MinHitPoints), index 4
   recomputes the derived hit points (hp/maxHp from gHpMaxOrAttackTarget/gHpMinOrHitFlag) or index 3 the armour class armorClass
   (GetArmourClassBase + 10). A key that changed nothing but set the done flag redraws the value and
   ends. Returns the field index to move to (-2 for the confirm key). Called by ovl0:1ADD. Purpose
   unknown: the per-index special cases and formats. */
/*@ ovl0:135D */ int EditStatField(register int index, char far *value, char far *percent)
{
    int key;
    int amount;
    int count;
    int flag;
    struct Node far *list;
    struct Node far *node;
    register int move;

    list = BuildRowNodeList(0x1f, 0xa);
    node = FindNode(list, index + 1);
    DrawCharPanel(gChargenMember, 3);
    FlushKeyBuffer();
    if (index == 0)
        sprintf(gFileNameBuf, gFmtStrengthOnEntry, FormatStrengthPercent(*value, *percent));
    else
        sprintf(gFileNameBuf, gFmtValueOnEntry, *value);
    LockHidePointer();
    CopyRegion(((node->x + 0x20) >> 3) - 0x12, node->y - 0x40, (node->x + 0x20) >> 3, node->y,
        5, node->height, 2, 0);
    DrawShadowText(gFileNameBuf, node->x + 0x20, node->y, 6, 0);
    LockShowPointer();
    flag = 0;
    count = *percent;
    move = index;
    if (index == 6) {
        gHpMaxOrAttackTarget = MaxHitPoints(gChars[gChargenMember].classIndex, gChars[gChargenMember].constitution, gChars[gChargenMember].levels[0],
            gChars[gChargenMember].levels[1], gChars[gChargenMember].levels[2]);
        gHpMinOrHitFlag = MinHitPoints(gChars[gChargenMember].classIndex, gChars[gChargenMember].constitution, gChars[gChargenMember].levels[0],
            gChars[gChargenMember].levels[1], gChars[gChargenMember].levels[2]);
    }
    while (!flag) {
        amount = *value;
        AnimateMemberBox();
        key = WaitKeyOrPause(list);
        switch (key) {
        case 0x60:
            move--;
            move %= 7;
            flag = 1;
            break;
        case 0x62:
            move++;
            move %= 7;
            flag = 1;
            break;
        case 0xd:
        case 0x66:
        case 0x6a:
        case 0x8008:
            FlashPanelIcon(0xa);
            amount++;
            break;
        case 0xc:
        case 0x5c:
        case 0x69:
        case 0x8009:
            FlashPanelIcon(0xb);
            amount--;
            break;
        case 0x61:
        case 0x6e:
        case 0x800a:
            FlashPanelIcon(9);
            flag = 1;
            key = -1;
            break;
        default:
            if (key < 0) {
                key &= 0xff;
                key--;
                if (key != index) {
                    move = key;
                    flag = 1;
                }
            }
            break;
        }
        if (amount != *value) {
            int old;
            if (index == 0) {
                while (amount > 0x12) {
                    amount--;
                    count++;
                }
                while (count > 0 && amount < 0x12) {
                    amount++;
                    count--;
                }
            }
            if (index == 0) {
                amount = gStatMin[index] < amount ? amount : gStatMin[index];
                amount = (gStatMax[index] & 0xff) > amount ? amount : gStatMax[index] & 0xff;
                *value = amount;
                if (amount < 0x12 || gStatMax[index] < 0x13)
                    count = 0;
                if (count > 0x64)
                    count = 0x64;
                *percent = count;
            } else {
                amount = gStatMin[index] < amount ? amount : gStatMin[index];
                amount = gStatMax[index] > amount ? amount : gStatMax[index];
                *value = amount;
            }
            if (index == 6)
                gChars[gChargenMember].maxHp = amount;
            if (index == 0)
                sprintf(gFileNameBuf, gFmtStrengthChanged, FormatStrengthPercent(*value, *percent));
            else
                sprintf(gFileNameBuf, gFmtValueChanged, *value);
            LockHidePointer();
            CopyRegion(((node->x + 0x20) >> 3) - 0x12, node->y - 0x40, (node->x + 0x20) >> 3,
                node->y, 5, node->height, 2, 0);
            DrawShadowText(gFileNameBuf, node->x + 0x20, node->y, 6, 0);
            if (index == 4) {
                old = gChars[gChargenMember].hp;
                gHpMaxOrAttackTarget = MaxHitPoints(gChars[gChargenMember].classIndex, gChars[gChargenMember].constitution, gChars[gChargenMember].levels[0],
                    gChars[gChargenMember].levels[1], gChars[gChargenMember].levels[2]);
                gHpMinOrHitFlag = MinHitPoints(gChars[gChargenMember].classIndex, gChars[gChargenMember].constitution, gChars[gChargenMember].levels[0],
                    gChars[gChargenMember].levels[1], gChars[gChargenMember].levels[2]);
                gChars[gChargenMember].hp = gChars[gChargenMember].maxHp
                    = gChars[gChargenMember].hp < gHpMinOrHitFlag ? gHpMinOrHitFlag : gChars[gChargenMember].hp;
                gChars[gChargenMember].hp = gChars[gChargenMember].maxHp
                    = gChars[gChargenMember].hp > gHpMaxOrAttackTarget ? gHpMaxOrAttackTarget : gChars[gChargenMember].hp;
                if (gChars[gChargenMember].hp != old) {
                    sprintf(gFileNameBuf, gFmtHpChanged, gChars[gChargenMember].hp);
                    CopyRegion(0xf, 0x48, 0x21, 0x88, 5, 8, 2, 0);
                    DrawShadowText(gFileNameBuf, 0x108, 0x88, 0xf, 0);
                }
            }
            if (index == 3) {
                old = gChars[gChargenMember].armorClass;
                gChars[gChargenMember].armorClass = GetArmourClassBase(amount) + 0xa;
                if (gChars[gChargenMember].armorClass != old) {
                    sprintf(gFileNameBuf, gFmtArmorClassChanged, gChars[gChargenMember].armorClass);
                    CopyRegion(0xf, 0x40, 0x21, 0x80, 5, 8, 2, 0);
                    DrawShadowText(gFileNameBuf, 0x108, 0x80, 0xf, 0);
                }
            }
            LockShowPointer();
            flag = 0;
        } else if (flag) {
            if (key == -1)
                move = 0xfffe;
            if (index == 0)
                sprintf(gFileNameBuf, gFmtStrengthOnExit, FormatStrengthPercent(*value, *percent));
            else
                sprintf(gFileNameBuf, gFmtValueOnExit, *value);
            gDrawPage = 0;
            LockHidePointer();
            gDrawText(gFileNameBuf, (node->x + 0x20) << 3, node->y, 0xf, 0);
            LockShowPointer();
        }
    }
    return move;
}
/* ovl0:1ADD: Lets the player edit the rolled member's six abilities and hit points one field at a
   time within the stat limits. Edits one of the seven ability/stat fields of the current character.
   It draws the page through ovl0:065B(mode 3) and loops over the field index i (0..6); for each it
   opens the field with EditStatField(i, &gChars[gChargenMember].<field>, second) and redraws the page.
   Field 0 passes both strength and strengthPct; fields 1..6 pass intelligence, wisdom, dexterity, constitution, charisma and hp with no second
   field. A return of -2 leaves, a negative return wraps i to 6, then i is taken modulo 7. Finally
   it redraws the page through ovl0:065B(mode 1). Called by ovl0:0DDC. */
/*@ ovl0:1ADD */ void ModifyCharacterStats(void)
{
    int i;

    i = 0;
    FlushKeyBuffer();
    DrawCharPanel(gChargenMember, 3);
    while (i >= 0 && i <= 6) {
        switch (i) {
        case 0:
            i = EditStatField(i, &gChars[gChargenMember].strength, &gChars[gChargenMember].strengthPct);
            break;
        case 1:
            i = EditStatField(i, &gChars[gChargenMember].intelligence, (char far *)0);
            break;
        case 2:
            i = EditStatField(i, &gChars[gChargenMember].wisdom, (char far *)0);
            break;
        case 3:
            i = EditStatField(i, &gChars[gChargenMember].dexterity, (char far *)0);
            break;
        case 4:
            i = EditStatField(i, &gChars[gChargenMember].constitution, (char far *)0);
            break;
        case 5:
            i = EditStatField(i, &gChars[gChargenMember].charisma, (char far *)0);
            break;
        case 6:
            i = EditStatField(i, &gChars[gChargenMember].hp, (char far *)0);
            break;
        }
        DrawCharPanel(gChargenMember, 3);
        if (i == -2)
            break;
        if (i < 0)
            i = 6;
        i %= 7;
    }
    DrawCharPanel(gChargenMember, 1);
}
/* ovl0:1BF2: rolls a fresh set of stats for character `who` of the party being created. For the
   three class slots it copies gClassStartLevels[class * 4 + slot] into the record's levels and the 32-bit
   5000 / gClassStartLevelsNumClasses[class * 4] into its exp; it then builds the class combat tables (ovl0:1EA9) and
   rolls the six abilities: each is RollAbilityScore plus the per-race-pair bonus gRaceAbilityBonus, clamped
   up to gStatMin; then, when the strength roll is exactly 18, the 1d100 percentile roll is ORed
   into the high byte (the low byte stays 18), and finally clamped down to gStatMax. The order matters
   for the 0x12 case. The six values are stored into the record's stat pairs (strength/baseStrength, strengthPct/baseStrengthPct and
   intelligence/m12..charisma/m1A), armorClass becomes GetArmourClassBase(dexterity) + 10, and the hit points hp/maxHp become
   RollHitPoints(class, constitution, the three levels slots). Called by ovl0:0DDC and ovl0:23EC. */
/*@ ovl0:1BF2 */ void RollCharacterStats(int who)
{
    int scores[6];
    int i;

    for (i = 0; i < 3; i++) {
        gChars[who].levels[i] = gClassStartLevels[gChars[who].classIndex * 4 + i];
        gChars[who].experience[i] = 0x1388 / gClassStartLevelsNumClasses[gChars[who].classIndex * 4];
    }
    BuildAbilityLimits();
    for (i = 0; i < 6; i++) {
        scores[i] = RollAbilityScore() + gRaceAbilityBonus[gChars[who].race >> 1][i];
        if (scores[i] < gStatMin[i])
            scores[i] = gStatMin[i];
        if (i == 0 && scores[0] == 0x12)
            scores[0] |= RollDice(1, 0x64, 0) << 8;
        if (scores[i] > gStatMax[i])
            scores[i] = gStatMax[i];
    }
    gChars[who].strength = gChars[who].baseStrength = scores[0] & 0xff;
    gChars[who].strengthPct = gChars[who].baseStrengthPct = (scores[0] & 0xff00) >> 8;
    gChars[who].intelligence = gChars[who].m12 = scores[1];
    gChars[who].wisdom = gChars[who].m14 = scores[2];
    gChars[who].dexterity = gChars[who].m16 = scores[3];
    gChars[who].constitution = gChars[who].m18 = scores[4];
    gChars[who].charisma = gChars[who].m1A = scores[5];
    gChars[who].armorClass = GetArmourClassBase(scores[3]) + 0xa;
    gChars[who].hp = gChars[who].maxHp = RollHitPoints(gChars[who].classIndex, gChars[who].constitution,
        gChars[who].levels[0], gChars[who].levels[1], gChars[who].levels[2]);
}
/* ovl0:1EA9: builds the ability limits of the new character gChargenMember. For each of the
   six slots it takes the larger of gClassMinAbilities[classIndex][i] and gRaceMinAbilities[race>>1][i] into gStatMin[i],
   copies gRaceMaxAbilities[race>>1][i] into gStatMax[i], and when the class byte gClassCombatGroup[classIndex] is nonzero
   forces gStatMax[0] to 0x12. Called by ovl0:1BF2. */
/*@ ovl0:1EA9 */ void BuildAbilityLimits(void)
{
    int cls;
    int row;
    int i;

    cls = gChars[gChargenMember].race >> 1;
    row = gChars[gChargenMember].classIndex;
    for (i = 0; i < 6; i++) {
        if (gClassMinAbilities[row][i] > gRaceMinAbilities[cls][i])
            gStatMin[i] = gClassMinAbilities[row][i];
        else
            gStatMin[i] = gRaceMinAbilities[cls][i];
        gStatMax[i] = gRaceMaxAbilities[cls][i];
        if (gClassCombatGroup[row])
            gStatMax[0] = 0x12;
    }
}
/* ovl0:1F66: opens the option list for the current character's race and returns the chosen value.
   It draws the character's panel box (DrawRect), clips and labels it, then repeatedly runs
   UpdateOptionList(1, gRaceNames, 0L, 0xffffffffL, 0) until it returns a value other than -1, redrawing
   the panel row ovl0:1224 after each -1, and stores the value's parity in gNewMemberFemale. Called by
   ovl0:23EC. */
/*@ ovl0:1F66 */ int PickRace(void)
{
    int choice;

    gDrawPage = 0;
    LockHidePointer();
    DrawRect(gMemberBoxX[gChargenMember], gMemberBoxY[gChargenMember], gMemberBoxX[gChargenMember] + 0x20, gMemberBoxY[gChargenMember] + 0x21, 0xc);
    CopyRegion(0, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    DrawShadowText(gStrSelectRace, 0x93, 0x43, 9, 0);
    LockShowPointer();
    FlushKeyBuffer();
    gPanels[1].sel = 0;
    DrawOptionList(1, gRaceNames, 0xffffffffL, 0, 0);
    while ((choice = UpdateOptionList(1, gRaceNames, 0L, 0xffffffffL, 0)) == -1)
        AnimateMemberBox();
    gNewMemberFemale = choice % 2;
    return choice;
}
/* ovl0:2051: returns the index of the `n`th set bit of `mask` (n = 1 is the lowest), scanning up to
   `count` bits; when fewer than `n` bits are set it returns `count`. No direct caller found. */
/*@ ovl0:2051 */ int FindNthSetBit(int mask, int n, int count)
{
    int remaining;
    int i;

    remaining = n;
    for (i = 0; i < count; i++) {
        if (mask & (1 << i)) {
            remaining--;
            if (remaining == 0)
                break;
        }
    }
    return i;
}
/* ovl0:207A: the class picker of character creation. `race` is a party member's race index; it is
   halved to index the per-race-pair class-availability mask gRaceClassMasks, and bit 2 is cleared from
   that mask when any of the four named characters has an alignment of 6 or more. It counts the
   available classes with CountMaskBits, sizes the shared panel 2 to them, draws the class names
   gClassNames through the option list and loops on the mouse or UpdateOptionList until it returns a choice
   or the mouse's confirm keys 0xc6/0xc7. Returns the chosen class index, or 0x6e when the mouse
   confirmed (drawn as icon 5 flashed). Called by ovl0:23EC (CreateCharacter) for the new character's
   class pick, given its race. The `key &&` guard in the mouse branch is redundant (neither 0xc6 nor
   0xc7 is 0) but the original has its `or ax,ax` test, so the source wrote it. */
/*@ ovl0:207A */ int PickClass(int race)
{
    register int mask;
    register int sel;
    int done;
    int key;

    mask = -1;
    for (sel = 0; sel < 4; sel++) {
        if (gChars[sel].name[0] && gChars[sel].alignment >= 6)
            mask = -5;
    }
    race /= 2;
    sel = CountMaskBits(15, gRaceClassMasks[race] & mask, 0);
    gPanels[2].height = sel;
    gPanels[2].sel = 0;
    FlushKeyBuffer();
    AnimateMemberBox();
    LockHidePointer();
    CopyRegion(0, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    DrawShadowText(gStrSelectClass, 0x93, 0x43, 9, 0);
    DrawPanelIcon(5, 0, 0);
    LockShowPointer();
    DrawOptionList(2, gClassNames, gRaceClassMasks[race] & mask, 0, 0);
    done = gClickX = gClickY = 0;
    sel = -1;
    while (!done && sel == -1) {
        AnimateMemberBox();
        if (IsMouseInRect(0x108, 0xab, 0x12f, 0xbb)) {
            key = ReadKey();
            if (key && (key == 0xc6 || key == 0xc7)) {
                WaitKeyEvent();
                done = 1;
            }
        } else
            sel = UpdateOptionList(2, gClassNames, 0L, gRaceClassMasks[race] & mask, 0);
    }
    if (done) {
        FlashPanelIcon(5);
        sel = 0x6e;
    }
    return sel;
}
/* ovl0:2238: the alignment picker of character creation, the twin of ovl0:207A. `classIndex` is a
   party member's class index; it indexes the per-class alignment-availability mask gClassAlignmentMasks, and
   bits 6..8 are cleared from that mask when any of the four named characters has class 2. It counts
   the available alignments with CountMaskBits, sizes the shared panel 3 to them, draws the alignment
   names gAlignmentNames through the option list and loops with `while (!done && UpdateOptionList(...) == -1)`:
   each pass redraws the panel animation (ovl0:1224) and sets `done` for the keys UpdateOptionList
   did not handle 0x6e/0x32 or the mouse buttons 0x41/0x42 whose last click (gClickX, gClickY) lies in
   the list box; a confirmed entry (UpdateOptionList not -1) ends the loop. Returns
   the chosen alignment index, or 0x6e when it was confirmed by the mouse or the keys 0x6e/0x32
   (icon 5 flashed). Called by ovl0:23EC (CreateCharacter) for the new member's alignment pick, given
   its class. */
/*@ ovl0:2238 */ int PickAlignment(int classIndex)
{
    register int mask;
    register int sel;
    int done;

    mask = -1;
    for (sel = 0; sel < 4; sel++) {
        if (gChars[sel].name[0] && gChars[sel].classIndex == 2)
            mask = 0xfe3f;
    }
    sel = CountMaskBits(9, gClassAlignmentMasks[classIndex] & mask, 0);
    gPanels[3].height = sel;
    gPanels[3].sel = 0;
    AnimateMemberBox();
    LockHidePointer();
    CopyRegion(0, 0, 0x12, 0x40, 0x14, 0x80, 2, 0);
    DrawShadowText(gStrSelectAlignment, 0x93, 0x43, 9, 0);
    DrawPanelIcon(5, 0, 0);
    LockShowPointer();
    DrawOptionList(3, gAlignmentNames, gClassAlignmentMasks[classIndex] & mask, 0, 0);
    FlushKeyBuffer();
    done = 0;
    while (!done
        && (sel = UpdateOptionList(3, gAlignmentNames, 0L, gClassAlignmentMasks[classIndex] & mask, 0)) == -1) {
        AnimateMemberBox();
        if (gUnhandledOptionKey == 0x6e || gUnhandledOptionKey == 0x32)
            done = 1;
        if (gUnhandledOptionKey == 0x41 || gUnhandledOptionKey == 0x42) {
            if (IsPointInRect(gClickX, gClickY, 0x109, 0xab, 0x12f, 0xbb))
                done = 1;
        }
    }
    if (done) {
        sel = 0x6e;
        FlashPanelIcon(5);
    }
    return sel;
}
/* ovl0:23EC: the character-creation driver. It walks a three-step loop over the new member
   gChargenMember:
   step 0 picks its race (PickRace), step 1 its class (ovl0:207A PickClass) and step 2 its alignment
   (ovl0:2238 PickAlignment); each of steps 1 and 2, when its picker reported confirmed (0x6e, the
   mouse's accept), steps the index back one instead of forward, so the loop ends only after all three
   were taken; step 0 sets no confirmed value. It then rolls the member's stats (ovl0:1BF2), opens the
   panel loop (ovl0:0DDC) and, until the name is non-empty, redraws the row and page, edits the name
   field with EditTextField and redraws the centred name. Called by ovl0:28F9. Purpose unknown: row
   ds:DD12. */
/*@ ovl0:23EC */ void CreateCharacter(void)
{
    int step;
    int confirmed;
    int len;

    SetTextWindow(2);
    step = 0;
    do {
        confirmed = 0;
        switch (step) {
        case 0:
            gChars[gChargenMember].race = PickRace();
            break;
        case 1:
            confirmed = (gChars[gChargenMember].classIndex = PickClass(gChars[gChargenMember].race)) == 0x6e;
            break;
        case 2:
            confirmed = (gChars[gChargenMember].alignment = PickAlignment(gChars[gChargenMember].classIndex)) == 0x6e;
            break;
        }
        step++;
        if (confirmed)
            step -= 2;
    } while (step != 3);
    RollCharacterStats(gChargenMember);
    ReviewRolledStats();
    gChars[gChargenMember].name[0] = 0;
    do {
        DrawPanelRow(gUnwrittenPanelRow);
        DrawCharPanel(gChargenMember, 0);
        LockHidePointer();
        DrawShadowText(gStrNamePrompt, 0x95, 0x64, 9, 0);
        len = EditTextField(gChars[gChargenMember].name, 0x18, 0x64, 0xa, 0xf, 0, 8);
        LockShowPointer();
        DrawCharName(gChargenMember, len, 2);
    } while (gChars[gChargenMember].name[0] == 0);
}
/* ovl0:2547: loads the character panel's screens and slices its icon buffers. It allocates the 0x13
   0x3c-byte gNodePtrs nodes, loads three named screens (gFileNameChargenA, gFileNameChargen, gFileNameChargenB) and slices
   the 0x2c panel cells gPortraitCells ((i%10)*4, (i/10)*0x20) and ten row cells gBoxAnimCells (i*4, 0) out
   of gCgaMapDefault through ds:8082, builds the 17-icon table and blits gCgaMapDefault with CopyPage
   (the (3,4) region and a panel clip in video modes 0 and 1, the (3,2) region in every other mode).
   Called by ovl0:28F9. */
/*@ ovl0:2547 */ void InitPanelScreens(void)
{
    int i;

    for (i = 0; i < 0x13; i++)
        gNodePtrs[i] = (struct Node far *)MemAlloc(0x3cL, 0);
    LoadOverlayScreen(gFileNameChargenA, 5, 3);
    for (i = 0; i < 0x2c; i++)
        gPortraitCells[i] = gGetSpriteCellRle((i % 10) * 4, (i / 10) * 0x20, 4, 0x20, gCgaMapDefault);
    gDrawPage = 0;
    LoadScreenImage(gFileNameChargen, gCgaMapDefault, 5, 3, 0);
    LoadOverlayScreen(gFileNameChargenB, 5, 3);
    for (i = 0; i < 0xa; i++)
        gBoxAnimCells[i] = gGetSpriteCellRle(i * 4, 0, 4, 0x20, gCgaMapDefault);
    BuildIconTable();
    if (gGraphicsChoice == 0 || gGraphicsChoice == 1) {
        CopyPage(3, 4, FP_OFF(gCgaMapDefault), FP_SEG(gCgaMapDefault));
        CopyRegion(0, 0, 0, 0, 0x28, 0xc8, 4, 2);
    } else
        CopyPage(3, 2, FP_OFF(gCgaMapDefault), FP_SEG(gCgaMapDefault));
    gDrawPage = 0;
    CopyRegion(0x12, 0x40, 0, 0, 0x14, 0x80, 0, 2);
    LockShowPointer();
}
/* ovl0:26C6: gives each of the first four characters its class starting equipment. It copies item
   0x23 into gChars[0]'s slot 2, then for each character sets m01 = 1, food = 0x64, index00 = i and slot
   3 = item 0xa; for each of the class's four table entries gClassStartItems[m20][j] it looks up gStartItems[idx]'s
   item and slot: an item of -1 is skipped, a negative slot takes the first free of slots 0..2, an
   occupied slot is skipped, item 5 for race >> 1 == 3 becomes 0x24, the item is copied into a free
   slot (the first free from slot 2) and stored. Finally it rebuilds the character's armour class
   through ComputeArmourClass. Called by ovl0:28F9. Purpose unknown: the item numbers. */
/*@ ovl0:26C6 */ void GiveStartingEquipment(void)
{
    int j;
    int i;
    int idx;
    struct CharRec far *rec;
    register int item;
    register int slot;

    item = CopyItemToFreeSlot(0x23);
    gChars[0].items[2] = item;
    for (i = 0; i < 4; i++) {
        rec = &gChars[i];
        rec->status = 1;
        rec->food = 0x64;
        rec->index = i;
        item = CopyItemToFreeSlot(0xa);
        rec->items[3] = item;
        for (j = 0; j < 4; j++) {
            idx = gClassStartItems[rec->classIndex * 4 + j];
            item = gStartItems[idx].item;
            slot = gStartItems[idx].slot;
            if (item == -1)
                continue;
            if (slot < 0) {
                slot = 0;
                if (rec->items[slot] != 0)
                    slot = 1;
                if (rec->items[slot] != 0)
                    slot = 2;
            }
            if (slot != 2 && rec->items[slot] != 0)
                continue;
            if (item == 5 && (rec->race >> 1) == 3)
                item = 0x24;
            item = CopyItemToFreeSlot(item);
            if (slot != 2) {
                rec->items[slot] = item;
            } else {
                slot = 2;
                while (rec->items[slot] != 0)
                    slot++;
                rec->items[slot] = item;
            }
        }
        ComputeArmourClass(i);
    }
}
/* ovl0:280D: for each of the first four characters whose class byte
   gClassBaseBits[gChars[i].classIndex] has bit 1
   (value 2) set, sets the 32-bit spell book gChars[i].spellBook to 0x26c, the starting mage spells. Called by
   ovl0:28F9. */
/*@ ovl0:280D */ void GiveStartingSpellBooks(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        if (gClassBaseBits[gChars[i].classIndex] & 2)
            gChars[i].spellBook = 0x26c;
    }
}

/* ovl0:284A: for each of the first four character records, copies the eight bytes at +0x0D, +0x0F,
   +0x11, +0x13, +0x15, +0x17, +0x19 and +0x1B one byte up (to +0x0E, +0x10, ..., +0x1C) through a
   far pointer walked by 0xf3: the current abilities and hit points become the base ones before
   the game starts. Called by ovl0:28F9. */
/*@ ovl0:284A */ void SetBaseStats(void)
{
    struct CharRec far *rec;
    int i;

    rec = (struct CharRec far *)gChars;
    for (i = 0; i < 4; i++, rec++) {
        rec->baseStrength = rec->strength;
        rec->baseStrengthPct = rec->strengthPct;
        rec->m12 = rec->intelligence;
        rec->m14 = rec->wisdom;
        rec->m16 = rec->dexterity;
        rec->m18 = rec->constitution;
        rec->m1A = rec->charisma;
        rec->maxHp = rec->hp;
    }
}

/* ovl0:28B0: releases the panel's buffers: the whole 0x43-entry table gPortraitCells (the buffers from
   ds:FC8C up to ds:FD98, which this unit only partly sets), then the 17-icon table through
   FreeIconTable, then the 0x13-entry table gNodePtrs. Called by ovl0:28F9. */
/*@ ovl0:28B0 */ void FreePanelBuffers(void)
{
    int i;

    for (i = 0; i < 0x43; i++)
        farfree(gPortraitCells[i]);
    FreeIconTable();
    for (i = 0; i < 0x13; i++)
        farfree(gNodePtrs[i]);
}
/* ovl0:28F9: the character panel's main loop. It loads the screens, sets text window 2, shows a
   report line and the five-row list, then until a finished flag `flag` is set it draws the current
   character's row list, waits for a key through ovl0:00F7, applies the key: the function-key toggles
   0x5c/0x66 flip gChargenMember bit 0 and 0x60/0x62 flip bit 1, the negative-key character switch
   ((key & 0xf) - 1; 4 sets `flag`), and the enter/confirm keys (0x61, 0x2b, 0x3d) which open the
   character editor ovl0:0EE5/23EC. When `flag` is set it flashes icon 0xf, shows report line 2,
   runs GiveStartingEquipment, refreshes the character fields (ovl0:280D, 284A), frees the panel buffers and
   hides the pointer. Called by LoadGamePanel (ovl2:0846). */
/*@ ovl0:28F9 */ void RunCharacterPanel(void)
{
    int i;
    int save;
    struct Node far *list;
    register int flag;
    register int key;

    InitPanelScreens();
    flag = 0;
    SetTextWindow(2);
    ShowSelectBoxPrompt();
    list = BuildRowNodeList(0, 5);
    PlaySound(0x14);
    do {
        DrawRowList(gChargenMember + 6);
        key = WaitKeyOrPause(list);
        if (key != 0) {
            switch (key) {
            case 0x5c:
            case 0x66:
                gChargenMember ^= 1;
                break;
            case 0x60:
            case 0x62:
                gChargenMember ^= 2;
                break;
            }
            DrawRowList(-1);
        }
        if (key < 0) {
            key = (key & 0xf) - 1;
            if (key == 4)
                flag = 1;
            else {
                gChargenMember = key;
                key = 0x2b;
            }
        }
        if (key == 0x61 || key == 0x2b || key == 0x3d) {
            DrawRowList(-1);
            if (gChars[gChargenMember].name[0]) {
                save = gChargenMember;
                if (ViewCharacter())
                    flag = 1;
                if (gChars[gChargenMember].name[0] == 0 && save != gChargenMember)
                    CreateCharacter();
            } else
                CreateCharacter();
            list = BuildRowNodeList(0, 5);
            ShowSelectBoxPrompt();
        }
        if (flag) {
            for (i = 0; i < 4; i++)
                if (gChars[i].name[0] == 0)
                    flag = 0;
        }
    } while (flag == 0);
    FlashPanelIcon(0xf);
    ShowEnteringGame();
    GiveStartingEquipment();
    GiveStartingSpellBooks();
    SetBaseStats();
    FreePanelBuffers();
    LockHidePointer();
}
