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

extern char gFB34; /*@ ds:FB34 */           /* the index of the party member being moved, -1 when
                                               none (src/EOV2.C) */
extern int gFBCC; /*@ ds:FBCC */            /* a page number 0..3 (kept modulo 4 by this unit,
                                               src/E0D1C.C) */
extern int gSoundVolume; /*@ ds:FBCA */     /* the global sound volume (src/E0D1C.C) */
extern char gEFB2; /*@ ds:EFB2 */           /* set to 0xff when the sound page changes */
extern char gDCAA; /*@ ds:DCAA */           /* a 1..2 step counter (0753) */
extern int gEFD2; /*@ ds:EFD2 */            /* set to 0xff by 08E7 */
extern char gF542[]; /*@ ds:F542 */         /* a message buffer passed to 0EF3:2541 when gFlag is set */
extern char gDF4C; /*@ ds:DF4C */           /* set to 1 when an action took effect (src/E0D1C.C) */

extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern int f0D1C_0080(int value, signed char page); /*@ 0D1C:0080 */
extern int f0D1C_02DC(void); /*@ 0D1C:02DC */
extern void SelectCharacter(int value); /*@ 0D1C:003F */
extern void f0D1C_006D(int first, int second); /*@ 0D1C:006D */
extern void f0D1C_0078(int value); /*@ 0D1C:0078 */
extern void f0BD2_0C8E(int slot, int flag); /*@ 0BD2:0C8E */
extern void f0BD2_1001(int slot, int flag); /*@ 0BD2:1001 */
extern int f0BD2_07DE(int who, int item, int volume, int kind, int page, int type); /*@ 0BD2:07DE */
extern void f1414_05A4(int a, int b); /*@ 1414:05A4 */
extern void f1414_0B2E(void); /*@ 1414:0B2E */
extern void f07A9_010C(int n); /*@ 07A9:010C */
extern void f122E_0001(int n); /*@ 122E:0001 */
extern void f122E_0896(int a, int b); /*@ 122E:0896 */
extern void f0DE8_0D5E(int who); /*@ 0DE8:0D5E */
extern void BuildList4(void); /*@ 0CF7:01CC */
extern void Unpack0(char far *src, void far *dst, long len); /*@ 1BA8:0006 */
extern void f0EF3_2541(char far *text); /*@ 0EF3:2541 */

/* A character record: the name and the hunger byte are the fields this unit touches (src/E0A72.C). */
struct CharRec {
    char pad00[2];
    char name[0x0B];        /* +2: the character's name, NUL padded (src/E0D1C.C) */
    char pad0D[0x23 - 0x0D];
    unsigned char m23;      /* +0x23: a hunger/food value clamped to 100 by EatFood (07E0) */
    char pad24[0xF3 - 0x24];
};
extern struct CharRec gChars[]; /*@ ds:F580 */

/* A 14-byte ds:8DD2 record: m02 is the item type and m0B a second field (src/E0D1C.C). */
struct Entry14 {
    unsigned char flags;    /* +0 */
    signed char icon;       /* +1 */
    signed char m02;        /* +2 */
    char pad03;
    int m04;                /* +4 */
    char pad06[0x0B - 0x06];
    signed char m0B;        /* +0xB */
    char pad0C[0x0E - 0x0C];
};
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */
extern signed char g1A1B[]; /*@ ds:1A1B */  /* per-page sound table (src/E0D1C.C) */
extern int g36B0[]; /*@ ds:36B0 */          /* a per-slot word (screen row) compared with g5E12 */
extern int g5E12; /*@ ds:5E12 */
extern unsigned g5E1C; /*@ ds:5E1C */       /* src/E1658.C */
extern signed char g36D0[]; /*@ ds:36D0 */  /* a per-row four-byte action table (09EF) */
extern char gFlag; /*@ ds:F3B9 */           /* picks the "a" table set when nonzero (src/E0CF7.C) */
extern char gA998; /*@ ds:A998 */           /* src/EOV2.C */
extern int gMouseDisabled; /*@ ds:814A */   /* nonzero disables the mouse (src/E07A9.C) */
extern int gFBC8; /*@ ds:FBC8 */            /* the current action (src/E0D1C.C) */
extern int gFBCE; /*@ ds:FBCE */            /* the item index shown by the mouse (src/E07A9.C) */
extern void far *gA9C0; /*@ ds:A9C0 */       /* the current 6x6 font (src/E07A9.C) */
extern void far *gList; /*@ ds:FB48 */      /* the head of the list built last (src/E0CF7.C) */
extern long gFC5A; /*@ ds:FC5A */           /* a far timeout counter (0E8A) */
extern int gFC58; /*@ ds:FC58 */            /* the saved sound volume (0E8A) */
extern int g8492; /*@ ds:8492 */            /* the low word of the timeout base (src/EOV6.C) */
extern int g8494; /*@ ds:8494 */            /* the high word of the timeout base (0E8A) */
extern int g8134; /*@ ds:8134 */            /* set to 0 by the map screen (src/E14EF.C) */
extern char gFileNameBuf[]; /*@ ds:FE6E */  /* the text buffer sprintf formats into (src/E082C.C) */
extern unsigned char far gSheetDECORATE[]; /*@ 2069:0000 */  /* the DECORATE sheet (src/EOV2.C) */
extern unsigned char far gSheet2067[]; /*@ 2067:0000 */      /* the sheet at segment 2067 (0F72) */

extern void f0BD2_0AF2(int a, int b, int c); /*@ 0BD2:0AF2 */
extern void f0BD2_0458(int a, int b, int c); /*@ 0BD2:0458 */
extern void f0BD2_01D0(void far *head, int key, int item, int sound); /*@ 0BD2:01D0 */
extern int f0BD2_02B9(void far *head, int key, int item); /*@ 0BD2:02B9 */
extern void f0BD2_0917(int who, int slot, int one); /*@ 0BD2:0917 */
extern int f16A5_005A(void far *list); /*@ 16A5:005A */
extern void f1547_04A0(int who); /*@ 1547:04A0 */
extern void f122E_1DA8(int a, int b); /*@ 122E:1DA8 */
extern void f1414_04E7(void); /*@ 1414:04E7 */
extern void TurnUndead(void); /*@ 0D1C:0A43 */
extern void RedrawCharacters(void); /*@ 0D1C:0583 */
extern void PlaySoundDefault(int sound); /*@ 177D:00F3 */
extern void PlaySoundVolume(int sound, int volume); /*@ 177D:0106 */
extern void SetTextWindow(int n); /*@ 1DE7:0009 */
extern void LoadScreenImage(char far *name, char far *extra, int srcKind, int dstKind, int mode); /*@ 082C:051B */
extern void f1A47_0169(char far *font); /*@ 1A47:0169 */
void EnterMapScreen(void);
void ResetMapScreen(void);
extern void fOvl3_0A7C(int n); /*@ 210C:007A */   /* overlay 3 entry, ovl3:0A7C */
extern void fOvl4_1F51(int n); /*@ 2116:005C */   /* overlay 4 entry, ovl4:1F51 */
extern void fOvl4_2039(void); /*@ 2116:0043 */    /* overlay 4 entry, ovl4:2039 */
extern int sprintf(char *buf, char *fmt, ...); /*@ 0000:57AE */

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

/* 0ACD:0264: the "move/swap party member" action. When f0D1C_0670 accepts the record's slot it
   hides the pointer. With no member being moved (gFB34 == -1) it takes this slot as the moving
   member, redraws it and calls f1414_05A4(0, 1); otherwise it swaps the moving member's 243-byte
   record with this slot's through the gSegDF4E buffer (three Unpack0 copies), redraws both,
   rebuilds list 1 and calls f1414_0B2E. The gFlag redraws bracket both ends of the swap. Returns
   the record's +4. */
/*@ 0ACD:0264 */ int Action0264(struct Action far *action)
{
    int old;
    int slot;

    if (!f0D1C_0670(action->slot, 1))
        return action->result;
    LockHidePointer();
    if (gFlag)
        f0EF3_2541(gF542);
    if (gFB34 == -1) {
        gFB34 = action->slot;
        if (gFlag)
            f0EF3_2541(gF542);
        BuildList4();
        f1547_0991(gFB34);
        f1414_05A4(0, 1);
    } else {
        old = gFB34;
        slot = action->slot;
        Unpack0((char far *)(gChars + gFB34), gSegDF4E, 0xF3L);
        Unpack0((char far *)(gChars + slot), (void far *)(gChars + gFB34), 0xF3L);
        Unpack0((char far *)gSegDF4E, (void far *)(gChars + slot), 0xF3L);
        f1414_05A4(0, 0);
        gFB34 = -1;
        f1547_0991(old);
        SelectCharacter(slot);
        f1547_0991(slot);
        BuildList1();
        f1414_0B2E();
    }
    LockShowPointer();
    return action->result;
}

/* 0ACD:0393: cancels the move: hides the pointer, calls f1414_05A4(0, 0), clears the moving member
   gFB34, redraws it, rebuilds list 1 and shows the pointer. Returns the record's +4. */
/*@ 0ACD:0393 */ int Action0393(struct Action far *action)
{
    int old;

    LockHidePointer();
    f1414_05A4(0, 0);
    old = gFB34;
    gFB34 = -1;
    f1547_0991(old);
    BuildList1();
    LockShowPointer();
    return action->result;
}

/* 0ACD:03CD: moves the selected action row gF51A down (nonzero) or up (zero), wrapping it into
   0..2, then passes it to f0D1C_006D with 9. */
/*@ 0ACD:03CD */ void StepActionRow(int down)
{
    f0D1C_0078(gF51A);
    if (down)
        gF51A++;
    else
        gF51A--;
    if (gF51A < 0)
        gF51A = 2;
    if (gF51A > 2)
        gF51A = 0;
    f0D1C_006D(gF51A, 9);
}

/* 0ACD:0415: Purpose unknown: when f0D1C_0670 accepts the record's slot it hides the pointer, sets
   a flag from g36B0[slot] against g5E12 and dispatches to f0BD2_0C8E or f0BD2_1001 (by the low
   bits of g5E1C), then shows the pointer. Returns 1. */
/*@ 0ACD:0415 */ int Action0415(struct Action far *action)
{
    char slot = action->slot;
    char flag;

    if (!f0D1C_0670(slot, 1))
        return 1;
    LockHidePointer();
    if (g36B0[slot] <= g5E12)
        flag = 1;
    else
        flag = 0;
    if ((g5E1C & 0x7F) == 1)
        f0BD2_0C8E(slot, flag);
    else
        f0BD2_1001(slot, flag);
    LockShowPointer();
    return 1;
}
/* 0ACD:0494: the "use item on the selected character" action. It takes the current item from
   f0D1C_02DC and passes it, the selected character gA99A, the volume, the per-page kind
   g1A1B[slot + gFBCC * 4], the page gFBCC and the item type to f0BD2_07DE; a nonzero result
   redraws the icon and sets gDF4C. Returns the record's +4. */
/*@ 0ACD:0494 */ int Action0494(struct Action far *action)
{
    int item = f0D1C_02DC();

    if (item) {
        if (f0BD2_07DE(gA99A, item, gSoundVolume, g1A1B[action->slot + gFBCC * 4], gFBCC,
                       gTable8DD2[item].m02)) {
            f07A9_010C(0);
            gDF4C = 1;
        }
    }
    return action->result;
}

/* 0ACD:04F8: plays the sound for the current page: f0D1C_0080 maps gSoundVolume and gFBCC to a
   sound, played through f122E_0001, and the "You can't go that way." fallback runs when it is -1.
   Returns the record's +4. */
/*@ 0ACD:04F8 */ int Action04F8(struct Action far *action)
{
    int x = f0D1C_0080(gSoundVolume, gFBCC);

    if (x != -1)
        f122E_0001(x);
    else
        MoveBlocked();
    return action->result;
}

/* 0ACD:0529: the same sound dispatch as 04F8 for page (gFBCC + 2) % 4. Returns the record's +4. */
/*@ 0ACD:0529 */ int Action0529(struct Action far *action)
{
    int page = (gFBCC + 2) % 4;
    int x = f0D1C_0080(gSoundVolume, page);

    if (x != -1)
        f122E_0001(x);
    else
        MoveBlocked();
    return action->result;
}

/* 0ACD:056C: steps the page gFBCC down (wrapping to 3), marks the page changed (gEFB2 = 0xff)
   and sets gDF4C. Returns the record's +4. */
/*@ 0ACD:056C */ int Action056C(struct Action far *action)
{
    gFBCC--;
    gEFB2 = 0xff;
    if (gFBCC < 0)
        gFBCC = 3;
    gDF4C = 1;
    return action->result;
}

/* 0ACD:0593: steps the page gFBCC up modulo 4, marks the page changed and sets gDF4C. Returns the
   record's +4. */
/*@ 0ACD:0593 */ int Action0593(struct Action far *action)
{
    gFBCC++;
    gEFB2 = 0xff;
    gFBCC %= 4;
    gDF4C = 1;
    return action->result;
}

/* 0ACD:05BA: the sound dispatch of 04F8 for page gFBCC - 1 (wrapping to 3). Returns the record's
   +4. */
/*@ 0ACD:05BA */ int Action05BA(struct Action far *action)
{
    int page = gFBCC - 1;
    int x;

    if (page < 0)
        page = 3;
    x = f0D1C_0080(gSoundVolume, page);
    if (x != -1)
        f122E_0001(x);
    else
        MoveBlocked();
    return action->result;
}

/* 0ACD:05FF: the sound dispatch of 04F8 for page (gFBCC + 1) % 4. Returns the record's +4. */
/*@ 0ACD:05FF */ int Action05FF(struct Action far *action)
{
    int page = gFBCC + 1;
    int x;

    page %= 4;
    x = f0D1C_0080(gSoundVolume, page);

    if (x != -1)
        f122E_0001(x);
    else
        MoveBlocked();
    return action->result;
}

/* 0ACD:0644 stub: blocked: the original copies the 4-byte table at ds:36BC and the 16-byte table
   at ds:36C0 into stack locals with F_SCOPY (0000:78C9), which pushes the destination segment
   before the offset; the pinned compiler's struct copy pushes the offset first (whitepaper 2.1.1,
   eov6). The rest of the function (the sound/spell dispatch through gB894 and g194B) cannot be
   written until the copies are. */
/*@ 0ACD:0644 stub */ int Action0644(struct Action far *action)
{
    return action->result;
}

/* 0ACD:0753: advances the menu step gDCAA, wrapping above 2 to 1, then moves the selected row
   gF51A (step 2 passes it to f0D1C_006D with 9, otherwise f0D1C_0078). */
/*@ 0ACD:0753 */ void StepMenuPhase(void)
{
    gDCAA++;
    if (gDCAA > 2)
        gDCAA = 1;
    if (gDCAA == 2)
        f0D1C_006D(gF51A, 9);
    else
        f0D1C_0078(gF51A);
}

/* 0ACD:078C: selects the record's party slot: when f0D1C_0670 accepts it, the old row is
   redrawn, SelectCharacter switches to the slot, the new row is drawn and the pointer shown;
   returns 0. When it is not accepted, returns 1. */
/*@ 0ACD:078C */ int Action078C(struct Action far *action)
{
    if (!f0D1C_0670(action->slot, 1))
        return 1;
    LockHidePointer();
    f0D1C_0078(gF51A);
    SelectCharacter(action->slot);
    f0D1C_006D(gF51A, 9);
    LockShowPointer();
    return 0;
}

/* 0ACD:07E0: the "eat" action for the character at `index`. When f0D1C_0670 rejects it (mask 5)
   it formats "%s isn't capable of eating food!" with the character's name and draws it. Otherwise
   it takes the current item and, when its type is 0x1F or 0x31, adds the item's m0B to the
   character's m23 clamped to 100, clears the item's m04, redraws the icon and plays sound 9; for
   any other nonzero item it draws "You may only eat food!". index is the character index, not a
   pointer. */
/*@ 0ACD:07E0 */ void EatFood(int index)
{
    struct CharRec far *rec = gChars + index;
    int item;

    if (!f0D1C_0670(index, 5)) {
        sprintf(gFileNameBuf, "%s isn't capable of eating food!\r", rec->name);
        DrawText(gFileNameBuf);
        return;
    }
    item = f0D1C_02DC();
    if (gTable8DD2[item].m02 == 0x1F || gTable8DD2[item].m02 == 0x31) {
        if (rec->m23 + gTable8DD2[item].m0B > 100)
            rec->m23 = 100;
        else
            rec->m23 += gTable8DD2[item].m0B;
        gTable8DD2[item].m04 = -1;
        f07A9_010C(0);
        f1547_04A0(index);
        PlaySound(9);
    } else if (item != 0) {
        DrawText("You may only eat food!\r");
    }
}

/* 0ACD:08D1: eats for the selected character gA99A. Returns the record's +4. */
/*@ 0ACD:08D1 */ int Action08D1(struct Action far *action)
{
    EatFood(gA99A);
    return action->result;
}

/* 0ACD:08E7: Purpose unknown: derives a key from gSoundVolume and gFBCC (f0D1C_00DA), sets
   gEFD2 to 0xff, passes the key and the page to f122E_0896 and the selected character gA99A to
   f0DE8_0D5E. */
/*@ 0ACD:08E7 */ void Action08E7(void)
{
    int x = f0D1C_00DA(gSoundVolume, gFBCC);

    gEFD2 = 0xff;
    f122E_0896(x, gFBCC);
    f0DE8_0D5E(gA99A);
}

/* 0ACD:091C draft: a switch over a four-entry jump table at 0ACD:09E7, cases 1 and 3 shared.
   Everything matches except tail merging: with -O our build merges `gF51A = 0; break;` of case 0
   into the end of case 2, and `gF51A = 1; break;` of case 2 into case 0; the original keeps both
   (11 bytes longer). Without -O, -Z has no effect and jump chains stay (13 bytes longer); -G uses
   add sp; -P (C++) still merges. Tried: early break, if/else, for(;;) with inner break, default
   label, no final break, volatile. Open: another switch, or the compiler's tail-merge rule. */
/*@ 0ACD:091C draft */ void Turn091C(int how)
{
    int old = gA99A;

    switch (how) {
    case 0:
        if (gF51A)
            gF51A = 0;
        else
            for (;;) {
                gA99A -= 2;
                if (gA99A < 0)
                    gA99A += 6;
                if (f0D1C_0670(gA99A, 1)) {
                    gF51A = 1;
                    break;
                }
            }
        break;
    case 1:
    case 3:
        if (f0D1C_0670(gA99A, 1))
            gA99A ^= 1;
        break;
    case 2:
        if (!gF51A)
            gF51A = 1;
        else
            for (;;) {
                gA99A += 2;
                if (gA99A > 5)
                    gA99A -= 6;
                if (f0D1C_0670(gA99A, 1)) {
                    gF51A = 0;
                    break;
                }
            }
        break;
    }
    f1547_0991(old);
    if (gA99A != old)
        f1547_0991(gA99A);
}

/* 0ACD:09EF: sets the selected action row gF51A from the four-byte table g36D0[old * 4 + dir] and
   redraws the old and new rows through f0BD2_0917. dir is the direction. */
/*@ 0ACD:09EF */ void SetActionRow(int dir)
{
    int old = gF51A;

    gF51A = g36D0[gF51A * 4 + dir];
    f0BD2_0917(old, 1, 0);
    f0BD2_0917(gF51A, 1, 0);
}

/* 0ACD:0A34 draft: the row/character turn dispatcher: does nothing in the rest phase
   (gDCAA == 5), calls Turn091C in mode 0 or SetActionRow in mode 1. Dir is the direction. Our
   build merges case 0's argument cleanup into case 1's (one byte shorter); the original keeps it
   apart, the same tail-merge class as 091C (see its note). */
/*@ 0ACD:0A34 draft */ void Action0A34(int dir)
{
    if (gDCAA == 5)
        return;
    switch (gFB44) {
    case 0:
        Turn091C(dir);
        break;
    case 1:
        SetActionRow(dir);
        break;
    }
}

/* 0ACD:0A61 draft: the list scroll/move handler. Not written yet. */
/*@ 0ACD:0A61 draft */ void Action0A61(int dir)
{
    (void)dir;
}

/* 0ACD:0BA6 draft: the list pick handler. Not written yet. */
/*@ 0ACD:0BA6 draft */ void Action0BA6(void)
{
}

/* 0ACD:0D9D draft: the key dispatcher over the nine-entry table at cs:0E48. Not written yet. */
/*@ 0ACD:0D9D draft */ void Action0D9D(int key)
{
    (void)key;
}

/* 0ACD:0E6C: gets the current list node from 16A5:005A(gList) and dispatches it through 0D9D. */
/*@ 0ACD:0E6C */ void Action0E6C(void)
{
    int key = f16A5_005A(gList);

    Action0D9D(key);
}

/* 0ACD:0E8A draft: the map timeout/encounter loop. Not written yet. Called from the start-up. */
/*@ 0ACD:0E8A draft */ void RunMapLoop(void)
{
}

/* 0ACD:0F72: enters the map screen: shows the inventory when gA998 is set, otherwise 1033; shows
   the pointer, pushes gFBC8 to the overlay front end and clears gA998; then resets the flags and
   the character index, clears gFlag, rebuilds list 1, loads PLAYFLD and INVENT through
   LoadScreenImage (into gSheetDECORATE and gSheet2067), selects the font, clears the status flag,
   redraws the characters, shows the pointer and sets text window 7. */
/*@ 0ACD:0F72 */ void EnterMapScreen(void)
{
    if (!gA998) {
        ResetMapScreen();
    } else {
        if (!gMouseDisabled)
            f07A9_010C(gFBCE);
        fOvl3_0A7C(gFBC8);
        gA998 = 0;
    }
    fOvl4_2039();
    gFlag = 0;
    gA99A = 0;
    gDCAA = 1;
    BuildList1();
    LoadScreenImage("PLAYFLD", (char far *)gSheetDECORATE, 5, 3, 0);
    LoadScreenImage("INVENT", (char far *)gSheet2067, 5, 3, 2);
    f1A47_0169(gA9C0);
    g8115 = 0;
    RedrawCharacters();
    fOvl4_1F51(1);
    LockShowPointer();
    SetTextWindow(7);
}

/* 0ACD:1033: resets the map screen's music and page: selects overlay front end 1, sets the volume
   to 0x1EA and the page to 0, and clears the icon when the mouse is enabled. */
/*@ 0ACD:1033 */ void ResetMapScreen(void)
{
    fOvl3_0A7C(1);
    gSoundVolume = 0x1EA;
    gFBCC = 0;
    if (!gMouseDisabled)
        f07A9_010C(0);
}
