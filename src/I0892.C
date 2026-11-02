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
extern void FileRead(void far *buf, unsigned len, int handle); /*@ 0B5D:03F6 */
extern int RunOverlayRequest(void far *req);     /*@ 0B4E:005B */
extern int LargestFreeBlock(void);               /*@ 094F:023D */
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
extern void far *f1A47_0005(char far *name);     /*@ 09B5:000E */
extern void f1A47_0169(void far *font);          /*@ 09B5:0172 */
extern void far *GetDriverBlock(void);           /*@ 0000:03AA: the i_text sound driver data block */
extern void RebaseDriverBlock(void);             /*@ 0000:03FA */
extern int heapcheck(void);                      /*@ 0000:7F20 */
extern long atol(char far *s);                   /*@ 0000:66A5 */
extern int strcmp(char far *a, char far *b);     /*@ 0000:73A3 */
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

/* INTRO.EXE's overlay 0 (the intro playback); main runs it when g1A34 is set (:0158). */
extern int Ovl0Main(int argc, char far * far *argv); /*@ ovl0:0000 */

/* The far object Int3FHandler (09E1:0000) main installs as the INT 3Fh handler (:0074). Declared as
   an object, not a function: the matcher cannot map a far function pointer taken from another unit
   passed as an argument, but an object at 09E1:0000 gives the same segment:offset (src/E07C8.C:200). */
extern unsigned char far Int3FHandler;           /*@ 09E1:0000 */

int Main(int argc, char far * far *argv);
int StartupGame(void);
void ShutdownGame(void);
int ExitHook(void);
int AllocationFailure(void);

/*@ 0892:0000 stub */ int Main(int argc, char far * far *argv) { return 0; }

/*@ 0892:0241 stub */ int StartupGame(void) { return 0; }

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
