/* Unit at EOB.EXE 0ACD:0008 (4182 bytes, 35 functions): the adventure-screen action handlers.
   Most entries are reached through a 60-byte DGROUP action record (struct Action) whose +0x38
   field is the party slot and whose +4 is the value the handler returns; they draw the block
   messages ("You can't go that way."), move and swap party members, move and pick items, eat,
   browse the item list, and move and turn the party. RunMapLoop (0ACD:0E8A) is the
   map timeout loop and this unit's only caller of TurnUndead. Names that are not established say
   what a function does. */

/*@flags -ml -O -Z -v */

/* A 60-byte action record reached through a far pointer. Only the two fields the handlers touch
   are named; everything else is padding. The 60 bytes are the DGROUP record grid pitch
   (src/E0CF7.C:9); the named fields end at +0x3A (58 bytes), so the real size is not proven. */
struct Action {
    char pad00[4];
    int result;             /* +4: the value the handler returns to its caller */
    char pad06[0x38 - 6];
    int slot;               /* +0x38: the party slot the action applies to */
};

extern signed char gInventoryMember; /*@ ds:A99A */    /* the selected character index (src/E0D1C.C) */
extern char gCursorSlot; /*@ ds:F51A */           /* the selected action row: 0..2 in the action menu
                                               (StepActionRow 0ACD:03CD wraps it) and 0x10 for the
                                               character's own item list (0ACD:0A61 and 0BA6; the same
                                               tests at src/E122E.C:471 and :485) (src/E0D1C.C) */
extern char gPanelMode; /*@ ds:FB44 */           /* a mode byte: 0, 1 and 2 gate the action handlers
                                               (src/E0D1C.C) */
extern int gMatchedNodeKey; /*@ ds:FC82 */            /* an action mode: 0, 1 and 2 pick the branches of the
                                               0034, 008C and 00EB handlers */
extern int gStatusLine; /*@ ds:8115 */            /* nonzero while a status line is up (src/E082C.C) */
extern void far *gSegDF4E; /*@ ds:DF4E */   /* the 19EC:0002(slot 5) far buffer (src/E07A9.C) */

extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern int MemberIs(int who, int mask); /*@ 0D1C:0670 */
extern void RedrawMember(int dir); /*@ 1547:0991 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void RedrawRowArea(int one); /*@ 0BD2:0ABE */
extern void UseSelectedItem(int slot); /*@ 0BD2:0BA0 */
extern void BuildList1(void); /*@ 0CF7:0028 */
extern void BuildList2(void); /*@ 0CF7:00B4 */
extern void BuildList3(void); /*@ 0CF7:0140 */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void (far *gFn804A)(int x, int y, int w, int h, void far *buf); /*@ ds:804A */ /* src/EOV2.C */

extern char gIconTable; /*@ ds:FB34 */           /* the index of the party member being moved, -1 when
                                               none; src/E1414.C:106 agrees ("the index of the party
                                               member being moved, -1 when none"). src/E0EF3.C:227
                                               declares the same address as void far *gIconTable[]
                                               (far sprite pointers). This unit reads the byte as a
                                               member index */
extern int gPartyDir; /*@ ds:FBCC */            /* the direction the party faces, 0..3 (kept
                                               modulo 4 by TurnLeft and TurnRight) */
extern int gPartyCell; /*@ ds:FBCA */     /* the party's map cell: MoveParty (122E:0001)
                                               sets it; 0ACD:0644/0A61 drop items into its
                                               gMazeItems list */
extern char gPickViewPos; /*@ ds:EFB2 */           /* the floor position last picked from: set to 0xff when
                                               the party turns (056C, 0593) and used by PickListItem (0BA6) as the
                                               page index 0..3 into gViewPosAhead and gViewToCellPos */
extern char gDCAA; /*@ ds:DCAA */           /* the menu/rest phase: 1 and 2 are the menu steps
                                               (0ACD:0753 wraps a value above 2 back to 1 and
                                               EnterMapScreen 0ACD:0F72 sets 1); 5 is the rest phase
                                               the handlers skip (0ACD:0A34, 0A61 and 0D9D). No writer
                                               of 5 was found in the program */
extern int gEFD2; /*@ ds:EFD2 */            /* set to 0xff by 08E7 */
extern int gPickFirstItem; /*@ ds:F3BA */            /* the item just picked, -1 when none (0BA6) */
extern int gViewPosAhead[]; /*@ ds:374A */          /* a per-page word flag read by 0BA6 */
extern char gF542[]; /*@ ds:F542 */         /* the action record passed to RedrawPartyPanel
                                               (0EF3:2541) when gFlag is set (0ACD:0264);
                                               RedrawPartyPanel takes it as struct ActRec far * */
extern char gViewDirty; /*@ ds:DF4C */           /* set to 1 when an action took effect (src/E0D1C.C) */
extern char gMaze[]; /*@ ds:B894 */         /* nine-byte object records (src/E0D1C.C) */
extern signed char gWallTypeFlags[]; /*@ ds:194B */  /* per-object bit flags (src/E0D1C.C) */
extern signed char gA9C6; /*@ ds:A9C6 */    /* an offset into a gMaze record (src/E0DE8.C) */

extern int StepCell(int cell, int dir); /*@ 0D1C:00DA */
extern int StepIfOpen(int cell, signed char dir); /*@ 0D1C:0080 */
extern int GetCurrentItem(void); /*@ 0D1C:02DC */
extern void SelectCharacter(int value); /*@ 0D1C:003F */
extern void NoOpAfterCursorMove(int first, int second); /*@ 0D1C:006D */
extern void NoOpBeforeCursorMove(int value); /*@ 0D1C:0078 */
extern void SwapSelectedItem(int slot, int flag); /*@ 0BD2:0C8E */
extern void UseItemSlot(int slot, int flag); /*@ 0BD2:1001 */
extern int AddDDEffect(int who, int item, int cell, int kind, int page, int type); /*@ 0BD2:07DE */
extern void ArmEffect(int a, int b); /*@ 1414:05A4 */
extern void RearmCharEffects(void); /*@ 1414:0B2E */
extern void SetMouseItem(int n); /*@ 07A9:010C */
extern void MoveParty(int cell); /*@ 122E:0001 */
extern void DispatchSlotHandler(int a, int b); /*@ 122E:0896 */
extern void ComputeArmourClass(int who); /*@ 0DE8:0D5E */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern void BuildList4(void); /*@ 0CF7:01CC */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
struct ActRec;      /* the action record 0EF3:2541 takes: the same +4 result word and +0x38 member,
                       a byte in ActRec (src/E0EF3.C:292) and a word here (:18) */
extern int RedrawPartyPanel(struct ActRec far *rec); /*@ 0EF3:2541 */

/* A character record: this unit touches the name, the hunger byte food and the m97 item sub-table
   (src/E0A72.C). */
struct CharRec {
    char pad00[2];
    char name[0x0B];        /* +2: the character's name, NUL padded (src/E0D1C.C) */
    char pad0D[0x23 - 0x0D];
    unsigned char food;     /* +0x23: a hunger/food value clamped to 100 by EatFood (07E0) */
    char pad24[0x97 - 0x24];
    char m97[0xF3 - 0x97];  /* +0x97: the character's own item sub-table; RemoveListItem
                               (0BD2:02B9) takes an item from it and InsertListItem
                               (0BD2:01D0) puts one back (0ACD:0A61 and 0BA6) */
};
extern struct CharRec gChars[]; /*@ ds:F580 */

/* A 14-byte item record (ITEM.DAT, gItems at ds:8DD0): type is the item type, key the record's key (-1 when free)
   and value the amount EatFood (07E0) adds to the character's food (the meaning is inferred; the
   src/E0D1C.C:275 comment says what type, value and key mean is not established). */
struct ItemRecord {
    signed char nameIndex;        /* +0 (ds:8DD0): indexes the item-name table at segment 0x1F28 */
    signed char identifiedNameIndex;        /* +1 (ds:8DD1): the second name index */
    unsigned char flags;    /* +2 (ds:8DD2): bit 0x20 gates 0BD2:0AF2/0C8E, 0x40 is set by
                               0BD2:040E and tested by 0BD2:05B5; src/E07A9.C calls it flags */
    signed char icon;       /* +3 (ds:8DD3): the icon number */
    signed char type;        /* +4 (ds:8DD4): the item type */
    signed char tag;        /* +5 (ds:8DD5): 01D0 stores its 4th argument here and 02B9 matches it
                               against its `key`. No caller shows a sound (01D0's callers pass a
                               slot kind, E0ACD.C:561-568, a roll, E08E6.C:807, or an effect kind | 4,
                               0BD2:07DE), so Purpose unknown: a per-entry tag */
    int key;                /* +6 (ds:8DD6): the key; -1 when the record is free */
    int prevLink;                /* +8 (ds:8DD8): one link of the circular list; 0BD2:01D0 splices a
                               new item in after this record through it */
    int nextLink;                /* +0xa (ds:8DDA): the other link; the list walks (0BD2:0385, 040E,
                               04AA) follow it */
    signed char m0A;        /* +0xc (ds:8DDC): gFBC8 or -1, set by 01D0 */
    signed char value;        /* +0xd (ds:8DDD): the item's bonus or effect value: the signed bonus
                               printed as "%s +%d" and "Cursed %s %d", the spell index into ds:1278
                               and ds:12E0, the wand effect index g38FA[m0B], and the scroll and
                               potion values (0BD2:05B5, 0D2D, 0EE8, 1001) */
};
extern struct ItemRecord gItems[]; /*@ ds:8DD0 */
extern signed char gViewToCellPos[]; /*@ ds:1A1B */  /* per-page sound table (src/E0D1C.C) */
extern int gHandSplitY[]; /*@ ds:36B0 */          /* a per-slot word (screen row) compared with gHitY */
extern int gHitY; /*@ ds:5E12 */            /* the y the hit test uses (the walk is 16A5:00BD, src/E16A5.C) */
extern unsigned g5E1C; /*@ ds:5E1C */       /* the left-button mode word: ClickHandSlot (0ACD:0415)
                                               takes its low seven bits, 1 selects SwapSelectedItem,
                                               anything else UseItemSlot (src/E1658.C); src/E16A5.C:506
                                               tests g5E1C for the left button, g5E1E for the right */
extern signed char gSlotNeighbours[]; /*@ ds:36D0 */  /* a per-row four-byte action table (09EF) */
extern signed char gFrontPosByDir[]; /*@ ds:373C */  /* a table with two entries per page: 0A61 reads
                                               gFrontPosByDir[gPartyDir * 2 + gMemberSide[gInventoryMember]] */
extern signed char gMemberSide[]; /*@ ds:3744 */  /* a per-character byte table read by 0A61 */
extern char gMazeItems[]; /*@ ds:B899 */         /* the nine-byte record list head (src/E0D1C.C) */
extern char gFlag; /*@ ds:F3B9 */           /* picks the "a" table set when nonzero (src/E0CF7.C);
                                               this unit uses it as a redraw gate in PickOrSwapMember
                                               (0ACD:0264) and clears it in EnterMapScreen
                                               (0ACD:0F72) */
extern char gA998; /*@ ds:A998 */           /* the saved-game load flag: 0xff while a load runs,
                                               0xfe after a saved game loads and 0 otherwise
                                               (src/EOV2.C); EnterMapScreen (0ACD:0F72) tests it */
extern int gMouseDisabled; /*@ ds:814A */   /* nonzero disables the mouse (src/E07A9.C) */
extern int gFBC8; /*@ ds:FBC8 */            /* the current action (src/E0D1C.C) */
extern int gFBCE; /*@ ds:FBCE */            /* the item index shown by the mouse (src/E07A9.C) */
extern void far *gFont6x6; /*@ ds:A9C0 */       /* the current 6x6 font (src/E07A9.C) */
extern void far *gList; /*@ ds:FB48 */      /* the head of the list built last (src/E0CF7.C) */
extern unsigned long gNextRandomSoundTick; /*@ ds:FC5A */  /* the map timeout counter (0E8A) */
extern int gUnreadPartyCellCopy; /*@ ds:FC58 */            /* the party's cell saved by the map loop (0E8A) */
extern unsigned long g8492; /*@ ds:8492 */  /* the 32-bit counter the 1Ch handler bumps (src/EOV6.C) */
extern int g8134; /*@ ds:8134 */            /* set to 0 by the map screen (src/E14EF.C) */
extern char gFileNameBuf[]; /*@ ds:FE6E */  /* the text buffer sprintf formats into (src/E082C.C) */
extern unsigned char far gSheetDECORATE[]; /*@ 2069:0000 */  /* the PLAYFLD sheet, loaded by
                                               EnterMapScreen (0ACD:0F72) (src/EOV3.C:27) */
extern unsigned char far gSheet2067[]; /*@ 2067:0000 */      /* the INVENT sheet, loaded by
                                               EnterMapScreen (0ACD:0F72) (src/EOV3.C:26) */

extern int CanPlaceItemInSlot(int a, int b, int c); /*@ 0BD2:0AF2 */
extern void PushListItem(int a, int b, int c); /*@ 0BD2:0458 */
extern void InsertListItem(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int RemoveListItem(void far *head, int key, int item); /*@ 0BD2:02B9 */
extern void DrawItemRow(int who, int slot, int one); /*@ 0BD2:0917 */
struct Node;    /* the screen list node 16A5:005A walks (src/E16A5.C) */
/* 16A5:005A is void in src/E16A5.C, but its last statement leaves DispatchNodeEvent's result in AX
   and this unit consumes it, so it is declared int here. DispatchNodeEvent (16A5:00BD) returns the
   key a node took, the key itself when no node takes it (`if (result == 0) result = key;`) or 0 for
   a mouse-only call (src/E16A5.C:185 to 188 and :523 to 524). */
extern int DispatchKeyWait(struct Node far *head); /*@ 16A5:005A */
extern void DrawMemberFood(int who); /*@ 1547:04A0 */
extern void RunCellScript(int cell, int mode); /*@ 122E:1DA8 */
extern void RunEffects(void); /*@ 1414:04E7 */
extern void TurnUndead(void); /*@ 0D1C:0A43 */
extern void RedrawCharacters(void); /*@ 0D1C:0583 */
extern void PlaySoundDefault(int sound); /*@ 177D:00F3 */
extern void PlaySoundAtCell(int sound, int cell); /*@ 177D:0106 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void LoadScreenImage(char far *name, char far *extra, int srcKind, int dstKind, int mode); /*@ 082C:051B */
extern void SetFont(char far *font); /*@ 1A47:0169 */
void EnterMapScreen(void);
void ResetMapScreen(void);
extern void LoadMapScreen(int level); /*@ 210C:007A */   /* overlay 3 entry, ovl3:0A7C (src/EOV3.C:540) */
extern void DrawViewFrame(int full); /*@ 2116:005C */    /* overlay 4 entry, ovl4:1F51 (src/EOV4.C) */
extern void NoOpAfterMapLoad(void); /*@ 2116:0043 */    /* overlay 4 entry, ovl4:2039, a no-op (src/EOV4.C) */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */

/* 0ACD:0008: draws "You can't go that way.", plays the bump sound and drops any pending key. The
   fallback of the movement keys (04F8..05FF) when StepIfOpen finds the way closed. */
/*@ 0ACD:0008 */ void MoveBlocked(void)
{
    DrawText("You can't go that way.\r");
    PlaySound(0x1D);
    FlushKeyBuffer();
}

/* 0ACD:0028: Purpose unknown: returns the action record's +4 word. Called through the action
   table; no direct caller found. */
/*@ 0ACD:0028 */ int GetActionResult(struct Action far *action)
{
    return action->result;
}

/* 0ACD:0034: Opens the inventory screen for the clicked portrait's member. when the action mode gMatchedNodeKey is nonzero or MemberIs accepts the
   record's slot, it hides the pointer, sets the mode byte to 1, selects the slot (mode 0 only),
   asks RedrawRowArea(1) and rebuilds list 2. Returns the record's +4. */
/*@ 0ACD:0034 */ int OpenInventory(struct Action far *action)
{
    if (gMatchedNodeKey != 0 || MemberIs(action->slot, 1)) {
        LockHidePointer();
        gPanelMode = 1;
        if (gMatchedNodeKey == 0)
            gInventoryMember = action->slot;
        RedrawRowArea(1);
        BuildList2();
        LockShowPointer();
    }
    return action->result;
}

/* 0ACD:008C: when the action mode is 1 or 2 the selected character index is forced to 0 or 1;
   otherwise it steps the index down (wrapping 0 to 5) until MemberIs accepts it. Then hides the
   pointer, asks RedrawRowArea(0) and shows the pointer again. Returns the record's +4. */
/*@ 0ACD:008C */ int SelectPrevMember(struct Action far *action)
{
    if (gMatchedNodeKey == 1)
        gInventoryMember = 0;
    else if (gMatchedNodeKey == 2)
        gInventoryMember = 1;
    else
        do {
            gInventoryMember--;
            if (gInventoryMember < 0)
                gInventoryMember = 5;
        } while (!MemberIs(gInventoryMember, 1));
    LockHidePointer();
    RedrawRowArea(0);
    LockShowPointer();
    return action->result;
}

/* 0ACD:00EB: steps the selected character index up, or sets it from the record's slot (2 gives a
   base of 2, anything else 0) when the action mode is 1 (base + 2) or 2 (base + 3), wrapping 5 to
   0. When the new index is accepted it hides the pointer, asks RedrawRowArea(0), shows the pointer
   and returns the record's +4; when it is not, the old index is restored and it returns 1. */
/*@ 0ACD:00EB */ int SelectNextMember(struct Action far *action)
{
    int base;
    int old = gInventoryMember;

    if (action->slot == 2)
        base = 2;
    else
        base = 0;
    if (gMatchedNodeKey == 1)
        gInventoryMember = base + 2;
    else if (gMatchedNodeKey == 2)
        gInventoryMember = base + 3;
    else
        do {
            gInventoryMember++;
            if (gInventoryMember > 5)
                gInventoryMember = 0;
        } while (!MemberIs(gInventoryMember, 1));
    if (!MemberIs(gInventoryMember, 1)) {
        gInventoryMember = old;
        return 1;
    }
    LockHidePointer();
    RedrawRowArea(0);
    LockShowPointer();
    return action->result;
}

/* 0ACD:018E: toggles the list mode byte gPanelMode between 1 (rebuilds list 3) and 2, asks
   RedrawRowArea(0) and returns the record's +4. */
/*@ 0ACD:018E */ int ToggleCharacterInfo(struct Action far *action)
{
    if (gPanelMode == 1) {
        BuildList3();
        gPanelMode = 2;
    } else {
        BuildList2();
        gPanelMode = 1;
    }
    RedrawRowArea(0);
    return action->result;
}

/* 0ACD:01C0: passes the record's slot to UseSelectedItem with the pointer hidden. Returns the
   record's +4. */
/*@ 0ACD:01C0 */ int ClickInventorySlot(struct Action far *action)
{
    LockHidePointer();
    UseSelectedItem(action->slot);
    LockShowPointer();
    return action->result;
}

/* 0ACD:01E3: clears the action screen: hides the pointer, sets the status-line flag to 2, blits
   the slot-5 buffer gSegDF4E into the 0x12 by 0xa8 field, resets the selected row and mode, redraws
   all six character rows, clears the status flag, clips the field, rebuilds list 1 and shows the
   pointer. Returns the record's +4. */
/*@ 0ACD:01E3 */ int Action01E3(struct Action far *action)
{
    int i;

    LockHidePointer();
    gStatusLine = 2;
    gFn804A(0, 0, 0x12, 0xa8, gSegDF4E);
    gCursorSlot = 0;
    gPanelMode = 0;
    for (i = 0; i < 6; i++)
        RedrawMember(i);
    gStatusLine = 0;
    ClipRect(0, 0, 0x16, 0, 0x12, 0xa8, 2, 0);
    BuildList1();
    LockShowPointer();
    return action->result;
}

/* 0ACD:0264: the "move/swap party member" action. When MemberIs accepts the record's slot it
   hides the pointer. With no member being moved (gIconTable == -1) it takes this slot as the moving
   member, rebuilds list 4, redraws the member and calls ArmEffect(0, 1); otherwise it swaps the
   moving member's 243-byte record with this slot's through the gSegDF4E buffer (three Unpack0
   copies), calls ArmEffect(0, 0), clears the moving member, redraws both members, selects the slot,
   rebuilds list 1 and calls RearmCharEffects. A gFlag-gated RedrawPartyPanel runs before both
   branches and a second one runs in the pick-up branch after gIconTable is set; the swap branch has
   no gFlag-gated redraw after the swap. Returns the record's +4. */
/*@ 0ACD:0264 */ int PickOrSwapMember(struct Action far *action)
{
    int old;
    int slot;

    if (!MemberIs(action->slot, 1))
        return action->result;
    LockHidePointer();
    if (gFlag)
        RedrawPartyPanel((struct ActRec far *)gF542);
    if (gIconTable == -1) {
        gIconTable = action->slot;
        if (gFlag)
            RedrawPartyPanel((struct ActRec far *)gF542);
        BuildList4();
        RedrawMember(gIconTable);
        ArmEffect(0, 1);
    } else {
        old = gIconTable;
        slot = action->slot;
        Unpack0((char far *)(gChars + gIconTable), gSegDF4E, 0xF3L);
        Unpack0((char far *)(gChars + slot), (void far *)(gChars + gIconTable), 0xF3L);
        Unpack0((char far *)gSegDF4E, (void far *)(gChars + slot), 0xF3L);
        ArmEffect(0, 0);
        gIconTable = -1;
        RedrawMember(old);
        SelectCharacter(slot);
        RedrawMember(slot);
        BuildList1();
        RearmCharEffects();
    }
    LockShowPointer();
    return action->result;
}

/* 0ACD:0393: cancels the move: hides the pointer, calls ArmEffect(0, 0), clears the moving member
   gIconTable, redraws it, rebuilds list 1 and shows the pointer. Returns the record's +4. */
/*@ 0ACD:0393 */ int CancelMemberSwap(struct Action far *action)
{
    int old;

    LockHidePointer();
    ArmEffect(0, 0);
    old = gIconTable;
    gIconTable = -1;
    RedrawMember(old);
    BuildList1();
    LockShowPointer();
    return action->result;
}

/* 0ACD:03CD: redraws the current action row through NoOpBeforeCursorMove, moves the selected action row
   gCursorSlot down (nonzero) or up (zero), wrapping it into 0..2, then passes it to NoOpAfterCursorMove with 9. */
/*@ 0ACD:03CD */ void StepActionRow(int down)
{
    NoOpBeforeCursorMove(gCursorSlot);
    if (down)
        gCursorSlot++;
    else
        gCursorSlot--;
    if (gCursorSlot < 0)
        gCursorSlot = 2;
    if (gCursorSlot > 2)
        gCursorSlot = 0;
    NoOpAfterCursorMove(gCursorSlot, 9);
}

/* 0ACD:0415: Handles a click on a member's hand slot: the left button swaps the mouse item with the hand, the other uses the hand's item. when MemberIs accepts the record's slot it hides the pointer, sets
   a flag from gHandSplitY[slot] against gHitY and dispatches to SwapSelectedItem or UseItemSlot (by the low
   bits of g5E1C), then shows the pointer. Returns 1. */
/*@ 0ACD:0415 */ int ClickHandSlot(struct Action far *action)
{
    char slot = action->slot;
    char flag;

    if (!MemberIs(slot, 1))
        return 1;
    LockHidePointer();
    if (gHandSplitY[slot] <= gHitY)
        flag = 1;
    else
        flag = 0;
    if ((g5E1C & 0x7F) == 1)
        SwapSelectedItem(slot, flag);
    else
        UseItemSlot(slot, flag);
    LockShowPointer();
    return 1;
}

/* 0ACD:0494: the "use item on the selected character" action. It takes the current item from
   GetCurrentItem and passes it, the selected character gInventoryMember, the party's cell, the kind
   gViewToCellPos[slot + gPartyDir * 4], the page gPartyDir and the item type to AddDDEffect; a nonzero result sets
   the mouse item to none (SetMouseItem(0)) and sets gViewDirty. Returns the record's +4. */
/*@ 0ACD:0494 */ int ThrowMouseItem(struct Action far *action)
{
    int item = GetCurrentItem();

    if (item) {
        if (AddDDEffect(gInventoryMember, item, gPartyCell, gViewToCellPos[action->slot + gPartyDir * 4], gPartyDir,
                       gItems[item].type)) {
            SetMouseItem(0);
            gViewDirty = 1;
        }
    }
    return action->result;
}

/* 0ACD:04F8: the forward key: moves the party one cell ahead (StepIfOpen(gPartyCell, gPartyDir),
   then MoveParty), or runs MoveBlocked ("You can't go that way.") when the way is closed. Returns
   the record's +4. */
/*@ 0ACD:04F8 */ int MoveForward(struct Action far *action)
{
    int x = StepIfOpen(gPartyCell, gPartyDir);

    if (x != -1)
        MoveParty(x);
    else
        MoveBlocked();
    return action->result;
}

/* 0ACD:0529: the backward key: as MoveForward along (gPartyDir + 2) % 4. Returns the record's +4. */
/*@ 0ACD:0529 */ int MoveBackward(struct Action far *action)
{
    int page = (gPartyDir + 2) % 4;
    int x = StepIfOpen(gPartyCell, page);

    if (x != -1)
        MoveParty(x);
    else
        MoveBlocked();
    return action->result;
}

/* 0ACD:056C: turns the party left: gPartyDir down (wrapping to 3), gPickViewPos reset to 0xff, gViewDirty set.
   Returns the record's +4. */
/*@ 0ACD:056C */ int TurnLeft(struct Action far *action)
{
    gPartyDir--;
    gPickViewPos = 0xff;
    if (gPartyDir < 0)
        gPartyDir = 3;
    gViewDirty = 1;
    return action->result;
}

/* 0ACD:0593: turns the party right: gPartyDir up modulo 4, gPickViewPos reset to 0xff, gViewDirty set. Returns
   the record's +4. */
/*@ 0ACD:0593 */ int TurnRight(struct Action far *action)
{
    gPartyDir++;
    gPickViewPos = 0xff;
    gPartyDir %= 4;
    gViewDirty = 1;
    return action->result;
}

/* 0ACD:05BA: the step-left key: as MoveForward along gPartyDir - 1 (wrapping to 3). Returns the
   record's +4. */
/*@ 0ACD:05BA */ int MoveLeft(struct Action far *action)
{
    int page = gPartyDir - 1;
    int x;

    if (page < 0)
        page = 3;
    x = StepIfOpen(gPartyCell, page);
    if (x != -1)
        MoveParty(x);
    else
        MoveBlocked();
    return action->result;
}

/* 0ACD:05FF: the step-right key: as MoveForward along (gPartyDir + 1) % 4. Returns the record's +4. */
/*@ 0ACD:05FF */ int MoveRight(struct Action far *action)
{
    int page = gPartyDir + 1;
    int x;

    page %= 4;
    x = StepIfOpen(gPartyCell, page);

    if (x != -1)
        MoveParty(x);
    else
        MoveBlocked();
    return action->result;
}

/* 0ACD:0644: the item-slot action: moves the item in the floor slot at `action` into or out of a
   cell's item list. It never reads gInventoryMember. For slot 0 or 1 the cell is the party's,
   gPartyCell; for a higher slot it is the cell ahead, gPartyCell plus stepOffset[gPartyDir] (-32, 1,
   32, -1 as bytes, the same steps as gDirCellOffset), and the action only proceeds when the wall there (gMaze/gWallTypeFlags) has one of the bits 0, 1 or 3 set
   (gWallTypeFlags[...] & 0xB). The per-page, per-slot kind is slotKind[]; a nonzero current item is queued in
   the cell's list through InsertListItem (then RunCellScript(cell, 4)), otherwise the first
   matching entry of that list is removed through RemoveListItem and shown with SetMouseItem (then
   RunCellScript(cell, 8)). Any effect sets gViewDirty. Returns 1. The two tables are local arrays
   initialised from constant lists, as the original copies them with F_SCOPY (0000:78C9) in the
   segment-first order that form produces (whitepaper 2.1.1). */
/*@ 0ACD:0644 */ int ClickFloorItem(struct Action far *action)
{
    char stepOffset[4] = { 0xE0, 0x01, 0x20, 0xFF };
    char slotKind[4][4] = {
        { 0, 1, 2, 3 },
        { 1, 3, 0, 2 },
        { 3, 2, 1, 0 },
        { 2, 0, 3, 1 },
    };
    register int slot;
    register int cell;
    int removed;

    slot = action->slot;
    if (slot < 2) {
        cell = gPartyCell;
    } else {
        cell = gPartyCell + stepOffset[gPartyDir];
        if (!(gWallTypeFlags[gMaze[cell * 9 + gA9C6]] & 0xB))
            return 1;
    }
    /* one register variable: slot is reused to hold the per-page kind, as the original does. */
    slot = slotKind[gPartyDir][slot];
    if (GetCurrentItem() == 0) {
        removed = RemoveListItem((void far *)(gMazeItems + cell * 9), slot, -1);
        if (removed) {
            RunCellScript(cell, 8);
            SetMouseItem(removed);
            gViewDirty = 1;
        }
    } else {
        InsertListItem((void far *)(gMazeItems + cell * 9), cell, GetCurrentItem(), slot);
        RunCellScript(cell, 4);
        SetMouseItem(0);
        gViewDirty = 1;
    }
    return 1;
}

/* 0ACD:0753: advances the menu step gDCAA, wrapping above 2 to 1, then moves the selected row
   gCursorSlot (step 2 passes it to NoOpAfterCursorMove with 9, otherwise NoOpBeforeCursorMove). */
/*@ 0ACD:0753 */ void StepMenuPhase(void)
{
    gDCAA++;
    if (gDCAA > 2)
        gDCAA = 1;
    if (gDCAA == 2)
        NoOpAfterCursorMove(gCursorSlot, 9);
    else
        NoOpBeforeCursorMove(gCursorSlot);
}

/* 0ACD:078C: selects the record's party slot: when MemberIs accepts it, the old row is
   redrawn, SelectCharacter switches to the slot, the new row is drawn and the pointer shown;
   returns 0. When it is not accepted, returns 1. */
/*@ 0ACD:078C */ int SelectClickedMember(struct Action far *action)
{
    if (!MemberIs(action->slot, 1))
        return 1;
    LockHidePointer();
    NoOpBeforeCursorMove(gCursorSlot);
    SelectCharacter(action->slot);
    NoOpAfterCursorMove(gCursorSlot, 9);
    LockShowPointer();
    return 0;
}

/* 0ACD:07E0: the "eat" action for the character at `index`. When MemberIs rejects it (mask 5)
   it formats "%s isn't capable of eating food!" with the character's name and draws it. Otherwise
   it takes the current item and, when its type is 0x1F or 0x31, adds the item's value (the food
   value) to the character's m23 clamped to 100, sets the item's key to -1 (freeing the record),
   redraws the icon and plays sound 9; for
   any other nonzero item it draws "You may only eat food!". index is the character index, not a
   pointer. */
/*@ 0ACD:07E0 */ void EatFood(int index)
{
    struct CharRec far *rec = gChars + index;
    int item;

    if (!MemberIs(index, 5)) {
        sprintf(gFileNameBuf, "%s isn't capable of eating food!\r", rec->name);
        DrawText(gFileNameBuf);
        return;
    }
    item = GetCurrentItem();
    if (gItems[item].type == 0x1F || gItems[item].type == 0x31) {
        if (rec->food + gItems[item].value > 100)
            rec->food = 100;
        else
            rec->food += gItems[item].value;
        gItems[item].key = -1;
        SetMouseItem(0);
        DrawMemberFood(index);
        PlaySound(9);
    } else if (item != 0) {
        DrawText("You may only eat food!\r");
    }
}

/* 0ACD:08D1: eats for the selected character gInventoryMember. Returns the record's +4. */
/*@ 0ACD:08D1 */ int FeedInventoryMember(struct Action far *action)
{
    EatFood(gInventoryMember);
    return action->result;
}

/* 0ACD:08E7: Runs the handler of the wall the party faces in the cell ahead, as a click on the view does, then recomputes the armour class of gInventoryMember. derives a key from gPartyCell and gPartyDir (StepCell), sets
   gEFD2 to 0xff, passes the key and the page to DispatchSlotHandler and the selected character gInventoryMember to
   ComputeArmourClass. */
/*@ 0ACD:08E7 */ void ActivateWallAhead(void)
{
    int x = StepCell(gPartyCell, gPartyDir);

    gEFD2 = 0xff;
    DispatchSlotHandler(x, gPartyDir);
    ComputeArmourClass(gInventoryMember);
}

/* 0ACD:091C: the "turn" action, a switch over the four-entry jump table at 0ACD:09E7 (cases 1 and
   3 shared). Case 0 resets the action row gCursorSlot to 0 when it is already set, otherwise steps the
   selected character down by 2 (modulo 6) until MemberIs accepts it and sets gCursorSlot to 1. Case 2
   is the mirror: it sets gCursorSlot to 1 when it is clear, otherwise steps the character up by 2
   (modulo 6) until accepted and clears gCursorSlot. Cases 1 and 3 toggle the low bit of the character
   when accepted. It redraws the old character row and, when the character changed, the new one.
   `how` is the direction (the switch argument). The two searches are do/while loops: a `for(;;)`
   form makes the compiler cross-jump the two gCursorSlot stores (the original keeps them apart). */
/*@ 0ACD:091C */ void MoveHandCursor(int how)
{
    int old = gInventoryMember;

    switch (how) {
    case 0:
        if (gCursorSlot)
            gCursorSlot = 0;
        else {
            do {
                gInventoryMember -= 2;
                if (gInventoryMember < 0)
                    gInventoryMember += 6;
            } while (!MemberIs(gInventoryMember, 1));
            gCursorSlot = 1;
        }
        break;
    case 1:
    case 3:
        if (MemberIs(gInventoryMember, 1))
            gInventoryMember ^= 1;
        break;
    case 2:
        if (!gCursorSlot)
            gCursorSlot = 1;
        else {
            do {
                gInventoryMember += 2;
                if (gInventoryMember > 5)
                    gInventoryMember -= 6;
            } while (!MemberIs(gInventoryMember, 1));
            gCursorSlot = 0;
        }
        break;
    }
    RedrawMember(old);
    if (gInventoryMember != old)
        RedrawMember(gInventoryMember);
}

/* 0ACD:09EF: sets the selected action row gCursorSlot from the four-byte table gSlotNeighbours[old * 4 + dir] and
   redraws the old and new rows through DrawItemRow. dir is the direction. */
/*@ 0ACD:09EF */ void SetActionRow(int dir)
{
    int old = gCursorSlot;

    gCursorSlot = gSlotNeighbours[gCursorSlot * 4 + dir];
    DrawItemRow(old, 1, 0);
    DrawItemRow(gCursorSlot, 1, 0);
}

/* 0ACD:0A34: the row/character turn dispatcher: does nothing in the rest phase (gDCAA == 5), calls
   MoveHandCursor in mode 0 or SetActionRow in mode 1 with the direction dir. Case 0 jumps to the shared
   exit rather than breaking: a plain `break` makes the compiler cross-jump the two calls' argument
   cleanup, which the original keeps apart. */
/*@ 0ACD:0A34 */ void MoveKeyCursor(int dir)
{
    if (gDCAA == 5)
        return;
    switch (gPanelMode) {
    case 0:
        MoveHandCursor(dir);
        goto done;
    case 1:
        SetActionRow(dir);
    }
done:
    return;
}

/* 0ACD:0A61: the list move/use handler. In mode 2 (gPanelMode == 2) or the rest phase (gDCAA == 5) it
   only recomputes the armour class of gInventoryMember (ComputeArmourClass, 0DE8:0D5E). Otherwise it takes the
   current item (GetCurrentItem, or RemoveListItem over the character's +0x97 sub-table at row 0x10) and
   gives up without redrawing when the item's flags have bit 0x20 or the item is 0. It then computes
   the per-page value gFrontPosByDir[gPartyDir * 2 + gMemberSide[gInventoryMember]]: `used` zero runs InsertListItem over the
   cell's gMazeItems list and RunCellScript, otherwise AddDDEffect returns whether the use
   took effect. On success it clears the mouse item (SetMouseItem(0)) or, when the mouse is disabled
   and the row is 0x10, redraws the member (RedrawMember(gInventoryMember)), and sets gViewDirty and gPickViewPos; on no
   effect at row 0x10 it runs InsertListItem over the character's +0x97 table. Always recomputes the
   armour class of gInventoryMember at the end. */
/*@ 0ACD:0A61 */ void MoveListItem(int used)
{
    int item;
    int flag;
    int value;

    if (gPanelMode == 2)
        goto call;
    if (gDCAA == 5)
        goto call;
    if (gCursorSlot != 0x10) {
        item = GetCurrentItem();
        if (gItems[item].flags & 0x20)
            return;
    } else {
        item = RemoveListItem((void far *)gChars[gInventoryMember].m97, 0, -1);
    }
    if (item == 0)
        return;
    flag = 1;
    value = gMemberSide[gInventoryMember];
    value = gFrontPosByDir[gPartyDir * 2 + value];
    if (used == 0) {
        InsertListItem((void far *)(gMazeItems + gPartyCell * 9), gPartyCell, item, value);
        RunCellScript(gPartyCell, 4);
    } else {
        flag = AddDDEffect(gInventoryMember, item, gPartyCell, value, gPartyDir, gItems[item].type);
    }
    if (flag != 0) {
        if (gMouseDisabled == 0 || gCursorSlot != 0x10)
            SetMouseItem(0);
        else
            RedrawMember(gInventoryMember);
        gViewDirty = 1;
        gPickViewPos = 0xff;
    } else {
        if (gCursorSlot == 0x10)
            InsertListItem((void far *)gChars[gInventoryMember].m97, 0xFFFE, item, 0);
    }
call:
    ComputeArmourClass(gInventoryMember);
}

/* 0ACD:0BA6: the list pick handler. It takes the item list of the party's cell (or, when that
   list is empty, of the cell ahead, from StepCell) and scans up to five pages: while gPickFirstItem is -1 it
   steps the page byte gPickViewPos up (wrapping 0..3) and counts the pages, and when gPickViewPos is -1 (no page
   chosen yet) it first clears gPickFirstItem to -1. It picks the cell ahead (StepCell) when
   gViewPosAhead[gPickViewPos] is set, and asks RemoveListItem for the first item of that cell's gMazeItems
   list with the kind gViewToCellPos[gPartyDir * 4 + gPickViewPos]. A fresh item (nonzero and not the one gPickFirstItem already
   holds) sets the local flag (which ends the page loop) and is tested with CanPlaceItemInSlot (a check that
   returns whether the character may wear the item type, 0BD2:0AF2), then is shown: at row 0x10, when
   the check passes it is inserted into the character's own list with InsertListItem and the member is
   redrawn, otherwise it is pushed into the cell's list with PushListItem and RunCellScript; at
   any other row, when the check passes it goes onto the mouse with SetMouseItem (a second item
   already held is pushed back into the cell's list first), otherwise it is pushed into the
   cell's list. It sets gViewDirty and stops. A
   repeated item (nonzero and equal to gPickFirstItem) is inserted back with InsertListItem and a zero item
   runs nothing, both clearing gPickFirstItem. It recomputes the armour class of gInventoryMember (ComputeArmourClass,
   0DE8:0D5E) at the end. The success path is a positive `if (item != 0 && item != gPickFirstItem)` with the
   miss path as its `else`: with a `goto miss` the compiler stores the register variable item from DI
   at 0ACD:0C7F, but the original stores it from AX, which the if/else form produces. */
/*@ 0ACD:0BA6 */ void PickListItem(void)
{
    int flag;
    int value;
    int count;
    int cur;
    int cell;
    int item;

    cell = gPartyCell;
    if (*(int *)(gMazeItems + cell * 9) == 0) {
        if (*(int *)(gMazeItems + StepCell(cell, gPartyDir) * 9) == 0)
            goto done;
    }
    count = 0;
    flag = 0;
    do {
        if (gPickViewPos == -1)
            gPickFirstItem = -1;
        if (gPickFirstItem == -1) {
            gPickViewPos = (gPickViewPos + 1) & 3;
            count++;
        }
        if (gViewPosAhead[gPickViewPos] != 0)
            cell = StepCell(gPartyCell, gPartyDir);
        else
            cell = gPartyCell;
        value = gViewToCellPos[gPartyDir * 4 + gPickViewPos];
        item = RemoveListItem((void far *)(gMazeItems + cell * 9), value, -1);
        if (item != 0 && item != gPickFirstItem) {
            flag = 1;
            if (gPickFirstItem == -1)
                gPickFirstItem = item;
            if (gCursorSlot != 0x10) {
                cur = GetCurrentItem();
                if (CanPlaceItemInSlot(item, gInventoryMember, gCursorSlot)) {
                    if (cur != 0) {
                        PushListItem(cell, cur, value);
                        RunCellScript(cell, 4);
                    }
                    SetMouseItem(item);
                } else {
                    PushListItem(cell, item, value);
                    RunCellScript(cell, 4);
                }
            } else {
                if (CanPlaceItemInSlot(item, gInventoryMember, gCursorSlot)) {
                    InsertListItem((void far *)gChars[gInventoryMember].m97, 0xFFFE, item, 0);
                    RedrawMember(gInventoryMember);
                } else {
                    PushListItem(cell, item, value);
                    RunCellScript(cell, 4);
                }
            }
            gViewDirty = 1;
        } else {
            if (item != 0) {
                InsertListItem((void far *)(gMazeItems + cell * 9), cell, item, value);
                RunCellScript(cell, 4);
            }
            gPickFirstItem = -1;
        }
    } while (flag == 0 && count < 5);
done:
    ComputeArmourClass(gInventoryMember);
}

/* 0ACD:0D9D: the key dispatcher for the map screen. It acts only while the mouse is disabled and
   outside the rest phase (gDCAA != 5). Keys 0x12/0x20/0x2e/0x1f set the direction dir 0/1/2/3, key
   0x34 runs ActivateWallAhead, key 0x17 calls UseItemSlot(gInventoryMember, gCursorSlot), keys 0x21 and 0x15 call
   MoveListItem(0) and MoveListItem(1) and key 0x23 calls PickListItem (0ACD:0BA6). The four action
   cases are skipped in mode 2 (gPanelMode == 2), and a direction set is passed to MoveKeyCursor unless the
   mode is 2. */
/*@ 0ACD:0D9D */ void HandleMapKey(int key)
{
    int dir;

    if (!gMouseDisabled)
        return;
    if (gDCAA == 5)
        return;
    dir = -1;
    switch (key) {
    case 0x34:
        ActivateWallAhead();
        break;
    case 0x17:
        if (gPanelMode != 2)
            UseItemSlot(gInventoryMember, gCursorSlot);
        break;
    case 0x21:
        if (gPanelMode != 2)
            MoveListItem(0);
        break;
    case 0x15:
        if (gPanelMode != 2)
            MoveListItem(1);
        break;
    case 0x23:
        if (gPanelMode != 2)
            PickListItem();
        break;
    case 0x12:
        dir = 0;
        break;
    case 0x20:
        dir = 1;
        break;
    case 0x2e:
        dir = 2;
        break;
    case 0x1f:
        dir = 3;
        break;
    }
    if (dir == -1)
        return;
    if (gPanelMode == 2)
        return;
    MoveKeyCursor(dir);
}

/* 0ACD:0E6C: waits for one key event and dispatches it to the list head gList (16A5:005A), then
   passes DispatchNodeEvent's result (16A5:00BD: the key a node took, the key itself when no node
   takes it, or 0 for a mouse-only call) to the key dispatcher HandleMapKey. */
/*@ 0ACD:0E6C */ void PollMapInput(void)
{
    int key = DispatchKeyWait(gList);

    HandleMapKey(key);
}

/* 0ACD:0E8A: the map timeout/encounter loop, run from the start-up after the item tables are
   loaded. It arms the timeout gNextRandomSoundTick from one (1d10 + 3) roll times 0x12 plus the 32-bit counter
   g8492, enters the map screen, then loops: saves the party's cell, services the key dispatcher
   (PollMapInput) and RunEffects, runs the overlay effect when gViewDirty is set, and when gNextRandomSoundTick has run
   below g8492 while gFBC8 is in 1..3 re-arms the timeout and plays sound 0x1E at a cell near the party
   (gPartyCell plus 0 to 11).
   Each pass plays the default sound and runs TurnUndead. */
/*@ 0ACD:0E8A */ void RunMapLoop(void)
{
    gNextRandomSoundTick = g8492 + (long)(RollDice(1, 10, 3) * 0x12);
    EnterMapScreen();
    LockShowPointer();
    gUnreadPartyCellCopy = gPartyCell;
    g8134 = 0;
    for (;;) {
        gUnreadPartyCellCopy = gPartyCell;
        PollMapInput();
        RunEffects();
        if (gViewDirty)
            DrawViewFrame(1);
        if (gNextRandomSoundTick < g8492) {
            if (gFBC8 > 0 && gFBC8 < 4) {
                gNextRandomSoundTick = g8492 + (long)(RollDice(1, 10, 3) * 0x12);
                PlaySoundAtCell(0x1E, gPartyCell + RollDice(1, 12, -1));
            }
        }
        PlaySoundDefault(0);
        TurnUndead();
    }
}

/* 0ACD:0F72: enters the map screen. When gA998 is clear (a normal entry) it calls ResetMapScreen
   (0ACD:1033); when it is nonzero (0xff while a load runs, 0xfe after a saved game loads, src/EOV2.C)
   it shows the mouse item (SetMouseItem(gFBCE), unless the mouse is disabled), loads level gFBC8's
   map and inventory (LoadMapScreen, ovl3:0A7C) and clears gA998. Then it clears gFlag, resets the
   character index and the menu phase, rebuilds list 1, loads PLAYFLD and INVENT through
   LoadScreenImage (into gSheetDECORATE and gSheet2067), selects the font, clears the status
   flag, redraws the characters, shows the pointer and sets text window 7. */
/*@ 0ACD:0F72 */ void EnterMapScreen(void)
{
    if (!gA998) {
        ResetMapScreen();
    } else {
        if (!gMouseDisabled)
            SetMouseItem(gFBCE);
        LoadMapScreen(gFBC8);
        gA998 = 0;
    }
    NoOpAfterMapLoad();
    gFlag = 0;
    gInventoryMember = 0;
    gDCAA = 1;
    BuildList1();
    LoadScreenImage("PLAYFLD", (char far *)gSheetDECORATE, 5, 3, 0);
    LoadScreenImage("INVENT", (char far *)gSheet2067, 5, 3, 2);
    SetFont(gFont6x6);
    gStatusLine = 0;
    RedrawCharacters();
    DrawViewFrame(1);
    LockShowPointer();
    SetTextWindow(7);
}

/* 0ACD:1033: resets the map screen to its initial state: loads map level 1 (LoadMapScreen,
   ovl3:0A7C), sets gPartyCell to 0x1EA and the page gPartyDir to 0, and clears the mouse item
   (SetMouseItem(0)) when the mouse is enabled. */
/*@ 0ACD:1033 */ void ResetMapScreen(void)
{
    LoadMapScreen(1);
    gPartyCell = 0x1EA;
    gPartyDir = 0;
    if (!gMouseDisabled)
        SetMouseItem(0);
}
