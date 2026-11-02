/* Module at EOB.EXE 07A9:000A (487 bytes, the whole code segment): Turbo C++ 1.00, -ml -Z -O.
   StartupAlloc (07A9:000A) runs first. In code order: it makes the start-up far allocations
   through MemAlloc (7500 bytes at ds:8DCA, 6000 at ds:F43E, 120 at ds:DCAC the ADLIB sound map,
   4096 at ds:DCFC, 1600 at ds:F534 the ITEMRMP buffer; the sizes are the MemAlloc arguments at
   07A9:002F, 0046, 005D, 0074 and 008B, and whitepaper 4 names only ds:F534 and ds:DCAC), reads
   the segments of slots 7, 5 and 3 from the 19EC:0002 page/buffer table (ds:FC0C), loads the
   sound driver (177D:000A, 07A9:00D2), records the shared unpack work buffer as gSegsFC0C[7]
   with size 0xFC00 (1B82:0217, 07A9:00DF; 0xFC00 is slot 7's buffer size only in video case 4,
   src/E07C8.C:247, while cases 0 to 3 give slot 7 0xCB20 bytes, src/E07C8.C:213, 222, 231,
   237), clears ds:810E (07A9:00E6), loads the item tables (LoadItemData, 07A9:00EC), selects
   the 6x6 font as the current font (1A47:0169(gA9C0), 07A9:00F9) and reads ITEMRMP (LoadDecorateIcons,
   Ovl2 entry 8, 07A9:0100, then 0ACD:0E8A, 07A9:0105). The second function (07A9:010C SetMouseItem)
   redraws the item icon for the current slot (the one the mouse shows, ds:FBCE). Overlay
   entries are reached through the stub at 2106 (entries 2 and 8). Placeholder names remain where
   no evidence is given. */

/* Meaningful constants of this unit. */
/* Entry14.flags bit that gates the icon colour-row load in SetMouseItem (07A9:015C); the same test
   is in src/E0D1C.C:474. */
#define ITEM_FLAG_ICON 0x80
/* the MemAlloc flag that returns a paragraph-aligned, offset-0 block (src/E19BC.C:56-59) */
#define MEMALLOC_ALIGNED 0x20
/* the slot 7 page/buffer size, 0xFC00 bytes in video case 4 (src/E07C8.C:247) */
#define SLOT7_BUFFER_SIZE 0xFC00
/* the gIconsDDE4 index that aliases gDF48, the item description buffer (0xDDE4 + 0x59 * 4 = 0xDF48);
   slots 1..0x58 hold the item icons (src/EOV2.C:290) */
#define ICON_SLOT_DESC 0x59
/* the mouse pointer's hot spot, the centre of a 16 by 16 icon */
#define MOUSE_HOTSPOT 8

struct Entry14 {            /* 14-byte records at ds:8DD2 (ITEM.DAT records, whitepaper 4.4); the
                               same layout src/E0D1C.C:95 uses */
    unsigned char flags;    /* +0: bit 0x80 gates the icon draw in SetMouseItem (07A9:015C). Other
                               code tests 0x20 (0ACD:0A91, 0BD2:0B5E, 0BD2:0CB4), 0x08 (0DE8:082E)
                               and 0x10 (0DE8:0890); the byte is decremented (0EF3:266E) and stored
                               (122E:1D2B). */
    signed char icon;       /* +1: the item's icon number, an index into gIconsDDE4 (read at
                               07A9:0141, sign-extended by the cwde at 07A9:0145) */
    signed char m02;        /* +2: compared by 0D1C:00F1 and 0D1C:01CA (src/E0D1C.C) */
    char pad03;
    int m04;                /* +4: set to -1 by 0D1C:01CA (src/E0D1C.C) */
    char pad06[0x0B - 0x06];
    signed char m0B;        /* +0xB: compared by 0D1C:00F1 (src/E0D1C.C) */
    char pad0C[0x0E - 0x0C];
};

extern char gF43C; /*@ ds:F43C */  /* written by StartupAlloc (07A9:000D, set to 1); Purpose unknown */
extern char gFB45; /*@ ds:FB45 */  /* written by StartupAlloc (07A9:0024, set to 0); Purpose unknown */
extern void far *gBuf8DCA; /*@ ds:8DCA */  /* start-up far allocation, 7500 bytes (07A9:002F) */
extern void far *gBufF43E; /*@ ds:F43E */  /* start-up far allocation, 6000 bytes (07A9:0046) */
extern unsigned char far *gSoundMap; /*@ ds:DCAC */  /* the 120-byte ADLIB game-number to
                                                         driver-number map (src/E177D.C:13) */
extern void far *gBufDCFC; /*@ ds:DCFC */  /* start-up far allocation, 4096 bytes (07A9:0074) */
extern char far *gBufF534; /*@ ds:F534 */  /* the 1600-byte ITEMRMP buffer (07A9:008B,
                                              whitepaper 4.4) */
extern void far *gSegDCA6; /*@ ds:DCA6 */  /* slot 7's buffer segment, f19EC_0002(7) = MK_FP(gSegsFC0C[7], 0)
                                              (src/E19EC.C:20, 07A9:00A3). One entry of the table at ds:FC0C,
                                              not the table; it is the same word SetUnpackBuffer gets at
                                              07A9:00DB (`push word [ds:FC1A]`) */
extern void far *gSegDF4E; /*@ ds:DF4E */  /* slot 5's buffer segment, f19EC_0002(5) (07A9:00BA); written
                                              only, no reader found: Purpose unknown */
extern void far *gSegEFC2; /*@ ds:EFC2 */  /* slot 3's buffer segment, f19EC_0002(3) (07A9:00CB); written
                                              only, no reader found: Purpose unknown */
/* the page/buffer segment table (src/E07C8.C:49, src/E19EC.C:15), whose eighth word at ds:FC1A
   is slot 7, the segment gSegDCA6 holds (f19EC_0002(7), 07A9:00A3). */
extern unsigned gSegsFC0C[]; /*@ ds:FC0C */
/* the live MemAlloc block count (src/E19BC.C:73 adds one, src/E19BC.C:95 takes one away).
   StartupAlloc sets it to 0 at 07A9:00E6, after its five allocations, so they are no longer
   counted: Purpose unknown. */
extern unsigned g810E; /*@ ds:810E */
extern void far *gA9C0; /*@ ds:A9C0 */  /* the 6x6 font loaded from FONT6.FNT (src/E07C8.C:48,
                                           loaded at 07C8:0252) */
extern int g811F; /*@ ds:811F */  /* the current window's record index (src/E1DE7.C:45); src/EOV2.C:476
                                     and :524 index gRegions[g811F] */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */
extern int gMouseDisabled; /*@ ds:814A */  /* nonzero disables the mouse pointer code: the test at
                                              07A9:0148 (`cmp [814A],0; je`) runs SetMouseItem's body
                                              when zero; src/E0D1C.C:109 says the same */
extern int gFBCE; /*@ ds:FBCE */  /* the item index shown by the mouse (src/E0D1C.C:108); SetCurrentItem
                                     stores it (0D1C:0321) */
extern unsigned char gFBD2; /*@ ds:FBD2 */
extern int g8102; /*@ ds:8102 */  /* graphics choice, the argument digit after 0199 (src/E07C8.C:27);
                                     SetMouseItem tests it as "a colour row exists" */
/* a far function pointer called with a pointer to a colour table: a 16-byte row of the 1600-byte
   ITEMRMP buffer (ds:F534 + n<<4), the one-byte dummy at ds:0090, or the tables at ds:1CD3 and
   ds:1CE3 that other callers pass (08E6:0DFC `mov ax,0x1ce3` and 08E6:0E02 `mov ax,0x1cd3`,
   both before `lcall [ds:80D2]` at 08E6:0E06). That the rows are 16 bytes is [H] for ITEMRMP
   (whitepaper 4.4: 100 rows of 16 colours); the two DGROUP tables are not measured here, so
   [H] there too. No writer of ds:80D2 in the resident code (a literal-address search). */
extern void (far *gFn80D2)(void far *map); /*@ ds:80D2 */
extern void far *g80E2; /*@ ds:80E2 */  /* Purpose unknown: only passed as the third argument of
                                           f17A6_197E (07A9:0197) */
extern void far *gDF48; /*@ ds:DF48 */  /* the 0x12C-byte item description buffer (src/EOV2.C:253, 292):
                                           LoadItemSheets (src/EOV2.C:255) allocates it with
                                           MemAlloc(0x12C, 0) and stores it here (ovl2:041D calls
                                           00C0:0007 with (300, 0), ovl2:0425..0428 stores the returned
                                           far pointer) */
/* far pointers to item icon images, not names. ovl2:0029..0033 writes entry 0 (ds:DDE4 and
   ds:DDE6) from a separate call through [ds:8086] with cell (0,0) and the sheet gOverlayBuf
   (ds:113F, src/EOV2.C:149, passed to gGetSpriteCell at src/EOV2.C:209); the loop at ovl2:03D0
   starts with `mov si,1` and fills entries 1..0x58 from the same call with a 16x16 cell position
   on the 20-per-row ITEMICN sheet (the string at ds:677D). That entry 0 is "none" is a reading;
   the bytes show only that it comes from a different source. */
extern void far *gIconsDDE4[]; /*@ ds:DDE4 */

extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
extern void far *f19EC_0002(int kind); /*@ 19EC:0002 */
extern void LoadSoundDriver(void); /*@ 177D:000A */
extern void SetUnpackBuffer(unsigned seg, unsigned size); /*@ 1B82:0217 */
/* the overlay entry at stub 2106:002A resolves to ovl2:09CC LoadItemData (src/EOV2.C:449). */
extern void LoadItemData(void); /*@ 2106:002A */
extern void f1A47_0169(char far *font); /*@ 1A47:0169 */
/* the overlay entry at stub 2106:0048 resolves to ovl2:0577 LoadDecorateIcons (src/EOV2.C:328), which
   leads to the ITEMRMP read (ovl2:0484, via ovl2:0171; docs/design/WHITEPAPER.md, the ITEMRMP.VGA/EGA
   row). The stub is named after its target, like LoadItemData above. */
extern void LoadDecorateIcons(void); /*@ 2106:0048 */
extern void RunMapLoop(void); /*@ 0ACD:0E8A */
extern void SetCurrentItem(int n); /*@ 0D1C:0304 */
extern void f0BD2_05B5(int n); /*@ 0BD2:05B5 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
extern void f17A6_197E(void far *icon, void far *a, void far *b, int c); /*@ 17A6:197E */
/* 1BB1:0002 stores the hot-spot offset and a pointer shape record; the caller passes the hot spot
   (8,8) when n > 0 and (0,0) otherwise, and the record gIconsDDE4[n] in both cases (see below). */
extern void SetMousePointerShape(int hotX, int hotY, void far *icon); /*@ 1BB1:0002 */

/* 07A9:000A: the start-up routine called from `main` (07C8:017F passes argc, argv through). The name
   is the registry's and covers only its first part, the five far allocations below; it also loads
   the sound driver, the item tables, the font and the ITEMRMP icons. In code order it makes the
   start-up far allocations through MemAlloc (7500 at ds:8DCA, 6000 at ds:F43E, 120 at ds:DCAC the
   ADLIB sound map, 4096 at ds:DCFC, 1600 at ds:F534 the ITEMRMP buffer; the sizes are the MemAlloc
   arguments at 07A9:002F, 0046, 005D, 0074 and 008B, and whitepaper 4 names only ds:F534 and
   ds:DCAC), reads the segments of slots 7, 5 and 3 from the 19EC:0002 page/buffer table (ds:FC0C),
   loads the sound driver (177D:000A, 07A9:00D2), records the shared unpack work buffer as
   gSegsFC0C[7] with size 0xFC00 (1B82:0217, 07A9:00DF; see the
   header for the size's dependence on the video mode), clears ds:810E (07A9:00E6), loads the
   item tables (LoadItemData, Ovl2 stub entry 2, 07A9:00EC), selects the 6x6 font as the current
   font (1A47:0169(gA9C0), 07A9:00F9), and reads ITEMRMP (LoadDecorateIcons, Ovl2 stub entry 8,
   07A9:0100, then 0ACD:0E8A, 07A9:0105). Sets ds:F43C = 1 and ds:FB45 = 0. */
/*@ 07A9:000A */ void StartupAlloc(int argc, char far * far *argv)
{
    gF43C = 1;
    /* QUIRK 07A9:0012: the test's result is unused: both branches reach 07A9:0024, so the body
       is empty (07A9:0012..0024). */
    if (argc == 0 && argv[1][1])
        ;
    gFB45 = 0;
    /* only this block asks for flag 0x20 (the others pass 0): E19BC.C:56-59 turns it into an
       offset-0 block, the block shape E1B82.C:17-18 describes for the work buffer. Why this
       block alone gets the flag is Purpose unknown. */
    gBuf8DCA = MemAlloc(7500L, MEMALLOC_ALIGNED);
    gBufF43E = MemAlloc(6000L, 0);
    gSoundMap = MemAlloc(120L, 0);
    gBufDCFC = MemAlloc(4096L, 0);
    gBufF534 = MemAlloc(1600L, 0);
    gSegDCA6 = f19EC_0002(7);
    gSegDF4E = f19EC_0002(5);
    gSegEFC2 = f19EC_0002(3);
    LoadSoundDriver();
    SetUnpackBuffer(gSegsFC0C[7], SLOT7_BUFFER_SIZE);
    g810E = 0;
    LoadItemData();
    f1A47_0169(gA9C0);
    LoadDecorateIcons();
    RunMapLoop();
}

/* 07A9:010C: sets the item shown by the mouse to `n` and shows its icon. SetCurrentItem stores the
   index n in ds:FBCE (gFBCE, the item index shown by the mouse, src/E0D1C.C:108) unless
   gMouseDisabled; src/E0D1C.C:270 and :298 say an item is cleared and redrawn through it. A nonzero
   n also passes " taken.\r" (ds:0094) to 1DE7:060A after 0BD2:05B5 when g811F == 7, and the record
   gTable8DD2[n].icon then supplies the icon number. When the current record gTable8DD2[gFBCE] has
   flag 0x80 and gFBD2 bit 0 is set, it selects the icon's 16-byte colour row (gBufF534 + (n << 4),
   or the dummy byte at ds:0090 when g8102 == 0), passes it to ds:80D2, passes the icon to
   17A6:197E, and then passes gIconsDDE4[ICON_SLOT_DESC] to 1BB1:0002 at (8,8), or at (0,0) when the
   number is 0. Why g811F == 7 gets the " taken." message is Purpose unknown. */
/*@ 07A9:010C */ void SetMouseItem(int n)
{
    SetCurrentItem(n);
    if (n) {
        if (g811F == 7) {
            f0BD2_05B5(n);
            DrawText(" taken.\r");
        }
        n = gTable8DD2[n].icon;
    }
    if (!gMouseDisabled) {
        if ((gTable8DD2[gFBCE].flags & ITEM_FLAG_ICON) && (gFBD2 & 1)) {
            if (g8102)
                gFn80D2(gBufF534 + (n << 4));   /* the icon's 16-byte colour row */
            else
                /* QUIRK 07A9:0184: when g8102 == 0 the argument is the one-byte literal at
                   ds:0090, a dummy. g8102 only chooses the ITEMRMP name, "VGA" (ds:6790) when
                   it is 3 and "EGA" (ds:6794) otherwise (ovl2:0432..0446); the ITEMRMP read
                   itself (ovl2:0478) has no branch on g8102. Why g8102 == 0 gets the dummy is
                   Purpose unknown. */
                gFn80D2("");
            f17A6_197E(gIconsDDE4[n], gDF48, g80E2, g8102);
            /* gIconsDDE4[0x59] is gDF48 (0xDDE4 + 0x59 * 4 = 0xDF48), the same pointer just passed
               as the second argument: the icon passed to 1BB1:0002 for a record whose flag has
               0x80 set. gDF48's writer is in the overlay (ovl2:0425). */
            n = ICON_SLOT_DESC;
        }
        if (n > 0)
            SetMousePointerShape(MOUSE_HOTSPOT, MOUSE_HOTSPOT, gIconsDDE4[n]);
        else
            SetMousePointerShape(0, 0, gIconsDDE4[n]);
    }
}
