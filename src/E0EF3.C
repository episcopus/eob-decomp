/* Unit at EOB.EXE 0EF3:000E (13219 bytes, 104 functions): the character panel's timed effects and
   its use-item, cast-spell and condition actions. The functions read and change the 243-byte
   character records at ds:F580 (struct CharRec), the current member pointers ds:F51C (a far
   pointer) and ds:DF52, and the per-member byte tables ds:56E7.. that describe an effect record.
   They add effect records through 0EF3:097B, apply a char effect through 1414:05ED (the interval
   computed in ticks, 0x222 per turn), and redraw through 1547:0991 / 0D1C:0583. Names are
   placeholders until the evidence names the game concept. */

/*@flags -ml -O -Z */

/* A 243-byte character record at ds:F580; only the fields this unit reads or writes (src/E0A72.C,
   src/E0BD2.C, src/EOV6.C name overlapping views). */
struct CharRec {
    signed char m00;        /* +00: the member index (0..5) this record is */
    unsigned char m01;      /* +01: status bits */
    char name[0x0B];        /* +02: the character's name, NUL padded */
    signed char m0D;        /* +0D: a class index */
    char pad0E;
    signed char m0F;        /* +0F: a level or score */
    char pad10[0x15 - 0x10];
    signed char m15;        /* +15: an index read by 0A72:019D */
    char pad16[0x1B - 0x16];
    signed char hp;         /* +1B: current hit points */
    signed char maxhp;      /* +1C: maximum hit points */
    signed char m1D;        /* +1D: the armour-class byte */
    signed char m1E;        /* +1E: a condition bit set */
    signed char m1F;        /* +1F */
    signed char m20;        /* +20: an index into a class table */
    char pad21[0x23 - 0x21];
    signed char m23;        /* +23: set to 100 by 0EF3:1DC0 and 0BD2:0D2D */
    signed char m24[0x37 - 0x24]; /* +24: per-class-slot bytes, indexed by a FindClassSlot result */
    signed char known[30];  /* +37: spell slots */
    signed char memorized[30]; /* +55: spell slots */
    char pad73[0x77 - 0x73];
    int m77[27];            /* +77: carried-item slots; slot 0 and 1 hold the equipped items */
    char padAD[0xD5 - 0xAD];
    signed char mD5[10];    /* +D5: ten per-member bytes (spell slots) */
    signed char mDF;        /* +DF */
    signed char mE0;        /* +E0: a countdown */
    char padE1;
    signed char mE2;        /* +E2: hit points pending from a bandage-like effect */
    unsigned long mE3;      /* +E3: effect flags, a 32-bit mask */
    char padE7[0xF3 - 0xE7];
};

/* A 14-byte item record at ds:8DD2 (the same records src/E07A9.C calls Entry14 and src/E0BD2.C
   views two bytes earlier at ds:8DD0). */
struct Entry14 {
    unsigned char flags;    /* +0 (ds:8DD2) */
    signed char icon;       /* +1 */
    signed char m02;        /* +2: the item type */
    char pad03;
    int m04;                /* +4: the key; -2 when the record is freed here */
    char pad06[0x0A - 0x06];
    signed char m0A;        /* +0A */
    signed char m0B;        /* +0B */
    char pad0C[0x0E - 0x0C];
};

/* A 16-byte item-type record at ds:A9F0 (src/E0BD2.C names the same table). */
struct ItemType {
    int m00;                /* +00: a word 0EF3:035A writes from its first argument */
    int m02;                /* +02: a word 0EF3:035A writes from its second argument */
    unsigned char m04;      /* +04: set to 0xE2 when the type is free (FreeItemRecord, 0EF3:035A) */
    signed char m05;        /* +05 */
    signed char m06;        /* +06: cleared by 0EF3:035A */
    signed char m07;        /* +07 */
    signed char m08;        /* +08 */
    signed char m09;        /* +09 */
    signed char m0A;        /* +0A */
    signed char m0B;        /* +0B */
    signed char m0C;        /* +0C */
    char pad0D;
    int m0E;                /* +0E */
};

/* A 14-byte item record at ds:8DD0 (src/E0BD2.C's struct ItemRecord; src/E0D1C.C views the same
   table two bytes in at ds:8DD2 as Entry14). */
struct ItemRecord {
    signed char m00;        /* +00 */
    signed char m01;        /* +01 */
    unsigned char flags;    /* +02: bit 0x20 gates E0BD2.C's branch */
    signed char icon;       /* +03: the icon number */
    signed char m02;        /* +04: the item type */
    signed char m03;        /* +05 */
    int m04;                /* +06: the key; -2 when the record is free (0EF3:0418) */
    int m06;                /* +08: one link of the circular list */
    int m08;                /* +0A: the other link */
    signed char m0A;        /* +0C */
    signed char m0B;        /* +0D: the value 0BD2:01D0 returns */
};

/* The record 0EF3:0A3F and its callers read: an effect target with a member index at +0xB and a
   bit mask at +0xC. Only the fields this unit touches are named. */
struct Tgt {
    signed char m00;        /* +00: set to 0 then 2 by 0EF3:00CD */
    char pad01[3];
    int m04;                /* +04: the target cell, compared with gPartyCell */
    char m06;               /* +06 */
    char m07;               /* +07 */
    int m08;                /* +08: an index into ds:55E6 (0EF3:0A3F compares it with 5) */
    char pad0A;
    signed char m0B;        /* +0B: a member index */
    char m0C;               /* +0C: the effect flag bits */
};

/* The 28-byte object records at ds:8A82 (src/E08E6.C); only the fields this unit touches. */
struct Obj28 {
    signed char b0;         /* +00: an index into the gTable1A3B records */
    signed char kind;       /* +01 */
    int id;                 /* +02 */
    char pad04[0x08 - 0x04];
    signed char type;       /* +08: set to 0x0A by 0EF3:1530 and 0EF3:1A30 */
    char pad09[0x0B - 0x09];
    signed char timer;      /* +0B: set to 0x0F by 0EF3:1530 and 0EF3:1A30 */
    char pad0C[0x1C - 0x0C];
};

/* The 27-byte records at ds:1A3B (src/E08E6.C names it struct Entry27); only +0x0F here. */
struct Entry27 {
    char pad00[2];
    unsigned char m02;      /* +02: an index into the 0A72:0006 table (0EF3:1702) */
    char pad03[0x0F - 0x03];
    long m0F;               /* +0F: tested with 1 by 0EF3:1530 and 2 by 0EF3:1A30 */
    char pad13[0x1B - 0x13];
};

extern struct CharRec gChars[];         /*@ ds:F580 */
extern struct Entry14 gTable8DD2[];     /*@ ds:8DD2 */
extern struct ItemRecord gItems[];      /*@ ds:8DD0 */
extern struct ItemType gItemTypes[];    /*@ ds:A9F0 */
/* The six item-type slots 0x33..0x38 the game creates at runtime live at ds:AD20, the same 96
   bytes src/EOV3.C names gAD20 (its saved-game buffer). */
extern struct ItemType gAD20[];         /*@ ds:AD20 */
extern struct Obj28 gObjs[];            /*@ ds:8A82 */
extern struct Entry27 gTable1A3B[];     /*@ ds:1A3B */

extern struct CharRec far *gCurRec;     /*@ ds:F51C */ /* the far pointer to the current record */
extern signed char gCurMember;          /*@ ds:DCFA */ /* the member an effect targets, redrawn last */
extern signed char gPanelMember;        /*@ ds:DF52 */ /* the member whose panel is open */
extern int g8DCE;                       /*@ ds:8DCE */ /* a condition slot number, nonzero while in use */
extern signed char gF3B8;               /*@ ds:F3B8 */ /* a view byte passed to 0EF3:097B */
extern signed char gEFB8;               /*@ ds:EFB8 */ /* an effect value passed to 0EF3:0302 */
extern signed char g8DD5;               /*@ ds:8DD5 */ /* cleared by 0EF3:0418 */
extern signed char gA999;               /*@ ds:A999 */ /* the member 0EF3:1233 redraws */
extern signed char gA99A;               /*@ ds:A99A */ /* the selected character index */
extern int gFC74;                       /*@ ds:FC74 */ /* saved with gFC76 by 0EF3:026F and restored */
extern int gFC76;                       /*@ ds:FC76 */
extern char gF43D;                      /*@ ds:F43D */
extern unsigned char gFBD2;             /*@ ds:FBD2 */
extern char gFB44;                      /*@ ds:FB44 */
extern int gFBC8;                       /*@ ds:FBC8 */
extern int gFBCC;                       /*@ ds:FBCC */
extern int gPartyCell;                  /*@ ds:FBCA */
extern char gDF4C;                      /*@ ds:DF4C */
extern char gFileNameBuf[];             /*@ ds:FE6E */

extern signed char gAt5717[];          /*@ ds:5717 */ /* a per-member byte: nonzero when the member
                                                     can use melee effects (read by 0EF3:0E1B) */
extern signed char gTable5733[];        /*@ ds:5733 */ /* four cell deltas walked by 0EF3:17D8 */

extern char gText57A3[];                /*@ ds:57A3 */
extern char gText57F1[];                /*@ ds:57F1 */
extern char gText5809[];                /*@ ds:5809 */
extern char gText5830[];                /*@ ds:5830 */
extern char gText587A[];                /*@ ds:587A */
extern char gText58AF[];                /*@ ds:58AF */
extern char gText58BB[];                /*@ ds:58BB */
extern char gText58C6[];                /*@ ds:58C6 */
extern char gText58D1[];                /*@ ds:58D1 */
extern char gText58ED[];                /*@ ds:58ED */

extern void SwapChars(char far *pa, char far *pb); /*@ 082C:07A4 */
extern void RedrawMember(int dir); /*@ 1547:0991 */
extern void RedrawCharacters(void); /*@ 0D1C:0583 */
extern void ApplyCharEffect(char who, long interval, char value, char firstEmpty); /*@ 1414:05ED */
extern void ComputeArmourClass(int who); /*@ 0DE8:0D5E */
extern void HealCharacter(int who, int amount); /*@ 0DE8:0ADB */
extern int far RollDice(char count, char sides, char bonus); /*@ 082C:0458 */
extern int RollToHit(int who, int obj, int slot, int flag); /*@ 0DE8:0007 */
extern void DamageCharacter(int who, int amount); /*@ 0DE8:09B9 */
extern int HalveSaveDamageA(int who, int value); /*@ 0DE8:0F98 */
extern int FindClassSlot(int value, int classIndex); /*@ 0A72:007B */
extern int GetIndexedByte(char index, char limit, char offset); /*@ 0A72:0006 */
extern int GetClassWord(char classIndex); /*@ 0A72:019D */
extern void SayAndBeep(char far *text); /*@ 082C:0B8F */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void WaitTicks(int n); /*@ 082C:0425 */
extern void ClearCharEffectSlot(int who, int what); /*@ 1414:0C2D */
extern int f0D1C_0BAB(int obj); /*@ 0D1C:0BAB */
extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern int f0D1C_02DC(void); /*@ 0D1C:02DC */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern int f0BD2_0106(int who, int row); /*@ 0BD2:0106 */
extern int Obj15B4(char who, int value); /*@ 08E6:15B4 */
extern void DamageObject(struct Obj28 far *obj, int n); /*@ 08E6:0FBE */
extern void ClearObjectType(struct Obj28 far *obj); /*@ 08E6:000B */
extern void Obj17FD(int far *list, int key, int field, char flags, char scanAll, char scanFiltered); /*@ 08E6:17FD */
extern void SetMouseItem(int n); /*@ 07A9:010C */
extern void f1BF3_00A8(int dx, int dy); /*@ 1BF3:00A8 */
extern int sprintf(char far *buf, char far *fmt, ...); /*@ 0000:57AE */
extern void BuildList1(void); /*@ 0CF7:0028 */
extern void BuildList2(void); /*@ 0CF7:00B4 */
extern void BuildList3(void); /*@ 0CF7:0140 */
extern void BuildList5(void); /*@ 0CF7:0211 */

extern signed char g55E6[];      /*@ ds:55E6 */ /* an animation index per Tgt.m08 */
extern void far *gAnimTable[];   /*@ ds:5615 */ /* far-pointer animation templates, indexed by
                                                   0EF3:00CD and passed to 0EF3:2A53 */
extern int gAngleA9C8[];         /*@ ds:A9C8 */ /* 18 words searched for the target cell by 0EF3:00CD
                                                   (the heading table of EOV4.C) */
extern signed char g55EF[];      /*@ ds:55EF */ /* DrawViewSlots start slot per object index */
extern signed char g1A09[];      /*@ ds:1A09 */ /* per-slot object index (EOV4.C) */
extern int g1D61[];              /*@ ds:1D61 */ /* per-slot, per-part x table (EOV4.C) */
extern int g8115;                /*@ ds:8115 */ /* nonzero while a status line is up (E082C.C) */
extern int gDD18;                /*@ ds:DD18 */ /* the animation's origin x, written here and read by
                                                   0EF3:2A53 */
extern int gDD1A;                /*@ ds:DD1A */ /* the animation's origin y */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void ComputeViewSpan(int slot, int far *left, int far *right, int region); /*@ 2116:0052 */
extern void DrawViewFrame(int full); /*@ 2116:005C */
extern void DrawViewSlots(int start); /*@ 2116:0025 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void f0EF3_2A53(int a, int b, int c, int d, int e, void far *p); /*@ 0EF3:2A53 */

/* 0EF3:000E: bubble-sorts each of the ten six-byte rows that start at a character's +0x37 spell
   field, swapping the first two bytes of a row while the row's weight decreases. The weight
   pushes an empty byte (0) to the end (0x50) and a negative byte after the positive ones
   (0x28 plus its magnitude); a row's first two bytes are its sort key. The row pointer is far
   (the caller ovl1:1ACD reaches this unit's DGROUP through a far pointer); `who` is a
   party-member index. Called from overlay 1 (ovl1:1ACD). */
/*@ 0EF3:000E */ void SortSpellSlots(int who)
{
    int i;
    int swapped;
    int j;
    signed char far *pair;
    signed char far *row;
    register int wa;
    register int wb;

    row = gChars[who].known;
    for (i = 0; i < 10; i++) {
        do {
            swapped = 0;
            for (j = 0; j < 5; j++) {
                pair = &row[j];
                wa = *pair;
                wb = pair[1];
                if (wa == 0)
                    wa = 0x50;
                else if (wa < 0)
                    wa = -wa + 0x28;
                if (wb == 0)
                    wb = 0x50;
                else if (wb < 0)
                    wb = -wb + 0x28;
                if (wa > wb) {
                    SwapChars(pair, pair + 1);
                    swapped = 1;
                }
            }
        } while (swapped);
        row += 6;
    }
}

/* 0EF3:00CD: play the melee hit animation on the view slot the target's cell points at. It maps
   the record's cell (+4) to a word index in the 18-word heading table ds:A9C8, rejects the last
   entry and entry 0x0F, then computes the slot's view span. It clears the record's +0 byte,
   redraws a full view frame, sets the status line flag, blanks the view box through the gFillRect
   hook, draws the view slots from g55EF, computes the span again, hides the pointer, sets the
   animation origin (ds:DD18 from g1D61, ds:DD1A = 0x30), plays sound 0x23 and runs the effect
   animation 0EF3:2A53 with the template gAnimTable[g55E6[m08]] chosen per object index. Called by
   the melee attacker 0EF3:0A3F on a hit with the record's flag bit 0x40, and by 1414:0285. */
/*@ 0EF3:00CD */ void AnimateMeleeHit(struct Tgt far *rec)
{
    void far *anim;
    int left;
    int right;
    int cell;
    int index;
    register int slot;
    register int objIndex;

    cell = rec->m04;
    index = g55E6[rec->m08];
    anim = gAnimTable[index];
    for (slot = 0; slot < 0x12; slot++) {
        if (gAngleA9C8[slot] == cell)
            break;
    }
    if (slot > 0x11)
        return;
    if (slot == 0xF)
        return;
    objIndex = g1A09[slot];
    ComputeViewSpan(slot, &left, &right, 5);
    if (right < left)
        return;
    rec->m00 = 0;
    DrawViewFrame(1);
    rec->m00 = 2;
    g8115 = 2;
    gFillRect(0, 0, 0xB0, 0x78, 0);
    DrawViewSlots(g55EF[objIndex]);
    g8115 = 0;
    ComputeViewSpan(slot, &left, &right, 5);
    LockHidePointer();
    gDD18 = g1D61[(slot * 5 + 4) * 2] + 0x58;
    gDD1A = 0x30;
    PlaySound(0x23);
    switch (objIndex) {
    case 0:
        f0EF3_2A53(3, 0x93, 0x23, 0x14, 7, anim);
        break;
    case 1:
        f0EF3_2A53(2, 0x93, 0x23, 0x14, 7, anim);
        break;
    case 2:
        f0EF3_2A53(1, 0x93, 0x23, 0x14, 7, anim);
        break;
    case 3:
        f0EF3_2A53(0, 0x1CC, 0x32, 0x14, 4, anim);
        break;
    }
    LockShowPointer();
}

/* 0EF3:026F: ends one condition slot on the panel member. It clears a bit at index g8DCE in the
   member's +0x1E condition byte, applies a 0x12-tick char effect whose value is the slot number
   plus two, redraws the member, then restores the panel member from ds:FC74, gF43D from ds:FC76
   and the slot number to zero, and rebuilds the active item list (BuildList1 to 3 by gFB44).
   Called by 0EF3:08CF. */
/*@ 0EF3:026F */ void FinishMemberEffect(void)
{
    g8DCE--;
    gChars[gPanelMember].m1E ^= 1 << g8DCE;
    ApplyCharEffect(gPanelMember, 0x12L, g8DCE + 2, 1);
    RedrawMember(gPanelMember);
    gPanelMember = gFC74;
    gF43D = gFC76;
    g8DCE = 0;
    switch (gFB44) {
    case 0:
        BuildList1();
        break;
    case 1:
        BuildList2();
        break;
    case 2:
        BuildList3();
        break;
    }
}

/* 0EF3:0302: apply a timed char effect to the current member: the interval is (count + extra)
   times span ticks, the value is the negated argument. Called by the panel's condition
   wrappers. */
/*@ 0EF3:0302 */ void ApplyTimedCharEffect(int value, int count, long span, int extra, int flag)
{
    long interval;

    interval = count * span + extra * span;
    ApplyCharEffect(gCurMember, interval, -value, flag);
}

/* 0EF3:035A: allocate the first free item-type record: walk the 16-byte records at ds:A9F0 from
   index 0x33 to 0x38 and stop at the first whose +4 type byte is 0xE2 (freed). It then fills the
   record from its arguments (the two words at +0 and +2, the type at +4, the bytes at +5, +7/+0A,
   +8/+0B and +9/+0C, and the word at +0xE) and zeroes +6, returning the record's index. Called by
   the item creators 0EF3:1197, 0EF3:1656 and 0EF3:1D38 to make a new item type. */
/*@ 0EF3:035A */ int CreateItemType(unsigned char word00, unsigned char word02, signed char type,
                                    signed char m05, signed char m0A, signed char m0B,
                                    signed char m0C, int word0E)
{
    struct ItemType far *rec;
    register int index;

    rec = gAD20;
    for (index = 0x33; index < 0x39; index++, rec++) {
        if (rec->m04 == 0xE2)
            break;
    }
    rec->m00 = word00;
    rec->m06 = 0;
    rec->m02 = word02;
    rec->m04 = type;
    rec->m05 = m05;
    rec->m0A = m0A;
    rec->m07 = m0A;
    rec->m0B = m0B;
    rec->m08 = m0B;
    rec->m0C = m0C;
    rec->m09 = m0C;
    rec->m0E = word0E;
    return index;
}

/* 0EF3:03D8: free item record `n`: clear its key to -2, its +0xA byte to -1 and free its type's
   +4 byte (0xE2). Called by 0EF3:1233. */
/*@ 0EF3:03D8 */ void FreeItemRecord(int n)
{
    gTable8DD2[n].m04 = -2;
    gTable8DD2[n].m0A = -1;
    gItemTypes[gTable8DD2[n].m02].m04 = 0xE2;
}

/* 0EF3:0418: allocate a free item record: walk the 14-byte records at ds:8DD0 from index 0x0B to
   0x10 and stop at the first whose +6 key is -2 (freed). It fills the record's flags (+2, from
   arg1 plus 0x20), icon (+3), type (+4) and +0xD byte, clears the global ds:8DD5 and the record's
   +0, +1, +6, +8 and +0xA fields, and returns the record's index (the item id). Called by the item
   creators 0EF3:1197, 0EF3:1656 and 0EF3:1D38. DRAFT: the original keeps the zero it stores in
   AL (`mov al,0; mov [ds:8DD5],al; cwde; ...`), a single char temporary reused for every store;
   our build either stores the immediates directly or keeps the temporary in DL (`make match`
   differs at the first zero store, +0x49). */
/*@ 0EF3:0418 draft */ int CreateItemRecord(unsigned char flags, unsigned char icon, signed char m0B,
                                              signed char type)
{
    signed char zero;
    struct ItemRecord far *rec;
    int index;

    rec = &gItems[0xB];
    for (index = 0xB; index < 0x11; index++, rec++) {
        if (rec->m04 == -2)
            break;
    }
    rec->flags = flags + 0x20;
    rec->icon = icon;
    rec->m02 = type;
    rec->m0B = m0B;
    zero = 0;
    g8DD5 = zero;
    rec->m04 = zero;
    rec->m01 = zero;
    rec->m00 = zero;
    rec->m08 = zero;
    rec->m06 = zero;
    return index;
}

/* 0EF3:0482: returns 1 when the record's carried-item slot 1 is empty, else 0; used to pick the
   slot a created item goes into (callers 0EF3:1197, 0EF3:1656, 0EF3:1D38). */
/*@ 0EF3:0482 */ int ItemSlot1Empty(struct CharRec far *rec)
{
    return rec->m77[1] == 0;
}

/* 0EF3:0498: the character's class-slot 1 value: the byte at +0x24 indexed by the class table
   slot for class 1, or 1 when there is none. While a condition slot is in use (g8DCE) it returns
   9, and for the current member when the action is not yet 7 it returns 5. Called by the panel
   actions. */
/*@ 0EF3:0498 */ int GetClassSlot1Value(int who)
{
    int slot;

    if (g8DCE != 0)
        return 9;
    if (who == -1) {
        if (gFBC8 < 7)
            return 5;
        return 9;
    }
    slot = FindClassSlot(1, gChars[who].m20);
    if (slot > -1)
        return gChars[who].m24[slot];
    return 1;
}

/* 0EF3:04F7: the character's class-slot 2 value, or class-slot 4 value less 8 when that is over
   eight, else 1. Same g8DCE and current-member guards as 0EF3:0498. Called by the panel actions
   and 0DE8:0763. */
/*@ 0EF3:04F7 */ int GetClassSlot24Value(int who)
{
    int slot;

    if (g8DCE != 0)
        return 9;
    if (who == -1) {
        if (gFBC8 < 7)
            return 5;
        return 9;
    }
    slot = FindClassSlot(2, gChars[who].m20);
    if (slot > -1)
        return gChars[who].m24[slot];
    slot = FindClassSlot(4, gChars[who].m20);
    if (slot > -1) {
        if (gChars[who].m24[slot] > 8)
            return gChars[who].m24[slot] - 8;
        return 1;
    }
    return 1;
}

/* 0EF3:059D: refuse the action: say the message at ds:57A3, play sound 0x4F and rebuild the item
   list (BuildList5). Called by the panel action at 0EF3:2676. */
/*@ 0EF3:059D */ void ReportActionRefused(void)
{
    SayAndBeep(gText57A3);
    PlaySound(0x4F);
    BuildList5();
}

/*@ 0EF3:05BD stub */ void f0EF3_05BD(int a, int b) {}

/* 0EF3:0665: run the "flash" animation for one member or the whole party: eight passes over the
   living members (m01 bit 0, and either no panel is open or `who` is the panel member), each
   drawing through 0EF3:05BD and waiting two ticks, then redraw each member. `who` is a member
   index, or -1 for the whole party. Called by 0EF3:08CF and the item effect at 0BD2:0D2D.
   DRAFT: the original loads `who` into SI and compares SI; our build compares the parameter in
   memory (`make match` differs at the first compare, `register int who` makes it worse). */
/*@ 0EF3:0665 draft */ void FlashPartyMembers(int who)
{
    int first;
    int last;
    int pass;
    int i;

    if (who == -1) {
        first = 0;
        last = 5;
    } else {
        first = last = who;
    }
    for (pass = 0; pass < 8; pass++) {
        for (i = first; i <= last; i++) {
            if ((gChars[i].m01 & 1) && (gFB44 == 0 || i == gA99A))
                f0EF3_05BD(i, pass);
        }
        WaitTicks(2);
    }
    for (i = first; i <= last; i++)
        RedrawMember(i);
}

/*@ 0EF3:06E7 stub */ void f0EF3_06E7(void) {}

/*@ 0EF3:08CF stub */ int f0EF3_08CF(void far *rec) {}

/*@ 0EF3:097B stub */ void f0EF3_097B(char a, int b, int c, int d, int e) {}

/*@ 0EF3:0A3F stub */ int f0EF3_0A3F(void far *rec, int a, int b, int c, int n) {}

/*@ 0EF3:0D78 stub */ void f0EF3_0D78(int a, int b) {}

/* 0EF3:0E1B: hit one object with a melee effect: when the panel member has the per-member flag
   ds:5717 cleared, say the string ds:5809 with the member's name; otherwise find the object the
   current page faces (0D1C:00DA and 08E6:15B4). A member with no object gets the string ds:57F1;
   otherwise, when RollToHit accepts the object and 0D1C:0BAB does not reject it, roll count d
   sides plus bonus and damage the object. Called by 0EF3:1B43, 0EF3:1EF3 and 0EF3:1FF9.
   DRAFT: the body is right but the original spills the 0D1C:00DA result to [bp-2] and keeps the
   08E6:15B4 result in SI and the damage in DI; our build puts the 00DA result in DI instead
   (`make match` differs at the branch after the call). */
/*@ 0EF3:0E1B draft */ void CastMeleeEffect(int count, int sides, int bonus)
{
    int target;
    int obj;
    int dmg;

    if (gAt5717[gPanelMember] != 0) {
        target = f0D1C_00DA(gPartyCell, gFBCC);
        obj = Obj15B4(gPanelMember, target);
        if (obj != -1) {
            if (RollToHit(gPanelMember, obj, 0, 1)) {
                dmg = RollDice(count, sides, bonus);
                if (!f0D1C_0BAB(obj))
                    DamageObject(&gObjs[obj], dmg);
            }
        } else {
            sprintf(gFileNameBuf, gText57F1, gChars[gPanelMember].name);
            SayAndBeep(gFileNameBuf);
        }
    } else {
        sprintf(gFileNameBuf, gText5809, gChars[gPanelMember].name);
        SayAndBeep(gFileNameBuf);
    }
}

/* 0EF3:0F0B: when the current record's armour class is more than six above the class word of
   its +0x15 index, raise its +0xDF penalty byte to eight over the member's class-slot 1 value
   and recompute the armour class; otherwise say the string ds:5830 with the record's name. Called
   through a table. DRAFT: the body is right but the original keeps the 0A72:019D result in SI and
   a two-byte frame for the value; our build folds them away (`make match` differs at the frame). */
/*@ 0EF3:0F0B draft */ void FixArmourClass(void)
{
    if (gCurRec->m1D > GetClassWord(gCurRec->m15) + 6) {
        gCurRec->mDF = GetClassSlot1Value(gPanelMember) + 8;
        ComputeArmourClass(gCurMember);
    } else {
        sprintf(gFileNameBuf, gText5830, gCurRec->name);
        SayAndBeep(gFileNameBuf);
    }
}

/* 0EF3:0F86: recompute the armour class of the record's member (its +0 byte). Called by 0DE8:09B9
   and 0EF3:131A. */
/*@ 0EF3:0F86 */ void RecomputeArmourClass(struct CharRec far *rec)
{
    ComputeArmourClass(rec->m00);
}

/*@ 0EF3:0F99 stub */ void f0EF3_0F99(void) {}

/* 0EF3:1069: enter the item-swap mode: set gFBD2 bit 0, take the mouse item, move the pointer, and
   apply a 2-turn effect. Called through a table. */
/*@ 0EF3:1069 */ void EnterSwapMode(void)
{
    gFBD2 |= 1;
    SetMouseItem(f0D1C_02DC());
    f1BF3_00A8(0, 0);
    RedrawCharacters();
    ApplyTimedCharEffect(gEFB8, 0, 0x222L, 2, 1);
}

/* 0EF3:10AE: leave the item-swap mode: clear gFBD2 bit 0, take the mouse item and move the
   pointer. Called through a table. */
/*@ 0EF3:10AE */ void LeaveSwapMode(void)
{
    gFBD2 &= ~1;
    SetMouseItem(f0D1C_02DC());
    f1BF3_00A8(0, 0);
}

/* 0EF3:10D2: add the panel member's effect record of mode 0 through 0EF3:097B. */
/*@ 0EF3:10D2 */ void AddEffectRecord0(void)
{
    f0EF3_097B(gPanelMember, 0, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:10F2: run the record's melee effect through 0EF3:0A3F with 1d4 and half the member's
   class-slot 1 value. */
/*@ 0EF3:10F2 */ void RunMelee1d4(struct Tgt far *rec)
{
    f0EF3_0A3F(rec, 1, 4, 1, (GetClassSlot1Value(rec->m0B) - 1) >> 1);
}

/* 0EF3:1122: set the panel member's effect flag bit 1, recompute the member's armour class and
   apply a 5-turn effect. */
/*@ 0EF3:1122 */ void SetAttackEffect(void)
{
    struct CharRec far *rec;

    rec = &gChars[gPanelMember];
    rec->mE3 |= 2;
    ComputeArmourClass(gPanelMember);
    ApplyTimedCharEffect(gEFB8, 0, 0x222L, 5, 1);
}

/* 0EF3:1176: clear the record's effect flag bit 1 and recompute its member's armour class. Called
   by 0DE8:0D5E. */
/*@ 0EF3:1176 */ void ClearAttackEffect(struct CharRec far *rec)
{
    rec->mE3 &= ~2L;
    ComputeArmourClass(rec->m00);
}

/* 0EF3:1197: give the panel member the item of effect mode 0: 0EF3:035A creates the drawing node,
   0EF3:0418 the item record, and the node's slot is the one 0EF3:0482 picks; then redraw and
   apply a 1-turn effect. Called through a table. DRAFT: the body is right but the original keeps
   the 0482 result in DI and the 035A result in SI with no stack frame; every declaration order
   and `register` form tried spills one local (`make match` shows the difference at the prologue). */
/*@ 0EF3:1197 draft */ void CreateItemForEffect0(void)
{
    register int slot, index;

    index = ItemSlot1Empty(gCurRec);
    slot = CreateItemType(0, 0, 0, 0xF, 1, 8, GetClassSlot1Value(gPanelMember), 1);
    gCurRec->m77[index] = CreateItemRecord(0x10, 0x52, 0, slot);
    RedrawMember(gPanelMember);
    ApplyTimedCharEffect(gEFB8, 0, 0x222L, 1, 1);
}

/* 0EF3:121B: 1 when the item id is 0x0B through 0x11, the range 0EF3:1233 discards. */
/*@ 0EF3:121B */ int IsDiscardedItem(unsigned n)
{
    return n >= 0xB && n <= 0x11;
}

/* 0EF3:1233: for the first of the record's two equipped slots whose item is in the 0x0B..0x11
   range, free the item (0EF3:03D8) and clear the slot, then redraw the member at ds:A999. Called
   by 0EF3:16DC. */
/*@ 0EF3:1233 */ void DiscardEquippedItems(struct CharRec far *rec)
{
    int i;
    int item;

    for (i = 0; i < 2; i++) {
        item = rec->m77[i];
        if (IsDiscardedItem(item)) {
            FreeItemRecord(item);
            rec->m77[i] = 0;
            break;
        }
    }
    RedrawMember(gA999);
}

/* 0EF3:127F: set the current record's effect flag bit 2 and redraw the current member. */
/*@ 0EF3:127F */ void SetEffectFlag4(void)
{
    gCurRec->mE3 |= 4;
    RedrawMember(gCurMember);
}

/* 0EF3:12A1: clear the record's effect flag bit 2 and redraw the current member. Called by
   0DE8:0763 and 0EF3:131A. */
/*@ 0EF3:12A1 */ void ClearEffectFlag4(struct CharRec far *rec)
{
    rec->mE3 &= ~4L;
    RedrawMember(gCurMember);
}

/* 0EF3:12C2: add the panel member's effect record of mode 1 through 0EF3:097B. */
/*@ 0EF3:12C2 */ void AddEffectRecord1(void)
{
    f0EF3_097B(gPanelMember, 1, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:12E3: run the record's melee effect through 0EF3:0A3F with 2d4 and a third of the
   member's class-slot 1 value. */
/*@ 0EF3:12E3 */ void RunMelee2d4(struct Tgt far *rec)
{
    f0EF3_0A3F(rec, 2, 4, 0, GetClassSlot1Value(rec->m0B) / 3);
}

/* 0EF3:1315: an empty function reached through a table; nothing to do. */
/*@ 0EF3:1315 */ void EmptyFunction1315(void) {}

/*@ 0EF3:131A stub */ void f0EF3_131A(void) {}

/* 0EF3:13FD: add the panel member's effect record of mode 2 through 0EF3:097B. */
/*@ 0EF3:13FD */ void AddEffectRecord2(void)
{
    f0EF3_097B(gPanelMember, 2, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:141E: run the record's 1d6 melee effect through 0EF3:0A3F. */
/*@ 0EF3:141E */ void RunMelee1d6(struct Tgt far *rec)
{
    f0EF3_0A3F(rec, 1, 6, 0, GetClassSlot1Value(rec->m0B));
}

/* 0EF3:144A: add the panel member's effect record of mode 3 through 0EF3:097B. */
/*@ 0EF3:144A */ void AddEffectRecord3(void)
{
    f0EF3_097B(gPanelMember, 3, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:146B: run the record's 5d6 melee effect through 0EF3:0A3F. */
/*@ 0EF3:146B */ void RunMelee5d6(struct Tgt far *rec)
{
    f0EF3_0A3F(rec, 5, 6, 0, GetClassSlot1Value(rec->m0B));
}

/* 0EF3:1497: mark every living member with effect flag bit 3, set the current member to each in
   turn and apply a 3-turn effect. Called through a table. */
/*@ 0EF3:1497 */ void MarkLivingMembers(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        if (gChars[i].m01 & 1) {
            gChars[i].mE3 |= 8L;
            gCurMember = i;
            ApplyTimedCharEffect(gEFB8, 3, 0x222L, 1, 1);
        }
    }
}

/* 0EF3:14F9: clear the record's effect flag bit 3 (value 8) and leave the member alone. */
/*@ 0EF3:14F9 */ void ClearEffectFlag8(struct CharRec far *rec)
{
    rec->mE3 &= ~8L;
}

/* 0EF3:150F: add the panel member's effect record of mode 4 through 0EF3:097B. */
/*@ 0EF3:150F */ void AddEffectRecord4(void)
{
    f0EF3_097B(gPanelMember, 4, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:1530: list the objects that match the record's cell and field with Obj17FD, then walk the
   list: for every object 0D1C:0BAB does not reject, remember that one was found and, when the
   object's character record has bit 0 of +0x0F set, set its type byte to 0x0A and its timer to
   0x0F. Returns 1 when any object was listed. Called through a table. */
/*@ 0EF3:1530 */ int UpdateCellObjectsBit1(struct Tgt far *rec)
{
    int buffer[5];
    int far *list;
    int found;
    struct Obj28 far *obj;

    found = 0;
    list = buffer;
    Obj17FD(list, rec->m04, rec->m06, rec->m07, 1, 1);
    for (; *list != -1; ) {
        if (f0D1C_0BAB(*list) == 0) {
            obj = &gObjs[*list];
            list++;
            found = 1;
            if (gTable1A3B[obj->b0].m0F & 1) {
                obj->type = 0x0A;
                obj->timer = 0x0F;
            }
        } else {
            list++;
        }
    }
    return found;
}

/* 0EF3:15D0: set every member's effect flag bit 2 and redraw the party rows. */
/*@ 0EF3:15D0 */ void MarkAllMembersFlag4(void)
{
    struct CharRec far *rec;
    int i;

    rec = gChars;
    for (i = 0; i < 6; i++, rec++) {
        rec->mE3 |= 4L;
    }
    RedrawCharacters();
}

/* 0EF3:1609: add the panel member's effect record of mode 5 through 0EF3:097B. */
/*@ 0EF3:1609 */ void AddEffectRecord5(void)
{
    f0EF3_097B(gPanelMember, 5, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:162A: run the record's 1d6 melee effect through 0EF3:0A3F. */
/*@ 0EF3:162A */ void RunMelee1d6B(struct Tgt far *rec)
{
    f0EF3_0A3F(rec, 1, 6, 0, GetClassSlot1Value(rec->m0B));
}

/* 0EF3:1656: give the panel member the item of effect mode 6, like 0EF3:1197 but with the item
   record at a different row. Called through a table. DRAFT: same register-allocation difference
   as 0EF3:1197 at the prologue. */
/*@ 0EF3:1656 draft */ void CreateItemForEffect6(void)
{
    register int slot;
    register int index;

    index = ItemSlot1Empty(gCurRec);
    slot = CreateItemType(0, 0, 0, 0xF, GetClassSlot1Value(gPanelMember) / 2, 6, 0, 1);
    gCurRec->m77[index] = CreateItemRecord(0x18, 0x53, 0, slot);
    RedrawMember(gPanelMember);
    ApplyTimedCharEffect(gEFB8, 0, 0x222L, 1, 1);
}

/* 0EF3:16DC: clamp the record's hit points to its maximum, then run the equipped-item clear of
   0EF3:1233. */
/*@ 0EF3:16DC */ void ClampHitPoints(struct CharRec far *rec)
{
    if (rec->hp > rec->maxhp)
        rec->hp = rec->maxhp;
    DiscardEquippedItems(rec);
}

/* 0EF3:1702: after 0EF3:06E7, walk the 30 objects and for each whose id is the object the current
   page faces (0D1C:00DA), whose character record's +0x0F long has bit 0 or 1 and which 0D1C:0BAB
   does not reject, roll 1d20 and clear the object's type when the roll is under the value
   0A72:0006 gives for the character record's +2 index. Called by the wand/turn-undead paths.
   DRAFT: the body is right but the original spills `target` to the frame (0x0A bytes); our build
   keeps it in a register (`make match` differs at sub sp). */
/*@ 0EF3:1702 draft */ void ClearObjectsFacingCell(void)
{
    struct Entry27 far *rec;
    struct Obj28 far *obj;
    int target;
    int i;

    f0EF3_06E7();
    target = f0D1C_00DA(gPartyCell, gFBCC);
    obj = gObjs;
    for (i = 0; i < 0x1E; i++, obj++) {
        rec = &gTable1A3B[obj->b0];
        if (obj->id == target && (rec->m0F & 3) != 0 && f0D1C_0BAB(i) == 0) {
            if (RollDice(1, 0x14, 0) < GetIndexedByte(0, rec->m02, 4))
                ClearObjectType(obj);
        }
    }
}

/* 0EF3:17B7: add the panel member's effect record of mode 6 through 0EF3:097B. */
/*@ 0EF3:17B7 */ void AddEffectRecord6(void)
{
    f0EF3_097B(gPanelMember, 6, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:17D8: run the record's 1d6 melee effect, then for each of the four cell deltas at ds:5733
   add the delta to the record's cell (masked to 0x3FF), run the effect again and restore the
   cell. Returns the first run's result. */
/*@ 0EF3:17D8 */ int RunMeleeAroundCell(struct Tgt far *rec)
{
    int found;
    int i;
    int cell;

    found = f0EF3_0A3F(rec, 1, 6, 0, GetClassSlot1Value(rec->m0B));
    if (found) {
        for (i = 0; i < 4; i++) {
            cell = rec->m04;
            rec->m04 += gTable5733[i];
            rec->m04 &= 0x3FF;
            f0EF3_0A3F(rec, 1, 6, 0, GetClassSlot1Value(rec->m0B));
            rec->m04 = cell;
        }
    }
    return found;
}

/* 0EF3:1865: roll 1d4 plus half the panel member's class-slot 1 value into the current record's
   +0xE0 countdown. */
/*@ 0EF3:1865 */ void SetBandageCountdown(void)
{
    gCurRec->mE0 = RollDice(1, 4, 0) + (GetClassSlot1Value(gPanelMember) >> 1);
}

/* 0EF3:1894: clear the record's +0xE0 countdown. Called by 0EF3:18A2. */
/*@ 0EF3:1894 */ void ClearBandageCountdown(struct CharRec far *rec)
{
    rec->mE0 = 0;
}

/* 0EF3:18A2: tick the record's +0xE0 countdown: return 0 when it is already zero, else decrement
   it and, when it reaches zero, clear it through 0EF3:1894; return 1. Called from 08E6:10F3. */
/*@ 0EF3:18A2 */ int TickBandageCountdown(struct CharRec far *rec)
{
    if (rec->mE0 != 0) {
        if (--rec->mE0 == 0)
            ClearBandageCountdown(rec);
        return 1;
    }
    return 0;
}

/* 0EF3:18D3: an empty function reached through a table; nothing to do. */
/*@ 0EF3:18D3 */ void EmptyFunction18D3(void) {}

/*@ 0EF3:18D8 stub */ void f0EF3_18D8(void) {}

/* 0EF3:1A0F: add the panel member's effect record of mode 7 through 0EF3:097B. */
/*@ 0EF3:1A0F */ void AddEffectRecord7(void)
{
    f0EF3_097B(gPanelMember, 7, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:1A30: list the objects that match the record's cell and field with Obj17FD, then walk the
   list exactly as 0EF3:1530 does, but testing bit 1 of the object's character record +0x0F and
   setting its type byte to 0x0A and its timer to 0x0F. Returns 1 when any object was listed.
   Called through a table. */
/*@ 0EF3:1A30 */ int UpdateCellObjectsBit2(struct Tgt far *rec)
{
    int buffer[5];
    int far *list;
    int found;
    struct Obj28 far *obj;

    found = 0;
    list = buffer;
    Obj17FD(list, rec->m04, rec->m06, rec->m07, 1, 1);
    for (; *list != -1; ) {
        if (f0D1C_0BAB(*list) == 0) {
            obj = &gObjs[*list];
            list++;
            found = 1;
            if (gTable1A3B[obj->b0].m0F & 2) {
                obj->type = 0x0A;
                obj->timer = 0x0F;
            }
        } else {
            list++;
        }
    }
    return found;
}

/* 0EF3:1AD0: toggle the swap mode: when gFBD2 bit 4 is clear apply a 6-turn effect, set the bit
   and redraw the party rows; otherwise say the message at ds:587A. Called through a table. */
/*@ 0EF3:1AD0 */ void ToggleSwapFlag(void)
{
    if ((gFBD2 & 0x10) == 0) {
        ApplyTimedCharEffect(gEFB8, 6, 0x222L, 0, 1);
        gFBD2 |= 0x10;
        RedrawCharacters();
    } else {
        SayAndBeep(gText587A);
    }
}

/* 0EF3:1B13: clear bit 4 of the display byte gFBD2; the swap mode is off again. */
/*@ 0EF3:1B13 */ void ClearSwapFlag(void)
{
    gFBD2 &= ~0x10;
}

/* 0EF3:1B20: heal the current member by 1d8. Called through a table. */
/*@ 0EF3:1B20 */ void HealCurrent1d8(void)
{
    HealCharacter(gCurMember, RollDice(1, 8, 0));
}

/* 0EF3:1B43: apply the 1d8 melee effect at 0EF3:0E1B. Called through a table. */
/*@ 0EF3:1B43 */ void CastMelee1d8(void)
{
    CastMeleeEffect(1, 8, 0);
}

/* 0EF3:1B5A: set the current record's effect flag bit 0x200 and apply a 3-turn effect. */
/*@ 0EF3:1B5A */ void SetEffectFlag200(void)
{
    gCurRec->mE3 |= 0x200L;
    ApplyTimedCharEffect(gEFB8, 0, 0x222L, 3, 1);
}

/* 0EF3:1B8D: clear the record's effect flag bit 0x200, the marker 0EF3:1B5A sets. */
/*@ 0EF3:1B8D */ void ClearEffectFlag200(struct CharRec far *rec)
{
    rec->mE3 &= ~0x200L;
}

/*@ 0EF3:1BA3 stub */ void f0EF3_1BA3(void) {}

/* 0EF3:1C45: undo a bandage-like effect on the record: when its +0xE2 byte is set subtract it
   from the hit points and clear it, then clear the record's effect flag bit 0x40. Called by
   0BD2:0D2D and 0EF3:1F0B. */
/*@ 0EF3:1C45 */ void UndoBandage(struct CharRec far *rec)
{
    if (rec->mE2 != 0) {
        rec->hp -= rec->mE2;
        rec->mE2 = 0;
    }
    rec->mE3 &= ~0x40L;
}

/* 0EF3:1C75: when the current member's m01 bit 1 is set, set the current record's effect flag bit
   0x80 and apply a 0x7FF8-tick effect; otherwise say the message at ds:58AF. Called through a
   table. */
/*@ 0EF3:1C75 */ void ApplyConditionEffect(void)
{
    if (gChars[gCurMember].m01 & 2) {
        gCurRec->mE3 |= 0x80L;
        ApplyTimedCharEffect(gEFB8, 1, 0x7FF8L, GetClassSlot24Value(gPanelMember), 1);
    } else {
        DrawText(gText58AF);
    }
}

/* 0EF3:1CD2: clear the record's effect flag bit 0x80. */
/*@ 0EF3:1CD2 */ void ClearEffectFlag80(struct CharRec far *rec)
{
    rec->mE3 &= ~0x80L;
}

/* 0EF3:1CE8: clear a member's m01 bit 1 and effect flag bit 0x80, clear its effect slot 0x22 and
   redraw it. Called by 0EF3:1F0B and 0BD2:0D2D. */
/*@ 0EF3:1CE8 */ void ClearPoisonEffect(int who)
{
    gChars[who].m01 &= ~2;
    gChars[who].mE3 &= ~0x80L;
    ClearCharEffectSlot(who, -0x22);
    RedrawMember(who);
}

/* 0EF3:1D38: give the panel member the item of effect mode 4, like 0EF3:1197 but with the item
   record at a different row. Called through a table. DRAFT: same register-allocation difference
   as 0EF3:1197 at the prologue. */
/*@ 0EF3:1D38 draft */ void CreateItemForEffect4(void)
{
    register int slot;
    register int index;

    index = ItemSlot1Empty(gCurRec);
    slot = CreateItemType(0, 0, 0, 0xF, 1, 4, 4, 1);
    gCurRec->m77[index] = CreateItemRecord(0, 0x54, 0, slot);
    RedrawMember(gPanelMember);
    ApplyTimedCharEffect(gEFB8, 4, 0x222L, GetClassSlot24Value(gPanelMember) / 2, 1);
}

/* 0EF3:1DC0: set every member's +0x23 level byte to 100. */
/*@ 0EF3:1DC0 */ void SetMemberLevels(void)
{
    int i;

    for (i = 0; i < 6; i++)
        gChars[i].m23 = 100;
}

/* 0EF3:1DDF: set the current record's effect flag bit 0x100, recompute the panel member's armour
   class and apply a 5-turn effect. */
/*@ 0EF3:1DDF */ void SetEffectFlag100(void)
{
    gCurRec->mE3 |= 0x100L;
    ComputeArmourClass(gPanelMember);
    ApplyTimedCharEffect(gEFB8, 0, 0x222L, 5, 1);
}

/* 0EF3:1E1D: clear the record's effect flag bit 0x100 and recompute the panel member's armour
   class. */
/*@ 0EF3:1E1D */ void ClearEffectFlag100(struct CharRec far *rec)
{
    rec->mE3 &= ~0x100L;
    ComputeArmourClass(gPanelMember);
}

/* 0EF3:1E3E: set gFBD2 bit 5, apply a 2-turn effect and redraw the party rows. */
/*@ 0EF3:1E3E */ void SetSwapFlag20(void)
{
    gFBD2 |= 0x20;
    ApplyTimedCharEffect(gEFB8, 0, 0x222L, 2, 1);
    RedrawCharacters();
}

/* 0EF3:1E6C: clear gFBD2 bit 5 and redraw the party rows. */
/*@ 0EF3:1E6C */ void ClearSwapFlag20(void)
{
    gFBD2 &= ~0x20;
    RedrawCharacters();
}

/* 0EF3:1E7E: clear m01 bit 2 on at most the first four members that have it and redraw the party
   rows. */
/*@ 0EF3:1E7E */ void ClearParalysisFromFour(void)
{
    int remaining;
    int i;

    remaining = 4;
    for (i = 0; i < 6; i++) {
        if ((gChars[i].m01 & 4) && remaining) {
            gChars[i].m01 &= ~4;
            remaining--;
        }
    }
    RedrawCharacters();
}

/* 0EF3:1ECC: heal the current member by 2d8+1. Called through a table. */
/*@ 0EF3:1ECC */ void HealCurrent2d8(void)
{
    int amount;

    amount = RollDice(2, 8, 1);
    HealCharacter(gCurMember, amount);
}

/* 0EF3:1EF3: apply the 2d8+1 melee effect at 0EF3:0E1B. Called through a table. */
/*@ 0EF3:1EF3 */ void CastMelee2d8(void)
{
    CastMeleeEffect(2, 8, 1);
}

/* 0EF3:1F0B: when the current record is alive (m01 nonzero) clear its poison effect through
   0EF3:1CE8, else say the message at ds:58BB. */
/*@ 0EF3:1F0B */ void CureOrReportPoison(void)
{
    if (gCurRec->m01 != 0)
        ClearPoisonEffect(gCurMember);
    else
        SayAndBeep(gText58BB);
}

/* 0EF3:1F33: set every member's effect flag bit 0x200, apply a 0x1554-tick effect whose value is
   the panel member's class-slot 2/4 value, and redraw the party rows. Called through a table. */
/*@ 0EF3:1F33 */ void MarkAllFlag200(void)
{
    struct CharRec far *rec;
    int i;

    rec = gChars;
    for (i = 0; i < 6; i++, rec++) {
        rec->mE3 |= 0x200L;
    }
    ApplyTimedCharEffect(gEFB8, 0, 0x1554L, GetClassSlot24Value(gPanelMember), 1);
    RedrawCharacters();
}

/* 0EF3:1F8F: clear every member's effect flag bit 0x200 and redraw the party rows. */
/*@ 0EF3:1F8F */ void ClearAllFlag200(void)
{
    struct CharRec far *rec;
    int i;

    rec = gChars;
    for (i = 0; i < 6; i++, rec++) {
        rec->mE3 &= ~0x200L;
    }
    RedrawCharacters();
}

/* 0EF3:1FC8: an empty function reached through a table; nothing to do. */
/*@ 0EF3:1FC8 */ void EmptyFunction1FC8(void) {}

/* 0EF3:1FCD: an empty function reached through a table; nothing to do. */
/*@ 0EF3:1FCD */ void EmptyFunction1FCD(void) {}

/* 0EF3:1FD2: heal the current member by 3d8+3. Called through a table. */
/*@ 0EF3:1FD2 */ void HealCurrent3d8(void)
{
    int amount;

    amount = RollDice(3, 8, 3);
    HealCharacter(gCurMember, amount);
}

/* 0EF3:1FF9: apply the 3d8+3 melee effect at 0EF3:0E1B. Called through a table. */
/*@ 0EF3:1FF9 */ void CastMelee3d8(void)
{
    CastMeleeEffect(3, 8, 3);
}

/* 0EF3:2011: add the panel member's effect record of mode 8 through 0EF3:097B. */
/*@ 0EF3:2011 */ void AddEffectRecord8(void)
{
    f0EF3_097B(gPanelMember, 8, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:2032: run the record's 6d8 melee effect through 0EF3:0A3F. */
/*@ 0EF3:2032 */ void RunMelee6d8(struct Tgt far *rec)
{
    f0EF3_0A3F(rec, 6, 8, 0, 0);
}

/* 0EF3:2050: when the current record is at -10 hit points and its +0x1F byte over two is not
   exactly one, set it to 1 and redraw the member; otherwise say the message at ds:58C6. */
/*@ 0EF3:2050 */ void ReviveAtMinusTen(void)
{
    if (gCurRec->hp == -10 && (gCurRec->m1F >> 1) != 1) {
        gCurRec->hp = 1;
        RedrawMember(gCurMember);
    } else {
        SayAndBeep(gText58C6);
    }
}

/* 0EF3:208A: run the record's melee effect through 0EF3:0A3F with 0d0 and a bonus of 0xC. Called
   through a table. DRAFT: the body is right but the original keeps the 0A72:019D result in SI and
   a two-byte frame for the value; our build folds them away (`make match` differs at the frame). */
/*@ 0EF3:208A */ void RunMelee0(struct Tgt far *rec)
{
    f0EF3_0A3F(rec, 0, 0, 0xC, 1);
}

/* 0EF3:20A8: the "damage one party member" effect: when the record's cell is the party cell, roll
   1d6 and walk up to six members from that roll; the first member not protected (0D1C:0670 with
   mask 3) that fails its save takes 100 damage and is announced with the string ds:58D1; always
   return 1 after a hit or a full walk (a start of 6 reads the record after the party, a QUIRK
   0EF3:20A8). Called through a table. */
/*@ 0EF3:20A8 */ int DamageOneMember(struct Tgt far *rec)
{
    int tries;
    int who;

    if (rec->m04 == gPartyCell) {
        who = RollDice(1, 6, 0);
        for (tries = 0; tries < 6; tries++) {
            if (f0D1C_0670(who, 3) && HalveSaveDamageA(who, 1) == 0) {
                DamageCharacter(who, 0x64);
                sprintf(gFileNameBuf, gText58D1, gChars[who].name);
                DrawText(gFileNameBuf);
                break;
            }
            who++;
            if (who >= 6)
                who = 0;
        }
        return 1;
    }
    return 0;
}

/* 0EF3:2145: the "damage the party" effect: when the record's cell is the party cell, say the
   string ds:58ED, roll 1d4 and damage that many members with 100 points, skipping the members
   0D1C:0670 protects (mask 3) and those whose +0x24 class-slot 0 value is 8 or more. Returns 1
   when the cell matched, else 0. Called through a table. */
/*@ 0EF3:2145 */ int DamageParty(struct Tgt far *rec)
{
    int count;
    int i;

    if (rec->m04 == gPartyCell) {
        DrawText(gText58ED);
        count = RollDice(1, 4, 0);
        for (i = 0; i < 6; i++) {
            if (f0D1C_0670(i, 3) && gChars[i].m24[0] < 8) {
                DamageCharacter(i, 0x64);
                count--;
            }
            if (count == 0)
                break;
        }
        return 1;
    }
    return 0;
}

/* 0EF3:21BC: heal the current member by twice the panel member's +0x24 class-slot 0 value. */
/*@ 0EF3:21BC */ void HealFromClassSlot(void)
{
    HealCharacter(gCurMember, gChars[gPanelMember].m24[0] * 2);
}

/*@ 0EF3:21E0 stub */ void f0EF3_21E0(int who, int flag) {}

/*@ 0EF3:2295 stub */ void f0EF3_2295(void) {}

/*@ 0EF3:2541 stub */ void f0EF3_2541(char far *text) {}

/*@ 0EF3:25BD stub */ void f0EF3_25BD(void) {}

/* 0EF3:2604: use the item in one of a member's rows. When the item's type is 0x22 or 0x23, or
   its type is 0x30 and its flags low six bits are exactly 1, run 0BD2:0106(who, slot); for a
   type 0x30 item with any other flag count, just decrement its flags. Called through a table.
   DRAFT: the body is right but the original computes who*0xF3 before reading slot and keeps the
   item in SI; our build caches slot in DI first (`make match` differs at the first instruction). */
/*@ 0EF3:2604 draft */ void UseItemRow(int who, int slot)
{
    int item;

    item = gChars[who].m77[slot];
    if (gTable8DD2[item].m02 == 0x22 || gTable8DD2[item].m02 == 0x23) {
        f0BD2_0106(who, slot);
    } else if (gTable8DD2[item].m02 == 0x30) {
        if ((gTable8DD2[item].flags & 0x3F) == 1)
            f0BD2_0106(who, slot);
        else
            gTable8DD2[item].flags--;
    }
}

/*@ 0EF3:2676 stub */ void f0EF3_2676(int a, int b) {}

/* 0EF3:28ED: save the panel member in ds:FC74 and gF43D in ds:FC76, start a condition slot
   (g8DCE = c + 1), make `a` the panel member, set gF43D when `b` is over 0x19, and run the panel
   action 0EF3:2676(b, c) that uses them. Called by the item-use and turn-undead paths. */
/*@ 0EF3:28ED */ void StartPanelAction(char who, int action, int kind)
{
    gFC74 = gPanelMember;
    gFC76 = gF43D;
    g8DCE = kind + 1;
    gPanelMember = who;
    gF43D = (action > 0x19);
    f0EF3_2676(action, kind);
}

/*@ 0EF3:2929 stub */ void f0EF3_2929(void) {}

/*@ 0EF3:2A53 stub */ void f0EF3_2A53(int a, int b, int c, int d, int e, void far *p) {}

/*@ 0EF3:2EBC stub */ void f0EF3_2EBC(void) {}
