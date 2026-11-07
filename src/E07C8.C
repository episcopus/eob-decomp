/* Unit at EOB.EXE 07C8:0001 (1610 bytes, 5 functions): the program's main, its start-up, and its
   shut-down. main (07C8:0001) reads the command line (graphics, sound, mouse, the "homie" marker,
   the video mode to restore), installs the INT 3Fh handler and the two allocation/exit hooks, then
   runs 07C8:0199: it sets the video mode, allocates the page/buffer table at ds:FC0C, loads the
   overlays and the two fonts and turns sound on. For the Tandy graphics choice (gGraphicsChoice 1)
   only it checks the memory chain, reserves 2 x 32 KB below the probed video segment and stores
   atol(gArgv->number) in gMemTopSeg (:311; ReserveMemoryBelowVideo(3) returns the Tandy buffer
   base, written at :345; whether that value is the video top is [H], src/E1990.ASM:386-397).
   For the VGA choice it clears A000:0000. On the way out 07C8:058A stops sound, calls the exit hook, removes the mouse handler, shuts the overlay cache down, restores the INT 1Ch vector, removes the keyboard, restores the video mode, puts back the old INT 3Fh vector, shrinks memory for the Tandy graphics choice only (TrimMemoryToVideo, :476-477), and exits. 07C8:061D returns 1 (the exit hook at ds:80FA) and 07C8:0625 is the allocation failure hook at ds:810A. WHITEPAPER 2.6 and 4.4. Compiled -ml -O -Z. */

#include <dos.h>

#ifndef NULL
#define NULL 0
#endif
/* Named constants for values whose meaning the code establishes (READABILITY 4). */
#include "EOB.H"
#define PAGE_52000 0xCB20L    /* a 52000-byte buffer page */
enum { DEFAULT_TEXT_ATTR1 = 0x0F, DEFAULT_TEXT_ATTR2 = 0x0C, INT3F_VECTOR = 0x3F,
       PALETTE_ENTRY15_OFFSET = 0x2D, DAC_MAX = 0x3F,   /* top of a 6-bit DAC [H] */
       TEXT_SCREEN_SEG = 0xB800, VGA_SCREEN_SEG = 0xA000,
       PALETTE_SIZE = 0x300, PAGE_32000 = 0x7D00, PAGE_32576 = 0x7F40,
       VGA_SEG_A200 = 0xA200, VGA_SEG_A400 = 0xA400, VGA_SEG_A800 = 0xA800,
       VGA_SEG_AC00 = 0xAC00 };
struct ArgVector {          /* the program's argument vector, which the C library's _argv (ds:8894)
                               points at (the rebuild links ds:8894 to the library's _argv,
                               playbook 50 item 23) */
    char far *arg[6];       /* argv[0] to argv[5]: not read in src/ */
    char far *number;       /* argv[6]: the numeric argument StartupGame parses with atol */
};

extern int gGraphicsChoice;                     /*@ ds:8102: graphics choice: argv[1][0] + 1 at :229, less
                                         0x31 in 0199 (:298), so the digit 0..4 indexes
                                         gOverlayFiles (ds:3074, the five name pointers in EOB.EXE
                                         order: 0 cga, 1 tandy, 2 ega, 3 mcga/VGA, 4 vga;
                                         whitepaper 2.6: 1 Tandy, 3 VGA) */
extern char gSoundCard;               /*@ ds:B892: sound choice, argument digit minus '1' (0 AdLib,
                                         1 PC speaker, 2 Tandy); the handlers cover values 0 to 2.
                                         The digit '4' gives 3, '0' gives -1 and '5' to '9' give 4 to 8,
                                         and neither the start-up switch (:409) nor the shut-down
                                         switch (:449) handles those, so no card is started for them;
                                         gSoundOn (:231) is 1 for every digit except '4'. src/E177D.C:52 declares it unsigned char, but an unsigned char here does not match (the switch promotes it differently), so char keeps the code) */
extern int gSoundOn;                  /*@ ds:80E6: nonzero unless sound was '4' at start-up, and
                                         toggled at run time (`xor word ptr [gSoundOn],1` at
                                         src/E1AAD.ASM:974) and written by the sound option
                                         (src/EOV1.ASM:3741). src/E1AAD.ASM:17 declares it BYTE
                                         and src/EOV1.ASM:115 WORD; int keeps the code */
extern unsigned char gTandySelected;           /*@ ds:009E: records that the Tandy card was chosen: its only
                                         store is 1 on StartupGame's Tandy sound path (07C8 image 0x81E8,
                                         next to gTandyOn = 1); the PC speaker path leaves it at its load
                                         value. PlaySound and PlaySoundAtCell test it == 1 to gate the
                                         gTandyOn toggle (src/E177D.C:193; re/renames.toml, ds:009E) */
extern unsigned char gTandyOn;        /*@ ds:8A67: the driver select: 1 is Tandy, 0 the PC speaker
                                         (src/E1948.ASM:7, tested at :279, :379, :500, :527 and
                                         :553). StartupGame sets it to 1 on the Tandy path (:435),
                                         ShutdownGame clears it (:462) */
extern unsigned gRestoreMode;         /*@ ds:84A0: the video mode restored on exit: set from argv[5]
                                         (:235), put back through SetVideoMode(7) on exit (:474, :494)
                                         and on the error paths (:302, :321, :419; src/E1DDD.C:12-21) */
extern void interrupt (far *gSavedInt3F)();  /*@ ds:FC08: the previous INT 3Fh vector: main saves the
                                         vector in place before it installs its handler, here (:236-237),
                                         and ShutdownGame puts it back (:475). src/E1A72.ASM:158 calls it
                                         the previous INT 3Fh vector and 1A72:0006 chains to it */
extern int (far *gExitHook)(void);      /*@ ds:80FA: exit hook main sets to 07C8:061D (:244); its
                                         target returns int (ExitHook, :486) and the result is
                                         ignored at :467 */
extern int (far *gAllocFailHook)(void); /*@ ds:810A: allocation failure hook. The image starts it at OutOfMemory (19BC:02B6, src/E19BC.C:23-25); main replaces it with 07C8:0625 (:245), so at run time it is AllocationFailure, not OutOfMemory */
extern int gMouseHandlerInstalled;    /*@ ds:8148: 1 once the mouse driver handler was installed
                                         (src/E1A72.ASM:337), 0 when no driver is found (:310,
                                         the L0118 path); declared at src/E1A72.ASM:92-96 */
extern int gMouseDisabled;            /*@ ds:814A: nonzero disables the mouse handler (src/E0ACD.C:111,
                                         src/E122E.C:48, src/E1C00.ASM:281-290). main sets it to 1
                                         when no driver is installed (:267) and otherwise to
                                         !mouseChoice (:269), so the mouse argument decides it */
extern unsigned char gStartupDone;           /*@ ds:3092: start-up-completed flag: set to 1 at :406, just
                                         before the sound card is started (:409-437), and every
                                         `return 1` of StartupGame (:439) comes after it; the only
                                         reader in src/ is main at :265, which then sets the mouse and
                                         calls StartupAlloc (nothing else in src/ reads ds:3092) */
extern unsigned char gOverlayCacheUp;           /*@ ds:3056: set to 1 at :402 right after InitOverlayCache and
                                         tested only by ShutdownGame (:470) before
                                         ShutdownOverlayCache; no other unit reads ds:3056 */
extern int gKeyboardOptions;                     /*@ ds:80EA: the keyboard driver's option bits (load-time
                                         value 0110h; the driver tests bits 2, 80h and 200h of its copy
                                         kModifierState, src/E1AAD.ASM:208, :619), set to 1 by main (:261), after
                                         InstallKeyboard (:246), so the install copy (InstallKeyboard,
                                         1AAD:0718, src/E1AAD.ASM:832) takes the load-time value,
                                         not 1; the value 1 reaches kModifierState only through the
                                         key-state query _GetKeyState (1AAD:05F1,
                                         src/E1AAD.ASM:587-612, the copy at :573), which copies on
                                         every call */
extern int gTextAttr2;                /*@ ds:811B: the second text attribute, the fill colour: ClearTextRegion
                                         fills the window with it (src/E1DE7.C:241), the redrawn scroll line fills
                                         with it and passes it to the draw-text hook (src/E1DE7.C:270-271); main sets DEFAULT_TEXT_ATTR2 (0xC) at start-up (:263); declared in src/E122E.C:74 and src/E1DE7.C:44, read with gTextAttr1 in src/EOV2.C:566 */
extern int gTextAttr1;                /*@ ds:811D: the first text attribute, set by main to
                                         DEFAULT_TEXT_ATTR1 (0xF) at start-up (:262) and passed to the draw-text
                                         hook (src/E1DE7.C:271); declared in src/E122E.C:74 and src/E1DE7.C:42, read with gTextAttr2 in src/EOV2.C:566. That it is the foreground colour is [H]: the game meaning of the two attributes is not established (src/E1A29.C:27-28) */
extern void (far *gSetPalette)(void far *table); /*@ ds:80BA: the palette setter (src/EOV4.C:1251, :1060), called with the palette buffers (:253, :258, :388, :398); Purpose unknown beyond the palette: nothing in src/ assigns ds:80BA */
extern void (far *gClearPage)(void);     /*@ ds:8056: a graphics driver entry (slot 11 of the ds:802A
                                         table) that clears the current draw page's segment (MCGA.OVL
                                         0693, EGA.OVL 07FB; re/renames.toml, ds:8056); StartupGame calls it
                                         once the page table is built, before the fonts load. Nothing in
                                         src/ assigns ds:8056 */
extern unsigned char far *gPaletteBuf;      /*@ ds:FE68: the palette buffer StartupGame allocates. The
                                         gGraphicsChoice - 1 case labels 0 and 1 (graphics choices 1 and 2, :385)
                                         get 16 bytes, of which only byte 0 (:386, 0) and byte 0xf
                                         (:387, 0xf) are written, the other 14 keeping what MemAlloc
                                         returned; the setter is given gBuiltinPalette (:388), not this, so those 16
                                         bytes have no reader in src/. Case labels 2 and 3 (:392) get 0x300
                                         bytes; main loads eobpal.col over it for choices 3 and 4 (:257), and the VGA choice (gGraphicsChoice == 3) has src/EOV3.C:662 load `<script>.PAL` over it and src/EOV4.C:1270-1277 send it to the setter when the request flag gLevelPalettePending is set (set at src/EOV3.C:767) and clear gLevelPalettePending */
extern unsigned char gBuiltinPalette[];  /*@ ds:3057: the table passed to the palette setter for graphics choices 1 and 2 (no eobpal.col). Its length is unknown: no source or doc gives it, and the 16 bytes written at :385-387 belong to the gPaletteBuf buffer, not this. Whether it is a palette is [H] (src/EOV4.C:1251) */
extern void far *gFont8x8;            /*@ ds:FE5A: the 8x8 font loaded from Font8.fnt (whitepaper
                                         4.4, docs/design/WHITEPAPER.md:171: FONT8.FNT is 8 x 8,
                                         checked by 1A47:0005; the file is 0x104 + 128 x 8 bytes) */
extern void far *gFont6x6;               /*@ ds:A9C0: the 6x6 font loaded from FONT6.FNT (whitepaper
                                         4.4: FONT6.FNT is 6 x 6, checked by 1A47:0005; the file
                                         is 0x104 + 128 x 6 bytes) */
extern unsigned gPageSegs[];          /*@ ds:FC0C: the page/buffer segment table */
extern unsigned far gMemTopSeg;       /*@ 1990:0002: the segment where program memory must end
                                         (src/E1990.ASM:20-22); StartupGame stores the number atol
                                         parses from the string that gArgv->number points at over it
                                         on the Tandy path only (:311). Whether that value is the video top is [H] (src/E1990.ASM:386-397) */
extern struct ArgVector far *gArgv;   /*@ ds:8894: the C library's _argv, which the startup
                                         code sets; StartupGame reads argv[6] through it
                                         (gArgv->number, :311) */
extern char far *gOverlayFiles[];     /*@ ds:3074: the five overlay name pointers (cga/tga/ega/
                                         mcga/vga), indexed by the graphics choice; the order is
                                         the EOB.EXE name order and whitepaper 2.6 (1 Tandy,
                                         3 VGA). Index 4 names vga.ovl, a file EOB.EXE names but
                                         which is not in this copy (whitepaper 2.5, the section
                                         before 2.6); the VGA mode loads MCGA.OVL (whitepaper 2.6, oracle trace) */
extern struct OverlayRequest gOverlayRequest; /*@ ds:8121: the request RunOverlayRequest runs */

extern void PrintString(char far *s);             /*@ 19EA:0000 */
extern int WaitGameKey(void);                     /*@ 1AAD:053A: blocks for a game key and returns
                                                     its key code (the result of ProcessKeyEvent,
                                                     src/E1AAD.ASM:517-523); every call here ignores
                                                     the result */
extern void RemoveKeyboard(void);                     /*@ 1AAD:09F6 */
extern void SetVideoMode(int index); /*@ 1DDD:002D: INT 10h set video mode from the ds:84A2 table,
                                      or, for index 7 or more, from gRestoreMode; src/E1DDD.C:6-21
                                      and :43-49 */
extern void TrimMemoryToVideo(void);              /*@ 1990:027D */
extern void RemoveMouseHandler(void);                   /*@ 1A72:0285 */
extern void ShutdownOverlayCache(void);                     /*@ 1C0E:13FD */
extern void RemoveTickHandler(void);         /*@ 1D8F:0038: restores the saved INT 1Ch tick handler
                                         (src/E1D8F.ASM:1-7) */
extern void SoundDriverSilenceToOneVoice(void);               /*@ 1948:046B */
extern void SoundDriverUninstall(void);           /*@ 1948:03DA */
extern void StopAdlibDriver(void);                /*@ 0000:3FCA: the e_text AdLib driver off: restores
                                                     the saved INT 08h vector, resets the AdLib through
                                                     ResetVoices and stops PIT channel 0 (src/E_TEXT.ASM:495-522) */
extern void exit(int status);                     /*@ 0000:50F3 */
extern long atol(char far *s);                    /*@ 0000:5D63 */
extern int strcmp(char far *a, char far *b);      /*@ 0000:679D */
extern long LargestFreeBlock(void);               /*@ 19BC:023D */
extern void LoadFile(char far *name, void far *buf, unsigned len); /*@ 1B82:0009 */
extern void StartupAlloc(int argc, char far * far *argv); /*@ 07A9:000A */
extern void InstallKeyboard(void);                     /*@ 1AAD:0718 */
extern void InstallHardErr(void);                 /*@ 1A63:0019 */
extern void InstallTickHandler(void);         /*@ 1D8F:0010: installs the INT 1Ch tick handler and saves
                                         the old vector (src/E1D8F.ASM:1-7) */
/* 16A5:0D78: stores its 32-bit second argument as a far pointer at ds:FC7C (the callee reads
   [bp+8] into ds:FC7C and [bp+0xa] into ds:FC7E) and ignores the first word argument. main's call
   SetNodeCallback(0, 0L) (:264) therefore clears the node callback: from then until something sets
   it, the callback the mouse dispatcher fires for the node under the pointer does nothing
   (src/E16A5.C:6-7, :258-259, :679-684). The word first is what makes Turbo C push 0L then the
   word, exactly as the original does. */
extern void SetNodeCallback(int arg, long value);      /*@ 16A5:0D78 */
extern void far *LoadFont(char far *name);  /*@ 1A47:0005: opens a bitmap font file, validates
                                                 it and returns it whole (src/E1A47.C:1-6) */
extern void SetFont(void far *font);  /*@ 1A47:0169: remembers the font and its two geometry
                                            bytes as the current font (src/E1A47.C:1-6) */
extern int CheckMemoryChain(void);                /*@ 1990:0071 */
extern unsigned ReserveMemoryBelowVideo(int steps); /*@ 1990:0133: probes the segment at the top of
                                                 video memory and reserves (steps-1) x 32 KB below
                                                 it; the caller passes 3 (a 64 KB reserve), the only
                                                 call in src/ (src/E1990.ASM:216-225) */
extern void InitMouse(void);                      /*@ 1A72:00DA */
extern int RunOverlayRequest(void far *req);      /*@ 1C00:0055 */
extern void InitOverlayCache(void);                     /*@ 1C0E:13E7 */
extern void SetTextWindow(int n);                    /*@ 1DE7:0009 */
extern void CalibrateRetrace(void);               /*@ 1A65:000E */
extern void SoundDriverInstall(void);             /*@ 1948:0377 */
extern void SoundDriverResume(void);              /*@ 1948:0458 */
extern void StartAdlibDriver(void);               /*@ 0000:3F77: the e_text AdLib driver on: hooks
                                                     INT 08h (at 3E66), programs PIT channel 0 and
                                                     resets the AdLib through ResetVoices (src/E_TEXT.ASM:451-493) */
extern long time(long far *tloc);                 /*@ 0000:5831 */
extern void srand(unsigned seed);                 /*@ 0000:55A6 */
extern void far *memset(void far *dst, int c, unsigned n); /*@ 0000:6384 */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 19BC:0007 */
/* Declared as a far object, not a function: the matcher cannot map a far function pointer taken from
   another unit and passed as an argument, but an object at 1A72:0006 gives the same segment:offset
   when its address is taken. Int3FHandler is the INT 3Fh handler main installs (E1A72.ASM). */
extern unsigned char far Int3FHandler;            /*@ 1A72:0006 */

int StartupGame(void);
void ShutdownGame(void);
int ExitHook(void);
int AllocationFailure(void);

/*@data */
/* the data of e07c8, ds:3056 to ds:31AC (342 bytes, 5 relocations): drafted by make data-skeleton from the original's bytes; each item at its annotation, in address order. */
unsigned char gOverlayCacheUp = 0x00; /*@ ds:3056 */
unsigned char gBuiltinPalette[16] = { /*@ ds:3057 */
    0x00, 0x05, 0x03, 0x02, 0x0A, 0x0E, 0x0C, 0x06, 0x04, 0x0B, 0x09, 0x01,
    0x00, 0x08, 0x07, 0x0F
};
char gFont8Name[] = "Font8.fnt"; /*@ ds:3067: a named array, not a literal: the code's two uses share it */
unsigned char g3071[3] = { 0x00, 0x00, 0x00 }; /*@ ds:3071 */
char far *gOverlayFiles[5] = { "cga.ovl", "tga.ovl", "ega.ovl", "mcga.ovl", "vga.ovl" }; /*@ ds:3074 */
unsigned char g3088[10] = { 0xCB, 0x02, 0x45, 0x4F, 0x42, 0x2E, 0x45, 0x58, 0x45, 0x00 }; /*@ ds:3088 */
unsigned char gStartupDone = 0x00; /*@ ds:3092 */
char gFontErrorText[] = "\r\nUnable to load font "; /*@ ds:3093: before the pool, so an array */
char gReinstallText[] = ". Reinstall program.\r\n"; /*@ ds:30AA */
/* ds:30EA-30F0: the code's (a literal or template, 'homie'): written by its function */
/* ds:30F0-3114: the code's (a literal or template, '\r\nNot enough memory to run program.'): written by its function */
/* ds:3114-311F: the code's (a literal or template, 'eobpal.col'): written by its function */
/* ds:311F-3139: the code's (a literal or template, '\r\nMemory chain corrupt!\r\n'): written by its function */
/* ds:3139-3154: the code's (a literal or template, '\r\nCould not load overlay "'): written by its function */
/* ds:3154-317A: the code's (a literal or template, '".  Press a key to return to DOS...\r\n'): written by its function */
/* ds:317A-3184: the code's (a literal or template, 'FONT6.FNT'): written by its function */
/* ds:3184-31AC: the code's (a literal or template, '\r\nAllocation failure, returning to DOS!'): written by its function */

/* 07C8:0001: main. Reads the four arguments the launcher passes (graphics, sound and mouse, all one
   character, and the five-letter "homie" marker) and the video mode to restore, installs the
   INT 3Fh handler (keeping the old vector in ds:FC08) and the exit and allocation failure hooks,
   calls InstallKeyboard (:246), InstallHardErr (:247) and the INT 1Ch tick handler install
   InstallTickHandler (:248), then runs the start-up 07C8:0199. The launcher is INTRO.EXE, which START1.EXE
   starts and which passes the arguments on (whitepaper 2.6).
   When start-up succeeds it switches on the graphics choice reduced by 1 (gGraphicsChoice - 1): choices 1
   and 2 take the built-in colour table at ds:3057 (length unknown; whether it is a palette is [H]), choices 3 and 4 load eobpal.col into
   ds:FE68, both through the far function at ds:80BA; a graphics choice of 0 (CGA) matches neither
   case and sets no palette. It then calls 16A5:0D78, and, when gStartupDone is set, sets the mouse disabled
   flag from the mouse argument (:265-269) and calls StartupAlloc. Returns 0, and
   always runs the shut-down 07C8:058A after the start-up `if`; only the homie check (:233) and the
   memory check (:240) return before it, and that memory-check return comes after the INT 3Fh vector is set (:237), so it leaves Int3FHandler installed and does not put back the old vector ds:FC08. The function name is `Main`, not `main`, because the
   matcher's link stub already defines `_main` for the C0 startup. */
/*@ 07C8:0001 */ int Main(int argc, char far * far *argv)
{
    int mouseChoice;                        /* argv[3][0] - '0': 1 turns the mouse on at :269, 0
                                               leaves it disabled */

    gGraphicsChoice = argv[1][0] + 1;
    gSoundCard = argv[2][0] - '1';
    gSoundOn = argv[2][0] != '4';
    mouseChoice = argv[3][0] - '0';
    if (strcmp(argv[4], "homie"))
        return 0;
    gRestoreMode = atol(argv[5]);
    gSavedInt3F = getvect(INT3F_VECTOR);
    setvect(INT3F_VECTOR, (void interrupt (far *)())&Int3FHandler);
    /* 0xFDE8L is 65000, the largest single page the table asks MemAlloc for (choice 3, :360);
       LargestFreeBlock returns bytes (src/E19BC.C:4-5, :189), so below it even that page has no room */
    if (LargestFreeBlock() < 0xFDE8L) {
        PrintString("\r\nNot enough memory to run program.");
        return 0;
    }
    gExitHook = ExitHook;
    gAllocFailHook = AllocationFailure;
    InstallKeyboard();
    InstallHardErr();
    InstallTickHandler();
    if (StartupGame() == 1) {
        switch (gGraphicsChoice - 1) {
        case 0:
        case 1:
            gSetPalette(gBuiltinPalette);
            break;
        case 2:
        case 3:
            LoadFile("eobpal.col", gPaletteBuf, PALETTE_SIZE);
            gSetPalette(gPaletteBuf);
            break;
        }
        gKeyboardOptions = 1;
        gTextAttr1 = DEFAULT_TEXT_ATTR1;
        gTextAttr2 = DEFAULT_TEXT_ATTR2;
        SetNodeCallback(0, 0L);                /* clears the node callback ds:FC7C */
        if (gStartupDone) {
            if (!gMouseHandlerInstalled)
                gMouseDisabled = 1;
            else
                gMouseDisabled = !mouseChoice;
            StartupAlloc(argc, argv);
        }
    }
    ShutdownGame();
    return 0;
}
/* 07C8:0199: the start-up. First it reduces gGraphicsChoice to the graphics choice itself: main stored
   argv[1][0] + 1 (:229) and this subtracts 0x31 (:298), so the result is digit - '0', the
   graphics choice (1 Tandy, 3 VGA; whitepaper 2.6). For the Tandy graphics choice it checks the
   memory chain (CheckMemoryChain) and calls ReserveMemoryBelowVideo, which stores the end of DOS
   memory in gMemTopSeg (src/E1990.ASM:20-22) and returns gVideoTopSeg, the base of the reserve
   below the probed segment, not the probe itself (src/E1990.ASM:27-28, :237-238); it writes that
   base as memtop into the page table slots 2 and 3 (:345), which src/E1990.ASM:27-30 calls the
   base of a 64000 byte Tandy buffer [H], a size this file does not give. It then overwrites gMemTopSeg
   with atol(gArgv->number) (:311; whether it is the video top is [H], src/E1990.ASM:386-397). For the VGA graphics choice it clears
   A000:0000 with memset. Then it sets the video mode, resets the mouse, loads the graphics overlay
   named by gGraphicsChoice through the table at ds:3074 (cga/tga/ega/mcga/vga.ovl), builds the page/buffer
   segment table at ds:FC0C for the graphics choice, loads the two fonts, sets up the palette buffer,
   starts the overlay manager, seeds rand from the clock and turns the chosen sound card on. Returns
   1 when it got that far, 0 on the errors it prints, and 0 on the `fail` path, taken before the
   reduce when gGraphicsChoice is 0x36 (main added 1 to argv[1][0] at :229, so argv[1][0] is '5'), without
   printing. The three `goto`s into the sound cases (:411, :413, :415) keep the three sound bodies behind `fail`, which is the order the original has. */
/*@ 07C8:0199 */ int StartupGame(void)
{
    int memtop;

    if (gGraphicsChoice == 0x36)
        goto fail;
    gGraphicsChoice -= 0x31;
    switch (gGraphicsChoice) {
    case 1:
        if (CheckMemoryChain()) {
            SetVideoMode(7);
            PrintString("\r\nMemory chain corrupt!\r\n");
            WaitGameKey();
            return 0;
        }
        SetVideoMode(gGraphicsChoice);
        memtop = ReserveMemoryBelowVideo(3);   /* 3 steps reserves 2 x 32 KB = 64 KB below the probe */
        /* the segment where program memory must end: atol parses the numeric string the far pointer
           gArgv->number (+0x18) points at (the C library's start-up code, C0.ASM, sets _argv; the string is an argument the
           launcher passes, src/E1990.ASM:23-26) */
        gMemTopSeg = (unsigned)atol(gArgv->number);
        break;
    case 3:
        memset(MK_FP(VGA_SCREEN_SEG, 0), 0, PAGE_64000);
        break;
    }
    SetVideoMode(gGraphicsChoice);
    InitMouse();
    gOverlayRequest.name = gOverlayFiles[gGraphicsChoice];
    if (RunOverlayRequest(&gOverlayRequest) == -1) {
        SetVideoMode(7);
        PrintString("\r\nCould not load overlay \"");
        PrintString(gOverlayRequest.name);
        PrintString("\".  Press a key to return to DOS...\r\n");
        WaitGameKey();
        return 0;
    }
    /* the page/buffer segment table ds:FC0C (src/E19EC.C:1-2, src/E07A9.C:11), eight segment words,
       filled per graphics choice: screen pages and the pages backing the buffers. Choices 0 and 2 use
       0xFA00 (64000), 0x7D00 (32000) and 0xCB20 (52000); choice 1 uses 0x7F40 (32576) for slots 4
       and 5 (:346) and 0xCB20 for slots 6 and 7 (:347), with no 0xFA00; choice 3 uses 0xFDE8 (65000)
       for slots 2 and 3 (:360); choice 4 uses 0xFA00 (slots 1, 3 and 5) and 0xFC00 (slot 7). */
    switch (gGraphicsChoice) {
    case 0:
        gPageSegs[0] = gPageSegs[1] = TEXT_SCREEN_SEG;
        gPageSegs[2] = gPageSegs[3] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gPageSegs[4] = gPageSegs[5] = FP_SEG(MemAlloc(PAGE_32000, MEMALLOC_ALIGNED));
        gPageSegs[6] = gPageSegs[7] = FP_SEG(MemAlloc(PAGE_52000, MEMALLOC_ALIGNED));
        break;
    case 1:
        /* the original writes slots 0 and 1 as two immediate stores and slots 2 and 3 through ax
           (the Tandy graphics choice); keep the separate statements so the code is byte for byte. */
        gPageSegs[0] = TEXT_SCREEN_SEG;
        gPageSegs[1] = TEXT_SCREEN_SEG;
        gPageSegs[3] = gPageSegs[2] = memtop;
        gPageSegs[4] = gPageSegs[5] = FP_SEG(MemAlloc(PAGE_32576, MEMALLOC_ALIGNED));
        gPageSegs[6] = gPageSegs[7] = FP_SEG(MemAlloc(PAGE_52000, MEMALLOC_ALIGNED));
        break;
    case 2:
        gPageSegs[0] = VGA_SCREEN_SEG;
        gPageSegs[2] = VGA_SEG_A200;
        gPageSegs[3] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gPageSegs[4] = VGA_SEG_A200;
        gPageSegs[5] = FP_SEG(MemAlloc(PAGE_32000, MEMALLOC_ALIGNED));
        gPageSegs[6] = VGA_SEG_A200;
        gPageSegs[7] = FP_SEG(MemAlloc(PAGE_52000, MEMALLOC_ALIGNED));
        break;
    case 3:
        gPageSegs[0] = gPageSegs[1] = VGA_SCREEN_SEG;
        gPageSegs[2] = gPageSegs[3] = FP_SEG(MemAlloc(PAGE_65000, MEMALLOC_ALIGNED));
        gPageSegs[4] = gPageSegs[5] = FP_SEG(MemAlloc(PAGE_32000, MEMALLOC_ALIGNED));
        gPageSegs[6] = gPageSegs[7] = FP_SEG(MemAlloc(PAGE_52000, MEMALLOC_ALIGNED));
        break;
    case 4:
        gPageSegs[0] = VGA_SCREEN_SEG;
        gPageSegs[1] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gPageSegs[2] = VGA_SEG_A400;
        gPageSegs[3] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gPageSegs[4] = VGA_SEG_A800;
        gPageSegs[5] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gPageSegs[6] = VGA_SEG_AC00;
        gPageSegs[7] = FP_SEG(MemAlloc(PAGE_64512, MEMALLOC_ALIGNED));
        break;
    }
    gClearPage();
    gFont8x8 = LoadFont(gFont8Name);
    gFont6x6 = LoadFont("FONT6.FNT");
    if (!gFont8x8 || !gFont6x6)
        goto font_error;
    SetFont(gFont8x8);       /* makes the 8x8 font current after both fonts load; why it is set
                                   here, after the loads, is not in the file */
    switch (gGraphicsChoice - 1) {
    case 0:
    case 1:
        gPaletteBuf = MemAlloc(0x10L, 0);
        gPaletteBuf[0] = 0;
        gPaletteBuf[0xf] = 0xf;
        gSetPalette(gBuiltinPalette);
        break;
    case 2:
    case 3:
        gPaletteBuf = MemAlloc(PALETTE_SIZE, 0);
        memset(gPaletteBuf, 0, PALETTE_SIZE);
        /* fallback palette: entry 15 to the top 6-bit DAC value (PALETTE_ENTRY15_OFFSET is
           15 x 3 bytes into the buffer, DAC_MAX the top of a 6-bit DAC [H]); main loads eobpal.col
           over it for choices 3 and 4 (:257) */
        memset(gPaletteBuf + PALETTE_ENTRY15_OFFSET, DAC_MAX, 3);
        gSetPalette(gPaletteBuf);
        break;
    }
    InitOverlayCache();
    gOverlayCacheUp = 1;
    CalibrateRetrace();
    srand(time(NULL));
    SetTextWindow(0);
    gStartupDone = 1;
    /* the three sound bodies sit after `fail` in the original, so the cases jump forward to them
       and share the one `return 1` tail; see the function comment. */
    switch (gSoundCard) {
    case 0:
        goto adlib;
    case 1:
        goto speaker;
    case 2:
        goto tandy;
    }
    goto done;
font_error:
    SetVideoMode(7);              /* index 7 restores gRestoreMode (the argv[5] mode) */
    PrintString(gFontErrorText);
    PrintString(gFont8Name);   /* the message is a fixed string: it names Font8.fnt even when the 6x6 font was the one that failed */
    PrintString(gReinstallText);
    WaitGameKey();
    return 0;
fail:
    return 0;
adlib:
    StartAdlibDriver();
    goto done;
speaker:
    SoundDriverInstall();
    goto done;
tandy:
    SoundDriverInstall();
    gTandyOn = 1;
    gTandySelected = 1;
    SoundDriverResume();
done:
    return 1;
}

/* 07C8:058A: the shut-down path, called by main. Stops sound the way the chosen card needs (AdLib
   driver off for gSoundCard 0, Tandy driver silenced to one voice and uninstalled for 2, only
   uninstalled for 1), calls the exit hook at ds:80FA, removes the mouse handler, stops the overlay
   manager, restores the INT 1Ch tick handler (RemoveTickHandler, :472), removes the keyboard (:473),
   restores the mode (argv[5]; :474) and the old INT 3Fh vector, then, for the Tandy graphics choice only (gGraphicsChoice == 1, :476; also on the failed memory path), shrinks the last block to gMemTopSeg: the atol value when StartupGame stored it (:311), but its loaded 0 on the failed memory path, where StartupGame returns before that store (src/E1990.ASM:6-10, :388-391). Whether that atol value is the video top is [H]. exit(). */
/*@ 07C8:058A */ void ShutdownGame(void)
{
    switch (gSoundCard) {
    case 0:
        StopAdlibDriver();
        break;
    case 1:
        SoundDriverUninstall();
        break;
    case 2:
        /* silences the three Tandy voices and falls back to one voice; the driver's flags end the
           song at the next tick and the end-of-song count then clears gPlaying (src/E1948.ASM:823-
           812), so the song ends, it is not paused (one voice may sound one more note first), and
           nothing resumes it before the uninstall below */
        SoundDriverSilenceToOneVoice();
        gTandyOn = 0;
        SoundDriverUninstall();
        break;
    }
    if (gExitHook)
        gExitHook();
    if (gMouseHandlerInstalled)
        RemoveMouseHandler();
    if (gOverlayCacheUp)
        ShutdownOverlayCache();
    RemoveTickHandler();
    RemoveKeyboard();
    SetVideoMode(7);              /* index 7 restores gRestoreMode (the argv[5] mode) */
    setvect(INT3F_VECTOR, gSavedInt3F);
    if (gGraphicsChoice == 1)
        TrimMemoryToVideo();
    exit(0);
}

/* 07C8:061D: The default exit hook main installs at ds:80FA: it does nothing and returns 1, so
   ShutdownGame's call through gExitHook is a no-op until something replaces it. Returns 1 and does
   nothing else. main installs it as the far function
   pointer at ds:80FA (:244) and 07C8:058A calls it through that pointer during shut-down; it is the
   exit hook main installs, until something else replaces it; nothing outside this unit assigns ds:80FA. */
/*@ 07C8:061D */ int ExitHook(void)
{
    return 1;
}

/* 07C8:0625: the allocation failure hook. Restores the mode from gRestoreMode (argv[5]; :494),
   prints "Allocation failure, returning to DOS!", waits for a key and runs the shut-down path. main
   stores it at ds:810A (:245), replacing the image's OutOfMemory (19BC:02B6, src/E19BC.C:23-25); the pointer MemAlloc (19BC:0007) calls when a far allocation fails. */
/*@ 07C8:0625 */ int AllocationFailure(void)
{
    SetVideoMode(7);              /* index 7 restores gRestoreMode (the argv[5] mode) */
    PrintString("\r\nAllocation failure, returning to DOS!");
    WaitGameKey();
    ShutdownGame();
    return 0;
}
