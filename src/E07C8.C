/* Unit at EOB.EXE 07C8:0001 (1610 bytes, 5 functions): the program's main, its start-up, and its
   shut-down. main (07C8:0001) reads the command line (graphics, sound, mouse, the "homie" marker,
   the video mode to restore), installs the INT 3Fh handler and the two allocation/exit hooks, then
   runs 07C8:0199, which sets the video mode, sizes memory to the probed segment (the video-top
   reading is [H], src/E1990.ASM:386-397), allocates the page/buffer table at ds:FC0C, loads the
   overlays and the two fonts and turns sound on; on the way out 07C8:058A stops sound, disables the
   mouse handler, restores the old INT 3Fh vector and exits.
   07C8:061D returns 1 (the default exit hook at ds:80FA) and 07C8:0625 is the allocation failure
   hook at ds:810A. WHITEPAPER 2.6 and 4.4. Compiled -ml -O -Z. */

#include <dos.h>

#ifndef NULL
#define NULL 0
#endif

struct Unk8894 {            /* the object g8894 points at, ds:8894; Purpose unknown: nothing in
                               src/ or docs/ writes ds:8894 (src/E1990.ASM:23-26) */
    char pad[0x18];
    char far *number;       /* +0x18: a far pointer to a numeric string */
};

struct OverlayRequest {     /* the request 1C00:0055 runs, ds:8121 */
    void far *name;         /* +0: the overlay file name (word +0 the offset, word +2 the segment) */
    void far *table;        /* +4: a far pointer (offset then segment) to a table the overlay entry's
                               pointer walks (src/E1C00.ASM:121-127); Purpose unknown: nothing in
                               src/ says who writes ds:8125/ds:8127; this unit reads only `name` */
};

extern int g8102;                     /*@ ds:8102: graphics choice: argv[1][0] + 1 at :153, less
                                         0x31 in 0199 (:216), so the digit 0..4 indexing
                                         gOverlayFiles (0 cga, 1 tandy, 2 ega, 3 mcga/VGA, 4 vga) */
extern char gSoundCard;               /*@ ds:B892: sound choice, argument digit minus '1' (0 AdLib,
                                         1 PC speaker, 2 Tandy; src/E177D.C:52 declares it unsigned
                                         char, the value is 0..2 in both; char keeps the code) */
extern int gSoundOn;                  /*@ ds:80E6: nonzero unless sound was '4' */
extern unsigned char gSpeakerMode;    /*@ ds:009E: 1 only on the Tandy path (:350); the PC speaker
                                         path (:345) leaves it 0. PlaySound (src/E177D.C:182) gates
                                         the gTandyOn toggle on it; Purpose unknown beyond that */
extern unsigned char gTandyOn;        /*@ ds:8A67: the driver select: 1 is Tandy, 0 the PC speaker
                                         (src/E1948.ASM:7, tested at :218 and later). StartupGame
                                         sets it to 1 on the Tandy path (:349), ShutdownGame clears
                                         it (:375) */
extern unsigned g84A0;                /*@ ds:84A0: video mode argument, restored on exit */
extern void interrupt (far *gFC08)();  /*@ ds:FC08: the INT 3Fh vector main replaced */
extern void (far *gFn80FA)(void);     /*@ ds:80FA: exit hook main sets to 07C8:061D */
extern int (far *gAllocFailHook)(void); /*@ ds:810A: allocation failure hook, main sets 07C8:0625 */
extern int gMouseHandlerInstalled;    /*@ ds:8148: the mouse driver handler was installed (1 at
                                         install, 0 when no driver is found; src/E1A72.ASM:70-74) */
extern int gMouseDisabled;            /*@ ds:814A: nonzero disables the mouse handler (src/E0ACD.C:111,
                                         src/E122E.C:48, src/E1C00.ASM:281-290); main sets it to 1
                                         when no driver is installed (:191-193) */
extern unsigned char g3092;           /*@ ds:3092: set to 1 at :320, before the sound card is started
                                         (:323-331); main tests it at :189 */
extern unsigned char g3056;           /*@ ds:3056: set to 1 at :316 right after InitOverlayCache and
                                         tested only by ShutdownGame (:383) before
                                         ShutdownOverlayCache; no other unit reads ds:3056 */
extern int g80EA;                     /*@ ds:80EA: set to 1 during start-up, meaning unknown */
extern int gTextAttr2;                /*@ ds:811B: the second text attribute, the frame and fill
                                         colour, default 0x0C (src/E122E.C:75, src/E1DE7.C:39,
                                         src/EOV2.C:500) */
extern int gTextAttr1;                /*@ ds:811D: the first text attribute, the foreground colour,
                                         default 0x0F (src/E122E.C:74, src/E1DE7.C:38,
                                         src/EOV2.C:500) */
extern void (far *gFn80BA)(void far *table); /*@ ds:80BA: far function, takes a colour table */
extern void (far *gFn8056)(void);     /*@ ds:8056: far function called before the fonts load */
extern unsigned char far *gFE68;      /*@ ds:FE68: the allocated palette buffer: eobpal.col for modes
                                         2 and 3 (:309, :312); modes 0 and 1 allocate 16 bytes here
                                         and fill it 0..0x0F (:302-304), but gFn80BA gets
                                         gPalette3057 (:305) */
extern unsigned char gPalette3057[];  /*@ ds:3057: the 16-byte colour table used when no palette loads */
extern void far *gFE5A;               /*@ ds:FE5A: the 8x8 font loaded from Font8.fnt */
extern void far *gA9C0;               /*@ ds:A9C0: the 6x6 font loaded from FONT6.FNT */
extern unsigned gSegsFC0C[];          /*@ ds:FC0C: the page/buffer segment table */
extern unsigned far gMemTopSeg;       /*@ 1990:0002: the segment where program memory must end
                                         (src/E1990.ASM:20-22); StartupGame stores the atol of
                                         g8894+0x18 over it on the Tandy path only (:229). Whether
                                         that value is the video top is [H] (src/E1990.ASM:386-397) */
extern struct Unk8894 far *g8894;     /*@ ds:8894: pointer whose +0x18 is a numeric string;
                                         Purpose unknown: nothing in src/ or docs/ writes ds:8894
                                         (src/E1990.ASM:23-26) */
extern void far *gOverlayFiles[];     /*@ ds:3074: the cga/tga/ega/mcga/vga overlay names */
extern struct OverlayRequest gOverlayRequest; /*@ ds:8121: the request RunOverlayRequest runs */

extern void PrintString(char far *s);             /*@ 19EA:0000 */
extern void WaitGameKey(void);                     /*@ 1AAD:053A */
extern void RemoveKeyboard(void);                     /*@ 1AAD:09F6 */
extern void f1DDD_002D(int index); /*@ 1DDD:002D: sets the video mode from the ds:84A2 table,
                                      or, for index 7 or more, from argv[5] (ds:84A0, the mode
                                      restored on exit; src/E1DDD.C:43-49) */
extern void TrimMemoryToVideo(void);              /*@ 1990:027D */
extern void RemoveMouseHandler(void);                   /*@ 1A72:0285 */
extern void ShutdownOverlayCache(void);                     /*@ 1C0E:13FD */
extern void f1D8F_0038(void);                     /*@ 1D8F:0038 */
extern void SoundDriverSilenceToOneVoice(void);               /*@ 1948:046B */
extern void SoundDriverUninstall(void);           /*@ 1948:03DA */
extern void StopAdlibDriver(void);                /*@ 0000:3FCA: the e_text AdLib driver off: restores
                                                     the saved INT 08h vector, resets the AdLib through
                                                     ResetVoices and stops PIT channel 0 (src/E_TEXT.ASM:125-152) */
extern void exit(int status);                     /*@ 0000:50F3 */
extern long atol(char far *s);                    /*@ 0000:5D63 */
extern int strcmp(char far *a, char far *b);      /*@ 0000:679D */
extern long LargestFreeBlock(void);               /*@ 19BC:023D */
extern void LoadFile(char far *name, void far *buf, unsigned len); /*@ 1B82:0009 */
extern void StartupAlloc(int argc, char far * far *argv); /*@ 07A9:000A */
extern void InstallKeyboard(void);                     /*@ 1AAD:0718 */
extern void InstallHardErr(void);                 /*@ 1A63:0019 */
extern void f1D8F_0010(void);                     /*@ 1D8F:0010 */
/* 16A5:0D78: stores its 32-bit second argument as a far pointer at ds:FC7C (the callee reads
   [bp+8] into ds:FC7C and [bp+0xa] into ds:FC7E) and ignores the first word argument, so main calls
   it as SetNodeCallback(0, 0L): the word first is what makes Turbo C push 0L then the word, exactly as
   the original does. */
extern void SetNodeCallback(int arg, long value);      /*@ 16A5:0D78 */
extern void far *f1A47_0005(char far *name);      /*@ 1A47:0005 */
extern void f1A47_0169(void far *font);           /*@ 1A47:0169 */
extern int CheckMemoryChain(void);                /*@ 1990:0071 */
extern unsigned ReserveMemoryBelowVideo(int n);   /*@ 1990:0133 */
extern void InitMouse(void);                      /*@ 1A72:00DA */
extern int RunOverlayRequest(void far *req);      /*@ 1C00:0055 */
extern void InitOverlayCache(void);                     /*@ 1C0E:13E7 */
extern void SetTextWindow(int n);                    /*@ 1DE7:0009 */
extern void CalibrateRetrace(void);               /*@ 1A65:000E */
extern void SoundDriverInstall(void);             /*@ 1948:0377 */
extern void SoundDriverResume(void);              /*@ 1948:0458 */
extern void StartAdlibDriver(void);               /*@ 0000:3F77: the e_text AdLib driver on: hooks
                                                     INT 08h (at 3E66), programs PIT channel 0 and
                                                     resets the AdLib through ResetVoices (src/E_TEXT.ASM:81-123) */
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
int f07C8_061D(void);
int AllocationFailure(void);

/* 07C8:0001: main. Reads the four arguments the launcher passes (graphics, sound and mouse, all one
   character, and the five-letter "homie" marker) and the video mode to restore, installs the
   INT 3Fh handler and the exit and allocation failure hooks, then runs the start-up 07C8:0199.
   When start-up succeeds it takes the first switch on the graphics choice: a built-in 16-byte
   colour table at ds:3057 for the first two modes, otherwise it loads eobpal.col into ds:FE68, both
   through the far function at ds:80BA, and then calls 16A5:0D78 and StartupAlloc. Returns 0, and
   always runs the shut-down 07C8:058A after the start-up `if`; only the homie check (:159) and the
   memory check (:164) return before it. The function name is `Main`, not `main`, because the
   matcher's link stub already defines `_main` for the C0 startup. */
/*@ 07C8:0001 */ int Main(int argc, char far * far *argv)
{
    int mouse;

    g8102 = argv[1][0] + 1;
    gSoundCard = argv[2][0] + 0xcf;
    gSoundOn = argv[2][0] != '4';
    mouse = argv[3][0] + 0xffd0;
    if (strcmp(argv[4], "homie"))
        return 0;
    g84A0 = atol(argv[5]);
    gFC08 = getvect(0x3f);
    setvect(0x3f, (void interrupt (far *)())&Int3FHandler);
    if (LargestFreeBlock() < 0xFDE8L) {
        PrintString("\r\nNot enough memory to run program.");
        return 0;
    }
    gFn80FA = f07C8_061D;
    gAllocFailHook = AllocationFailure;
    InstallKeyboard();
    InstallHardErr();
    f1D8F_0010();
    if (StartupGame() == 1) {
        switch (g8102 - 1) {
        case 0:
        case 1:
            gFn80BA(gPalette3057);
            break;
        case 2:
        case 3:
            LoadFile("eobpal.col", gFE68, 0x300);
            gFn80BA(gFE68);
            break;
        }
        g80EA = 1;
        gTextAttr1 = 0xf;
        gTextAttr2 = 0xc;
        SetNodeCallback(0, 0L);
        if (g3092) {
            if (!gMouseHandlerInstalled)
                gMouseDisabled = 1;
            else
                gMouseDisabled = !mouse;
            StartupAlloc(argc, argv);
        }
    }
    ShutdownGame();
    return 0;
}
/* 07C8:0199: the start-up. First it lowers g8102 by '1' (the argument digit is '1' past the choice),
   then sizes memory for the Tandy mode (CheckMemoryChain and ReserveMemoryBelowVideo, storing the
   probed segment in gMemTopSeg; the video-top reading is [H], src/E1990.ASM:386-397) or clears
   A000:0000 for the VGA mode, sets the video mode, resets the mouse, loads the graphics overlay
   named by g8102 through the table at ds:3074 (cga/tga/ega/mcga/vga.ovl), builds the page/buffer
   segment table at ds:FC0C for the mode, loads the two fonts, sets up the palette buffer, starts the
   overlay manager, seeds rand from the clock and turns the chosen sound card on. Returns 1 when it
   got that far, 0 on the errors it prints, and 0 on the `fail` path (g8102 == 0x36) without
   printing. The three `goto`s into the sound cases (:325, :327, :329) keep the three sound bodies
   behind `fail`, which is the order the original has. */
/*@ 07C8:0199 */ int StartupGame(void)
{
    int memtop;

    if (g8102 == 0x36)
        goto fail;
    g8102 -= 0x31;
    switch (g8102) {
    case 1:
        if (CheckMemoryChain()) {
            f1DDD_002D(7);
            PrintString("\r\nMemory chain corrupt!\r\n");
            WaitGameKey();
            return 0;
        }
        f1DDD_002D(g8102);
        memtop = ReserveMemoryBelowVideo(3);
        /* the segment where program memory must end: the numeric string at g8894+0x18 parsed as a
           number (its writer is Purpose unknown, src/E1990.ASM:23-26) */
        gMemTopSeg = (unsigned)atol(g8894->number);
        break;
    case 3:
        memset(MK_FP(0xa000, 0), 0, 0xfa00);
        break;
    }
    f1DDD_002D(g8102);
    InitMouse();
    gOverlayRequest.name = gOverlayFiles[g8102];
    if (RunOverlayRequest(&gOverlayRequest) == -1) {
        f1DDD_002D(7);
        PrintString("\r\nCould not load overlay \"");
        PrintString(gOverlayRequest.name);
        PrintString("\".  Press a key to return to DOS...\r\n");
        WaitGameKey();
        return 0;
    }
    /* the render/page table ds:FC0C, eight segment words, filled per graphics mode: screen pages
       and the pages backing the buffers. Modes 0 and 2 use 0xFA00 (64000), 0x7D00 (32000) and
       0xCB20 (52000); mode 1 uses 0x7F40 (32576) for slots 4 and 5 (:264) with no 0xFA00; mode 3
       uses 0xFDE8 (65000) for slots 2 and 3 (:278); mode 4 uses 0xFA00 (slots 1, 3 and 5) and
       0xFC00 (slot 7). */
    switch (g8102) {
    case 0:
        gSegsFC0C[0] = gSegsFC0C[1] = 0xb800;
        gSegsFC0C[2] = gSegsFC0C[3] = FP_SEG(MemAlloc(0xfa00L, 0x20));
        gSegsFC0C[4] = gSegsFC0C[5] = FP_SEG(MemAlloc(0x7d00L, 0x20));
        gSegsFC0C[6] = gSegsFC0C[7] = FP_SEG(MemAlloc(0xcb20L, 0x20));
        break;
    case 1:
        /* the original writes slots 0 and 1 as two immediate stores and slots 2 and 3 through ax
           (the Tandy mode); keep the separate statements so the code is byte for byte. */
        gSegsFC0C[0] = 0xb800;
        gSegsFC0C[1] = 0xb800;
        gSegsFC0C[3] = gSegsFC0C[2] = memtop;
        gSegsFC0C[4] = gSegsFC0C[5] = FP_SEG(MemAlloc(0x7f40L, 0x20));
        gSegsFC0C[6] = gSegsFC0C[7] = FP_SEG(MemAlloc(0xcb20L, 0x20));
        break;
    case 2:
        gSegsFC0C[0] = 0xa000;
        gSegsFC0C[2] = 0xa200;
        gSegsFC0C[3] = FP_SEG(MemAlloc(0xfa00L, 0x20));
        gSegsFC0C[4] = 0xa200;
        gSegsFC0C[5] = FP_SEG(MemAlloc(0x7d00L, 0x20));
        gSegsFC0C[6] = 0xa200;
        gSegsFC0C[7] = FP_SEG(MemAlloc(0xcb20L, 0x20));
        break;
    case 3:
        gSegsFC0C[0] = gSegsFC0C[1] = 0xa000;
        gSegsFC0C[2] = gSegsFC0C[3] = FP_SEG(MemAlloc(0xfde8L, 0x20));
        gSegsFC0C[4] = gSegsFC0C[5] = FP_SEG(MemAlloc(0x7d00L, 0x20));
        gSegsFC0C[6] = gSegsFC0C[7] = FP_SEG(MemAlloc(0xcb20L, 0x20));
        break;
    case 4:
        gSegsFC0C[0] = 0xa000;
        gSegsFC0C[1] = FP_SEG(MemAlloc(0xfa00L, 0x20));
        gSegsFC0C[2] = 0xa400;
        gSegsFC0C[3] = FP_SEG(MemAlloc(0xfa00L, 0x20));
        gSegsFC0C[4] = 0xa800;
        gSegsFC0C[5] = FP_SEG(MemAlloc(0xfa00L, 0x20));
        gSegsFC0C[6] = 0xac00;
        gSegsFC0C[7] = FP_SEG(MemAlloc(0xfc00L, 0x20));
        break;
    }
    gFn8056();
    gFE5A = f1A47_0005("Font8.fnt");
    gA9C0 = f1A47_0005("FONT6.FNT");
    if (!gFE5A || !gA9C0)
        goto font_error;
    f1A47_0169(gFE5A);
    switch (g8102 - 1) {
    case 0:
    case 1:
        gFE68 = MemAlloc(0x10L, 0);
        gFE68[0] = 0;
        gFE68[0xf] = 0xf;
        gFn80BA(gPalette3057);
        break;
    case 2:
    case 3:
        gFE68 = MemAlloc(0x300L, 0);
        memset(gFE68, 0, 0x300);
        memset(gFE68 + 0x2d, 0x3f, 3);
        gFn80BA(gFE68);
        break;
    }
    InitOverlayCache();
    g3056 = 1;
    CalibrateRetrace();
    srand(time(NULL));
    SetTextWindow(0);
    g3092 = 1;
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
    f1DDD_002D(7);              /* index 7 restores the argv[5] mode, ds:84A0 */
    PrintString("\r\nUnable to load font ");
    PrintString("Font8.fnt");
    PrintString(". Reinstall program.\r\n");
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
    gSpeakerMode = 1;
    SoundDriverResume();
done:
    return 1;
}

/* 07C8:058A: the shut-down path. Stops sound the way the chosen card needs (the AdLib driver off
   for gSoundCard 0, the Tandy driver silenced to one voice and uninstalled for 2, only uninstalled
   for 1), calls the exit hook at ds:80FA, disables the mouse handler, stops the overlay manager,
   restores the mode from argv[5] (ds:84A0) and the old INT 3Fh vector, returns the Tandy memory to
   the probed segment, and exits the program through the runtime's exit(). Called by main. */
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
        /* silences the three Tandy voices and falls back to one voice; it does not pause the song
           (gPlaying, the song base and the timer keep running, src/E1948.ASM:740-744), and nothing
           resumes it before the uninstall below */
        SoundDriverSilenceToOneVoice();
        gTandyOn = 0;
        SoundDriverUninstall();
        break;
    }
    if (gFn80FA)
        gFn80FA();
    if (gMouseHandlerInstalled)
        RemoveMouseHandler();
    if (g3056)
        ShutdownOverlayCache();
    f1D8F_0038();
    RemoveKeyboard();
    f1DDD_002D(7);              /* index 7 restores the argv[5] mode, ds:84A0 */
    setvect(0x3f, gFC08);
    if (g8102 == 1)
        TrimMemoryToVideo();
    exit(0);
}

/* 07C8:061D: Purpose unknown: returns 1 and does nothing else. main installs it as the far function
   pointer at ds:80FA and 07C8:058A calls it through that pointer during shut-down, so it is the
   default exit hook until something else replaces it. */
/*@ 07C8:061D */ int f07C8_061D(void)
{
    return 1;
}

/* 07C8:0625: the allocation failure hook. Restores the mode from argv[5] (ds:84A0), prints
   "Allocation failure, returning to DOS!", waits for a key and runs the shut-down path. main stores
   it at ds:810A, the pointer MemAlloc (19BC:0007) calls when a far allocation fails. */
/*@ 07C8:0625 */ int AllocationFailure(void)
{
    f1DDD_002D(7);              /* index 7 restores the argv[5] mode, ds:84A0 */
    PrintString("\r\nAllocation failure, returning to DOS!");
    WaitGameKey();
    ShutdownGame();
    return 0;
}
