/* Unit at EOB.EXE 0ACD:0008 (4182 bytes, 35 functions): the adventure-screen action handlers.
   Most entries are reached through a 60-byte DGROUP action record (struct Action) whose +0x38
   field is the party slot and whose +4 is the value the handler returns; they draw the block
   messages ("You can't go that way."), turn undead, eat, browse the item list, and set the sound
   volume and page globals. Names that are not established say what a function does. */

/*@flags -ml -O -Z */

/* A 60-byte action record reached through a far pointer. Only the two fields the handlers touch
   are named; everything else is padding. */
struct Action {
    char pad00[4];
    int result;             /* +4: the value the handler returns to its caller */
    char pad06[0x38 - 6];
    int slot;               /* +0x38: the party slot the action applies to */
};

extern signed char gA99A; /*@ ds:A99A */    /* the selected character index (src/E0D1C.C) */
extern char gF51A; /*@ ds:F51A */           /* the selected action row 0..2 (src/E0D1C.C) */
extern char gFB44; /*@ ds:FB44 */           /* a mode byte: 0, 1 and 2 gate the action handlers
                                               (src/E0D1C.C) */
extern int gFC82; /*@ ds:FC82 */            /* an action mode: 0, 1 and 2 pick the branches of the
                                               0034, 008C and 00EB handlers */
extern int g8115; /*@ ds:8115 */            /* nonzero while a status line is up (src/E082C.C) */
extern void far *gSegDF4E; /*@ ds:DF4E */   /* the 19EC:0002(slot 5) far buffer (src/E07A9.C) */

extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void FlushKeyBuffer(void); /*@ 1AAD:0480 */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern void f1547_0991(int dir); /*@ 1547:0991 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void f0BD2_0ABE(int one); /*@ 0BD2:0ABE */
extern void f0BD2_0BA0(int slot); /*@ 0BD2:0BA0 */
extern void BuildList1(void); /*@ 0CF7:0028 */
extern void BuildList2(void); /*@ 0CF7:00B4 */
extern void BuildList3(void); /*@ 0CF7:0140 */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */
extern void (far *gFn804A)(int x, int y, int w, int h, void far *buf); /*@ ds:804A */ /* src/EOV2.C */

/* 0ACD:0008: draws "You can't go that way.", plays the bump sound and drops any pending key. The
   fallback the sound-volume handlers (04F8..05FF) run when the volume lookup yields -1. */
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

/* 0ACD:0034: Purpose unknown: when the action mode gFC82 is nonzero or f0D1C_0670 accepts the
   record's slot, it hides the pointer, sets the mode byte to 1, selects the slot (mode 0 only),
   asks f0BD2_0ABE(1) and rebuilds list 2. Returns the record's +4. */
/*@ 0ACD:0034 */ int Action0034(struct Action far *action)
{
    if (gFC82 != 0 || f0D1C_0670(action->slot, 1)) {
        LockHidePointer();
        gFB44 = 1;
        if (gFC82 == 0)
            gA99A = action->slot;
        f0BD2_0ABE(1);
        BuildList2();
        LockShowPointer();
    }
    return action->result;
}

/* 0ACD:008C: when the action mode is 1 or 2 the selected character index is forced to 0 or 1;
   otherwise it steps the index down (wrapping 0 to 5) until f0D1C_0670 accepts it. Then hides the
   pointer, asks f0BD2_0ABE(0) and shows the pointer again. Returns the record's +4. */
/*@ 0ACD:008C */ int Action008C(struct Action far *action)
{
    if (gFC82 == 1)
        gA99A = 0;
    else if (gFC82 == 2)
        gA99A = 1;
    else
        do {
            gA99A--;
            if (gA99A < 0)
                gA99A = 5;
        } while (!f0D1C_0670(gA99A, 1));
    LockHidePointer();
    f0BD2_0ABE(0);
    LockShowPointer();
    return action->result;
}

/* 0ACD:00EB: steps the selected character index up, or sets it from the record's slot (2 gives a
   base of 2, anything else 0) when the action mode is 1 (base + 2) or 2 (base + 3), wrapping 5 to
   0. When the new index is accepted it hides the pointer, asks f0BD2_0ABE(0), shows the pointer
   and returns the record's +4; when it is not, the old index is restored and it returns 1. */
/*@ 0ACD:00EB */ int Action00EB(struct Action far *action)
{
    int base;
    int old = gA99A;

    if (action->slot == 2)
        base = 2;
    else
        base = 0;
    if (gFC82 == 1)
        gA99A = base + 2;
    else if (gFC82 == 2)
        gA99A = base + 3;
    else
        do {
            gA99A++;
            if (gA99A > 5)
                gA99A = 0;
        } while (!f0D1C_0670(gA99A, 1));
    if (!f0D1C_0670(gA99A, 1)) {
        gA99A = old;
        return 1;
    }
    LockHidePointer();
    f0BD2_0ABE(0);
    LockShowPointer();
    return action->result;
}

/* 0ACD:018E: toggles the list mode byte gFB44 between 1 (rebuilds list 3) and 2, asks
   f0BD2_0ABE(0) and returns the record's +4. */
/*@ 0ACD:018E */ int Action018E(struct Action far *action)
{
    if (gFB44 == 1) {
        BuildList3();
        gFB44 = 2;
    } else {
        BuildList2();
        gFB44 = 1;
    }
    f0BD2_0ABE(0);
    return action->result;
}

/* 0ACD:01C0: passes the record's slot to f0BD2_0BA0 with the pointer hidden. Returns the
   record's +4. */
/*@ 0ACD:01C0 */ int Action01C0(struct Action far *action)
{
    LockHidePointer();
    f0BD2_0BA0(action->slot);
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
    g8115 = 2;
    gFn804A(0, 0, 0x12, 0xa8, gSegDF4E);
    gF51A = 0;
    gFB44 = 0;
    for (i = 0; i < 6; i++)
        f1547_0991(i);
    g8115 = 0;
    ClipRect(0, 0, 0x16, 0, 0x12, 0xa8, 2, 0);
    BuildList1();
    LockShowPointer();
    return action->result;
}
