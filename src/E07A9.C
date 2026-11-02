/* Module at EOB.EXE 07A9:000A (487 bytes, the TLINK code segment that starts at 07A9:000A):
   Turbo C++ 1.00, -ml -Z -O. The 10 bytes before the segment, 07A9:0000 to 07A9:0009
   (mov [0071],ax; mov ax,[88A2]; mov [006F],ax; ret), are also code, but no unit covers them
   (re/units.toml). StartupAlloc (07A9:000A) is this unit's first function; main calls it at
   src/E07C8.C:157, inside `if (g3092)`, after it sets the palette, the text attributes and the
   callback (src/E07C8.C:140 to 152). In code order: it makes the start-up far
   allocations through MemAlloc (7500 bytes at ds:8DCA, 6000 at ds:F43E, a 120-byte sound map at
   ds:DCAC (game number to driver number, read from the driver file), 4096 at ds:DCFC, 1600 at
   ds:F534 the ITEMRMP buffer; the sizes are the MemAlloc arguments at 07A9:002F, 0046, 005D,
   0074 and 008B, and whitepaper 4 names only ds:F534 and ds:DCAC), reads the segments of
   slots 7, 5 and 3 from the 19EC:0002 page/buffer table (ds:FC0C), loads the sound driver
   (177D:000A, 07A9:00D2), passes the slot 7 segment (gSegsFC0C[7], the word at ds:FC1A) and size
   0xFC00 to SetUnpackBuffer (1B82:0217, 07A9:00DB/00DF; 0xFC00 is the unpacker's work-buffer
   requirement, see UNPACK_BUFFER_SIZE). Slot 7's own block is 0xCB20 bytes in cases 0 to 3
   (src/E07C8.C:213, 222, 231, 237) and 0xFC00 only in case 4 (src/E07C8.C:247). It clears ds:810E
   (07A9:00E6), loads the item tables (LoadItemData, 07A9:00EC), selects the 6x6 font as the
   current font (1A47:0169(gA9C0), 07A9:00F9) and loads the item sheets (LoadDecorateIcons, Ovl2
   entry 8, 07A9:0100), which reads ITEMRMP (FileRead at ovl2:047F, inside LoadItemSheets,
   src/EOV2.C:255 and :296, called at src/EOV2.C:333). It ends by running the map loop (RunMapLoop,
   0ACD:0E8A, 07A9:0105), which runs the game. The second function (07A9:010C SetMouseItem)
   records the item shown by the mouse (SetCurrentItem stores ds:FBCE) and installs the item's
   icon as the mouse pointer image (SetMousePointerShape, 1BB1:0002, at 07A9:01E6). Overlay entries
   are reached through the stub at 2106
   (entries 2 and 8). Placeholder names remain where no evidence is given. */

/* Meaningful constants of this unit. */
/* Entry14.flags bit that, with gFBD2 bit 0, gates the flagged branch of SetMouseItem (07A9:015C to
   01B1): the icon colour row, the translated copy and the pointer image; when either gate fails the
   branch is skipped (07A9:0161 and 0168, je 0x1bc). The same test is in src/E0D1C.C:478. The bit
   selects the colour-row draw path: in f0D1C_070F (src/E0D1C.C:478 to 484) a record whose flags
   have the bit, when gFBD2 bit 0 is set too, is drawn through its icon's colour row (gFn80D2) and
   f1A29_008B; any other record, or the same record with gFBD2 bit 0 clear, goes through
   f1A29_000F. The same byte is tested for 0x20, 0x08 and 0x10 elsewhere (see Entry14.flags
   below). */
#define ITEM_FLAG_COLOUR_ICON 0x80
/* the MemAlloc flag that returns a paragraph-aligned, offset-0 block: the bit is defined at
   src/E19BC.C:49 (described at :44-46), the block shape is described at src/E19BC.C:62-66 (the
   block starts at offset 0 of a paragraph, its header at offset 15), and the code is at
   src/E19BC.C:81 and :95-98 */
#define MEMALLOC_ALIGNED 0x20
/* the size SetUnpackBuffer is given for the shared unpack work buffer, 0xFC00 in every video
   mode (07A9:00D7). It is the unpacker's own work-buffer requirement, not the size of slot 7's
   block: the work segment holds the LZW dictionary the format 1 and format 2 decoders build
   (format 1's 3C00h work buffer, docs/design/WHITEPAPER.md:157; the decoders are called with it,
   src/E1B82.C:166 Unpack1 and :175 Unpack2), while the unpacked output goes to the destination
   segment. Format 1's dictionary stops below its 0x3C00 (cmp bp,0x3C00 at 1D9A:0214 and 1D9A:026A,
   src/E1D9A.ASM:336 and :369), inside slot 7's block; format 2's two tables run to 0xFC00 (the
   entry offsets at 2 * (code - 0x100), the lengths at +0x7E00, described at src/E1D9A.ASM:424-429
   and built at :483, :494 and :499), past the 0xCB20 block of cases 0 to 3, but no pinned file
   uses format 2 [H] (docs/design/WHITEPAPER.md:157). Slot 7's own block is 0xCB20 bytes in cases
   0 to 3 (src/E07C8.C:213, 222, 231, 237) and 0xFC00 only in case 4 (src/E07C8.C:247) */
#define UNPACK_BUFFER_SIZE 0xFC00
/* the gIconsDDE4 index that aliases gDF48, the translated icon image shown as the mouse pointer for
   flagged items (0xDDE4 + 0x59 * 4 = 0xDF48); slots 1..0x58 hold the item icons (src/EOV2.C:290) */
#define ICON_SLOT_TRANSLATED 0x59
/* the mouse pointer's hot spot, the centre of a 16x16 icon [H]: the 16x16 cell comes from
   src/EOV2.C:290 (gGetSpriteCell(..., 2, 0x10, ...)), and the unit of the cell's width argument
   is not cited */
#define MOUSE_HOTSPOT 8

struct Entry14 {            /* 14-byte records at ds:8DD2 (ITEM.DAT records, whitepaper 4.4); the
                               same layout src/E0D1C.C:96 uses */
    unsigned char flags;    /* +0: bit 0x80, with gFBD2 bit 0, gates the flagged branch of SetMouseItem
                               (07A9:015C): the colour row, the translated copy and the pointer image.
                               Other code tests 0x20 (0ACD:0A91, 0BD2:0B5E, 0BD2:0CB4), 0x08 (0DE8:082E)
                               and 0x10 (0DE8:0890); the byte is decremented (0EF3:266E) and stored
                               (122E:1D2B). */
    signed char icon;       /* +1: the item's icon number, an index into gIconsDDE4 (read at
                               07A9:0141 and sign-extended by the cbw, byte 98h, at 07A9:0145) */
    signed char m02;        /* +2: compared by 0D1C:00F1 and 0D1C:01CA (src/E0D1C.C) */
    char pad03;
    int m04;                /* +4: set to -1 by 0D1C:01CA (src/E0D1C.C) */
    char pad06[0x0B - 0x06];
    signed char m0B;        /* +0xB: compared by 0D1C:00F1 (src/E0D1C.C) */
    char pad0C[0x0E - 0x0C];
};

extern char gF43C; /*@ ds:F43C */  /* written by StartupAlloc (07A9:000D, set to 1); read by
                                      DrawMemberHp, which draws the "HP" label instead of
                                      "%3d of %-3d" when it is set (src/E1547.C:269, the comment at
                                      src/E1547.C:235-238). Purpose unknown */
extern char gFB45; /*@ ds:FB45 */  /* written by StartupAlloc (07A9:0024, set to 0); the overlay 3
                                      LoadMapScreen sets it to 1 (src/EOV3.C:686), and the overlay 4
                                      DrawViewFrame (ovl4:1F51, src/EOV4.C:878) tests it with video mode 3
                                      (src/EOV4.C:883) and clears it (src/EOV4.C:890; declared "request
                                      flag, cleared here", src/EOV4.C:857). What the request is is
                                      Purpose unknown */
extern void far *gBuf8DCA; /*@ ds:8DCA */  /* start-up far allocation, 7500 bytes (07A9:002F); the base
                                              of the 122E action-script driver's script (declared
                                              src/E122E.C:22): JumpToScriptOffset (122E:139D,
                                              src/E122E.C:1001) adds the script's word offset to it
                                              (src/E122E.C:1005, the comment at :999-1000), and
                                              PushScriptPointer (122E:1422, src/E122E.C:1034) does the
                                              same (src/E122E.C:1044, the comment at :1031-1033).
                                              EvaluateScriptExpr (122E:147F, a draft, src/E122E.C:1056)
                                              jumps to the base with the word offset at src/E122E.C:1217,
                                              and RunMemberScript (122E:1DA8, src/E122E.C:1399) starts its
                                              script walk at it (src/E122E.C:1423). Overlay
                                              3 uses it too: LoadMapScreen unpacks LEVEL<n>.INF into it
                                              (src/EOV3.C:573-574, "the 7500-byte unpacked level
                                              script", src/EOV3.C:200), reads the item records through
                                              it (the script word and item loop, src/EOV3.C:666-681) and
                                              passes gBuf8DCA + 0x1a to LoadWallTextures
                                              (src/EOV3.C:682). Why the start-up block holds the script
                                              is Purpose unknown */
extern void far *gBufF43E; /*@ ds:F43E */  /* start-up far allocation, 6000 bytes (07A9:0046); the
                                              overlay 3 LoadMapScreen fills it from the level graphics
                                              file (src/EOV3.C:579-580, "the 6000-byte level
                                              graphics file", src/EOV3.C:201 [H]), and the overlay 4
                                              page-buffer readers CopyViewColumn (its source
                                              assignment at src/EOV4.C:171) and ToggleViewColumn
                                              (source at src/EOV4.C:208, pixel read at :214) read it
                                              as the page buffer (src/EOV4.C:155-156). Purpose
                                              unknown */
extern unsigned char far *gSoundMap; /*@ ds:DCAC */  /* the 120-byte sound map (game number to
                                                         driver number), read from the driver file
                                                         (adlib.dat or pcgamesn.dat, src/E177D.C:2-3) */
extern void far *gBufDCFC; /*@ ds:DCFC */  /* start-up far allocation, 4096 bytes (07A9:0074); the
                                              overlay 3 level-state work buffer: PackLevelState
                                              (ovl3:0FD1, src/EOV3.C:692) fills it with the gB894
                                              bytes (src/EOV3.C:701-704) and compresses it
                                              (CompressLcw, src/EOV3.C:705; the "Map compression
                                              failure" message at :708); UnpackLevelState
                                              (ovl3:109D, src/EOV3.C:719) calls Unpack4 to fill
                                              it (src/EOV3.C:728) and reads it back into gB894
                                              (src/EOV3.C:729-731). E19FC.ASM:40 describes the caller
                                              that fills the block before compression (through
                                              ds:DCFC and ds:DCFE) */
extern char far *gBufF534; /*@ ds:F534 */  /* the 1600-byte ITEMRMP buffer (07A9:008B,
                                              whitepaper 4.4). Its readers: the ITEMRMP read
                                              (FileRead of 0x640 bytes into it, src/EOV2.C:296, in
                                              LoadItemSheets) and the icon colour row (src/E0D1C.C:480,
                                              gFn80D2(gBufF534 + (icon << 4))) */
extern void far *gSegDCA6; /*@ ds:DCA6 */  /* the far pointer f19EC_0002(7) returns (src/E19EC.C:20,
                                              07A9:00A3): offset 0, segment gSegsFC0C[7], the word at
                                              ds:FC1A that SetUnpackBuffer gets at 07A9:00DB. Overlay 3
                                              and 4 use it too: LoadWallTextures writes the
                                              0xCB20-byte texture block through it (src/EOV3.C:471),
                                              expands and masks it (src/EOV3.C:474-478 and :482; the
                                              comment above LoadWallTextures, src/EOV3.C:454-459), and
                                              DrawViewFrame draws from it (DrawTile(gSegDCA6), src/EOV4.C:881;
                                              declared "the far pointer DrawTile draws",
                                              src/EOV4.C:862) */
extern void far *gSegDF4E; /*@ ds:DF4E */  /* slot 5's far pointer, f19EC_0002(5) (07A9:00BA); the
                                              action screen's swap buffer: Action01E3 (src/E0ACD.C:263)
                                              blits it to the action field through gFn804A
                                              (src/E0ACD.C:269), Action0264 (src/E0ACD.C:287) copies a
                                              0xF3-byte character record through it (three Unpack0
                                              copies, src/E0ACD.C:307-309), and FindWordPanel
                                              passes it to gFn807E and gFn804A (src/EOV2.C:482, 522);
                                              RedrawRowArea (0BD2:0ABE, src/E0BD2.C:622) passes it to
                                              gSaveRect with the same 0x12 by 0xA8 field that
                                              Action01E3 blits (src/E0ACD.C:269).
                                              Overlay 3 uses it too: LoadObjects (ovl3:09AA,
                                              src/EOV3.C:503) reads 0x1000 bytes into it (FileRead at
                                              src/EOV3.C:513) and copies it back (src/EOV3.C:518-525),
                                              LoadWallSet (ovl3:004F, src/EOV3.C:250) puts its
                                              decorations at gSegDF4E + 0x3E80 (src/EOV3.C:265-266,
                                              274), and LoadWallTextures (ovl3:07A5, src/EOV3.C:460)
                                              expands the texture plane tables into it in EGA mode 2
                                              (src/EOV3.C:474-478) */
extern void far *gSegEFC2; /*@ ds:EFC2 */  /* slot 3's far pointer, f19EC_0002(3) (07A9:00CB); a
                                              shared overlay 3 buffer (src/EOV3.C:130 declares it
                                              with no comment), used four ways. Level state:
                                              LoadLevelState (ovl3:1147, src/EOV3.C:739) reads 0x7F8
                                              bytes of LEVELS.TMP into it (FileRead at
                                              src/EOV3.C:745) and unpacks them (UnpackLevelState,
                                              ovl3:109D, src/EOV3.C:719, called at src/EOV3.C:747);
                                              SaveLevelState (ovl3:11AF, src/EOV3.C:753) packs
                                              gSegEFC2 + 0xC800 back to LEVELS.TMP (the +0xC800
                                              normalisation at src/EOV3.C:758, PackLevelState at
                                              :759). Wall texture plane tables:
                                              LoadWallTextures (ovl3:07A5, src/EOV3.C:460) unpacks
                                              the 16-byte planes from gSegEFC2+2 and +0x12
                                              (src/EOV3.C:469-470) and the 0xCB20-byte block from
                                              gSegEFC2+0x22 (src/EOV3.C:471). The LEVELS.TMP screen
                                              block: SaveGame (ovl3:1368, src/EOV3.C:811) reads
                                              0x5FA0 bytes into it (src/EOV3.C:821) and writes them
                                              back (src/EOV3.C:839). The EOBDATA.SAV blocks: LoadGame
                                              (ovl3:15B9, src/EOV3.C:863) uses it as its buffer
                                              (src/EOV3.C:876 and :888) for the six sized block reads.
                                              Overlay 3 reads the far pointer again (ds:EFC2 and
                                              ds:EFC4): ovl3:1186, 119E,
                                              11B6 (ovl3:1147, the loader above), ovl3:0815, 0833,
                                              0855 (ovl3:07A5), ovl3:1396, 13C0, 14BF (ovl3:1368),
                                              ovl3:15C1, 164D (ovl3:15B9, the function that calls
                                              SetMouseItem). The resident E17A6 reader calls it the
                                              saved screen buffer (src/E17A6.ASM:22), read at
                                              src/E17A6.ASM:256-257 when g8102 != 2 [H]. What the
                                              buffer is is Purpose unknown */
/* the page/buffer segment table (src/E07C8.C:49, src/E19EC.C:15), whose eighth word at ds:FC1A
   is slot 7, the segment gSegDCA6 holds (f19EC_0002(7), 07A9:00A3). */
extern unsigned gSegsFC0C[]; /*@ ds:FC0C */
/* the net allocation counter: MemAlloc adds one at src/E19BC.C:112 and MemFree subtracts one at
   src/E19BC.C:137, so it is the number of successful MemAlloc calls minus the number of MemFree
   calls and can fall below the number of live blocks (src/E19BC.C:17-21). StartupAlloc sets it
   to 0 at 07A9:00E6, after its five allocations, so they are no longer counted: Purpose unknown. */
extern unsigned gAllocCount; /*@ ds:810E */
extern void far *gA9C0; /*@ ds:A9C0 */  /* the 6x6 font loaded from FONT6.FNT (src/E07C8.C:48 and
                                           :252, the call at 07C8:0419, the store at 07C8:0420); the
                                           6x6 size is eob/FONT6.FNT bytes 102h and 103h (6 and 6, the
                                           height and width bytes src/E1A47.C:81-85 reads),
                                           docs/design/WHITEPAPER.md:169 [V] */
extern int g811F; /*@ ds:811F */  /* this unit tests it as 7 to gate the " taken." message
                                     (07A9:011E, 0123); FindWordPanel saves it (src/EOV2.C:476) and
                                     indexes gRegions[g811F] with it (src/EOV2.C:489-492, 517-520) */
extern struct Entry14 gTable8DD2[]; /*@ ds:8DD2 */
extern int gMouseDisabled; /*@ ds:814A */  /* nonzero disables the mouse pointer code: the test at
                                              07A9:0148 (`cmp [814A],0; je`) runs SetMouseItem's body
                                              when zero. main sets it before StartupAlloc: 1 when
                                              !gMouseHandlerInstalled, else !mouse
                                              (src/E07C8.C:153-156);
                                              src/E0D1C.C:110 says the same */
extern int gFBCE; /*@ ds:FBCE */  /* the item index shown by the mouse (src/E0D1C.C:109); SetCurrentItem
                                     stores it (0D1C:0311) */
extern unsigned char gFBD2; /*@ ds:FBD2 */  /* bit 0, with Entry14.flags bit 0x80, gates the flagged
                                                 branch of SetMouseItem (07A9:0163 to 01B1); the same
                                                 bit gates the icon colour row in 0D1C:070F
                                                 (src/E0D1C.C:50); read at 07A9:0163 and
                                                 src/E0D1C.C:478 */
extern int g8102; /*@ ds:8102 */  /* the mode word SetMouseItem tests at 07A9:016A (the colour row
                                     against the ds:0090 dummy) and passes to TranslateImage (07A9:018F);
                                     the switch at 17A6:199E tests it (cases 0, 2 and 3). src/ reads
                                     it four ways: "graphics choice" (src/E07C8.C:27), "the video
                                     mode ... 3 and 4 select the 6-bit cells" (src/E1DE7.C:46), "the
                                     video mode the game initialised (3 is CGA-compatible)"
                                     (src/E082C.C:41; the bare phrase at src/E1547.C:55), and "nonzero: a colour row
                                     exists" (src/E0D1C.C:51 and src/E0BD2.C:100) */
/* a far function pointer whose argument is a colour row (an array of colour indices): a 16-byte
   row of the 1600-byte ITEMRMP buffer (ds:F534 + n<<4), the one-byte dummy at ds:0090, or the
   colour tables at ds:1CD3 and ds:1CE3 that other callers pass (08E6:0DFC `mov ax,0x1ce3` and
   08E6:0E02 `mov ax,0x1cd3`, both before `lcall [ds:80D2]` at 08E6:0E06). That the rows are 16
   bytes is [H] for ITEMRMP (whitepaper 4.4: 100 rows of 16 colours); the two DGROUP tables are
   not measured here, so [H] there too. No writer of ds:80D2 in EOB.EXE or the overlays (a
   literal-address search). */
extern void (far *gFn80D2)(void far *colors); /*@ ds:80D2 */
extern void far *g80E2; /*@ ds:80E2 */  /* passed as the third argument of TranslateImage (07A9:0197),
                                           which reads it as the xlatb table (BX and DX from [bp+0xe]
                                           and [bp+0x10], 17A6:1995 and 1998; xlatb at 17A6:19BE).
                                           Purpose unknown: what writes ds:80E2 is not checked here */
extern void far *gDF48; /*@ ds:DF48 */  /* the 0x12C-byte buffer LoadItemSheets allocates and stores
                                           here (the far call at ovl2:041D; the result is stored at
                                           ovl2:0425 and 0428). This unit
                                           shows it as the translated icon image: TranslateImage's
                                           destination (pushed at 07A9:019B and 019F), installed as the
                                           mouse pointer through gIconsDDE4[0x59] (07A9:01B9, 01C6 to
                                           01CE). The allocation's segment word 00C0 is an overlay
                                           segment fixup, a byte offset into TLINK's segment table
                                           (index times 8, docs/design/WHITEPAPER.md:100), so index 24,
                                           paragraph 19BC (build/re/eob.json), the segment that holds
                                           MemAlloc at offset 7: the callee is MemAlloc [H]. src/EOV2.C:253
                                           calls it the item description buffer [H] */
/* one far pointer per icon to the image it holds, indexed by icon number; slot 0 holds the
   pointer shape (src/EOV2.C:205-211). ovl2:0029..0033 writes entry 0 (ds:DDE4 and
   ds:DDE6) from a separate call through [ds:8086] with cell (0,0) and the sheet gOverlayBuf
   (ds:113F, src/EOV2.C:149, passed to gGetSpriteCell at src/EOV2.C:209); the loop at ovl2:03D0
   starts with `mov si,1` and fills entries 1..0x58 from the same call with a 16x16 cell position
   on the 20-per-row ITEMICN sheet (the string at ds:677D). */
extern void far *gIconsDDE4[]; /*@ ds:DDE4 */

extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
/* returns MK_FP(gSegsFC0C[kind | 1], 0): offset 0 and the segment word at the table slot
   (src/E19EC.C:20); the placeholder name stays. */
extern void far *f19EC_0002(int kind); /*@ 19EC:0002 */
extern void LoadSoundDriver(void); /*@ 177D:000A */
extern void SetUnpackBuffer(unsigned seg, unsigned size); /*@ 1B82:0217 */
/* the overlay entry at stub 2106:002A resolves to ovl2:09CC LoadItemData (src/EOV2.C:449). */
extern void LoadItemData(void); /*@ 2106:002A */
/* sets gFont1A47 = font and reads the font's width and height bytes (src/E1A47.C:81-85); the
   placeholder name stays. */
extern void f1A47_0169(char far *font); /*@ 1A47:0169 */
/* the overlay entry at stub 2106:0048 resolves to ovl2:0577 LoadDecorateIcons (src/EOV2.C:328), which
   leads to the ITEMRMP read (ovl2:047F, via ovl2:0171; docs/design/WHITEPAPER.md, the ITEMRMP.VGA/EGA
   row). The stub is named after its target, like LoadItemData above. */
extern void LoadDecorateIcons(void); /*@ 2106:0048 */
extern void RunMapLoop(void); /*@ 0ACD:0E8A */
extern void SetCurrentItem(int n); /*@ 0D1C:0304 */
/* 0BD2:05B5 draws item `item`'s name into the text buffer (src/E0BD2.C:426-437), with the flag
   bit 0x40 and name-table cases. Called at 07A9:0126 with the item index, just before the
   " taken.\r" message (07A9:012D), when g811F == 7. Why the name is asked for here is Purpose
   unknown. */
extern void DrawItemName(int item); /*@ 0BD2:05B5 */
extern void DrawText(char far *text); /*@ 1DE7:060A */
/* 17A6:197E copies from the first argument to the second through a translation table: it loads
   ds:si from the first (17A6:1989-198C; the loop uses movsw and lodsb) and es:di from the second
   (17A6:198F-1992; stosw and stosb); the third is the xlatb table, its offset and segment in BX
   and DX (17A6:1995, 1998) with `mov ds,dx` before each xlatb (17A6:19BE, 19E4, 19F2); the mode
   word (17A6:199B) picks the path (cases 0, 2 and 3 at 17A6:199E to 19C8). The call
   (07A9:0193..01B1) passes the icon far pointer first, then gDF48 (the translated icon image),
   g80E2 (Purpose unknown, its only reader is 07A9:0197) and g8102. */
extern void TranslateImage(void far *src, void far *dst, void far *xlat, int mode); /*@ 17A6:197E */
/* 1BB1:0002 stores the hot-spot offset and a pointer shape record; the caller passes the hot spot
   (8,8) when item > 0 and (0,0) otherwise, and the far pointer gIconsDDE4[item] in both cases (see below). */
extern void SetMousePointerShape(int hotX, int hotY, void far *icon); /*@ 1BB1:0002 */

/* 07A9:000A: the start-up routine called from `main` (07C8:017F passes argc, argv through). The name
   is the registry's and covers only its first part, the five far allocations below; besides them it
   loads the sound driver, the item tables, the font and the ITEMRMP icons, and its last call runs
   the map loop. In code order it makes the start-up far allocations through MemAlloc (7500 at
   ds:8DCA, 6000 at ds:F43E, a 120-byte sound map at ds:DCAC, 4096 at ds:DCFC, 1600 at
   ds:F534 the ITEMRMP buffer; the sizes are the MemAlloc arguments at 07A9:002F, 0046, 005D, 0074
   and 008B, and whitepaper 4 names only ds:F534 and ds:DCAC), reads the segments of slots 7, 5 and
   3 from the 19EC:0002 page/buffer table (ds:FC0C), loads the sound driver (177D:000A, 07A9:00D2),
   passes the slot 7 segment (gSegsFC0C[7], the word at ds:FC1A) and size 0xFC00 to SetUnpackBuffer
   (1B82:0217, 07A9:00DB; see the header for the size's relation to the video mode), clears ds:810E
   (07A9:00E6), loads the item tables (LoadItemData, Ovl2 stub entry 2, 07A9:00EC), selects the 6x6
   font as the current font (1A47:0169(gA9C0), 07A9:00F9), and loads the item sheets
   (LoadDecorateIcons, Ovl2 stub entry 8, 07A9:0100), which reads ITEMRMP (FileRead at ovl2:047F,
   inside LoadItemSheets, src/EOV2.C:255 and :296). It ends by running the map loop (RunMapLoop,
   0ACD:0E8A, 07A9:0105), which runs the game. Sets ds:F43C = 1 and ds:FB45 = 0. */
/*@ 07A9:000A */ void StartupAlloc(int argc, char far * far *argv)
{
    gF43C = 1;
    /* QUIRK 07A9:0012: the condition's result is unused, so the body is empty (07A9:0012..0024):
       with argc == 0 the bytes still read argv[1] (les bx,[bp+8]; les bx,es:[bx+4], 07A9:0018 and
       001B) and its byte 1 (cmp byte es:[bx+1],0, 07A9:001F); only the result is dropped, both
       branches reaching 07A9:0024, so a port keeps the read. */
    if (argc == 0 && argv[1][1])
        ;
    gFB45 = 0;
    /* of the five start-up allocations only this block asks for flag 0x20 (the other four pass 0;
       0x20 also appears in src/E07C8.C:211-247 and in the two Unpack allocations at src/E1B82.C:165
       and :174): the bit is defined at src/E19BC.C:49 (described at :44-46), and src/E19BC.C:62-66
       (code at :81 and :95-98) turns it into an offset-0 block, the block shape E1B82.C:21-25
       describes for the work buffer. Why this block alone gets the flag is Purpose unknown. */
    gBuf8DCA = MemAlloc(7500L, MEMALLOC_ALIGNED);
    gBufF43E = MemAlloc(6000L, 0);
    gSoundMap = MemAlloc(120L, 0);
    gBufDCFC = MemAlloc(4096L, 0);
    gBufF534 = MemAlloc(1600L, 0);
    gSegDCA6 = f19EC_0002(7);
    gSegDF4E = f19EC_0002(5);
    gSegEFC2 = f19EC_0002(3);
    LoadSoundDriver();
    SetUnpackBuffer(gSegsFC0C[7], UNPACK_BUFFER_SIZE);
    gAllocCount = 0;
    LoadItemData();
    f1A47_0169(gA9C0);
    LoadDecorateIcons();
    RunMapLoop();
}

/* 07A9:010C: sets the item shown by the mouse to `item` and shows its icon. SetCurrentItem stores the
   index item in ds:FBCE (gFBCE, the item index shown by the mouse, src/E0D1C.C:109) unless
   gMouseDisabled; src/E0D1C.C:274 says an item is cleared and redrawn through SetMouseItem, and the
   call SetMouseItem(0) is at src/E0D1C.C:302 (the tests leading to it at :298 and :300). A nonzero
   item also passes " taken.\r" (ds:0094) to 1DE7:060A after 0BD2:05B5 when g811F == 7, and the
   record gTable8DD2[item].icon then supplies the icon number. When the current record
   gTable8DD2[gFBCE] has flag 0x80 and gFBD2 bit 0 is set, the flagged branch runs (07A9:015C to
   01B1): the icon's 16-byte colour row (gBufF534 + (item << 4), item now the icon number, or the
   dummy byte at ds:0090 when g8102 == 0) is passed to ds:80D2, the icon is translated into gDF48
   by 17A6:197E, and the translated image becomes the mouse pointer (item set to ICON_SLOT_TRANSLATED, 0x59, so item > 0 at
   07A9:01BC, and gIconsDDE4[ICON_SLOT_TRANSLATED] passed to 1BB1:0002 at (8,8)). When either gate
   fails the branch is skipped (07A9:0161 and 0168, je 0x1bc) and the plain item icon becomes the
   pointer (gIconsDDE4[item] at 07A9:01BC onward); on that path the (0,0) call is when item is 0
   (the item is cleared) or its icon number is 0 or below (07A9:01BC `or si,si`, `jle 0x1d4` at
   07A9:01BE). Why g811F == 7 gets the " taken." message is Purpose unknown. */
/*@ 07A9:010C */ void SetMouseItem(int item)
{
    SetCurrentItem(item);
    if (item) {
        if (g811F == 7) {
            DrawItemName(item);
            DrawText(" taken.\r");
        }
        item = gTable8DD2[item].icon;   /* the item index becomes the icon number */
    }
    if (!gMouseDisabled) {
        /* the record of the index SetCurrentItem stored (gFBCE), not item, which is the icon
           number by now */
        if ((gTable8DD2[gFBCE].flags & ITEM_FLAG_COLOUR_ICON) && (gFBD2 & 1)) {
            if (g8102)
                gFn80D2(gBufF534 + (item << 4));   /* the icon's 16-byte colour row */
            else
                /* QUIRK 07A9:0184: when g8102 == 0 the argument is the one-byte literal at
                   ds:0090, a dummy. g8102 only chooses the ITEMRMP name, "VGA" (ds:6790) when
                   it is 3 and "EGA" (ds:6794) otherwise (ovl2:0432..0446); the ITEMRMP read
                   itself (ovl2:047F) has no branch on g8102. Why g8102 == 0 gets the dummy is
                   Purpose unknown. */
                gFn80D2("");
            TranslateImage(gIconsDDE4[item], gDF48, g80E2, g8102);
            /* gIconsDDE4[0x59] is gDF48 (0xDDE4 + 0x59 * 4 = 0xDF48), the same pointer just passed
               as the second argument: the translated icon image shown by 1BB1:0002 as the mouse
               pointer for a record whose flag has 0x80 set. gDF48's writer is in the overlay
               (ovl2:0425). */
            item = ICON_SLOT_TRANSLATED;
        }
        if (item > 0)
            SetMousePointerShape(MOUSE_HOTSPOT, MOUSE_HOTSPOT, gIconsDDE4[item]);
        else
            SetMousePointerShape(0, 0, gIconsDDE4[item]);
    }
}
