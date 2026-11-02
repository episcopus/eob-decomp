/* Unit at INTRO.EXE 0892:0000 (2288 bytes, 5 functions): INTRO.EXE's own start-up and its
   hand-off to EOB.EXE. Main (0892:0000) reads the launcher's arguments (the graphics, sound and
   mouse digits, the "homie" marker and the video mode to restore), installs the INT 3Fh handler and
   the exit and allocation-failure hooks, checks free memory, locates the EOB.EXE file (asking for
   the disk when it is missing) and runs StartupGame (0892:0241), the start-up that sets the video
   mode, loads the graphics overlay and the fonts and starts the sound card, then loads the palette
   and hands off to EOB.EXE with _execl. ShutdownGame (0892:0835) stops the sound card, calls the
   exit hook, removes the mouse handler and the keyboard, shuts the overlay cache down and restores
   the saved INT 3Fh vector. 0892:08C0 is the exit hook (returns 1) and 0892:08C8 the
   allocation-failure hook. WHITEPAPER 2.6; compiled -ml -O -Z -G. */

/*@flags -ml -O -Z -G */

#include <dos.h>

#ifndef NULL
#define NULL 0
#endif

#define INT3F_VECTOR 0x3F
/* LargestFreeBlock returns bytes; below 0xFDE8 (65000) even the largest page has no room (the same
   check and constant as EOB.EXE's main, src/E07C8.C:238-240). */
#define PAGE_65000 0xFDE8L
#define MEMALLOC_ALIGNED 0x20
#define PALETTE_SIZE 0x300
#define VGA_SCREEN_SEG 0xA000
/* the page/buffer sizes and video segments StartupGame's table uses (:0368 to :0540) */
#define PAGE_64000 0xFA00L
#define PAGE_32576 0x7F40L
#define PAGE_65024 0xFFDCL
#define PAGE_64512 0xFC00L
#define PALETTE_BUFFER_SIZE 0x10
#define TEXT_SCREEN_SEG 0xB800
#define VGA_SEG_A200 0xA200
#define VGA_SEG_A400 0xA400
#define VGA_SEG_A800 0xA800
#define VGA_SEG_AC00 0xAC00
/* the two hand-off strings and the sound data files are literals in DGROUP; the sizes the sound
   path reads (:06C1 to :0811) */
#define SOUND_MAP_SIZE 0x78L
#define DRIVER_BLOCK_SIZE 0x36B0L

/* the request RunOverlayRequest (0B4E:005B) runs: a far name pointer (offset then segment) at +0;
   this unit writes only `name`. Same layout as src/E07C8.C:35. */
struct OverlayRequest {
    char far *name;                 /* +0: the overlay file name */
    void far *table;                /* +4: Purpose unknown: this unit does not write it */
};

extern int gGraphicsChoice;              /*@ ds:1F8E: argv[1][0] + 1 (:0010), reduced by 0x31 in
                                             StartupGame (:0258) to the graphics choice (whitepaper 2.6) */
extern int gSoundChoice;                 /*@ ds:28C6: atol(argv[2]) (:0026); StartupGame switches on
                                             it (1 AdLib, 2 PC speaker, 3 Tandy) at :06A9, ShutdownGame
                                             at :0860, and the sound set-up reads its low word */
extern int gSavedDisk;                   /*@ ds:1A35: the drive getdisk returned before the overlay
                                             file is located (:004D), passed to setdisk (:0567, :01E4) */
extern int gOverlayDrive;                /*@ ds:1A37: 1 before the search (:0050), then the drive
                                             LocateOverlayFile returned (:00E9), one less after (:00F0);
                                             setdisk restores it before the hand-off (:01E4) */
extern void interrupt (far *gSavedInt3F)(); /*@ ds:28C8: the INT 3Fh vector getvect returns (:0061);
                                             put back by ShutdownGame (:08AA) */
extern int (far *gExitHook)(void);       /*@ ds:1F86: the exit hook main installs (:009C, 0892:08C0);
                                             ShutdownGame calls it (:0841) */
extern int (far *gAllocFailHook)(void);  /*@ ds:1F96: the allocation-failure hook main installs
                                             (:00A8, 0892:08C8) */
extern int g1FA9;                        /*@ ds:1FA9: set to 0xF by main after the palette (:0145) */
extern int g1FA7;                        /*@ ds:1FA7: set to 0xC by main after the palette (:014B) */
extern unsigned char g1A34;              /*@ ds:1A34: set to 1 by StartupGame when it succeeds
                                             (:0817); main runs the overlay when it is set (:0151) */
extern int g232C;                        /*@ ds:232C: the value f1DDD_0006 returns (:024B), itoa'd
                                             into g28A4 for the hand-off (:01BA) */
extern int g28C4;                        /*@ ds:28C4: itoa'd into g28B4 for the hand-off (:01CF) */
extern char g28A4[];                     /*@ ds:28A4: itoa's buffer for g232C (:01BA) */
extern char g28B4[];                     /*@ ds:28B4: itoa's buffer for g28C4 (:01CF) */
extern unsigned char far *gPaletteBuf;   /*@ ds:2D42: the palette buffer main allocates (:03A0) and
                                             loads palette.col into (:0121, :0202) */
extern void (far *gFn1F46)(void far *table); /*@ ds:1F46: the palette setter (:013E, :01AE, :0635),
                                             called with the built-in table ds:19E7 or gPaletteBuf */
extern void (far *gFn1EE2)(void);        /*@ ds:1EE2: called by StartupGame after the page table (:0544) */
extern int gMouseHandlerInstalled;       /*@ ds:1FD4: nonzero once the mouse handler is installed;
                                             ShutdownGame removes it then (:0845) */
extern unsigned char g19E6;              /*@ ds:19E6: set to 1 by StartupGame after InitOverlayCache
                                             (:0641); ShutdownGame shuts the cache down then (:0851) */
extern unsigned char g289B;              /*@ ds:289B: set to 1 on the Tandy sound path (:0807) and
                                             cleared by ShutdownGame (:087F) */
extern struct OverlayRequest gOverlayRequest; /*@ ds:1FAD: the request RunOverlayRequest runs (:02FE) */
extern char far *gOverlayFiles[];        /*@ ds:1A14: the graphics overlay name pointers, indexed by
                                             gGraphicsChoice (:02DF) */
extern unsigned gSegs28CC[];             /*@ ds:28CC: the page/buffer segment table StartupGame fills
                                             per graphics choice (:036B to :0540) */
extern void far *gFont8;                 /*@ ds:2D34: the 8x8 font f1A47_0005 loads (:0560) */
extern void far *gSoundBuf;              /*@ ds:28A0: the sound driver data buffer (:06D1, :07AC) */
extern unsigned char g19E7[];            /*@ ds:19E7: the built-in palette table main passes to the
                                             setter for graphics choices 0 and 1 (:010F) */
extern unsigned char g19F7[];            /*@ ds:19F7: 16 zero bytes, the table main passes to the
                                             setter for the Tandy choice when the memory top is set (:01AE) */

extern void PrintString(char far *s);            /*@ 097D:0000 */
extern void WaitGameKey(void);                   /*@ 0A19:052E */
extern void RemoveKeyboard(void);                /*@ 0A19:09EA */
extern void InstallKeyboard(void);               /*@ 0A19:070C */
extern void f1DDD_002D(int index);               /*@ 0D2B:0033 */
extern int f1DDD_0006(void);                     /*@ 0D2B:000C */
extern void RemoveMouseHandler(void);            /*@ 09E1:027F */
extern void InitMouse(void);                     /*@ 09E1:00D4 */
extern void ShutdownOverlayCache(void);          /*@ 0B5D:13F3 */
extern void InitOverlayCache(void);              /*@ 0B5D:13DD */
extern int LocateOverlayFile(char far *name);    /*@ 0B5D:1306 */
extern int FileOpen(char far *name, int mode);   /*@ 0B5D:00D1 */
extern void FileClose(int handle);               /*@ 0B5D:03B1 */
extern long FileRead(int handle, void far *buf, unsigned long len); /*@ 0B5D:03F6 */
extern int RunOverlayRequest(void far *req);     /*@ 0B4E:005B */
extern long LargestFreeBlock(void);              /*@ 094F:023D */
extern void far *MemAlloc(unsigned long size, int flags); /*@ 094F:0007 */
extern int CheckMemoryChain(void);               /*@ 0923:0071 */
extern unsigned ReserveMemoryBelowVideo(int steps); /*@ 0923:0133 */
extern void InstallHardErr(void);                /*@ 09D1:0022 */
extern void f1D8F_0010(void);                    /*@ 0CDD:0016 */
extern void f1D8F_0038(void);                    /*@ 0CDD:003E */
extern void FindLastDrive(void);                 /*@ 0992:0002 */
extern void SetTextWindow(int n);                /*@ 0D35:000F */
extern void CalibrateRetrace(void);              /*@ 09D4:0007 */
extern void SoundDriverInstall(void);            /*@ 084A:0376 */
extern void SoundDriverResume(void);             /*@ 084A:0457 */
extern void SoundDriverSilenceToOneVoice(void);  /*@ 084A:046A */
extern void SoundDriverUninstall(void);          /*@ 084A:03D9 */
extern void StopAdlibDriver(void);               /*@ 0000:43A5: the i_text AdLib driver off */
extern void StartAdlibDriver(void);              /*@ 0000:4352: the i_text AdLib driver on */
extern void far *f1A47_0005(char far *name);     /*@ 09B5:000E */
extern void f1A47_0169(void far *font);          /*@ 09B5:0172 */
extern void far *GetDriverBlock(void);           /*@ 0000:03AA: the i_text sound driver data block */
extern void RebaseDriverBlock(void);             /*@ 0000:03FA */
extern void LoadFile(char far *name, void far *buf, unsigned len); /*@ 0AED:000D: the file loader
                                                     (i0aed is e1b82 in another version, src/E1B82.C) */
extern int heapcheck(void);                      /*@ 0000:7F20 */
extern long atol(char far *s);                   /*@ 0000:66A5 */
extern int strcmp(char far *a, char far *b);     /*@ 0000:73A3 */
extern void far *memset(void far *dst, int c, unsigned n); /*@ 0000:6D01 */
extern long time(long far *tloc);                /*@ 0000:616B */
extern void srand(unsigned seed);                /*@ 0000:5F63 */
extern int getdisk(void);                        /*@ 0000:6753 */
extern int setdisk(int drive);                   /*@ 0000:6759 */
extern char *itoa(int value, char *string, int radix); /*@ 0000:6C4A */
extern int execl(char far *path, char far *arg0, ...); /*@ 0000:5717 */
/* 0000:549D: a file handle close: it range-checks the handle against the 21-entry table at
   ds:25E4, marks the slot unused and calls __close (0000:684A). Purpose unknown beyond that:
   main calls it with the word at 0E6C:00B8 (:018E). */
extern int CloseFileHandle(int handle);          /*@ 0000:549D */

/* 16-byte far object at 0E6C:00B8, the file handle main closes (:018E). The segment is a data
   segment of the library, not DGROUP. */
extern unsigned far g0E6C_00B8;                  /*@ 0E6C:00B8 */

/* 0923:0006: the word main tests before it sets the Tandy palette (:019A); i0923 is E1990 in another
   version, where this word is gMemTopSeg (src/E07C8.C:125). Purpose unknown beyond that. */
extern unsigned far g0923_0006;                  /*@ 0923:0006 */

/* INTRO.EXE's overlay 0 (the intro playback); main runs it when g1A34 is set (:0158). Reached
   through its stub entry 0E96:0070 (ovl0:0000 is entry 16 of the stub at 0E96, entries at +20h,
   five bytes each). */
extern int Ovl0Main(int argc, char far * far *argv); /*@ 0E96:0070 */

/* The far object Int3FHandler (09E1:0000) main installs as the INT 3Fh handler (:0074). Declared as
   an object, not a function: the matcher cannot map a far function pointer taken from another unit
   passed as an argument, but an object at 09E1:0000 gives the same segment:offset (src/E07C8.C:200). */
extern unsigned char far Int3FHandler;           /*@ 09E1:0000 */

int Main(int argc, char far * far *argv);
int StartupGame(void);
void ShutdownGame(void);
int ExitHook(void);
int AllocationFailure(void);

/* 0892:0000: main. Reads the launcher's arguments (graphics, sound, the "homie" marker), installs
   the INT 3Fh handler (keeping the old vector in ds:28C8) and the exit and allocation-failure hooks,
   checks free memory, locates EOB.EXE (asking for DISK2 while it is missing) and runs StartupGame
   (:0241). When it succeeds it loads the palette and, when the start-up flag is set, runs the intro
   overlay (ovl0:0000). It then probes EOB.EXE, shuts the intro's systems down, closes the overlay
   file handle, itoa's the two hand-off numbers, restores the drive and hands off to EOB.EXE with
   _execl. Returns 0 on the early paths and never returns from the hand-off. WHITEPAPER 2.6. */
/*@ 0892:0000 */ int Main(int argc, char far * far *argv)
{
    int handle;

    gGraphicsChoice = argv[1][0] + 1;
    gSoundChoice = atol(argv[2]);
    if (strcmp(argv[4], "homie"))
        return 0;
    gSavedDisk = getdisk();
    gOverlayDrive = 1;
    gSavedInt3F = getvect(INT3F_VECTOR);
    setvect(INT3F_VECTOR, (void interrupt (far *)())&Int3FHandler);
    if (LargestFreeBlock() < PAGE_65000) {
        PrintString("\r\nNot enough memory to run program.");
        return 0;
    }
    gExitHook = ExitHook;
    gAllocFailHook = AllocationFailure;
    InstallKeyboard();
    InstallHardErr();
    f1D8F_0010();
    FindLastDrive();
    while ((gOverlayDrive = LocateOverlayFile("EOB.EXE")) == 0) {
        PrintString("\r\nInsert \"DISK2\" into the other drive.\r\n\r\nPress any key when ready...\r\n");
        WaitGameKey();
    }
    gOverlayDrive--;
    if (StartupGame() == 1) {
        switch (gGraphicsChoice - 1) {
        case 0:
            gFn1F46(g19E7);
            break;
        case 1:
            gFn1F46(g19E7);
            break;
        case 2:
        case 3:
            LoadFile("palette.col", gPaletteBuf, PALETTE_SIZE);
            gFn1F46(gPaletteBuf);
            break;
        }
        g1FA9 = 0xF;
        g1FA7 = 0xC;
        if (g1A34)
            Ovl0Main(argc, argv);
    }
    handle = FileOpen("EOB.EXE", 1);
    FileClose(handle);
    ShutdownGame();
    CloseFileHandle(g0E6C_00B8);
    if (g0923_0006 && gGraphicsChoice == 1)
        gFn1F46(g19F7);
    itoa(g232C, g28A4, 10);
    itoa(g28C4, g28B4, 10);
    setdisk(gOverlayDrive);
    execl("EOB.EXE", "EOB.EXE", argv[1], argv[2], argv[3], argv[4], g28A4, g28B4, (char far *)0);
    return 0;
}

/* 0892:0241: the start-up. Reduces gGraphicsChoice to the graphics choice (main stored
   argv[1][0] + 1 at :0010, this subtracts 0x31 at :0258), then for the Tandy choice checks the
   memory chain and reserves 64 KB below the probed video segment, for the VGA choice clears
   A000:0000. It sets the video mode, resets the mouse, loads the graphics overlay named by
   gOverlayFiles[gGraphicsChoice] through RunOverlayRequest (printing the failure message on -1),
   builds the page/buffer segment table ds:28CC, loads Font8.fnt and sets the palette buffer, then
   starts the overlay cache, the retrace calibration, the random seed and the text window and starts
   the chosen sound card (1 AdLib loads sound.dat, 2 PC speaker and 3 Tandy load pcsound.dat and
   install the driver; Tandy resumes it and sets ds:289B). Returns 1, setting ds:1A34, or 0 on its
   error paths and for the gGraphicsChoice 0x36 fail path (argv[1][0] '5', which main made 0x36). */
/*@ 0892:0241 */ int StartupGame(void)
{
    int memtop;
    int slot;

    g232C = f1DDD_0006();
    if (gGraphicsChoice == 0x36)
        goto fail;
    gGraphicsChoice -= 0x31;
    switch (gGraphicsChoice) {
    case 1:
        if (CheckMemoryChain()) {
            f1DDD_002D(7);
            PrintString("\r\nMemory chain corrupt!\r\n");
            WaitGameKey();
            return 0;
        }
        f1DDD_002D(gGraphicsChoice);
        memtop = ReserveMemoryBelowVideo(3);
        break;
    case 3:
        memset(MK_FP(VGA_SCREEN_SEG, 0), 0, PAGE_64000);
        break;
    }
    f1DDD_002D(gGraphicsChoice);
    InitMouse();
    gOverlayRequest.name = gOverlayFiles[gGraphicsChoice];
    setdisk(gOverlayDrive);
    if (RunOverlayRequest(&gOverlayRequest) == -1) {
        f1DDD_002D(7);
        PrintString("\r\nCould not load overlay \"");
        PrintString(gOverlayRequest.name);
        PrintString("\".  Press a key to return to DOS...\r\n");
        WaitGameKey();
        return 0;
    }
    setdisk(gSavedDisk);
    switch (gGraphicsChoice) {
    case 0:
        gSegs28CC[0] = gSegs28CC[1] = TEXT_SCREEN_SEG;
        gSegs28CC[2] = gSegs28CC[3] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gSegs28CC[4] = gSegs28CC[5] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gSegs28CC[6] = gSegs28CC[7] = FP_SEG(MemAlloc(PAGE_64512, MEMALLOC_ALIGNED));
        break;
    case 1:
        gSegs28CC[0] = TEXT_SCREEN_SEG;
        gSegs28CC[1] = FP_SEG(MemAlloc(PAGE_32576, MEMALLOC_ALIGNED));
        gSegs28CC[2] = gSegs28CC[3] = memtop;
        gSegs28CC[4] = gSegs28CC[5] = FP_SEG(MemAlloc(PAGE_65024, MEMALLOC_ALIGNED));
        gSegs28CC[6] = gSegs28CC[7] = FP_SEG(MemAlloc(PAGE_64512, MEMALLOC_ALIGNED));
        break;
    case 2:
        gSegs28CC[0] = VGA_SCREEN_SEG;
        gSegs28CC[2] = VGA_SEG_A200;
        gSegs28CC[3] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gSegs28CC[4] = VGA_SEG_A400;
        gSegs28CC[5] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gSegs28CC[6] = VGA_SEG_A800;
        gSegs28CC[7] = FP_SEG(MemAlloc(PAGE_64512, MEMALLOC_ALIGNED));
        break;
    case 3:
        gSegs28CC[0] = gSegs28CC[1] = VGA_SCREEN_SEG;
        gSegs28CC[2] = gSegs28CC[3] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gSegs28CC[4] = gSegs28CC[5] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gSegs28CC[6] = gSegs28CC[7] = FP_SEG(MemAlloc(PAGE_64512, MEMALLOC_ALIGNED));
        break;
    case 4:
        gSegs28CC[0] = VGA_SCREEN_SEG;
        gSegs28CC[1] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gSegs28CC[2] = VGA_SEG_A400;
        gSegs28CC[3] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gSegs28CC[4] = VGA_SEG_A800;
        gSegs28CC[5] = FP_SEG(MemAlloc(PAGE_64000, MEMALLOC_ALIGNED));
        gSegs28CC[6] = VGA_SEG_AC00;
        gSegs28CC[7] = FP_SEG(MemAlloc(PAGE_64512, MEMALLOC_ALIGNED));
        break;
    }
    gFn1EE2();
    setdisk(gOverlayDrive);
    gFont8 = f1A47_0005("Font8.fnt");
    setdisk(gSavedDisk);
    if (!gFont8)
        goto font_error;
    f1A47_0169(gFont8);
    switch (gGraphicsChoice - 1) {
    case 0:
        gPaletteBuf = MemAlloc(PALETTE_BUFFER_SIZE, 0);
        break;
    case 1:
        gPaletteBuf = MemAlloc(PALETTE_BUFFER_SIZE, 0);
        gPaletteBuf[0] = 0;
        gPaletteBuf[0xf] = 0xf;
        gFn1F46(g19E7);
        break;
    case 2:
    case 3:
        gPaletteBuf = MemAlloc(PALETTE_SIZE, 0);
        memset(gPaletteBuf, 0, PALETTE_SIZE);
        memset(gPaletteBuf + 0x2d, 0x3f, 3);
        gFn1F46(gPaletteBuf);
        break;
    }
    InitOverlayCache();
    g19E6 = 1;
    CalibrateRetrace();
    srand(time(NULL));
    SetTextWindow(0);
    goto sound;
font_error:
    f1DDD_002D(7);
    PrintString("\r\nUnable to load font ");
    PrintString("Font8.fnt");
    PrintString(". Reinstall program.\r\n");
    WaitGameKey();
    return 0;
sound:
    switch (gSoundChoice) {
    case 1:
        gSoundBuf = MemAlloc(SOUND_MAP_SIZE, 0);
        slot = FileOpen("sound.dat", 1);
        FileRead(slot, gSoundBuf, SOUND_MAP_SIZE);
        FileRead(slot, GetDriverBlock(), DRIVER_BLOCK_SIZE);
        FileClose(slot);
        RebaseDriverBlock();
        StartAdlibDriver();
        break;
    case 2:
        gSoundBuf = MemAlloc(SOUND_MAP_SIZE, 0);
        slot = FileOpen("pcsound.dat", 1);
        FileRead(slot, gSoundBuf, SOUND_MAP_SIZE);
        FileRead(slot, GetDriverBlock(), DRIVER_BLOCK_SIZE);
        FileClose(slot);
        RebaseDriverBlock();
        SoundDriverInstall();
        break;
    case 3:
        gSoundBuf = MemAlloc(SOUND_MAP_SIZE, 0);
        slot = FileOpen("pcsound.dat", 1);
        FileRead(slot, gSoundBuf, SOUND_MAP_SIZE);
        FileRead(slot, GetDriverBlock(), DRIVER_BLOCK_SIZE);
        FileClose(slot);
        RebaseDriverBlock();
        SoundDriverInstall();
        g289B = 1;
        SoundDriverResume();
        break;
    }
    goto done;
fail:
    return 0;
done:
    g1A34 = 1;
    return 1;
}

/* 0892:0835: the shut-down path. Calls the exit hook, removes the mouse handler (when the mouse
   driver was installed) and the keyboard, shuts the overlay cache down (when StartupGame started
   it), stops the sound card the way the chosen one needs (the i_text AdLib driver off, or the PC
   speaker/Tandy driver uninstalled, with the Tandy voices silenced first), then checks the heap
   ("Memory Corrupt!!!" when heapcheck reports -1) and puts the saved INT 3Fh vector back. */
/*@ 0892:0835 */ void ShutdownGame(void)
{
    if (gExitHook)
        gExitHook();
    if (gMouseHandlerInstalled)
        RemoveMouseHandler();
    if (g19E6)
        ShutdownOverlayCache();
    switch (gSoundChoice) {
    case 1:
        StopAdlibDriver();
        break;
    case 2:
        SoundDriverUninstall();
        break;
    case 3:
        SoundDriverSilenceToOneVoice();
        g289B = 0;
        SoundDriverUninstall();
        break;
    }
    f1D8F_0038();
    RemoveKeyboard();
    if (heapcheck() == -1)
        PrintString("\r\nMemory Corrupt!!!\r\n");
    setvect(INT3F_VECTOR, gSavedInt3F);
}

/* 0892:08C0: Purpose unknown: returns 1 and does nothing else. Main installs it as the far function
   pointer at ds:1F86 (:009C), the exit hook ShutdownGame calls (:0841); nothing outside this unit
   assigns ds:1F86. */
/*@ 0892:08C0 */ int ExitHook(void)
{
    return 1;
}

/* 0892:08C8: the allocation-failure hook. Restores the video mode (index 7), prints "Allocation
   failure, returning to DOS!", waits for a key and runs the shut-down path. Main installs it at
   ds:1F96 (:00A8), the pointer MemAlloc (094F:0007) calls when a far allocation fails. */
/*@ 0892:08C8 */ int AllocationFailure(void)
{
    f1DDD_002D(7);
    PrintString("\r\nAllocation failure, returning to DOS!");
    WaitGameKey();
    ShutdownGame();
    return 0;
}
