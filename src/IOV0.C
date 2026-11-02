/* Unit at overlay 0 of INTRO.EXE (11741 bytes, 22 functions): the Westwood introduction
   sequence. RunIntro (ovl0:0000) is the overlay's entry, called from the front end at
   0892:0161 through stub entry 16; PlayIntroSequence (ovl0:003A) walks the scenes, each a
   function of this file named after the .CMP/.COL files it loads (TITLE-E/TITLE-V, TEXT,
   WTRDP1..3, ORB, TOWRMAGE, KING, ADVENTUR, HANDS, TUNNEL, ZOOMTUNL, AVALANCH, SSI, PRESENT,
   DAND, WESTWOOD). The whole unit is drawn through the driver pointers the front end fills at
   load time (ds:1EE2 and its neighbours), which this page only partly names. */

/*@flags -ml -O -Z -G */

#include <dos.h>

/* The driver entry points. The front end fills these words at load time (i0892 sets ds:1F46
   from its palette setup at 0892:011D..0142); their DGROUP neighbours ds:1E98..1ED0 hold far
   pointers into this program's own segments, so the block is a driver vtable [H]. Only the
   entries this unit calls are declared. */
extern void (far *gUnknown1EE2)(void);                              /*@ ds:1EE2 */
extern void (far *gDrawLine)(int x1, int y1, int x2, int y2, int color); /*@ ds:1EEE */
extern void (far *gFillRect)(int x1, int y1, int x2, int y2, int color); /*@ ds:1EF6 */
extern int (far *gGetPixel)(int x, int y);                          /*@ ds:1EFE */
extern void far *(far *gMakeImage)(int x, int y, int width, int height, char far *base); /*@ ds:1F0E */
extern void (far *gPutPixel)(int x, int y, int color);              /*@ ds:1F2A */
extern void (far *gSetPalette)(char far *palette);                  /*@ ds:1F46 */

/* Globals of this unit's DGROUP (INTRO.EXE, paragraph 0E9F). */
extern int gGraphicsMode;               /*@ ds:1F8E */
extern int gDrawPage;                   /*@ ds:1FA1 */
extern long gFrameDelay;                /*@ ds:2326 */
extern int gAbort;                      /*@ ds:2D1C */

/* The keyboard routines of the library (i0a19, the twin of EOB.EXE's 1AAD input unit). */
extern int ReadKey(void);               /*@ 0A19:0382 */
extern int WaitGameKey(void);           /*@ 0A19:052E */
extern void WaitFrame(void);            /*@ 098F:0023 */

/* 097F:002A: hands a copy job to the far routine at ds:8046 when two buffer slots hold
   distinct non-zero segments (src/E19EC.C:29). Its third and fourth arguments are a far
   pointer the handler uses as the source mask/scratch buffer. */
extern void f19EC_002A(int dstKind, int mode, int extraOff, int extraSeg); /*@ 097F:002A */

/* 0AED:000D: reads a named file into a far buffer, the EOB LoadFile at 1B82:0009 linked at
   another address (the twin unit is i0aed, no EOB twin). */
extern void LoadFile(char far *name, void far *buf, unsigned len); /*@ 0AED:000D */
extern void far *f19EC_0002(int kind); /*@ 097F:0002 */
extern int FileOpen(char far *name, int mode); /*@ 0B5D:00D1 */
extern long FileRead(int index, char far *buf, unsigned long len); /*@ 0B5D:03F6 */
extern void FileClose(int index); /*@ 0B5D:03B1 */
extern void Unpack4(char far *src, void far *dst, int len); /*@ 0CE9:0009 */

/* The sound driver. The INTRO addresses differ from EOB's (0000:400C -> 0000:43E7, 1948:0418
   -> 084A:0417); the names are the EOB ones (src/E177D.C:129, src/E1948.ASM:41). */
extern unsigned char far *gSoundMap;    /*@ ds:28A0 */  /* the game-number to driver-number map */
extern int gSoundCard;                  /*@ ds:28C6 */  /* 1 AdLib, 2 PC speaker, 3 Tandy */
extern unsigned char gTandyOn;          /*@ ds:289B */  /* the PC speaker / Tandy driver select */
extern void QueueSound(int sound);      /*@ 0000:43E7 */
extern void far *GetSoundData(int idx); /*@ 0000:03E3 */
extern void StartSound(void far *routine); /*@ 084A:0417 */
extern void SoundDriverResume(void);    /*@ 084A:0457 */

/* The palette and image buffers of this unit's DGROUP. gPalette is a far pointer the front end
   allocates and fills (i0892:05A2..061A); gOverlayBuf is the DGROUP image buffer f19EC_002A
   hands to its copy handler, EOB.EXE's gOverlayBuf at ds:113F. */
extern unsigned char far *gPalette;     /*@ ds:2D42 */
extern unsigned char gOverlayBuf[];     /*@ ds:1C0A */
extern unsigned char gEgaPalette[16];   /*@ ds:19E7 */

/* the delay of the Westwood logo card, in timer ticks: 50, read by Ovl0_2A99 (ds:1D28) */
extern int gWestwoodDelay;              /*@ ds:1D28 */
/* the delays of the four SSI logo cards (100, 50, 140, 140), read by Ovl0_2A99 (ds:1D2A) */
extern int gLogoDelays[4];              /*@ ds:1D2A */
/* the CMP images of the four SSI logo cards (AND, SSI, PRESENT, DAND), read by Ovl0_2A99 */
extern char far *gLogoFiles[4];         /*@ ds:1D32 */

/* 0CDA:0006: the pseudo-random byte generator of i0cda. It advances the 32-bit seed at
   ds:2310 (0x98761234 at load) with a shift and xor sequence and returns a byte; 0FD9 calls
   it and discards the result, the animations use it to pick star positions and colours. */
extern unsigned char RandomByte(void);  /*@ 0CDA:0006 */

/* the key code of the escape key, the one key the wait loop reacts to */
#define KEY_ESC 0x1B

/* Functions of this file, in address order (a call to one defined later compiles through the
   overlay's far call, which TLINK rewrites to nop; push cs; call near, the form the original
   shows as 90 0E E8). */
void PlayIntroSequence(void);
void Ovl0_015D(void);
void Ovl0_06E2(void);
void Ovl0_09AD(void);
void Ovl0_0B20(void);
void ClearBottomBar(void);
void WaitFrameDelay(void);
void DelayFrames(unsigned frames);
void Ovl0_1041(void);
int SignOf(int value);
void Ovl0_1265(void);
void Ovl0_15F9(void);
void Ovl0_1DF3(void);
void Ovl0_23A3(void);
void Ovl0_2913(void);
void Ovl0_29C9(void);
void LoadPalette(char far *name);
void LoadPackedImage(char far *name, int srcKind, int dstKind);
void Dissolve(int x, int y, int width, int height, int fromPage, int toPage);
void PlaySound(int sound);

/* Functions of this file, in address order (forward declarations: a call to one defined
   later compiles through the overlay's far call, which TLINK rewrites to nop; push cs; call
   near, the form the original shows as 90 0E E8). */
void PlayIntroSequence(void);
void Ovl0_2A99(void);

/* 0E96:0000: the introduction overlay's entry point. Clears the abort flag, for the CGA and
   EGA modes (gGraphicsMode 1 and 2) starts the driver through ds:1EE2 and applies the fixed
   palette at ds:19E7, plays the logo sequence (ovl0:2A99), then the scenes (ovl0:003A)
   unless a key aborted the intro. */
/*@ ovl0:0000 */ void RunIntro(void)
{
    gAbort = 0;
    if (gGraphicsMode == 1 || gGraphicsMode == 2) {
        gUnknown1EE2();
        gSetPalette(gEgaPalette);
    }
    Ovl0_2A99();
    if (!gAbort)
        PlayIntroSequence();
}

/*@ ovl0:003A stub */ void PlayIntroSequence(void) {}
/*@ ovl0:015D stub */ void Ovl0_015D(void) {}
/*@ ovl0:06E2 stub */ void Ovl0_06E2(void) {}
/*@ ovl0:09AD stub */ void Ovl0_09AD(void) {}
/*@ ovl0:0B20 stub */ void Ovl0_0B20(void) {}
/* 0E96:0FAA: clears the bottom bar of the screen, the rectangle from (0,168) to (319,199),
   with colour 12 on the page the caller wants visible (page 0), then puts the driver's
   current page (gDrawPage) back. Called around the title cards to wipe their text. */
/*@ ovl0:0FAA */ void ClearBottomBar(void)
{
    int page;

    page = gDrawPage;
    gDrawPage = 0;
    gFillRect(0, 0xA8, 0x13F, 0xC7, 0xC);
    gDrawPage = page;
}

/* 0E96:0FD9: waits out gFrameDelay, one pass per timer tick, polling the keyboard. A pending
   key is taken: the rest of the key ring is drained (esc sets the abort flag gAbort, so the
   caller skips the rest of the intro) and the delay is cleared, so any key skips the wait.
   RandomByte is called once per pass even when a key was drained; it advances the seed at
   ds:2310 and its result is not used here. */
/*@ ovl0:0FD9 */ void WaitFrameDelay(void)
{
    if (!gAbort)
        while (gFrameDelay) {
            if (ReadKey()) {
                while (ReadKey()) {
                    if (WaitGameKey() == KEY_ESC)
                        gAbort = 1;
                }
                gFrameDelay = 0;
            }
            RandomByte();
        }
}

/* 0E96:1025: waits `frames` timer ticks by setting gFrameDelay and calling WaitFrameDelay
   (unless a key already aborted the intro). The high word is cleared, so `frames` is zero
   extended: it cannot be made negative. */
/*@ ovl0:1025 */ void DelayFrames(unsigned frames)
{
    gFrameDelay = frames;
    if (!gAbort)
        WaitFrameDelay();
}

/*@ ovl0:1041 stub */ void Ovl0_1041(void) {}

/* 0E96:1248: the sign of `value` as -1, 0 or 1. Used by the animations to turn a direction
   delta into a step. */
/*@ ovl0:1248 */ int SignOf(int value)
{
    if (value < 0)
        return -1;
    if (value > 0)
        return 1;
    return 0;
}
/*@ ovl0:1265 stub */ void Ovl0_1265(void) {}
/*@ ovl0:15F9 stub */ void Ovl0_15F9(void) {}
/*@ ovl0:1DF3 stub */ void Ovl0_1DF3(void) {}
/*@ ovl0:23A3 stub */ void Ovl0_23A3(void) {}
/*@ ovl0:2913 stub */ void Ovl0_2913(void) {}
/*@ ovl0:29C9 stub */ void Ovl0_29C9(void) {}
/* 0E96:2A4E: reads a 768-byte .COL palette file into gPalette and applies it, only on the VGA
   mode (gGraphicsMode 3). The first colour (black) is cleared and one frame is waited before
   the driver is given the palette, so the screen blanks first. */
/*@ ovl0:2A4E */ void LoadPalette(char far *name)
{
    if (gGraphicsMode == 3) {
        LoadFile(name, gPalette, 0x300);
        gPalette[0] = gPalette[1] = gPalette[2] = 0;
        WaitFrame();
        gSetPalette(gPalette);
    }
}
/* 0E96:2A99: plays the Westwood logo card: loads WESTWOOD.CMP into the buffer slots 5/3,
   starts the driver, applies the WESTWOOD.COL palette, copies the buffers through
   f19EC_002A, makes sound 1 and shows the card for gWestwoodDelay ticks. Then, unless a key
   aborted, it shows the four SSI logo cards in turn (each from gLogoFiles into slots 5/3,
   held for its gLogoDelays entry, with the dissolve effect from page 4 to page 0 and sound
   again), and leaves a 50-tick delay set. */
/*@ ovl0:2A99 */ void Ovl0_2A99(void)
{
    int i;

    LoadPackedImage("WESTWOOD.CMP", 5, 3);
    gUnknown1EE2();
    LoadPalette("WESTWOOD.COL");
    f19EC_002A(3, 0, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    PlaySound(1);
    gFrameDelay = gWestwoodDelay;
    WaitFrameDelay();
    if (gAbort)
        return;
    for (i = 0; i < 4; i++) {
        gFrameDelay = gLogoDelays[i];
        LoadPackedImage(gLogoFiles[i], 5, 3);
        f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
        Dissolve(0, 0x32, 0x140, 0x66, 4, 0);
        WaitFrameDelay();
        if (gAbort)
            return;
    }
    gFrameDelay = 0x32;
}
/* 0E96:2B7F: loads a packed .CMP image into the buffer slot `dstKind`: slot `srcKind` is the
   read buffer, the file's first word is the packed length, and the rest is unpacked by the LCW
   decoder Unpack4 (0CE9:0009) into dstKind. The unpacked size is fixed at 0xFA00 (64000, the
   largest screen image) and the block's 8-byte header is skipped. The EOB twin is LoadPacked
   (src/E1B82.C:100), which dispatches on the format byte instead; these intro files are all
   format 4. QUIRK: the FileOpen result is not checked, and the count FileRead returns is
   discarded (as in the EOB twin). */
/*@ ovl0:2B7F */ void LoadPackedImage(char far *name, int srcKind, int dstKind)
{
    unsigned size;
    void far *src;
    void far *dst;
    int slot;

    src = f19EC_0002(srcKind);
    dst = f19EC_0002(dstKind);
    slot = FileOpen(name, 1);
    FileRead(slot, (char far *)&size, 2);
    FileRead(slot, (char far *)src, size);
    FileClose(slot);
    Unpack4((char far *)src + 8, dst, 0xFA00);
}
/*@ ovl0:2C0D stub */ void Dissolve(int x, int y, int width, int height, int fromPage, int toPage) {}
/* 0E96:2D4E: plays the game sound numbered `sound` (the argument is an index, not a driver
   number). The driver number comes from gSoundMap (a byte; 0xFF means silence). gSoundCard
   selects the path: 1 (AdLib) queues the driver number and returns, 2 (PC speaker) and 3
   (Tandy) start the speaker routine the low seven bits select through GetSoundData; an
   unknown card does nothing. On the Tandy path a nonzero driver number resumes the driver
   only while gTandyOn is clear. This is the EOB PlaySound (src/E177D.C:166) with a one-based
   gSoundCard, without the sound-on test and the 0x96 map cutoff.
   The map byte is kept both in mapByte and, zero extended, in driverSound (SI), which is why
   the index and the later tests share a register; the EOB twin keeps only the int.
   QUIRK 2DB4: like the EOB code the `driverSound < 0` test is dead (the map byte is zero
   extended), and the `driverSound > 0` branch stores 0 into gTandyOn where EOB stores 1. */
/*@ ovl0:2D4E */ void PlaySound(int sound)
{
    unsigned char mapByte;
    int driverSound;
    void far *routine;

    driverSound = sound;
    mapByte = gSoundMap[driverSound];
    if (mapByte == 0xFF)
        mapByte = 0;
    driverSound = mapByte;
    switch (gSoundCard) {
    case 1:
        QueueSound(driverSound);
        return;
    case 2:
        break;
    case 3:
        if (driverSound < 0 && gTandyOn) {
            gTandyOn = 0;
            SoundDriverResume();
        }
        if (driverSound > 0 && gTandyOn == 0) {
            gTandyOn = 0;
            SoundDriverResume();
        }
        break;
    default:
        return;
    }
    routine = GetSoundData(driverSound & 0x7F);
    StartSound(routine);
}