/* Unit at overlay 1 of EOB.EXE (segment 2102, 9864 bytes, 16 functions): the party display and
   inventory overlay (src/EOV1.ASM's header says what it does). The unit is C, compiled by TCC
   1.00 (escalation eov1, 2026-11-02): every function has TCC's frame, and ovl1:1153 reads a far
   array with the 16-bit displacement only TCC's own assembler writes (26 8B 87 05 00 at
   ovl1:1277), which TASM cannot. src/EOV1.ASM was written first, as assembly transcribed from
   the disassembly; it stays until this file has each function in C (playbook 30). Every
   function here is a stub until it is written in C. */

/*@flags -ml -O -Z -v */

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
    unsigned char message;      /* +0: the button's label, a gStrings2003 string index */
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
extern char far gStrings2003; /*@ 2003:0000 */
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

void DrawListNode(struct Node far *node, int mode1, int mode2, int mode3);

/*@ ovl1:0000 stub */ void RotateOutMember(void) {}
/*@ ovl1:00DD stub */ void BuildSpellSlots(void) {}
/*@ ovl1:025D stub */ void SpendSpellSlot(void) {}
/*@ ovl1:0416 stub */ void MemberCommand(void) {}
/*@ ovl1:0881 stub */ void ShowItemName(void) {}
/*@ ovl1:097A stub */ void SpellSlotScreen(void) {}
/*@ ovl1:10DE stub */ void BoolString(void) {}
/*@ ovl1:1107 stub */ void SaveOptionField(void) {}
/*@ ovl1:112D stub */ void DrawOptionField(void) {}

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

/*@ ovl1:13F7 stub */ void ShowMessage(void) {}
/*@ ovl1:1516 stub */ void FreeFarList(void) {}
/*@ ovl1:1551 stub */ void DrawListNode(struct Node far *node, int mode1, int mode2, int mode3) {}
/*@ ovl1:1691 stub */ void Ovl1_1691(void) {}
/*@ ovl1:1ACD stub */ void Ovl1_1ACD(void) {}
/*@ ovl1:243A stub */ void ChooseListEntry(void) {}
