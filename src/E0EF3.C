/* Unit at EOB.EXE 0EF3:000E (13219 bytes, 104 functions): the character panel's timed effects and
   its use-item, cast-spell and condition actions. The functions read and change the 243-byte
   character records at ds:F580 (struct CharRec), the current member pointers ds:F51C (a far
   pointer) and ds:DF52, and the per-member byte tables ds:56E7.. that describe an effect record.
   They add effect records through 0EF3:097B, apply a char effect through 1414:05ED (the interval
   computed in ticks, 0x222 per turn), and redraw through 1547:0991 / 0D1C:0583. Names are
   placeholders until the evidence names the game concept. */

/*@flags -ml -O -Z -v */

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
    long mAD[10];           /* +AD: a 32-bit value per spell slot, cleared when its spell is removed */
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

/* One of the five spell cells 0EF3:2295 fills, at ds:5773 (four bytes: two corners). */
struct CellRect {
    unsigned char x1;
    unsigned char y1;
    unsigned char x2;
    unsigned char y2;
};

/* The 18-byte effect records at ds:DD2C, ten of them (src/E1414.C and src/E122E.C name the same
   records and the callback at +0x0E). */
struct DDRec {
    char active;             /* +00: 1 when the record is live, 2 runs fn (1414:0172) */
    char pad01;
    int m02;                 /* +02 */
    int m04;                 /* +04: a character or object index */
    char m06;                /* +06 */
    signed char m07;         /* +07 */
    int m08;                 /* +08: an index into the 14-byte records at ds:8DD2 */
    char m0A;                /* +0A */
    char m0B;                /* +0B */
    char m0C;                /* +0C: flags, bit 0x20 tested by 1414:0285 */
    unsigned char m0D;       /* +0D */
    int (far *fn)(struct DDRec far *rec); /* +0E: called by 1414:0172 when active is 2 */
};

/* The 0x13-byte spell records at ds:1344 (src/EOV6.C names the table gSpells1344 and its +0 a far
   name pointer). This unit reads the apply callback at +5 (through gFnEFBA), the DD callback at
   +9 (copied into a DDRec.fn by 0EF3:097B) and the expire callback at +0xD (run by 0EF3:0D78 and
   0EF3:131A); the sound id at +0x11 is ds:1355, gEffectSound. */
struct Spell {
    char *name;                             /* +00: the spell's name */
    signed char flags;                      /* +04: bits 0..2 and 3 tested by 0EF3:0D78, bit 1 by 0EF3:2676 */
    void (far *apply)(void);                /* +05: run through gFnEFBA when the spell is cast */
    int (far *ddFn)(struct DDRec far *rec); /* +09: copied into DDRec.fn by 0EF3:097B */
    void (far *remove)(struct CharRec far *rec); /* +0D: run when the spell expires */
    signed char sound;                      /* +11: the sound id (ds:1355) */
    signed char m12;                        /* +12: bit 0 gates 0EF3:2676's first flag clear */
};

/* The per-mode table at ds:56E7, four bytes per mode (0EF3:097B): the index of the mode's spell
   in gSpells1344 and three bytes copied into a DDRec. */
struct EffectMode {
    signed char spell;       /* +00: an index into gSpells1344 */
    signed char m01;         /* +01 -> DDRec.m0D */
    signed char m02;         /* +02 -> DDRec.m0C */
    signed char m03;         /* +03 -> DDRec.m0A */
};

extern struct DDRec gDD2C[];            /*@ ds:DD2C */ /* ten 18-byte records */
extern struct Spell gSpells1344[];      /*@ ds:1344 */ /* 0x13-byte spell records */
extern struct EffectMode gEffectModes[]; /*@ ds:56E7 */ /* four bytes per effect mode */

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
extern int gF518;                       /*@ ds:F518 */ /* set while 08E6:0E48 has not run */
extern unsigned char gFBD2;             /*@ ds:FBD2 */
extern char gFB44;                      /*@ ds:FB44 */
extern int gFBC8;                       /*@ ds:FBC8 */
extern int gFBCC;                       /*@ ds:FBCC */
extern int gPartyCell;                  /*@ ds:FBCA */
extern char gDF4C;                      /*@ ds:DF4C */
extern char gFileNameBuf[];             /*@ ds:FE6E */
extern unsigned g5E1C;                  /*@ ds:5E1C */ /* a button/mode word (src/E0ACD.C) */
extern char gF542[];                    /*@ ds:F542 */ /* a message buffer passed to RedrawPartyPanel */
extern char far *g5763[];               /*@ ds:5763 */ /* far byte tables, one per page gFBCC */
extern int gText5737[];                 /*@ ds:5737 */ /* a word table of animation frames 0EF3:2EBC
                                                        indexes with its frame counter */
extern void Obj0E48(void);              /*@ 08E6:0E48 */ /* clears the object marks (src/E08E6.C) */
extern int ClampOrWrap(int value, int low, int high, int invert); /*@ 082C:0AB4 */
extern signed char gAt5717[];          /*@ ds:5717 */ /* a per-member byte: nonzero when the member
                                                     can use melee effects (read by 0EF3:0E1B) */
extern signed char gTable5733[];        /*@ ds:5733 */ /* four cell deltas walked by 0EF3:17D8 */
extern char gB894[];                    /*@ ds:B894 */ /* nine-byte records; +4 (ds:B898) is the
                                                       object's type byte tested by 0EF3:0A3F */
extern signed char g1A2B[];             /*@ ds:1A2B */ /* per-page sprite base (src/E0DE8.C) */
extern signed char gIconIndex571B[];    /*@ ds:571B */ /* a per-member icon index; a negative value
                                                           picks the 004D region drawing (0EF3:0F99) */
extern int gIconX5721[];                /*@ ds:5721 */ /* the icon's x, per member (0EF3:0F99) */
extern signed char gIconY572D[];        /*@ ds:572D */ /* the icon's y, per member (0EF3:0F99) */
extern void far *gIconTable[];          /*@ ds:FB34 */ /* far sprite pointers indexed by an icon
                                                           magnitude; entry 1 is ds:FB38 (src/EOV2.C
                                                           names the entries gThrownIcon0..2 singly) */
extern void DrawRegion000F(int mode, void far *src, int x, int y, int region); /*@ 082C:063B */
extern void DrawRegion004D(int mode, void far *src, int x, int y, int region); /*@ 082C:0681 */
extern void Obj16D6(int key, char count, char sides, char bonus, char flag); /*@ 08E6:16D6 */
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 19F3:000E */

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
extern char gText57C3[];                /*@ ds:57C3 */ /* drawn by 0EF3:08CF */
extern char gText57C8[];                /*@ ds:57C8 */ /* said by 0EF3:08CF when no member is set */
extern char gText57D9[];                /*@ ds:57D9 */ /* "%s's %s spell expires." (0EF3:0D78) */
extern char gText5859[];                /*@ ds:5859 */ /* "All spells on %s are dispelled." (0EF3:131A) */
extern char gText5899[];                /*@ ds:5899 */ /* "The aid spell fails!" (0EF3:1BA3) */
extern signed char gEffectSound[];      /*@ ds:1355 */ /* one record per effect value, stride 0x13;
                                                           the first byte is a sound id (0EF3:08CF) */
extern void (far *gFnEFBA)(void);       /*@ ds:EFBA */ /* a redraw hook run by 0EF3:08CF */
extern char gFlag;                      /*@ ds:F3B9 */ /* cleared by 0EF3:2541 (E0ACD.C) */
extern void far *gSegDF4E;              /*@ ds:DF4E */ /* the slot-5 buffer (E07A9.C) */
extern void (far *gRestoreRect)(int x, int y, int width, int height, void far *buf); /*@ ds:804E */
extern void (far *gFn804A)(int x, int y, int width, int height, void far *buf); /*@ ds:804A */
extern void (far *gSaveRect)(int x, int y, int width, int height, void far *buf); /*@ ds:80AE */
extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void);      /*@ 1BBE:02C2 */
extern void DrawViewRows(void);         /*@ 2116:0034 */ /* overlay 4 entry, ovl4:1EDE */
extern char far *gDD14;                 /*@ ds:DD14 */ /* a far byte table 0EF3:25BD scans */
extern signed char gDDE2;               /*@ ds:DDE2 */ /* the member 0EF3:25BD saves */
extern signed char gA9C5;               /*@ ds:A9C5 */ /* a counter 0EF3:25BD walks */
extern char far *gNameTable;            /*@ ds:EFCC */ /* the name table 0EF3:2295 reads, set to
                                                          g1278 or g12E0 by 0EF3:21E0 */
extern char far *g1278[];               /*@ ds:1278 */ /* far name pointers (src/E0BD2.C) */
extern char far *g12E0[];               /*@ ds:12E0 */ /* far name pointers (src/E0BD2.C) */
extern struct CellRect gSpellCells[];   /*@ ds:5773 */ /* five spell cells 0EF3:2295 fills */
extern char far *gSpellLabels[];        /*@ ds:5787 */ /* the "1ST".."5TH" row labels 0EF3:2295 draws */
extern char gText592B[];                /*@ ds:592B */ /* "ABORT SPELL" (0EF3:2295) */
extern char gText5937[];                /*@ ds:5937 */ /* "ABORT SPELL" (0EF3:2295) */
extern char gText5943[];                /*@ ds:5943 */ /* "You must have a free hand for this spell." (0EF3:2676) */
extern char gText596E[];                /*@ ds:596E */ /* "You can't have two of this spell type active." (0EF3:2676) */
extern char gText599D[];                /*@ ds:599D */ /* "%s casts %s." (0EF3:2676) */
extern char gText59AB[];                /*@ ds:59AB */ /* "ok" (0EF3:2676) */
extern signed char g1A1B[];             /*@ ds:1A1B */ /* per-page sound table (src/E0ACD.C) */
extern void movmem(void far *src, void far *dst, unsigned n); /*@ 0000:63A3 */
extern signed char gF57E;               /*@ ds:F57E */ /* a colour (src/E0BD2.C) */
extern signed char gF53A;               /*@ ds:F53A */ /* a colour (src/E0BD2.C) */
extern signed char gFB4C;               /*@ ds:FB4C */ /* a colour (src/E0BD2.C) */
extern signed char gA9C4;               /*@ ds:A9C4 */ /* a colour (src/EOV2.C) */
extern signed char gA928;               /*@ ds:A928 */ /* a colour (src/EOV2.C) */
extern signed char gFB7B;               /*@ ds:FB7B */ /* a colour (src/E1547.C) */
extern int gGraphicsChoice;                       /*@ ds:8102 */ /* the video mode (src/E07A9.C) */

/* The record 0EF3:08CF, 0EF3:2541 and 0EF3:25BD take: an action context with the member it applies
   to at +0x38 (-1 when none) and a result word at +4 the functions hand back. */
struct ActRec {
    char pad00[4];
    int m04;                /* +04: the result returned to the caller */
    char pad06[0x38 - 0x06];
    signed char m38;        /* +38: the member the action applies to, -1 when none */
};

/* The list node 0EF3:2929 handles: the same record with the action key at +0x38 read as a word
   (0EF3:08CF and 0EF3:25BD read it as a byte). */
struct ActionNode {
    char pad00[4];
    int m04;                /* +04: the result 0EF3:2929 returns */
    char pad06[0x38 - 0x06];
    int key;                /* +38: the action key */
};

extern int RedrawPartyPanel(struct ActRec far *rec); /*@ 0EF3:2541 */

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
extern int HalveSaveDamageB(char far *type, int value); /*@ 0DE8:0FCA */
extern int FindClassSlot(int value, int classIndex); /*@ 0A72:007B */
extern int GetIndexedByte(char index, char limit, char offset); /*@ 0A72:0006 */
extern int GetArmourClassBase(char classIndex); /*@ 0A72:019D */
extern void SayAndPlaySound(char far *text); /*@ 082C:0B8F */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void PlaySound(int sound); /*@ 177D:0070 */
extern void WaitTicks(int n); /*@ 082C:0425 */
extern void ClearCharEffectSlot(int who, int what); /*@ 1414:0C2D */
extern int f0D1C_0BAB(int obj); /*@ 0D1C:0BAB */
extern int f0D1C_00DA(int value, int page); /*@ 0D1C:00DA */
extern int GetCurrentItem(void); /*@ 0D1C:02DC */
extern int f0D1C_0670(int who, int mask); /*@ 0D1C:0670 */
extern int f0BD2_0106(int who, int row); /*@ 0BD2:0106 */
extern int Obj15B4(char who, int value); /*@ 08E6:15B4 */
extern void DamageObject(struct Obj28 far *obj, int n); /*@ 08E6:0FBE */
extern void ClearObjectType(struct Obj28 far *obj); /*@ 08E6:000B */
extern void Obj17FD(int far *list, int key, int field, char flags, char scanAll, char scanFiltered); /*@ 08E6:17FD */
extern int Obj106B(struct Obj28 far *obj, int who); /*@ 08E6:106B */
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
extern unsigned char gFlashPassMask[]; /*@ ds:5621 */ /* eight per-pass bytes */
extern unsigned char gFlashFrameMask[];/*@ ds:5629 */ /* four per-frame mask bytes */
extern signed char gFlashFrameShift[];/*@ ds:562d */ /* four per-frame shift counts */
struct FlashDelta { signed char dx; signed char dy; }; /* 2 bytes at ds:5631 */
extern struct FlashDelta gFlashDelta[]; /*@ ds:5631 */ /* four per-frame offsets */
extern int gFlashBaseX[];        /*@ ds:5639 */ /* six per-member x bases */
extern signed char gFlashBaseY[];/*@ ds:5645 */ /* six per-member y bases */
extern char far *gFlashFrames[]; /*@ ds:DD1C */ /* far image pointers, indexed by frame - 1 */
extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */
extern int gFlags849E;           /*@ ds:849E */ /* bit 1 gates the object animation frames
                                                    (src/E08E6.C) */
extern char far *gBufDCFC;       /*@ ds:DCFC */ /* the 4096-byte screen scratch buffer, one far
                                                    allocation (src/E07A9.C) */
extern unsigned long gFlashPass2[]; /*@ ds:564B */ /* per-pass long masks for 0EF3:06E7 */
extern unsigned long gFlashFrame2[];/*@ ds:5677 */ /* per-frame long masks for 0EF3:06E7 */
extern unsigned char gFlashShift2[];/*@ ds:56B7 */ /* per-frame shift for 0EF3:06E7 */
extern unsigned char gFlashX2[];    /*@ ds:56C7 */ /* per-frame x in pixels for 0EF3:06E7 */
extern unsigned char gFlashY2[];    /*@ ds:56D7 */ /* per-frame y in pixels for 0EF3:06E7 */
extern signed char g1A09[];      /*@ ds:1A09 */ /* per-slot object index (EOV4.C) */
extern int g1D61[];              /*@ ds:1D61 */ /* per-slot, per-part x table (EOV4.C) */
extern int gStatusLine;                /*@ ds:8115 */ /* nonzero while a status line is up (E082C.C) */
extern int gDD18;                /*@ ds:DD18 */ /* the animation's origin x, written here and read by
                                                   0EF3:2A53 */
extern int gDD1A;                /*@ ds:DD1A */ /* the animation's origin y */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int pattern); /*@ ds:806A */
extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int colour); /*@ ds:8062 */
extern void (far *gDrawText)(char far *text, int x, int y, int colour, int extra); /*@ ds:80CA */
extern void DrawFilledPanel(int x1, int y1, int x2, int y2, int colourA, int colourB, int fill); /*@ 082C:09D5 */
extern void ComputeViewSpan(int slot, int far *left, int far *right, int region); /*@ 2116:0052 */
extern void DrawViewFrame(int full); /*@ 2116:005C */
extern void DrawViewSlots(int start); /*@ 2116:0025 */
extern void LockHidePointer(void); /*@ 1BBE:0307 */
extern void LockShowPointer(void); /*@ 1BBE:0332 */
extern void f0EF3_2A53(int scale, int spread, int count, int divisor, /*@ 0EF3:2A53 */
                       int falloff, void far *frameTable);
extern unsigned char f1D8C_0000(void); /*@ 1D8C:0000 */ /* a 2-bit pseudo-random byte (E1D8C.ASM) */
extern int RandomRange(int low, int high); /*@ 1AAA:0005 */
extern int IsPointInsideRect(int x, int y, int left, int top, int right, /*@ 082C:0613 */
                             int bottom);
extern void WaitFrame(void); /*@ 1A16:0027 */
extern int (far *gFn8072)(int x, int y); /*@ ds:8072 */ /* saves the pixels under (x, y), returns a
                                                           handle (src/EOV5.C) */
extern void (far *gFn809E)(int x, int y, int handle); /*@ ds:809E */ /* puts a saved handle back
                                                                        (src/EOV5.C) */
extern int g579B[];              /*@ ds:579B */ /* a per-scale word bound 0EF3:2A53 clamps y to */
extern int gEF;                  /*@ ds:00EF */ /* the view region's left, in eighths (this unit) */
extern int gF1;                  /*@ ds:00F1 */ /* the view region's top (src/EOV4.C) */
extern int gF3;                  /*@ ds:00F3 */ /* the view region's width in eighths */
extern int gF5;                  /*@ ds:00F5 */ /* the view region's height (src/EOV4.C) */
extern int gFC62;                /*@ ds:FC62 */ /* 0EF3:2A53's and 0EF3:2EBC's loop index */
extern int gFC64;                /*@ ds:FC64 */ /* 0EF3:2A53's current animation frame's byte */
extern int gFC66;                /*@ ds:FC66 */ /* 0EF3:2A53's and 0EF3:2EBC's state: 0 ends the
                                                    loop, 1 redraws, 2 starts it */
extern int gFC68;                /*@ ds:FC68 */ /* the animation's origin x, saved from ds:DD18 */
extern int gFC6A;                /*@ ds:FC6A */ /* the animation's origin y, saved from ds:DD1A */
extern int gFC6C;                /*@ ds:FC6C */ /* a particle's x pixel */
extern int gFC6E;                /*@ ds:FC6E */ /* a particle's y pixel */
extern int gFC70;                /*@ ds:FC70 */ /* gStatusLine saved while the animation runs */
extern int gFC72;                /*@ ds:FC72 */ /* the handle gFn8072 returned under the pointer */

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
/*@ 0EF3:00CD */ void AnimateEffect(struct Tgt far *rec)
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
    gStatusLine = 2;
    gFillRect(0, 0, 0xB0, 0x78, 0);
    DrawViewSlots(g55EF[objIndex]);
    gStatusLine = 0;
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
   creators 0EF3:1197, 0EF3:1656 and 0EF3:1D38. The six clears are one chained assignment
   (`rec->m06 = rec->m08 = rec->m00 = rec->m01 = rec->m04 = g8DD5 = 0;`): the original keeps the
   value in AL, stores the char fields from AL and sign-extends it once per word field
   (`mov al,0; mov [ds:8DD5],al; cwde; ...`), which only the chained form produces. */
/*@ 0EF3:0418 */ int CreateItemRecord(int flags, int icon, int m0B, int type)
{
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
    rec->m06 = rec->m08 = rec->m00 = rec->m01 = rec->m04 = g8DD5 = 0;
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
    SayAndPlaySound(gText57A3);
    PlaySound(0x4F);
    BuildList5();
}

/* 0EF3:05BD: draw one flash frame for member `who` at pass `pass`: redraw the member, then for
   each of the four frame bits (the gFlashPassMask[pass] bits selected by gFlashFrameMask, shifted
   down by gFlashFrameShift) draw the image gFlashFrames[frame - 1] at the member's flash base
   (gFlashBaseX[who] * 8, gFlashBaseY[who], or 0xB5, 3 while a status line is up) plus the frame's
   gFlashDelta offset. Called by 0EF3:0665. */
/*@ 0EF3:05BD */ void DrawFlashFrame(int who, int pass)
{
    register int i;
    int x;
    int frame;
    register int y;

    RedrawMember(who);
    for (i = 0; i < 4; i++) {
        frame = (gFlashPassMask[pass] & gFlashFrameMask[i]) >> gFlashFrameShift[i];
        if (frame-- == 0)
            continue;
        if (gFB44 == 0) {
            x = gFlashBaseX[who] * 8;
            y = gFlashBaseY[who];
        } else {
            x = 0xB5;
            y = 3;
        }
        y += gFlashDelta[i].dy;
        x += gFlashDelta[i].dx - 8;
        f1A29_000F(0, gFlashFrames[frame], x, y, 0);
    }
}

/* 0EF3:0665: run the "flash" animation for one member or the whole party: eight passes over the
   living members (m01 bit 0, and either no panel is open or the member is the selected one gA99A),
   each drawing one frame through 0EF3:05BD and waiting two ticks, then redraw each member. `who`
   is a member index, or -1 for the whole party; the function reuses the parameter as the walk
   index. Called by 0EF3:08CF and the item effect at 0BD2:0D2D. */
/*@ 0EF3:0665 */ void FlashPartyMembers(int who)
{
    int first;
    int last;
    int pass;

    if (who == -1) {
        first = 0;
        last = 5;
    } else {
        first = last = who;
    }
    for (pass = 0; pass < 8; pass++) {
        for (who = first; who <= last; who++) {
            if ((gChars[who].m01 & 1) && (gFB44 == 0 || who == gA99A))
                DrawFlashFrame(who, pass);
        }
        WaitTicks(2);
    }
    for (who = first; who <= last; who++)
        RedrawMember(who);
}

/* 0EF3:06E7: flash damage over the view. It saves the sixteen 2-by-0x10 pixel cells described by
   gFlashX2/gFlashY2 into the scratch buffer (256 bytes each), then runs eleven passes: restore the
   cells, and for each of the sixteen frames take the unsigned long gFlashPass2[pass] masked by
   gFlashFrame2[i] and shifted right by gFlashShift2[i]; when the result is nonzero draw
   gFlashFrames[frame] at (gFlashX2[i], gFlashY2[i]) through 1A29:000F, then hide the pointer,
   re-clip the view box, show the pointer and wait two ticks. A final restore leaves the view as it
   was. It clears gFlags849E bit 1 for the duration. Called by the wand, turn-undead and
   clear-objects paths (0BD2:0EE8, 0D1C:0A43, 0EF3:1702). */
/*@ 0EF3:06E7 */ void FlashViewDamage(void)
{
    int x;
    int y;
    int frame;
    int i;
    int pass;

    gFlags849E &= ~2;
    HidePointerZone(0, 0, 0xB0, 0x78);
    ClipRect(0, 0, 0, 0, 0x16, 0x78, 0, 2);
    for (i = 0; i < 0x10; i++)
        gSaveRect(gFlashX2[i] >> 3, gFlashY2[i], 2, 0x10, gBufDCFC + (i << 8));
    for (pass = 0; pass < 0xB; pass++) {
        gStatusLine = 2;
        for (i = 0; i < 0x10; i++) {
            x = gFlashX2[i];
            y = gFlashY2[i];
            gFn804A(gFlashX2[i] >> 3, gFlashY2[i], 2, 0x10, gBufDCFC + (i << 8));
        }
        for (i = 0; i < 0x10; i++) {
            frame = (gFlashPass2[pass] & gFlashFrame2[i]) >> gFlashShift2[i];
            x = gFlashX2[i];
            y = gFlashY2[i];
            if (frame-- == 0)
                continue;
            f1A29_000F(2, gFlashFrames[frame], x, y, 0);
        }
        gStatusLine = 0;
        HidePointerZone(0, 0, 0xB0, 0x78);
        ClipRect(0, 0, 0, 0, 0x16, 0x78, 2, 0);
        ShowPointerZone();
        WaitTicks(2);
    }
    for (i = 0; i < 0x10; i++)
        gRestoreRect(gFlashX2[i] >> 3, gFlashY2[i], 2, 0x10, gBufDCFC + (i << 8));
    ShowPointerZone();
    gFlags849E |= 2;
}

/* 0EF3:08CF: apply one action record to the member it names (+0x38). It makes that member current
   and, while the member is alive (m01 bit 0), draws its name and the message ds:57C3, plays the
   effect's sound (the first byte of the ds:1355 record for gEFB8), flashes the member, runs the
   redraw hook ds:EFBA and redraws it; a dead member just returns the record's +4 word. When the
   record names no member (-1) it says ds:57C8. It then finishes the open condition slot through
   0EF3:026F when g8DCE is set, else redraws the panel through 0EF3:2541. Reached through a
   pointer; returns the record's +4 word. The alive test is an early return
   (`if ((m01 & 1) == 0) return rec->m04;`), which is why the original lays it out as
   `test ...; jne alive; jmp done` rather than an inverted `je`. */
/*@ 0EF3:08CF */ int ApplyMemberEffect(struct ActRec far *rec)
{
    gCurMember = rec->m38;
    if (gCurMember != -1) {
        gCurRec = &gChars[gCurMember];
        if ((gCurRec->m01 & 1) == 0)
            return rec->m04;
        DrawText(gCurRec->name);
        DrawText(gText57C3);
        PlaySound(gEffectSound[gEFB8 * 0x13]);
        FlashPartyMembers(gCurMember);
        gFnEFBA();
        RedrawMember(gCurMember);
    } else {
        SayAndPlaySound(gText57C8);
    }
    if (g8DCE != 0)
        FinishMemberEffect();
    else
        RedrawPartyPanel(rec);
    return rec->m04;
}

/* 0EF3:097B: add one cell effect: find the first free of the ten 18-byte records at ds:DD2C (its
   active byte clear) and, when one is found, put it in state 2 with m02 1, copy the mode entry
   gEffectModes[mode] into m0C, m0D and m0A, fill m0B, m04, m06, m07 and m08 from the arguments,
   and copy the DD callback of gSpells1344[gEffectModes[mode].spell] into the record's fn; set
   gDF4C. Called by the effect wrappers 0EF3:10D2 to 0EF3:2011. */
/*@ 0EF3:097B */ void StartCellEffect(char who, register int mode, int cell, int m06, int m07)
{
    int i;
    struct DDRec far *rec;

    rec = gDD2C;
    for (i = 0; i < 10; i++, rec++) {
        if (rec->active == 0)
            break;
    }
    if (i == 10)
        return;
    rec->active = 2;
    rec->m02 = 1;
    rec->m0C = gEffectModes[mode].m02;
    rec->m07 = m07;
    rec->m0D = gEffectModes[mode].m01;
    rec->m04 = cell;
    rec->m06 = m06;
    rec->m08 = mode;
    rec->m0A = gEffectModes[mode].m03;
    rec->m0B = who;
    mode = gEffectModes[mode].spell;
    rec->fn = gSpells1344[mode].ddFn;
    gDF4C = 1;
}

/* 0EF3:0A3F: run a melee effect on the record's target. The record's +0xC byte holds five flags:
   0x10 (the target is not a member), 1, 2, 4 and 8. When the target cell's object type byte
   (gB894[cell * 9 + 4]) has any of bits 0..2 and the record's +0xB member index is not negative
   (or flag 0x10 is set), list the objects at the cell with 08E6:17FD and for each one roll
   `count`d`sides`+`bonus` times `n`, halve it on a successful save (flag 8), and damage the object
   unless 0D1C:0BAB rejects it. Otherwise, when the cell is the party cell and the member index is
   negative (or flag 0x10 is set), either walk the six members (flag 2) or pick one from the
   g1A2B table and damage it. Finally, when anything was hit and the record's +0xC has bit 0x40,
   run the animation 0EF3:00CD; a record whose +8 is 5 clears the hit. Returns the hit flag.
   The picking branch keeps the `hit = 1` at the end of the chain, so its source is the negated
   form `if (m08 != 0 || (mE3 & 2) == 0) { ... } else hit = 1;`, not `if (m08 == 0 && (mE3 & 2))`. */
/*@ 0EF3:0A3F */ int DamageTargetCell(struct Tgt far *rec, int count, int sides, int bonus, int n)
{
    char dmg;
    char flag01;
    char flag02;
    char flag04;
    char flag08;
    char i;
    char hit;
    char flag10;
    int buffer[5];
    int far *list;
    int idx;

    flag10 = rec->m0C & 0x10;
    flag01 = rec->m0C & 1;
    flag02 = rec->m0C & 2;
    flag04 = rec->m0C & 4;
    flag08 = rec->m0C & 8;
    hit = 0;
    if (n <= 0)
        n = 1;
    if ((gB894[rec->m04 * 9 + 4] & 7) && (rec->m0B >= 0 || flag10)) {
        Obj17FD(buffer, rec->m04, rec->m06, rec->m07, flag02, flag01);
        list = buffer;
        for (; *list != -1; list++) {
            dmg = RollDice(count, sides, bonus) * n;
            if (RollToHit(rec->m0B, *list, 0, 0) || !flag04) {
                if (flag08)
                    dmg = HalveSaveDamageB((char far *)&gObjs[*list], dmg);
                hit = 1;
                if (!f0D1C_0BAB(*list))
                    DamageObject(&gObjs[*list], dmg);
            }
        }
    } else if (rec->m04 == gPartyCell && (rec->m0B < 0 || flag10)) {
        if (flag02) {
            for (i = 0; i < 6; i++) {
                if ((gChars[i].m01 & 1) == 0)
                    continue;
                if (Obj106B(gObjs, i) || !flag04) {
                    dmg = RollDice(count, sides, bonus) * n;
                    if (flag08)
                        dmg = HalveSaveDamageA(i, dmg);
                    hit = 1;
                    DamageCharacter(i, dmg);
                }
            }
        } else {
            idx = rec->m06 & 3;
            idx = g1A2B[gFBCC * 4 + idx];
            if (idx > 2 && (f0D1C_0670(5, 1) || f0D1C_0670(6, 1)) && RollDice(1, 2, -1))
                i = idx + 2;
            else
                i = idx;
            if (rec->m08 != 0 || (gChars[i].mE3 & 2) == 0) {
                if ((gChars[i].m01 & 1) && (Obj106B(gObjs, i) || !flag04)) {
                    dmg = RollDice(count, sides, bonus) * n;
                    if (flag08)
                        dmg = HalveSaveDamageA(i, dmg);
                    hit = 1;
                    DamageCharacter(i, dmg);
                }
            } else {
                hit = 1;
            }
        }
    }
    if (hit && (rec->m0C & 0x40)) {
        AnimateEffect(rec);
    } else if (rec->m08 == 5) {
        hit = 0;
    }
    return hit;
}

/* 0EF3:0D78: expire one spell on character `who`: format the message ds:57D9 with the member's
   name and the spell's name, say it, remember `who` in gA999, then run the spell's expire callback
   gSpells1344[spell].remove(&gChars[who]). When the spell's flags byte has bits 0..2 set redraw
   the member, else when bit 3 is set redraw every character. Called by 1414:07CA. */
/*@ 0EF3:0D78 */ void ExpireSpellEffect(register int spell, int who)
{
    struct CharRec far *rec;

    rec = &gChars[who];
    sprintf(gFileNameBuf, gText57D9, rec->name, gSpells1344[spell].name);
    SayAndPlaySound(gFileNameBuf);
    gA999 = who;
    gSpells1344[spell].remove(rec);
    if (gSpells1344[spell].flags & 7)
        RedrawMember(who);
    else if (gSpells1344[spell].flags & 8)
        RedrawCharacters();
}

/* 0EF3:0E1B: hit one object with a melee effect: when the panel member has the per-member flag
   ds:5717 cleared, say the string ds:5809 with the member's name; otherwise find the object the
   current page faces (0D1C:00DA and 08E6:15B4). A member with no object gets the string ds:57F1;
   otherwise, when RollToHit accepts the object and 0D1C:0BAB does not reject it, roll count d
   sides plus bonus and damage the object. Called by 0EF3:1B43, 0EF3:1EF3 and 0EF3:1FF9.
   The two `sprintf`+`SayAndPlaySound` message tails (the obj==-1 branch at 0EF3:0EAB and the outer
   else at 0EF3:0ED9) are identical from +0x14; the file's -v (debug information) keeps them apart
   as the original does, where a build without it cross-jumps them (WHITEPAPER 2.1.1). */
/*@ 0EF3:0E1B */ void CastMeleeEffect(int count, int sides, int bonus)
{
    int target;
    register int obj;
    register int dmg;

    if (gAt5717[gPanelMember] != 0) {
        target = f0D1C_00DA(gPartyCell, gFBCC);
        obj = Obj15B4(gPanelMember, target);
        if (obj != -1) {
            if (!RollToHit(gPanelMember, obj, 0, 1))
                return;
            dmg = RollDice(count, sides, bonus);
            if (!f0D1C_0BAB(obj))
                DamageObject(&gObjs[obj], dmg);
        } else {
            sprintf(gFileNameBuf, gText57F1, gChars[gPanelMember].name);
            SayAndPlaySound(gFileNameBuf);
        }
    } else {
        sprintf(gFileNameBuf, gText5809, gChars[gPanelMember].name);
        SayAndPlaySound(gFileNameBuf);
    }
}

/* 0EF3:0F0B: when the current record's armour class (+0x1D) is more than six above the class word
   of its +0x15 index (0A72:019D), raise its +0xDF penalty byte to eight over the member's
   class-slot 1 value and recompute the armour class; otherwise say the string ds:5830 with the
   record's name. Called through a table. */
/*@ 0EF3:0F0B */ void FixArmourClass(void)
{
    char penalty;
    register int base;

    base = GetArmourClassBase(gCurRec->m15);
    if (gCurRec->m1D > base + 6) {
        penalty = GetClassSlot1Value(gPanelMember) + 8;
        gCurRec->mDF = penalty;
        ComputeArmourClass(gCurMember);
    } else {
        sprintf(gFileNameBuf, gText5830, gCurRec->name);
        SayAndPlaySound(gFileNameBuf);
    }
}

/* 0EF3:0F86: recompute the armour class of the record's member (its +0 byte). Called by 0DE8:09B9
   and 0EF3:131A. */
/*@ 0EF3:0F86 */ void RecomputeArmourClass(struct CharRec far *rec)
{
    ComputeArmourClass(rec->m00);
}

/* 0EF3:0F99: draw one icon per party member in its panel cell, then strike the object the current
   page faces. For each of the six members it draws the far sprite gThrownIcon0[abs(icon) - 1] at
   (gIconX5721[i], gIconY572D[i]) with region 0: a positive icon (gIconIndex571B) goes through
   082C:063B, a negative one (through its magnitude) through 082C:0681. It then waits two ticks,
   takes the panel member's class-slot 1 value, finds the object the party cell faces (0D1C:00DA)
   and runs 08E6:16D6 on it for 1d3 plus twice that value, and sets ds:DF4C. Reached through the
   spell table (an apply callback). Purpose unknown: what the icon index means and what chooses the
   two region drawings. */
/*@ 0EF3:0F99 */ void DrawMemberIcons(void)
{
    int value;
    int i;
    int target;

    for (i = 0; i < 6; i++) {
        if (gIconIndex571B[i] < 0)
            DrawRegion004D(0, gIconTable[gIconIndex571B[i] < 0 ? -gIconIndex571B[i] : gIconIndex571B[i]],
                           gIconX5721[i], gIconY572D[i], 0);
        else
            DrawRegion000F(0, gIconTable[gIconIndex571B[i]], gIconX5721[i], gIconY572D[i], 0);
    }
    WaitTicks(2);
    value = GetClassSlot1Value(gPanelMember);
    target = f0D1C_00DA(gPartyCell, gFBCC);
    Obj16D6(target, 1, 3, value << 1, 1);
    gDF4C = 1;
}

/* 0EF3:1069: enter the item-swap mode: set gFBD2 bit 0, take the mouse item, move the pointer, and
   apply a 2-turn effect. Called through a table. */
/*@ 0EF3:1069 */ void EnterSwapMode(void)
{
    gFBD2 |= 1;
    SetMouseItem(GetCurrentItem());
    f1BF3_00A8(0, 0);
    RedrawCharacters();
    ApplyTimedCharEffect(gEFB8, 0, 0x222L, 2, 1);
}

/* 0EF3:10AE: leave the item-swap mode: clear gFBD2 bit 0, take the mouse item and move the
   pointer. Called through a table. */
/*@ 0EF3:10AE */ void LeaveSwapMode(void)
{
    gFBD2 &= ~1;
    SetMouseItem(GetCurrentItem());
    f1BF3_00A8(0, 0);
}

/* 0EF3:10D2: add the panel member's effect record of mode 0 through 0EF3:097B. */
/*@ 0EF3:10D2 */ void AddEffectRecord0(void)
{
    StartCellEffect(gPanelMember, 0, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:10F2: run the record's melee effect through 0EF3:0A3F with 1d4 and half the member's
   class-slot 1 value. */
/*@ 0EF3:10F2 */ void RunMelee1d4(struct Tgt far *rec)
{
    DamageTargetCell(rec, 1, 4, 1, (GetClassSlot1Value(rec->m0B) - 1) >> 1);
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
   apply a 1-turn effect. Called through a table. The 0482 result is `index` and the 035A result
   `slot`, declared in that order: the allocator then keeps index in DI and slot in SI with no
   stack frame (the reverse order spills slot). */
/*@ 0EF3:1197 */ void CreateItemForEffect0(void)
{
    int index, slot;

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
    StartCellEffect(gPanelMember, 1, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:12E3: run the record's melee effect through 0EF3:0A3F with 2d4 and a third of the
   member's class-slot 1 value. */
/*@ 0EF3:12E3 */ void RunMelee2d4(struct Tgt far *rec)
{
    DamageTargetCell(rec, 2, 4, 0, GetClassSlot1Value(rec->m0B) / 3);
}

/* 0EF3:1315: an empty function reached through a table; nothing to do. */
/*@ 0EF3:1315 */ void EmptyFunction1315(void) {}

/* 0EF3:131A: the dispel-magic spell. Say "All spells on %s are dispelled." with the current
   record's name, then for each of its ten spell slots: when the slot byte at +0xD5 is negative
   the slot's spell expire callback gSpells1344[-slot].remove(gCurRec) runs, the slot and its
   32-bit value at +0xAD are cleared; when the record's effect flag bit 2 is set ClearEffectFlag4
   runs, and when its +0xDF penalty byte is over zero its armour class is recomputed. After the
   loop the penalty, the effect flags (cleared twice) and the record's +0xE3 long are cleared and
   the record's member is redrawn. Reached through the spell table (record 12's apply callback). */
/*@ 0EF3:131A */ void DispelMagic(void)
{
    register int i;
    register int spell;

    sprintf(gFileNameBuf, gText5859, gCurRec->name);
    SayAndPlaySound(gFileNameBuf);
    for (i = 0; i < 10; i++) {
        spell = gCurRec->mD5[i];
        if (spell < 0) {
            gSpells1344[-spell].remove(gCurRec);
            gCurRec->mD5[i] = 0;
            gCurRec->mAD[i] = 0;
        }
        if (gCurRec->mE3 & 4)
            ClearEffectFlag4(gCurRec);
        if (gCurRec->mDF > 0)
            RecomputeArmourClass(gCurRec);
    }
    gCurRec->mDF = 0;
    gCurRec->mE3 = 0;
    gCurRec->mE3 = 0;
    RedrawMember(gCurRec->m00);
}

/* 0EF3:13FD: add the panel member's effect record of mode 2 through 0EF3:097B. */
/*@ 0EF3:13FD */ void AddEffectRecord2(void)
{
    StartCellEffect(gPanelMember, 2, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:141E: run the record's 1d6 melee effect through 0EF3:0A3F. */
/*@ 0EF3:141E */ void RunMelee1d6(struct Tgt far *rec)
{
    DamageTargetCell(rec, 1, 6, 0, GetClassSlot1Value(rec->m0B));
}

/* 0EF3:144A: add the panel member's effect record of mode 3 through 0EF3:097B. */
/*@ 0EF3:144A */ void AddEffectRecord3(void)
{
    StartCellEffect(gPanelMember, 3, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:146B: run the record's 5d6 melee effect through 0EF3:0A3F. */
/*@ 0EF3:146B */ void RunMelee5d6(struct Tgt far *rec)
{
    DamageTargetCell(rec, 5, 6, 0, GetClassSlot1Value(rec->m0B));
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
    StartCellEffect(gPanelMember, 4, gPartyCell, gF3B8, gFBCC);
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
    StartCellEffect(gPanelMember, 5, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:162A: run the record's 1d6 melee effect through 0EF3:0A3F. */
/*@ 0EF3:162A */ void RunMelee1d6B(struct Tgt far *rec)
{
    DamageTargetCell(rec, 1, 6, 0, GetClassSlot1Value(rec->m0B));
}

/* 0EF3:1656: give the panel member the item of effect mode 6, like 0EF3:1197 but with the item
   record at a different row and the class-slot value halved with `>> 1` (the original shifts,
   `sar ax,1`, where `/ 2` would call the divide routine). Called through a table. */
/*@ 0EF3:1656 */ void CreateItemForEffect6(void)
{
    int index, slot;

    index = ItemSlot1Empty(gCurRec);
    slot = CreateItemType(0, 0, 0, 0xF, GetClassSlot1Value(gPanelMember) >> 1, 6, 0, 1);
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

/* 0EF3:1702: after FlashViewDamage, walk the 30 objects and for each whose id is the object the
   current page faces (0D1C:00DA), whose character record's +0x0F long has bit 0 or 1 and which
   0D1C:0BAB does not reject, roll 1d20 and clear the object's type when the roll is under the value
   0A72:0006 gives for the character record's +2 index. Called by the wand/turn-undead paths. The
   threshold is a local and the clear uses &gObjs[i], not the walking pointer. */
/*@ 0EF3:1702 */ void ClearObjectsFacingCell(void)
{
    struct Entry27 far *rec;
    struct Obj28 far *obj;
    int target;
    int limit;
    int i;

    FlashViewDamage();
    target = f0D1C_00DA(gPartyCell, gFBCC);
    obj = gObjs;
    for (i = 0; i < 0x1E; i++, obj++) {
        rec = &gTable1A3B[obj->b0];
        if (obj->id == target && (rec->m0F & 3) != 0 && f0D1C_0BAB(i) == 0) {
            limit = GetIndexedByte(0, rec->m02, 4);
            if (RollDice(1, 0x14, 0) < limit)
                ClearObjectType(&gObjs[i]);
        }
    }
}

/* 0EF3:17B7: add the panel member's effect record of mode 6 through 0EF3:097B. */
/*@ 0EF3:17B7 */ void AddEffectRecord6(void)
{
    StartCellEffect(gPanelMember, 6, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:17D8: run the record's 1d6 melee effect, then for each of the four cell deltas at ds:5733
   add the delta to the record's cell (masked to 0x3FF), run the effect again and restore the
   cell. Returns the first run's result. */
/*@ 0EF3:17D8 */ int RunMeleeAroundCell(struct Tgt far *rec)
{
    int found;
    int i;
    int cell;

    found = DamageTargetCell(rec, 1, 6, 0, GetClassSlot1Value(rec->m0B));
    if (found) {
        for (i = 0; i < 4; i++) {
            cell = rec->m04;
            rec->m04 += gTable5733[i];
            rec->m04 &= 0x3FF;
            DamageTargetCell(rec, 1, 6, 0, GetClassSlot1Value(rec->m0B));
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

void AnimateConeOfCold(int count, int spread, int step, int cycle, int divisor,
                       int far *frameTable);
/* 0EF3:18D8: the cone of cold. It reads the panel member's class-slot 1 value, blanks the view
   (gStatusLine = 2, gFillRect, DrawViewSlots(7), gStatusLine = 0), hides the pointer, runs the cone drawing
   0EF3:2EBC(0x96, 0x32, 0xa, 1, 0x64, ds:5737) and shows it again. It sets gF518 and, for each of
   the seven rows of the per-page table g5763[gFBCC], lists the objects whose cell is the party cell
   plus the row's delta (masked to 0x3FF) with Obj17FD and damages each that 0D1C:0BAB does not
   reject by RollDice(classSlot, 4, classSlot) through 08E6:0FBE, then runs 08E6:0E48. Reached
   through the spell table (gSpells1344 record 24, "cone of cold", its apply callback). */
/*@ 0EF3:18D8 */ void CastConeOfCold(void)
{
    int buffer[5];
    int far *list;
    int classSlot;
    int cell;
    char far *table;
    int row;
    int dmg;

    classSlot = GetClassSlot1Value(gPanelMember);
    gStatusLine = 2;
    gFillRect(0, 0, 0xB0, 0x78, 0);
    DrawViewSlots(7);
    gStatusLine = 0;
    HidePointerZone(0, 0, 0xB0, 0x78);
    AnimateConeOfCold(0x96, 0x32, 0x0A, 1, 0x64, gText5737);
    ShowPointerZone();
    table = g5763[gFBCC];
    gF518 = 1;
    for (row = 0; row < 7; row++, table++) {
        cell = (gPartyCell + *table) & 0x3FF;
        Obj17FD((int far *)buffer, cell, 4, gFBCC, 1, 1);
        list = buffer;
        while (*list != -1) {
            dmg = RollDice(classSlot, 4, classSlot);
            if (f0D1C_0BAB(*list) == 0)
                DamageObject(&gObjs[*list], dmg);
            list++;
        }
    }
    Obj0E48();
}

/* 0EF3:1A0F: add the panel member's effect record of mode 7 through 0EF3:097B. */
/*@ 0EF3:1A0F */ void AddEffectRecord7(void)
{
    StartCellEffect(gPanelMember, 7, gPartyCell, gF3B8, gFBCC);
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
        SayAndPlaySound(gText587A);
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

/* 0EF3:1BA3: the "aid" spell. When the current member is not protected (0D1C:0670 with mask 3) it
   adds one 1d8 roll to the record's hit points, remembers the same amount in its +0xE2 byte and sets
   effect flag bit 0x40 (the amount 0EF3:1C45 takes back); otherwise it says "The aid spell fails!"
   (ds:5899). Either way it applies a 1-turn effect. Reached through the spell table (gSpells1344
   "aid", its apply callback). */
/*@ 0EF3:1BA3 */ void ApplyAidEffect(void)
{
    int amount;

    if (f0D1C_0670(gCurMember, 3)) {
        gCurRec->mE3 |= 0x40;
        amount = RollDice(1, 8, 0);
        if (gCurRec->hp < (signed char)(gCurRec->hp + amount)) {
            gCurRec->mE2 += amount;
            gCurRec->hp += amount;
        }
        ApplyTimedCharEffect(gEFB8, 1, 0x222L, 1, 1);
    } else {
        SayAndPlaySound(gText5899);
    }
}
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
   record at a different row and the class-slot value halved with `>> 1` (the original shifts,
   `sar ax,1`). Called through a table. */
/*@ 0EF3:1D38 */ void CreateItemForEffect4(void)
{
    int index, slot;

    index = ItemSlot1Empty(gCurRec);
    slot = CreateItemType(0, 0, 0, 0xF, 1, 4, 4, 1);
    gCurRec->m77[index] = CreateItemRecord(0, 0x54, 0, slot);
    RedrawMember(gPanelMember);
    ApplyTimedCharEffect(gEFB8, 4, 0x222L, GetClassSlot24Value(gPanelMember) >> 1, 1);
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
        SayAndPlaySound(gText58BB);
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
    StartCellEffect(gPanelMember, 8, gPartyCell, gF3B8, gFBCC);
}

/* 0EF3:2032: run the record's 6d8 melee effect through 0EF3:0A3F. */
/*@ 0EF3:2032 */ void RunMelee6d8(struct Tgt far *rec)
{
    DamageTargetCell(rec, 6, 8, 0, 0);
}

/* 0EF3:2050: when the current record is at -10 hit points and its +0x1F byte over two is not
   exactly one, set it to 1 and redraw the member; otherwise say the message at ds:58C6. */
/*@ 0EF3:2050 */ void ReviveAtMinusTen(void)
{
    if (gCurRec->hp == -10 && (gCurRec->m1F >> 1) != 1) {
        gCurRec->hp = 1;
        RedrawMember(gCurMember);
    } else {
        SayAndPlaySound(gText58C6);
    }
}

/* 0EF3:208A: run the record's melee effect through 0EF3:0A3F with 0d0 and a bonus of 0xC. Called
   through a table. DRAFT: the body is right but the original keeps the 0A72:019D result in SI and
   a two-byte frame for the value; our build folds them away (`make match` differs at the frame). */
/*@ 0EF3:208A */ void RunMelee0(struct Tgt far *rec)
{
    DamageTargetCell(rec, 0, 0, 0xC, 1);
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

void DrawSpellPanel(void);
/* 0EF3:21E0: open the member's spell panel: clear gDDE2, make `who` the panel member, set gF43D
   when `flag` is 1, point gNameTable at g12E0 (memorized) or g1278 (known), point gDD14 at the
   member's +0x55 or +0x37 spell slots and gA9C5 at its first nonzero slot. Restore the panel
   rectangle from gSegDF4E + 0x61a8 when gFlag is clear, then set gFlag, rebuild the item list
   (BuildList1) and run the panel body 0EF3:2295; returns 1. Called by the item class 5/6 path of
   0BD2:1001. */
/*@ 0EF3:21E0 */ int OpenSpellPanel(int who, char flag)
{
    gDDE2 = 0;
    gPanelMember = who;
    if (gF43D = (flag == 1)) {
        gNameTable = (char far *)g12E0;
        gDD14 = gChars[who].memorized;
    } else {
        gNameTable = (char far *)g1278;
        gDD14 = gChars[who].known;
    }
    gA9C5 = 0;
    while (gA9C5 < 6) {
        if (gDD14[gA9C5] > 0)
            break;
        gA9C5++;
    }
    if (!gFlag)
        gSaveRect(8, 0x7A, 0x0E, 0x37, (char far *)gSegDF4E + 0x61A8);
    gFlag = 1;
    BuildList1();
    DrawSpellPanel();
    return 1;
}

/* 0EF3:2295: draw the spell panel body. It sets gStatusLine, fills the panel's outer rectangle with
   gFillRect, then draws the five spell cells: for each cell (the gSpellCells rectangle) it picks
   three colours, gFB4C/gF53A/gF57E for the member's own row (i == gDDE2) and 3/1/-1 style
   (gA9C4/gA928/gFB7B) for the others when gGraphicsChoice is nonzero, or 1/2/3 and 0/1/2 when it is zero.
   It then draws the "1ST".."5TH" labels (gSpellLabels) and the name list box, and for each of the
   seven rows of the member's slot row (gDD14 + gDDE2*6) draws the spell's name from gNameTable
   (row 6, the abort row, gets ds:592B, or ds:5937 when it is the highlighted row gA9C5). Finally
   it clears gStatusLine, hides the pointer, clips a 9 by 0x35 box, draws the panel's inner line with
   gDrawLine and shows the pointer. Called by 0EF3:21E0, 0EF3:25BD and 0EF3:2929.
   DRAFT: the panel, cells, labels and row list match. The file's -v (since 2026-11-02) keeps the
   not-selected row-6 "ABORT SPELL" call (0EF3:1F4) apart from the shared `push ax; lcall
   gDrawText; add sp,0xc` tail, as the original does; the first difference is now 0EF3:2295+01A1,
   the `gA9C5 == i` test (ours `jne 0x201`, the original `je 0x201`), a question of the condition's
   source form. The cell
   rectangles are unsigned bytes (the original zero-extends with `mov ah,0`). */
/*@ 0EF3:2295 draft */ void DrawSpellPanel(void)
{
    register int i;
    register int cell;
    int slot;
    int colourA;
    int colourB;
    int fill;
    int hiColour;
    int textColour;

    gStatusLine = 2;
    gFillRect(0x46, 0x7a, 0xaf, 0xaf, gF57E);
    for (i = 0; i < 5; i++) {
        cell = i * 4;
        if (gGraphicsChoice == 0) {
            colourA = gDDE2 == i ? 1 : 0;
            colourB = gDDE2 == i ? 2 : 1;
            fill = gDDE2 == i ? 3 : 2;
            hiColour = 1;
            textColour = 0;
        } else {
            colourA = gDDE2 == i ? gFB4C : gA9C4;
            colourB = gDDE2 == i ? gF53A : gA928;
            fill = gDDE2 == i ? gF57E : gFB7B;
            hiColour = 6;
            textColour = 0xc;
        }
        DrawFilledPanel(((unsigned char near *)gSpellCells)[cell], ((unsigned char near *)gSpellCells)[cell + 1],
                        ((unsigned char near *)gSpellCells)[cell + 2], ((unsigned char near *)gSpellCells)[cell + 3],
                        colourA, colourB, fill);
    }
    if (gGraphicsChoice == 0) {
        fill = 2;
        hiColour = 0xc;
        textColour = 3;
    } else {
        fill = gF57E;
        hiColour = 0xb;
        textColour = 0xf;
    }
    for (i = 0; i < 5; i++)
        gDrawText(gSpellLabels[i], i * 0x15 + 0x49, 0x7c, 0xc, 0);
    DrawFilledPanel(0x47, 0x83, 0xaf, 0xae, gFB4C, gF53A, gF57E);
    for (i = 0; i < 7; i++) {
        slot = (gDD14 + gDDE2 * 6)[i];
        if (gA9C5 == i) {
            if (slot >= 0 && i < 6)
                gDrawText(((char far **)gNameTable)[slot], 0x49, i * 6 + 0x84, textColour,
                          hiColour);
            else if (i == 6)
                gDrawText(gText5937, 0x49, i * 6 + 0x84, textColour, hiColour);
        } else {
            if (slot >= 0 && i < 6)
                gDrawText(((char far **)gNameTable)[slot], 0x49, i * 6 + 0x84, textColour,
                          fill);
            else if (i == 6)
                gDrawText(gText592B, 0x49, i * 6 + 0x84, textColour, fill);
        }
    }
    gStatusLine = 0;
    HidePointerZone(0x48, 0x78, 0xb0, 0xaf);
    ClipRect(9, 0x7a, 9, 0x7a, 0xd, 0x35, 2, 0);
    gDrawLine(0x47, 0x7a, 0x47, 0xae, gF53A);
    ShowPointerZone();
}

/* 0EF3:2541: redraw the party panel after an action: clear the key-flag gFlag, hide the pointer
   and restore the panel rectangle (8, 0x7a, 0x0e by 0x37) from the slot-5 buffer gSegDF4E plus
   0x61a8, show the pointer, draw the view rows, and rebuild the active item list (BuildList1 to 3
   by gFB44). Returns the action record's +4 word. */
/*@ 0EF3:2541 */ int RedrawPartyPanel(struct ActRec far *rec)
{
    gFlag = 0;
    HidePointerZone(0x40, 0x78, 0xB0, 0xB1);
    gRestoreRect(8, 0x7A, 0x0E, 0x37, (char far *)gSegDF4E + 0x61A8);
    ShowPointerZone();
    DrawViewRows();
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
    return rec->m04;
}

/* 0EF3:25BD: Purpose unknown: save the action record's member (+0x38) in ds:DDE2, then hunt along
   that member's row (six bytes) of the far table ds:DD14 (row base gDDE2 * 6) for the first byte
   over zero, breaking at six. It then runs 0EF3:2295 (the item panel) and returns the record's +4
   word. Its caller and what the row holds are not established. */
/*@ 0EF3:25BD */ int f0EF3_25BD(struct ActRec far *rec)
{
    gDDE2 = rec->m38;
    gA9C5 = 0;
    while (gA9C5 < 6) {
        if ((gDD14 + gDDE2 * 6)[gA9C5] > 0)
            break;
        gA9C5++;
    }
    DrawSpellPanel();
    return rec->m04;
}

/* 0EF3:2604: use the item in one of a member's rows. It switches on the item's type: type 0x22 or
   0x23, or a type 0x30 item whose flags low six bits are exactly 1, runs 0BD2:0106(who, slot); any
   other type 0x30 item just decrements its flags. Called through a table. The type 0x30 test is
   `left = flags & 0x3F; if (--left == 0)`: the temporary takes DI, which is why the original reloads
   slot from the frame in the case bodies instead of caching it (a direct `== 1` test caches slot
   in DI and compares with `cmp ax,1`). */
/*@ 0EF3:2604 */ void UseItemRow(int who, int slot)
{
    int item, left;

    item = gChars[who].m77[slot];
    switch (gTable8DD2[item].m02) {
    case 0x22:
    case 0x23:
        f0BD2_0106(who, slot);
        break;
    case 0x30:
        left = gTable8DD2[item].flags & 0x3F;
        if (--left == 0)
            f0BD2_0106(who, slot);
        else
            gTable8DD2[item].flags--;
        break;
    }
}

/* 0EF3:2676: cast spell `action` from the panel, in context `kind`. It remembers the spell value
   gEFB8, clears the panel member's effect flag 4 when the spell's ds:1344+0x12 bit 0 says so and
   the member has flag 4 set, copies the spell's apply callback into gFnEFBA, sets gF3B8 from the
   per-page sound table g1A1B (the member index, minus two when it is over three), and, for a spell
   whose flags have bit 1, says "You must have a free hand..." when both equipped slots are taken
   or "...two of this spell type active" when either slot is a discarded item, and returns. When a
   condition slot is open (g8DCE) it uses the item row (0EF3:2604); otherwise it removes the
   selected memorized slot (shift with movmem, then store its negation at slot 5). It formats
   "%s casts %s." with the member and spell names and draws it; for a spell with flag 1 it reports
   it refused and returns. Otherwise it makes the member current (gCurMember, gCurRec), plays the
   spell's sound, flashes the party for flags 6 (the member) or 8 (all), draws "ok", runs the apply
   callback, redraws the member for flag 4, and finishes the member effect (0EF3:026F) or the party
   panel. Called by 0EF3:28ED and 0EF3:2929. */
/*@ 0EF3:2676 */ void CastSpell(int action, int kind)
{
    struct Spell far *spell;
    int value;
    int index;

    gEFB8 = action;
    if ((gSpells1344[action].m12 & 1) && (gChars[gPanelMember].mE3 & 4))
        ClearEffectFlag4(&gChars[gPanelMember]);
    spell = &gSpells1344[action];
    gFnEFBA = spell->apply;
    index = gPanelMember;
    if (index > 3)
        index -= 2;
    gF3B8 = g1A1B[gFBCC * 4 + index];
    if (spell->flags & 2) {
        if (gChars[gPanelMember].m77[0] != 0 && gChars[gPanelMember].m77[1] != 0) {
            SayAndPlaySound(gText5943);
            return;
        }
        if (IsDiscardedItem(gChars[gPanelMember].m77[0]) ||
            IsDiscardedItem(gChars[gPanelMember].m77[1])) {
            SayAndPlaySound(gText596E);
            return;
        }
    }
    if (g8DCE == 0) {
        value = (gDD14 + gDDE2 * 6)[gA9C5];
        if (gA9C5 < 5)
            movmem((gDD14 + gDDE2 * 6 + gA9C5) + 1, gDD14 + gDDE2 * 6 + gA9C5, 5 - gA9C5);
        (gDD14 + gDDE2 * 6)[5] = -value;
    } else {
        UseItemRow(gPanelMember, kind);
    }
    sprintf(gFileNameBuf, gText599D, gChars[gPanelMember].name, spell->name);
    DrawText(gFileNameBuf);
    if (spell->flags & 1) {
        ReportActionRefused();
        return;
    }
    gCurMember = gPanelMember;
    gCurRec = &gChars[gPanelMember];
    PlaySound(gSpells1344[action].sound);
    if (spell->flags & 6)
        FlashPartyMembers(gCurMember);
    else if (spell->flags & 8)
        FlashPartyMembers(-1);
    DrawText(gText59AB);
    gFnEFBA();
    if (gSpells1344[action].flags & 4)
        RedrawMember(gCurMember);
    if (g8DCE != 0)
        FinishMemberEffect();
    else
        RedrawPartyPanel((struct ActRec far *)gF542);
}

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
    CastSpell(action, kind);
}

/* 0EF3:2929: the spell panel's list callback. It reads the action key (the node's +0x38): key 6
   walks the member's six spell slots once, stepping gA9C5 by -1 or 1 (the g5E1C button bits 0 and
   7) and clamping it into 0..6 until it lands on a nonzero slot, then runs the panel body
   0EF3:2295; any other key accepts the slot only when its value is positive or the key is 7,
   sets gA9C5 to the key when the key is under six and, below six, runs the panel body and casts
   the slot's spell through 0EF3:2676 (its value plus 0x19 when gF43D is 1), else redraws the
   party panel. Returns the node's +4 word. QUIRK 0EF3:2969: `found` is read uninitialised when the
   first loop finds no slot. Reached through the list node's callback30. The slot value plus 0x19
   when gF43D is 1 is a ternary (the assigned local keeps the frame size). */
/*@ 0EF3:2929 */ int HandleSpellList(struct ActionNode far *node)
{
    int key;
    int found;
    int step;
    int i;
    int value;

    key = node->key;
    if (key == 6) {
        for (i = 0; i < 6; i++) {
            if ((gDD14 + gDDE2 * 6)[i] > 0) {
                found = 1;
                break;
            }
        }
        if (found == 0)
            return node->m04;
        step = (g5E1C & 0x81) ? -1 : 1;
        do {
            gA9C5 += step;
            gA9C5 = ClampOrWrap(gA9C5, 0, 6, 1);
            if ((gDD14 + gDDE2 * 6)[gA9C5] > 0)
                break;
        } while (gA9C5 != 6);
        DrawSpellPanel();
    } else {
        if ((gDD14 + gDDE2 * 6)[key] > 0 || key == 7) {
            if (key < 6)
                gA9C5 = key;
            if (gA9C5 < 6) {
                DrawSpellPanel();
                value = (gDD14 + gDDE2 * 6)[gA9C5];
                value += (gF43D == 1 ? 0x19 : 0);
                CastSpell(value, 0);
            } else {
                RedrawPartyPanel((struct ActRec far *)gF542);
            }
        }
    }
    return node->m04;
}

/* 0EF3:2A53: Purpose unknown: a screen-space particle animation. It fills seven 150-word rows of
   the gBufDCFC scratch buffer: row0/row1 (the x and y positions) are zeroed, row2/row3 the x and
   y velocities take a random value, row5 a random frame step, row6 the frame counter starts at the
   depth `scale << 8`. It then loops while gFC66 is set: the erase pass restores each particle's
   saved rectangle (gFn809E at its x, y), the draw pass moves row2 one toward zero, advances row3 by
   5, adds row2 to row0 and row3 to row1, and when the particle leaves the region negates half of
   the matching velocity (row2 for the x bounds, row3 for the y bounds), saves the pixels (gFn8072)
   into row4 and picks the animation byte frameTable[row6 >> 8] (sign-extended); a zero byte
   retires the particle (its step row5 is zeroed). `scale` is the depth (also the right-shift of the
   position and the high byte of row6), `spread` the random amplitude, `count` the particle number
   (capped at 0x96), `divisor` the frame-step divisor, `falloff` the extra spread shift,
   `frameTable` the animation template. Called by 0EF3:00CD. */
/*@ 0EF3:2A53 */ void f0EF3_2A53(int scale, int spread, int count, int divisor, int falloff,
                                  void far *frameTable)
{
    int far *row0;
    int far *row1;
    int far *row2;
    int far *row3;
    int far *row4;
    int far *row5;
    int far *row6;
    int far *base;
    int left;
    int right;
    int bottom;
    int floorLimit;
    int minX;
    int maxX;
    register int bound;
    register int top;

    gFlags849E &= ~2;
    bound = g579B[scale];
    if (scale)
        scale--;
    base = (int far *)gBufDCFC;
    floorLimit = 0;
    minX = -100;
    maxX = 0x114;
    top = gF1;
    bottom = top + gF5 - 1;
    left = gEF << 3;
    right = left + (gF3 << 3);
    row0 = base;
    row1 = base + 0x96;
    row2 = base + 0x12C;
    row3 = base + 0x1C2;
    row4 = base + 0x258;
    row5 = base + 0x2EE;
    row6 = base + 0x384;
    gFC70 = gStatusLine;
    if (count > 0x96)
        count = 0x96;
    gFC68 = gDD18;
    gFC6A = gDD1A;
    for (gFC62 = 0; gFC62 < count; gFC62++) {
        row0[gFC62] = row1[gFC62] = 0;
        row2[gFC62] = RandomRange(0, spread) - (spread >> 1);
        row3[gFC62] = RandomRange(0, spread) - (spread >> 1) - (spread >> (8 - falloff));
        row5[gFC62] = RandomRange(0x400 / divisor, 0x800 / divisor);
        row6[gFC62] = scale << 8;
    }
    gFC66 = 2;
    while (gFC66 != 0) {
        if (gFC66 != 2) {
            for (gFC62 = count - 1; gFC62 >= 0; gFC62--) {
                gFC6C = (row0[gFC62] >> 6 >> scale) + gFC68;
                gFC6E = (row1[gFC62] >> 6 >> scale) + gFC6A;
                if (gFC6E > bound)
                    gFC6E = bound;
                if (IsPointInsideRect(gFC6C, gFC6E, left, top, right, bottom))
                    gFn809E(gFC6C, gFC6E, row4[gFC62]);
            }
        }
        gFC66 = 0;
        for (gFC62 = 0; gFC62 < count; gFC62++) {
            if (row2[gFC62] > 0)
                row2[gFC62]--;
            else
                row2[gFC62]++;
            row0[gFC62] += row2[gFC62];
            row3[gFC62] += 5;
            row1[gFC62] += row3[gFC62];
            row6[gFC62] += row5[gFC62];
            gFC6C = (row0[gFC62] >> 6 >> scale) + gFC68;
            gFC6E = (row1[gFC62] >> 6 >> scale) + gFC6A;
            if (gFC6E >= bound || gFC6E < floorLimit)
                row3[gFC62] = 0 - (row3[gFC62] >> 1);
            if (gFC6C >= maxX || gFC6C < minX)
                row2[gFC62] = 0 - (row2[gFC62] >> 1);
            if (gFC6E > bound)
                gFC6E = bound;
            gStatusLine = 2;
            gFC72 = gFn8072(gFC6C, gFC6E);
            gStatusLine = 0;
            row4[gFC62] = gFn8072(gFC6C, gFC6E);
            gFC64 = ((char far *)frameTable)[row6[gFC62] >> 8];
            if (gFC64 != 0) {
                gFC66 = 1;
                if (gFC72 == 0 && IsPointInsideRect(gFC6C, gFC6E, left, top, right, bottom))
                    gFn809E(gFC6C, gFC6E, gFC64);
            } else {
                row5[gFC62] = 0;
            }
        }
        WaitFrame();
        WaitFrame();
    }
    gFlags849E |= 2;
    gStatusLine = gFC70;
}

/* 0EF3:2EBC: the cone-of-cold animation. It fills eight 150-word rows of the
   gBufDCFC scratch buffer with a set of particles: for each of `count` particles it picks a random
   `radius` (between spread>>2 and spread, with spread shifted left 6), accumulates a quadratic
   `total` by adding `speed` (growing by `step` each pass) until it reaches the radius, then places
   the particle on one of four edges chosen by the low two bits of f1D8C_0000 (the animation value
   word follows it), mirroring both velocities when the next bit is set. It stores a random frame
   step row5 and a random hold row7. The main loop erases each particle (gFn809E) except on the
   first pass, integrates row0/row1 by row2/row3 (accelerating or decelerating by `step` or
   `sum` = 7/8 of `step` depending on the sign of the position and velocity), advances the frame
   counter row6 by row5, saves the pixels (gFn8072 into row4, and into `handle` once divisor>>2
   passes have elapsed) and looks up the frame table row6>>8; a zero entry retires the particle
   (row5 = 0) and a live one with no held particle under the pointer draws it (gFn809E). It runs
   until no particle is live, restores gStatusLine, increments `cycle`, and clears bit 1 of ds:849E.
   `count` is capped at 0x96, `divisor` scales the frame range, `frameTable` is the frame word
   table. `cycle` is only incremented (its value never used here). Called by 0EF3:18D8 with
   (0x96, 0x32, 0x0A, 1, 0x64, gText5737). */
/*@ 0EF3:2EBC */ void AnimateConeOfCold(int count, int spread, int step, int cycle, int divisor,
                                  int far *frameTable)
{
    int current;
    int state;
    int originX;
    int originY;
    int x;
    int y;
    int savedAttr;
    int handle;
    int frameCount;
    int far *row0;
    int far *row1;
    int far *row2;
    int far *row3;
    int far *row4;
    int far *row5;
    int far *row6;
    int far *row7;
    int far *base;
    int radius;
    int speed;
    int sum;
    register int i;
    register int total;

    base = (int far *)gBufDCFC;
    row0 = base;
    row1 = base + 0x96;
    row2 = base + 0x12C;
    row3 = base + 0x1C2;
    row4 = base + 0x258;
    row5 = base + 0x2EE;
    row6 = base + 0x384;
    row7 = base + 0x41A;
    spread <<= 6;
    gFlags849E &= ~2;
    savedAttr = gStatusLine;
    if (count > 0x96)
        count = 0x96;
    originX = 0x58;
    originY = 0x30;
    for (i = 0; i < count; i++) {
        radius = RandomRange(spread >> 2, spread);
        speed = total = 0;
        while (total < radius) {
            speed += step;
            total += speed;
        }
        switch (f1D8C_0000() & 3) {
        case 0:
            row0[i] = 0x20;
            row1[i] = total;
            row2[i] = speed;
            row3[i] = 0;
            break;
        case 1:
            row0[i] = total;
            row1[i] = 0x20;
            row2[i] = 0;
            row3[i] = speed;
            break;
        case 2:
            row0[i] = 0x20;
            row1[i] = -total;
            row2[i] = speed;
            row3[i] = 0;
            break;
        case 3:
            row0[i] = -total;
            row1[i] = 0x20;
            row2[i] = 0;
            row3[i] = speed;
            break;
        }
        if (f1D8C_0000() & 1) {
            row2[i] *= -1;
            row3[i] *= -1;
        }
        row5[i] = RandomRange(0x400 / divisor, 0x800 / divisor);
        row6[i] = 0;
        row7[i] = RandomRange(0, divisor >> 2);
    }
    state = 2;
    frameCount = 0;
    while (state != 0) {
        if (state != 2) {
            for (i = count - 1; i >= 0; i--) {
                x = (row0[i] >> 6) + originX;
                y = (row1[i] >> 6) + originY;
                gFn809E(x, y, row4[i]);
            }
        }
        state = 0;
        sum = (step >> 1) + (step >> 2) + (step >> 3);
        for (i = 0; i < count; i++) {
            if (row7[i] != 0) {
                row7[i]--;
            } else {
                if (row0[i] > 0) {
                    if (row2[i] > 0)
                        row2[i] -= step;
                    else
                        row2[i] -= sum;
                } else {
                    if (row2[i] < 0)
                        row2[i] += step;
                    else
                        row2[i] += sum;
                }
                if (row1[i] > 0) {
                    if (row3[i] > 0)
                        row3[i] -= step;
                    else
                        row3[i] -= sum;
                } else {
                    if (row3[i] < 0)
                        row3[i] += step;
                    else
                        row3[i] += sum;
                }
                row0[i] += row2[i];
                row1[i] += row3[i];
                row6[i] += row5[i];
            }
            x = (row0[i] >> 6) + originX;
            y = (row1[i] >> 6) + originY;
            if (divisor >> 2 > frameCount) {
                handle = 0;
            } else {
                gStatusLine = 2;
                handle = gFn8072(x, y);
                gStatusLine = 0;
            }
            row4[i] = gFn8072(x, y);
            current = frameTable[row6[i] >> 8];
            if (current != 0) {
                state = 1;
                if (handle == 0 && row7[i] == 0)
                    gFn809E(x, y, current);
            } else {
                row5[i] = 0;
            }
        }
        WaitFrame();
        frameCount++;
    }
    gStatusLine = savedAttr;
    cycle++;
    gFlags849E |= 2;
}
