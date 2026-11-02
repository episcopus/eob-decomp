/* Unit at overlay 1 of EOB.EXE (segment 2102, 9864 bytes, 16 functions): the party display and
   inventory overlay (src/EOV1.ASM's header says what it does). The unit is C, compiled by TCC
   1.00 (escalation eov1, 2026-11-02): every function has TCC's frame, and ovl1:1153 reads a far
   array with the 16-bit displacement only TCC's own assembler writes (26 8B 87 05 00 at
   ovl1:1277), which TASM cannot. src/EOV1.ASM was written first, as assembly transcribed from
   the disassembly; it stays until this file has each function in C (playbook 30). Every
   function here is a stub until it is written in C. */

/*@flags -ml -O -Z -v */

/* the camp menus (gMenus indexes) */
#define MENU_CAMP 0
#define MENU_GAME_OPTIONS 1
#define MENU_PREFERENCES 2

/* gStrings2003 messages used as menu headers and status boxes */
#define MSG_SELECT_OPTION 1
#define MSG_CAMP 0x38
#define MSG_PREFERENCES 0x39
#define MSG_GAME_OPTIONS 0x3a

/* the click result of a button: 0x8000 + its node key (gButtonDefs index + 1) */
#define BTN_REST 0x8001
#define BTN_MEMORIZE 0x8002
#define BTN_PRAY 0x8003
#define BTN_SCRIBE 0x8004
#define BTN_PREFERENCES 0x8005
#define BTN_GAME_OPTIONS 0x8006
#define BTN_EXIT 0x8007
#define BTN_LOAD 0x8008
#define BTN_SAVE 0x8009
#define BTN_DROP 0x800a
#define BTN_QUIT 0x800b
#define BTN_OPTIONS_EXIT 0x800c
#define BTN_TUNES 0x800d
#define BTN_SOUNDS 0x800e
#define BTN_BAR_GRAPHS 0x800f
#define BTN_MOUSE 0x8010
#define BTN_PREFERENCES_EXIT 0x8011

/* keys DispatchKeyWait returns (src/EOV0.C names the same codes) */
#define KEY_FIRST_A 0x5b
#define KEY_LAST_A 0x5d
#define KEY_PREV 0x60
#define KEY_NEXT 0x62
#define KEY_FIRST_B 0x65
#define KEY_LAST_B 0x67
#define KEY_CONFIRM_A 0x2b
#define KEY_CONFIRM_B 0x3d
#define KEY_CONFIRM_C 0x61

/* the 60-byte screen list node (src/E0CF7.C, src/EOV0.C); only the fields this unit sets are named */
struct Node {
    struct Node far *next;      /* +00 */
    unsigned key;               /* +04: the sort key ListAdd compares */
    unsigned key06;             /* +06: the first key word the dispatcher matches */
    unsigned key08;             /* +08: the second key word the dispatcher matches */
    char pad0A[0x0E - 0x0A];
    unsigned flags;             /* +0E: the inactive bit and the layout bits */
    char pad10[0x1E - 0x10];
    int x;                      /* +1E: box left */
    int y;                      /* +20: box top */
    unsigned width;             /* +22: box width */
    unsigned height;            /* +24: box height */
    char pad26[0x34 - 0x26];
    struct ButtonDef far *def;    /* +34: the definition record the node was built from */
    unsigned defFlags;          /* +38: a copy of def->flags */
    char pad3A[0x3c - 0x3A];
};

/* the 9-byte button definitions at 208E:0000: a label message, a box, a key word and flag bits
   (ovl1:1153 builds one node from each; ovl1:1551 draws it) */
struct ButtonDef {
    signed char message;        /* +0: the button's label, a gStrings2003 string index, 0 for none */
    unsigned char x;            /* +1: box left, in 8-pixel columns */
    unsigned char y;            /* +2: box top */
    unsigned char w;            /* +3: box width, in 8-pixel columns */
    unsigned char h;            /* +4: box height */
    unsigned key;               /* +5: the first key word (node->key06) */
    unsigned flags;             /* +7: bit 0 fills the box, bit 2 centres the label (ovl1:1551) */
};

/* the seven five-byte menu records at ds:628C: a text window and the run of button definitions
   that make up one menu */
struct Menu {
    signed char message;        /* +0: the message index in gStrings2003 */
    signed char window;         /* +1: the text window, 0 for none */
    signed char first;          /* +2: the first gButtonDefs index */
    signed char count;          /* +3: how many entries */
    signed char colour;         /* +4: the colour of the message */
};

struct Region {                 /* 16-byte records at ds:009F (src/E1DE7.C) */
    int left;
    int top;
    int width;
    int height;
    int attr1;
    int attr2;                  /* +0A: the fill colour */
    int column;
    int row;
};

extern struct Region gRegions[]; /*@ ds:009F */
extern struct Menu gMenus[]; /*@ ds:628C */
extern struct ButtonDef far gButtonDefs[]; /*@ 208E:0000 */
struct ActRec;
struct CharRec {                /* the 243-byte character records at ds:F580 (src/E0A72.C) */
    char pad00[0x37];
    signed char known[30];      /* +37: the mage's spell slots: a spell number, negative while it is
                                   still to be memorised (src/EOV6.C) */
    signed char memorized[30];  /* +55: the cleric's spell slots, the same way */
    char pad73[0xE7 - 0x73];
    unsigned char lastDamage;   /* +E7: the last damage amount (src/E0DE8.C) */
    unsigned char lastAction[2]; /* +E8: the last action results (src/E0DE8.C) */
    char padEA[0xF3 - 0xEA];
};
extern struct CharRec gChars[]; /*@ ds:F580 */
extern char far gStrings2003; /*@ 2003:0000 */
extern unsigned char far gSheet2067[]; /*@ 2067:0000 */
extern unsigned char far gSheetDECORATE[]; /*@ 2069:0000 */
extern char gOverlayBuf[]; /*@ ds:113F */

struct Box {
    int x1;
    int y1;
    int x2;
    int y2;
};
extern struct Box gMemberBoxes[]; /*@ ds:62E2 */
extern void far *gBufDCFC; /*@ ds:DCFC */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int colour); /*@ ds:806A */
extern void SwapBlocks(char far *pa, char far *pb, char far *pc, int len); /*@ 082C:07C5 */
extern void f0D1C_035A(int who); /*@ 0D1C:035A */
extern void SelectCharacter(int who); /*@ 0D1C:003F */
extern void BuildList1(void); /*@ 0CF7:0028 */
extern void RearmCharEffects(void); /*@ 1414:0B2E */
extern char gF542[]; /*@ ds:F542 */
extern char gFB44; /*@ ds:FB44 */
extern char gFlag; /*@ ds:F3B9 */
extern int gFlags849E; /*@ ds:849E */
extern void far *gFont1A47; /*@ ds:FE4E */
extern void far *gFont8x8; /*@ ds:FE5A */
extern void far *gA9C0; /*@ ds:A9C0 */
extern void far *gSegDF4E; /*@ ds:DF4E */
extern int gTextRegion; /*@ ds:811F */
extern int gFD98; /*@ ds:FD98 */
extern char far *gMageSpellNames[]; /*@ ds:1278 */
extern char far *gClericSpellNames[]; /*@ ds:12E0 */
extern char gFDEA[]; /*@ ds:FDEA */
extern char gFileNameBuf[]; /*@ ds:FE6E */
extern char gFDBA[]; /*@ ds:FDBA */   /* nine 9-byte lists, one per spell level: the first byte counts the
                                          spell numbers that follow (BuildSpellSlots) */
extern char gFDE7[]; /*@ ds:FDE7 */   /* ten bytes: the spells taken at each level, index level - 1 (the
                                          screen shows allowance - this) */
extern char gFDEC[]; /*@ ds:FDEC */   /* 32 pairs of counters per spell number - 1: the copies held in all and
                                          the copies ready (SpendSpellSlot) */
extern char gFDEB[]; /*@ ds:FDEB */   /* the odd bytes of the pairs in gFDEC, indexed by 2 * spell number */
extern char gFDE6[]; /*@ ds:FDE6 */   /* copies held per spell level, index level (1 based) */
extern char gFD9A[]; /*@ ds:FD9A */   /* the level (1 to 5) of spell number i + 1, copied from gMap62AF or gMap62C9 */
extern char gMap62AF[]; /*@ ds:62AF */ /* the spell level of each mage spell, 0 terminated */
extern char gMap62C9[]; /*@ ds:62C9 */ /* the spell level of each cleric spell, 0 terminated */
extern void far *memset(void far *dst, int c, unsigned n); /*@ 0000:6384 */
extern char *strcpy(char far *dest, char far *src); /*@ 0000:67CD */
extern int gNeedsRest; /*@ ds:FE2C */
extern int g159; /*@ ds:0159 */ /* a copy of gF57E that ShowMessage and ovl6:0B9B store; no reader found */
extern int gNeedsRest; /*@ ds:FE2C */
extern int gTunesOn; /*@ ds:80E8 */
extern int gSoundOn; /*@ ds:80E6 */
extern char gBarGraphsOn; /*@ ds:F43C */
extern int gMouseDisabled; /*@ ds:814A */
extern int gMouseHandlerInstalled; /*@ ds:8148 */
extern int gGraphicsChoice; /*@ ds:8102 */
extern int gCursorX; /*@ ds:8138 */
extern int gCursorY; /*@ ds:813A */
extern char gKeyRemapFrom[]; /*@ ds:6457 */
extern char gKeyRemapTo[]; /*@ ds:6459 */
extern void (far *gDrawText)(char far *text, int x, int y, int colour, int extra); /*@ ds:80CA */
extern void (far *gSaveRect)(int x, int y, int w, int h, void far *buf); /*@ ds:80AE */
extern void (far *gFn807E)(int x, int y, int w, int h, void far *buf); /*@ ds:807E */
extern struct Node gNodeTemplate; /*@ ds:3A62 */
extern signed char gF57E; /*@ ds:F57E */
extern int gStatusLine; /*@ ds:8115 */
extern int gTextHeight; /*@ ds:FE60 */
extern int gTextWidth; /*@ ds:FE62 */
extern int gTextLeft; /*@ ds:FE64 */
extern int gTextTop; /*@ ds:FE66 */
extern char g5E20; /*@ ds:5E20 */

extern void SetTextWindowAndClear(int region); /*@ 1DE7:00B6 */
extern void f0D1C_0C21(int x, int y, int width, int height, int c, int d); /*@ 0D1C:0C21 */
extern char far *SkipStrings(char far *text, int count); /*@ 0D1C:07A7 */
extern void DrawShadowText(char far *text, int x, int y, int colour, int colour2); /*@ 082C:0898 */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void movmem(void far *src, void far *dst, unsigned n); /*@ 0000:63A3 */
extern struct Node far *ListAdd(struct Node far *head, struct Node far *item); /*@ 1658:001E */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern void farfree(void far *block); /*@ 0000:71EE */
extern int strlen(char far *s); /*@ 0000:67F6 */
extern void (far *gFn804A)(int x, int y, int w, int h, void far *buf); /*@ ds:804A */
extern int WaitGameKey(void); /*@ 1AAD:053A */
extern int ReadKey(void); /*@ 1AAD:038E */
extern int Action01E3(char far *text); /*@ 0ACD:01E3 */
extern int RedrawPartyPanel(struct ActRec far *rec); /*@ 0EF3:2541 */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern void RedrawMember(int who); /*@ 1547:0991 */
extern void f1A47_0169(void far *font); /*@ 1A47:0169 */
extern int heapcheck(void); /*@ 0000:74B0 */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */
extern int DispatchKeyWait(struct Node far *head); /*@ 16A5:005A */
extern struct Node far *FindNode(struct Node far *head, unsigned key); /*@ 1658:01A2 */
extern void WaitFrames(int n); /*@ 1A16:000B */
extern void LoadScreenImage(char far *name, char far *extra, int srcKind, int dstKind, int mode); /*@ 082C:051B */
extern void RedrawCharacters(void); /*@ 0D1C:0583 */
extern int FileOpen(char far *name, int mode); /*@ 1C0E:00DB */
extern void FileClose(int slot); /*@ 1C0E:03BB */
extern void ShutdownGame(void); /*@ 07C8:058A */
extern void SortSpellSlots(int who); /*@ 0EF3:000E */
extern int IsPointInRect(int x, int y, int left, int top, int right, int bottom); /*@ 14EF:00E1 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern int CampLoop(void); /*@ 212A:0039 */
extern int SaveGame(void); /*@ 210C:0089 */
extern int LoadGame(void); /*@ 210C:0039 */
extern void DrawViewFrame(int full); /*@ 2116:005C */

void DrawListNode(struct Node far *node, int panelFirst, int highlight, int panelSecond);
int ChooseListEntry(int message);

/* ovl1:0000: drops party member `who` (0 to 5) from the party: the "Drop Character" button of the camp
   menu (RunCampMenu passes the member MemberCommand(0x35) picked, "Select the character you wish to
   drop."). Nothing happens when `who` is negative or the slot holds no character (0D1C:0670 flag 1).
   0D1C:035A puts the member's items on the floor of the party's cell and clears its status bits.
   For a member of the four marching slots (below 4) the record of the reserve slot 5 (when it is
   occupied, else slot 4) is swapped into the vacated place through gBufDCFC and the member is
   redrawn; the reserve slot (`who` itself when it was 4 or 5) then has its portrait box in
   gMemberBoxes painted in colour 0xC. Finally character 0 is selected, the list rebuilt (0CF7:0028)
   and the character effects rearmed. */
/*@ ovl1:0000 */ void DropCharacter(int who)
{
    int slot;

    if (who < 0)
        return;
    if (!f0D1C_0670(who, 1))
        return;
    f0D1C_035A(who);
    slot = who;
    if (who < 4) {
        if (f0D1C_0670(5, 1))
            slot = 5;
        else
            slot = 4;
        SwapBlocks((char *)&gChars[slot], (char *)&gChars[who], gBufDCFC, 0xf3);
        RedrawMember(who);
    }
    slot -= 4;
    LockHidePointer();
    gFillRect(gMemberBoxes[slot].x1, gMemberBoxes[slot].y1, gMemberBoxes[slot].x2,
        gMemberBoxes[slot].y2, 0xc);
    LockShowPointer();
    SelectCharacter(0);
    BuildList1();
    RearmCharEffects();
}
/* ovl1:00DD: builds the spell tables of the spell-slot screen for character `who` (nothing happens
   for a negative `who`). `cleric` is 1 for the cleric and paladin screen (SpellSlotScreen's flag),
   0 for the mage screen; `counts` is the screen's five per-level spell allowances, `mask` the set of
   spells that may be shown (bit i is spell i + 1). It clears gFDBA (the nine 9-byte level lists), gFDE7
   (spells already taken per level), gFDEC (32 pairs of counters) and gFD9A, copies the spell-to-level
   map into gFD9A (gMap62C9 for clerics, gMap62AF for mages) and walks the 30 signed bytes of the
   character's spell slots (rec + 0x55 for a cleric, rec + 0x37 for a mage): a nonzero entry c names
   spell |c|; a positive one bumps its second counter gFDEB[2 * c] and each bumps its first,
   gFDEC[2 * (|c| - 1)], and the per-level total gFDE6[level]. Then each spell i of `mask` is appended
   to its level's list gFDBA[(level - 1) * 9] (the first byte counts the entries, the entry is i + 1),
   for a cleric only when the level has a nonzero allowance. Called by SpellSlotScreen. */
/*@ ovl1:00DD */ void BuildSpellSlots(int who, char far *counts, long mask, int cleric)
{
    int i;
    struct CharRec far *rec;
    char far *slots;
    int entry;
    int slot;

    if (who < 0)
        return;
    rec = &gChars[who];
    memset(gFDBA, 0, 0x2d);
    memset(gFDE7, 0, 0xa);
    memset(gFDEC, 0, 0x40);
    memset(gFD9A, 0, 0x20);
    if (cleric)
        strcpy(gFD9A, gMap62C9);
    else
        strcpy(gFD9A, gMap62AF);
    slots = rec->known;
    if (cleric)
        slots = rec->memorized;
    for (i = 0; i < 0x1e; i++) {
        entry = *slots++;
        if (entry) {
            if (entry > 0)
                gFDEB[entry * 2]++;
            else
                entry = -entry;
            entry--;
            gFDEC[entry * 2]++;
            gFDE6[gFD9A[entry]]++;
        }
    }
    for (i = 0; i < 0x20; i++) {
        if (mask & (1L << i)) {
            slot = gFD9A[i] - 1;
            if ((cleric && counts[slot]) || !cleric) {
                if (slot >= 0) {
                    gFDBA[slot * 9]++;
                    gFDBA[slot * 9 + gFDBA[slot * 9]] = i + 1;
                }
            }
        }
    }
}
/* ovl1:025D: writes the spells chosen on the spell-slot screen back into character `who`'s spell
   slots (nothing happens for a negative `who`); `cleric` picks the 30 signed bytes at rec + 0x55
   (cleric), else rec + 0x37 (mage). The slots are cleared first. gFDEC holds, for each of the 32
   spells i, a pair of counters: the first is how many copies the character holds in all, the second
   how many of them are ready (positive). The second counter is cut down to the first when it is the
   larger. While it is nonzero the spell is stored as i + 1 in the first free one of the six slots of
   its level (gFD9A[i] - 1, six slots per level) and both counters drop; the spell is then looked
   at again (the loop index steps back), so every copy gets a slot. A spell with only the first
   counter left is stored as -(i + 1) instead, a copy that is spent until the party rests, and
   gNeedsRest is set. Called by SpellSlotScreen when the screen is left. QUIRK ovl1:0401: the index
   steps back after every copy, also when the level has no free slot, so such a spell is visited again
   until its counters run out. Purpose unknown: the exact meaning of the counters. */
/*@ ovl1:025D */ void SpendSpellSlot(int who, int cleric)
{
    int i;
    struct CharRec far *rec;
    int slot;
    int k;

    if (who < 0)
        return;
    rec = &gChars[who];
    if (cleric)
        memset(rec->memorized, 0, 0x1e);
    else
        memset(rec->known, 0, 0x1e);
    for (i = 0; i < 0x20; i++) {
        if (gFDEC[i * 2] < gFDEC[i * 2 + 1])
            gFDEC[i * 2 + 1] = gFDEC[i * 2];
        if (gFDEC[i * 2 + 1] != 0) {
            gFDEC[i * 2 + 1]--;
            gFDEC[i * 2]--;
            slot = gFD9A[i] - 1;
            for (k = 0; k < 6; k++) {
                if (cleric) {
                    if (rec->memorized[slot * 6 + k] == 0) {
                        rec->memorized[slot * 6 + k] = i + 1;
                        break;
                    }
                } else {
                    if (rec->known[slot * 6 + k] == 0) {
                        rec->known[slot * 6 + k] = i + 1;
                        break;
                    }
                }
            }
            i--;
        } else if (gFDEC[i * 2] != 0) {
            gFDEC[i * 2]--;
            slot = gFD9A[i] - 1;
            gNeedsRest = 1;
            for (k = 0; k < 6; k++) {
                if (cleric) {
                    if (rec->memorized[slot * 6 + k] == 0) {
                        rec->memorized[slot * 6 + k] = -(i + 1);
                        break;
                    }
                } else {
                    if (rec->known[slot * 6 + k] == 0) {
                        rec->known[slot * 6 + k] = -(i + 1);
                        break;
                    }
                }
            }
            i--;
        }
    }
}
/*@ ovl1:0416 stub */ int MemberCommand(int key) { return 0; }
/* ovl1:0881: draws one row of the spell-slot screen: the name and the count of spell `spell` on row
   `row` (a negative row draws nothing). The row sits at y = row * 9 + 0x32, x = 8. For a nonzero
   `spell` it formats "%-18s %1d" into gFileNameBuf from the spell's name (gClericSpellNames when
   `cleric` is set, else gMageSpellNames; both are far name pointers indexed by the spell number) and
   its count, the byte gFDEA[spell * 2]; `noShadow` picks the plain text routine gDrawText, else
   DrawShadowText with gF57E as the shadow; the colour is 6 when `highlight` is set, else 1 (0xF when
   gGraphicsChoice is nonzero). For spell 0 with `noShadow` clear it blanks the row instead by
   filling x 6 to 0xA8, y to y + 8 with gF57E through gFillRect. The pointer is hidden while it
   draws. Called by SpellSlotScreen. */
/*@ ovl1:0881 */ void DrawSpellRow(int spell, int row, int cleric, int noShadow, int highlight)
{
    int y;
    int colour;

    colour = gGraphicsChoice == 0 ? 1 : 0xf;
    if (row < 0)
        return;
    y = row * 9 + 0x32;
    LockHidePointer();
    if (spell != 0) {
        sprintf(gFileNameBuf, "%-18s %1d",
            cleric ? gClericSpellNames[spell] : gMageSpellNames[spell], gFDEA[spell * 2]);
        if (noShadow)
            gDrawText(gFileNameBuf, 8, y, highlight ? 6 : colour, 0);
        else
            DrawShadowText(gFileNameBuf, 8, y, highlight ? 6 : colour, gF57E);
    } else if (!noShadow)
        gFillRect(6, y, 0xa8, y + 8, gF57E);
    LockShowPointer();
}
/*@ ovl1:097A stub */ void SpellSlotScreen(int member, int flag) {}
/* ovl1:10DE: returns the word that labels an option state: gStrings2003 message 0x15 "ON" when `on`
   is nonzero, else 0x16 "OFF". RunCampMenu formats it into its "Tunes are %-3s" style labels. */
/*@ ovl1:10DE */ char far *BoolString(int on)
{
    on = on ? 0x15 : 0x16;
    return SkipStrings(&gStrings2003, on);
}
/* ovl1:1107: saves the screen rectangle x 0, y 0x78, 0x16 by 0x18 (the "option field", the strip the
   camp menu is drawn over) into the far buffer at gSegDF4E + 0x4E20 through gSaveRect. Called by
   RunCampMenu before it draws a menu and after a loaded game's screens are replaced. */
/*@ ovl1:1107 */ void SaveOptionField(void)
{
    gSaveRect(0, 0x78, 0x16, 0x18, (char far *)gSegDF4E + 0x4e20);
}
/* ovl1:112D: draws the rectangle SaveOptionField saved back to the screen through gFn804A (the
   blit routine that puts the buffer at gSegDF4E + 0x4E20 at x 0, y 0x78, 0x16 by 0x18). */
/*@ ovl1:112D */ void DrawOptionField(void)
{
    gFn804A(0, 0x78, 0x16, 0x18, (char far *)gSegDF4E + 0x4e20);
}

/* ovl1:1153: builds the button list of menu `menu` (an index into the seven five-byte records at
   ds:628C) and returns its head, with the pointer hidden while the window is drawn. When the
   record names a text window (+1) it sets that window's fill colour gRegions[window].attr2 to gF57E,
   selects and clears the window and fills its rectangle with a bevelled panel (0D1C:0C21), then
   draws the record's header message (gStrings2003 string +0) at (5, 5) in the record's colour. For
   each of its `count` (+3) entries it allocates a 60-byte node, copies gNodeTemplate over it and
   fills it from the 9-byte definition gButtonDefs[first + i] (+2): key = entry number + 1, the two key
   words (the definition's key and key + 0x100), the box (x and width in 8-pixel columns, so << 3),
   flags 0x1100, the definition pointer at +0x34 and its flag word at +0x38. Each node is drawn
   once through DrawListNode (ovl1:1551) and chained with ListAdd. At the end it clips the window to
   the screen (19F3:000E with its own edges), sets g5E20 (the key-list walk then rebuilds its screen,
   src/E16A5.C), flushes the key buffer and clears gStatusLine again. Called by ovl1:0416 (menu 3),
   097A (menu 4), 1ACD (menus 1 and 6, and the menu `di` it chooses) and 243A (menu 5). The menu
   records are edited in place by ovl1:1ACD before the call (a record's header message). */
/*@ ovl1:1153 */ struct Node far *BuildMenuButtons(int menu)
{
    int i;
    struct Node far *list;
    struct Node far *node;
    int index;

    gStatusLine = 2;
    if (gMenus[menu].window) {
        gRegions[gMenus[menu].window].attr2 = gF57E;
        SetTextWindowAndClear(gMenus[menu].window);
        f0D1C_0C21(gTextLeft << 3, gTextTop, gTextWidth << 3, gTextHeight, 0, 0);
    }
    DrawShadowText(SkipStrings(&gStrings2003, gMenus[menu].message), 5, 5, gMenus[menu].colour, 0);
    list = 0;
    for (i = 0; i < gMenus[menu].count; i++) {
        index = i + gMenus[menu].first;
        node = MemAlloc(0x3cL, 0);
        movmem(&gNodeTemplate, node, 0x3c);
        node->key = index + 1;
        node->key06 = gButtonDefs[index].key;
        node->key08 = gButtonDefs[index].key + 0x100;
        node->x = gButtonDefs[index].x << 3;
        node->y = gButtonDefs[index].y;
        node->width = gButtonDefs[index].w << 3;
        node->height = gButtonDefs[index].h;
        node->def = &gButtonDefs[index];
        node->flags = 0x1100;
        node->defFlags = gButtonDefs[index].flags;
        DrawListNode(node, 0, 0, 0);
        if (list)
            list = ListAdd(list, node);
        else
            list = node;
    }
    LockHidePointer();
    ClipRect(gTextLeft, gTextTop, gTextLeft, gTextTop, gTextWidth, gTextHeight, 2, 0);
    LockShowPointer();
    g5E20 = 1;
    FlushKeyBuffer();
    gStatusLine = 0;
    return list;
}

/* ovl1:13F7: shows the gStrings2003 message `message` in a framed box (text window 11) and waits for
   a key or at most 0x78 frames. It hides the pointer, selects the 8x8 font gFont8x8, copies the
   shadow colour gF57E into g159, sets gStatusLine to 2 while it draws and fills and clips the window
   rectangle; the text goes at (left * 8 + 5, top + 5) in colour 0xF. It then polls ReadKey one frame
   at a time (WaitFrames(1)); a key press flushes the key buffer and ends the wait. Afterwards the
   status line, font and text window are restored and the key buffer flushed again. Called by
   RunCampMenu, MemberCommand and DrawListNode. */
/*@ ovl1:13F7 */ void ShowMessage(int message)
{
    int savedWindow;
    void far *savedFont;
    int savedStatus;
    int i;

    savedStatus = gStatusLine;
    savedFont = gFont1A47;
    savedWindow = gTextRegion;
    f1A47_0169(gFont8x8);
    LockHidePointer();
    g159 = gF57E;
    SetTextWindowAndClear(0xb);
    gStatusLine = 2;
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
    gStatusLine = savedStatus;
    f1A47_0169(savedFont);
    SetTextWindow(savedWindow);
    FlushKeyBuffer();
}
/* ovl1:1516: frees every node of a far linked list: each node's first four bytes are the far
   pointer to the next one, and the walk ends at a null pointer. Called by RunCampMenu,
   ChooseListEntry and the other screens with the head BuildMenuButtons returned. */
/*@ ovl1:1516 */ void FreeFarList(struct Node far *list)
{
    struct Node far *next;

    while (list) {
        next = list->next;
        farfree(list);
        list = next;
    }
}
/* ovl1:1551: draws the list node `node` (one button of a camp menu, built by BuildMenuButtons) in
   the text window; a null node draws nothing. With the node's definition `def` (node->def): when
   def->flags bit 0 is set the node's box is filled with the bevelled panel 0D1C:0C21 (panelFirst and
   panelSecond are its two mode arguments); then, when def->message is nonzero, the label
   gStrings2003[def->message] is drawn at the box origin plus (4, 3), or centred in the box when def
   flags bit 2 is set (x = (width - 8 * length) / 2 + 1, y = (height - 7) / 2). The label's colour is
   6 when `highlight` is nonzero, else 1 (0xF when gGraphicsChoice is nonzero). It goes through the
   text routine gDrawText when panelFirst or panelSecond is set (the panel already holds a
   background), else through DrawShadowText. The pointer is hidden while it draws. Called by
   BuildMenuButtons, MemberCommand, RunCampMenu and ChooseListEntry: (node, 0, 0, 1) un-highlights,
   (node, 0, 1, 1) highlights and (node, 1, 1, 1) draws the pressed button. */
/*@ ovl1:1551 */ void DrawListNode(struct Node far *node, int panelFirst, int highlight, int panelSecond)
{
    struct ButtonDef far *def;
    char far *text;
    int colour;
    int x;
    int y;

    if (node == 0)
        return;
    colour = gGraphicsChoice == 0 ? 1 : 0xf;
    def = node->def;
    LockHidePointer();
    if (def->flags & 1)
        f0D1C_0C21(node->x, node->y, node->width, node->height, panelFirst, panelSecond);
    if (def->message != 0) {
        text = SkipStrings(&gStrings2003, def->message);
        if (def->flags & 4) {
            x = (node->width - strlen(text) * 8) / 2 + 1;
            y = (node->height - 7) / 2;
        } else {
            x = 4;
            y = 3;
        }
        if (panelSecond || panelFirst)
            gDrawText(text, node->x + x, node->y + y, highlight ? 6 : colour, 0);
        else
            DrawShadowText(text, node->x + x, node->y + y, highlight ? 6 : colour, 0);
    }
    LockShowPointer();
}
/*@ ovl1:1691 stub */ void Ovl1_1691(void) {}
/* ovl1:1ACD: the camp menu, entered from the camp button on the game screen: it is the callback30 of
   the screen node at ds:3B8E (the far pointer 2102:0020 sits at ds:3BBE; the argument is that
   node, which the function overwrites and uses as a local). It saves the font and text window, shows
   the Camp menu (menu 0, header "Camp:") and then loops: it rebuilds the button list of `nextMenu`
   through BuildMenuButtons when a button asks for another menu (0 Camp, 1 Game Options, 2
   Preferences; -1 keeps the current one), waits for a key or a click on the list (DispatchKeyWait),
   maps the key 0x6E to the Exit button, turns the confirm keys 0x2B, 0x3D and 0x61 into a click on the
   highlighted node, flashes the clicked node, and switches on the button (0x8000 + its entry
   number, see gMenus and gButtonDefs):
   Rest Party (CampLoop; a nonzero result leaves the camp, otherwise the party has rested and
   gNeedsRest is cleared), Memorize Spells and Pray for Spells (MemberCommand picks the caster, then
   SpellSlotScreen), Scribe Scrolls (ovl1:1691), Preferences and Game Options (open the menu), Exit
   (leaves the camp from menu 0, goes back to menu 0 from the others; "Your party needs to rest to gain
   spells." first when gNeedsRest is set), Load Game and Save Game (after a yes/no box through
   ChooseListEntry), Drop Character (needs more than four members), Quit Game, the four
   Preferences toggles (tunes, sounds, bar graphs, mouse; each rewrites its "... are ON/OFF" label in
   gStrings2003) and the cursor keys: 0x5B and 0x65 light the first button; 0x60 and 0x62 step the
   highlight to the previous or next selectable button (definition flag bit 1), wrapping; 0x5D and
   0x67 light the last button and, when a button is already lit, go on as 0x60 (previous). Any
   other key looks for a selectable button under the mouse. The loop ends when `looping` is cleared
   (Exit, a rest that was cut short, a loaded game). It checks the far heap on every pass ("Far heap
   corrupt!"). On the way out it redraws the option field, the view frame, text window 10 and, when a
   sub-screen replaced it (`reloadInvent`), the INVENT sheet. Always returns 1 (callback30: the key
   list walk stops).
   QUIRK ovl1:20FF: the Exit test `key != 0x6E` is dead: 0x6E was remapped to 0x8007 above. */
/*@ ovl1:1ACD */ int RunCampMenu(struct Node far *node)
{
    void far *savedFont;
    int savedWindow;
    int looping;
    int key;
    struct Node far *list;
    struct Node far *lit;
    struct Node far *hot;
    int curMenu;
    int reloadInvent;
    int redrawParty;
    int i;
    int nextMenu;

    if (gFB44)
        Action01E3(gF542);
    if (gFlag)
        RedrawPartyPanel((struct ActRec far *)gF542);
    gFlags849E &= ~2;
    savedFont = gFont1A47;
    savedWindow = gTextRegion;
    for (i = 0; i < 6; i++) {
        if (f0D1C_0670(i, 1)) {
            gChars[i].lastDamage = 0;
            gChars[i].lastAction[0] = 0;
            gChars[i].lastAction[1] = 0;
            RedrawMember(i);
        }
    }
    LockHidePointer();
    SaveOptionField();
    LockShowPointer();
    f1A47_0169(gFont8x8);
    reloadInvent = 0;
    gFD98 = 0;
    redrawParty = 0;
    curMenu = -1;
    nextMenu = MENU_CAMP;
    list = 0;
    lit = 0;
    looping = 1;
    do {
        if (heapcheck() != 2) {
            gDrawText("Far heap corrupt!", 0, 0, 0xf, 0);
            FlushKeyBuffer();
            WaitGameKey();
        }
        if (nextMenu == MENU_PREFERENCES) {
            sprintf(SkipStrings(&gStrings2003, 0x11), "Tunes are %-3s", BoolString(gTunesOn));
            sprintf(SkipStrings(&gStrings2003, 0x12), "Sounds are %-3s", BoolString(gSoundOn));
            sprintf(SkipStrings(&gStrings2003, 0x13), "Bar Graphs are %-3s", BoolString(gBarGraphsOn));
            sprintf(SkipStrings(&gStrings2003, 0x14), "Mouse is %-3s", BoolString(!gMouseDisabled));
        }
        if (nextMenu != -1) {
            FreeFarList(list);
            gMenus[MENU_CAMP].message = MSG_SELECT_OPTION;
            if (nextMenu == MENU_CAMP)
                gMenus[MENU_CAMP].message = MSG_CAMP;
            if (nextMenu == MENU_PREFERENCES)
                gMenus[MENU_PREFERENCES].message = MSG_PREFERENCES;
            if (nextMenu == MENU_GAME_OPTIONS)
                gMenus[MENU_GAME_OPTIONS].message = MSG_GAME_OPTIONS;
            list = BuildMenuButtons(nextMenu);
            if (nextMenu != curMenu) {
                hot = list;
                lit = 0;
            }
            curMenu = nextMenu;
            nextMenu = -1;
        }
        key = DispatchKeyWait(list);
        key &= 0x80ff;
        for (i = 0; gKeyRemapFrom[i]; i++) {
            if (gKeyRemapFrom[i] == key) {
                key = gKeyRemapTo[i] + BTN_REST;
                break;
            }
        }
        if (key == KEY_CONFIRM_A || key == KEY_CONFIRM_B || key == KEY_CONFIRM_C)
            key = lit->key + 0x8000;     /* QUIRK ovl1:1D48: no test of lit, which is null after a click */
        node = FindNode(list, key & 0x7fff);
        if (node) {
            DrawListNode(lit, 0, 0, 1);
            DrawListNode(node, 1, 1, 1);
            WaitFrames(5);
            DrawListNode(node, 0, 1, 1);
            lit = 0;
            hot = node;
        }
        switch (key) {
        case BTN_SCRIBE:
            Ovl1_1691();
            nextMenu = MENU_CAMP;
            break;
        case BTN_DROP: {
            int members;

            members = 0;
            for (i = 0; i < 6; i++) {
                if (f0D1C_0670(i, 1))
                    members++;
            }
            if (members < 5)
                ShowMessage(0x2d);       /* "You cannot have less than four characters." */
            else {
                DropCharacter(MemberCommand(0x35));
                LockHidePointer();
                LoadScreenImage("PLAYFLD", gOverlayBuf, 5, 3, 0);
                reloadInvent = 1;
                LoadScreenImage("INVENT", gOverlayBuf, 5, 3, 2);
                gSaveRect(0, 0x78, 0x16, 0x18, (char far *)gSegDF4E + 0x4e20);
                f1A47_0169(gA9C0);
                RedrawCharacters();
                f1A47_0169(gFont8x8);
                LockShowPointer();
            }
            nextMenu = MENU_CAMP;
        }
        break;                           /* outside the block, as the original's tail is not merged */
        case BTN_REST:
            if (CampLoop())
                looping = 0;
            else
                gNeedsRest = 0;
            redrawParty = 1;
            nextMenu = MENU_CAMP;
            break;
        case BTN_MEMORIZE:
            SpellSlotScreen(MemberCommand(0x17), 0);
            nextMenu = MENU_CAMP;
            break;
        case BTN_PRAY:
            SpellSlotScreen(MemberCommand(0x1a), 1);
            nextMenu = MENU_CAMP;
            break;
        case BTN_PREFERENCES:
            nextMenu = MENU_PREFERENCES;
            break;
        case BTN_TUNES:
            gTunesOn = gTunesOn == 0;
            nextMenu = MENU_PREFERENCES;
            break;
        case BTN_SOUNDS:
            gSoundOn = gSoundOn == 0;
            nextMenu = MENU_PREFERENCES;
            break;
        case BTN_BAR_GRAPHS:
            gBarGraphsOn = gBarGraphsOn == 0;
            nextMenu = MENU_PREFERENCES;
            redrawParty = 1;
            break;
        case BTN_MOUSE:
            if (gMouseHandlerInstalled) {
                LockHidePointer();
                gMouseDisabled = gMouseDisabled == 0;
                nextMenu = MENU_PREFERENCES;
                redrawParty = 1;
                LockShowPointer();
            }
            break;
        case BTN_LOAD:
            if (ChooseListEntry(0x37)) {
                reloadInvent = 1;
                LockHidePointer();
                DrawOptionField();
                f1A47_0169(gA9C0);
                DrawViewFrame(1);
                if (LoadGame() == 0) {
                    f1A47_0169(gFont8x8);
                    SaveOptionField();
                    FreeFarList(list);
                    hot = list = BuildMenuButtons(MENU_GAME_OPTIONS);
                    LockShowPointer();
                    ShowMessage(0x10);
                    LockHidePointer();
                    nextMenu = MENU_GAME_OPTIONS;
                } else {
                    SaveOptionField();
                    looping = 0;
                }
                LockShowPointer();
                f1A47_0169(gFont8x8);
            } else
                nextMenu = MENU_GAME_OPTIONS;
            break;
        case BTN_SAVE:
            if (ChooseListEntry(0x36)) {
                int slot;

                slot = FileOpen("EOBDATA.SAV", 1);
                FileClose(slot);
                if (SaveGame())
                    ShowMessage(0xe);
                else
                    ShowMessage(0xf);
                if (gGraphicsChoice == 1) {
                    LoadScreenImage("PLAYFLD", (char far *)gSheetDECORATE, 5, 3, 2);
                    gStatusLine = 2;
                    gFn807E(0, 0x78, 0x16, 0x18, (char far *)gSegDF4E + 0x4e20);
                    gStatusLine = 0;
                    reloadInvent = 1;
                }
            }
            nextMenu = MENU_GAME_OPTIONS;
            break;
        case BTN_EXIT:
            if (gNeedsRest)
                ShowMessage(0x2c);
        case BTN_OPTIONS_EXIT:
        case BTN_PREFERENCES_EXIT:
            switch (curMenu) {
            case MENU_CAMP:
                if (key != 0x6e)
                    looping = 0;
                break;
            case MENU_GAME_OPTIONS:
            case MENU_PREFERENCES:
                nextMenu = MENU_CAMP;
                break;
            }
            break;
        case BTN_GAME_OPTIONS:
            nextMenu = MENU_GAME_OPTIONS;
            break;
        case BTN_QUIT:
            if (ChooseListEntry(0x2e))
                ShutdownGame();
            nextMenu = MENU_CAMP;
            break;
        case KEY_FIRST_A:
        case KEY_FIRST_B:
            hot = FindNode(list, gMenus[curMenu].first + 1);
            break;
        case KEY_LAST_A:
        case KEY_LAST_B:
            hot = FindNode(list, gMenus[curMenu].first + gMenus[curMenu].count);
            key = KEY_PREV;
        case KEY_PREV:
        case KEY_NEXT: {
            int step;
            int lo;
            int hi;

            if (lit == 0)
                break;
            step = 1;
            if (key == KEY_PREV)
                step = -1;
            i = lit->key + step;
            lo = gMenus[curMenu].first + 1;
            hi = lo + gMenus[curMenu].count - 1;
            for (;;) {
                if (i < lo)
                    i = hi;
                if (i > hi)
                    i = lo;
                if (gButtonDefs[i - 1].flags & 2)
                    break;
                i += step;
            }
            hot = FindNode(list, i);
        }
        break;
        default:
            if (!gMouseDisabled) {
                for (node = list; node; node = node->next) {
                    if ((node->defFlags & 2) &&
                        IsPointInRect(gCursorX, gCursorY, node->x, node->y, node->x + node->width,
                            node->y + node->height)) {
                        hot = node;
                        break;
                    }
                }
            }
            break;
        }
        if (gFD98 || redrawParty) {
            for (i = 0; i < 6; i++) {
                RedrawMember(i);
                SortSpellSlots(i);
            }
            gFD98 = 0;
            redrawParty = 0;
        }
        if (lit != hot && nextMenu == -1 && looping) {
            DrawListNode(lit, 0, 0, 1);
            DrawListNode(hot, 0, 1, 1);
            lit = hot;
        }
    } while (looping);
    FreeFarList(list);
    f1A47_0169(savedFont);
    gStatusLine = 2;
    DrawOptionField();
    SetTextWindow(savedWindow);
    DrawViewFrame(0);
    gStatusLine = 0;
    LockHidePointer();
    SetTextWindow(0xa);
    ClipRect(gTextLeft, gTextTop, gTextLeft, gTextTop, gTextWidth, gTextHeight, 2, 0);
    if (reloadInvent)
        LoadScreenImage("INVENT", (char far *)gSheet2067, 5, 3, 2);
    SetTextWindow(savedWindow);
    LockShowPointer();
    gStatusLine = 0;
    gFlags849E |= 2;
    return 1;
}

/*@ ovl1:243A stub */ int ChooseListEntry(int message) { return 0; }
