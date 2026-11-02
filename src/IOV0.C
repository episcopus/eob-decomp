/* Unit at overlay 0 of INTRO.EXE (11741 bytes, 22 functions): the Westwood introduction
   sequence. RunIntro (ovl0:0000) is the overlay's entry, called from the front end at
   0892:0161 through stub entry 16; PlayIntroSequence (ovl0:003A) walks the scenes, each a
   function of this file named after the .CMP/.COL files it loads (TITLE-E/TITLE-V, TEXT,
   WTRDP1..3, ORB, TOWRMAGE, KING, ADVENTUR, HANDS, TUNNEL, ZOOMTUNL, AVALANCH, SSI, PRESENT,
   DAND, WESTWOOD). The whole unit is drawn through the driver pointers the front end fills at
   load time (ds:1EE2 and its neighbours), which this page only partly names.
   The scene functions are named after the .CMP/.COL files they load, all written: PlayWtrdpScene
   (ovl0:015D, WTRDP1..3), PlayOrbScene (ovl0:06E2, ORB/TOWRMAGE), PlayTowerMageScene
   (ovl0:0B20, TOWRMAGE), PlayKingScene (ovl0:1265, KING/ADVENTUR), PlayHandsScene (ovl0:15F9,
   HANDS), PlayTunnelScene (ovl0:1DF3, WTRDP3/ORB/TUNNEL/ZOOMTUNL) and PlayAvalancheScene
   (ovl0:23A3, AVALANCH/ZOOMTUNL). They are long ClipRect/frame animations; every utility they
   use (LoadPackedImage, LoadPalette, PlaySound, Dissolve, BlitImage, DrawGrid, GrowFrame,
   WaitFrameDelay, f19EC_002A) is matched. */

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
extern int gGraphicsChoice;             /*@ ds:1F8E */  /* the graphics choice: argv[1][0] + 1 then 0x31 less (0892:0010, 0892:0258) */
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
   -> 084A:0417). The routine names follow EOB's 177D:0070 and 1948:0418; gSoundBuf and
   gSoundChoice are the names the front end gives them (0892:06D1, 0892:06A9). */
extern unsigned char far *gSoundBuf;    /*@ ds:28A0 */  /* the game-number to driver-number map */
extern int gSoundChoice;                  /*@ ds:28C6 */  /* 1 AdLib, 2 PC speaker, 3 Tandy */
extern unsigned char gTandyOn;          /*@ ds:289B */  /* the PC speaker / Tandy driver select */
extern void QueueSound(int sound);      /*@ 0000:43E7 */
extern void far *GetSoundData(int idx); /*@ 0000:03E3 */
extern void StartSound(void far *routine); /*@ 084A:0417 */
extern void SoundDriverResume(void);    /*@ 084A:0457 */

/* The palette and image buffers of this unit's DGROUP. gPaletteBuf is a far pointer the front end
   allocates and fills (i0892:05A2..061A); gOverlayBuf and gCopyBuf are the DGROUP buffers
   f19EC_002A hands to its copy handler, EOB.EXE's gOverlayBuf at ds:113F: ovl0:2A99 passes
   gOverlayBuf and ovl0:003A passes gCopyBuf (ds:1BEA) for the title copy and gOverlayBuf
   (ds:1C0A, 32 bytes into the same zeroed region) for the text copy. */
extern unsigned char far *gPaletteBuf;     /*@ ds:2D42 */
extern unsigned char gOverlayBuf[];     /*@ ds:1C0A */
extern unsigned char gCopyBuf[];        /*@ ds:1BEA */
extern unsigned char gEgaPalette[16];   /*@ ds:19E7 */
extern int gDriverState;                /*@ ds:1F76 */  /* Purpose unknown: a driver flag; ovl0:003A sets bit 0 only */

/* the delay of the Westwood logo card, in timer ticks: 50, read by PlayLogoSequence (ds:1D28) */
extern int gWestwoodDelay;              /*@ ds:1D28 */
/* the delays of the four SSI logo cards (100, 50, 140, 140), read by PlayLogoSequence (ds:1D2A) */
extern int gLogoDelays[4];              /*@ ds:1D2A */
/* the CMP images of the four SSI logo cards (AND, SSI, PRESENT, DAND), read by PlayLogoSequence */
extern char far *gLogoFiles[4];         /*@ ds:1D32 */

/* 0CDA:0006: the pseudo-random byte generator of i0cda. It advances the 32-bit seed at
   ds:2310 (0x98761234 at load) with a shift and xor sequence and returns a byte; 0FD9 calls
   it and discards the result, the animations use it to pick star positions and colours. */
extern unsigned char RandomByte(void);  /*@ 0CDA:0006 */
extern int RandomRange(int low, int high); /*@ 0A15:0009 */

/* the permutation tables Dissolve shuffles (ovl0:2C0D): one word per screen column and row */
extern int gOrderX[320];                /*@ ds:290C */
extern int gOrderY[200];                /*@ ds:2B8C */

/* the key code of the escape key, the one key the wait loop reacts to */
#define KEY_ESC 0x1B

/* Functions of this file, in address order (a call to one defined later compiles through the
   overlay's far call, which TLINK rewrites to nop; push cs; call near, the form the original
   shows as 90 0E E8). */
void PlayIntroSequence(void);
void PlayWtrdpScene(void);
void PlayOrbScene(void);
void BlitImage(int x1, int y1, int x2, int y2, int width, int height, int mode);
void PlayTowerMageScene(void);
void ClearBottomBar(void);
void WaitFrameDelay(void);
void DelayFrames(unsigned frames);
void GrowFrame(int left, int top, int dstLeft, int dstTop, int x, int y, int width, int height, int curWidth, int curHeight, int addW, int addH);
int SignOf(int value);
void PlayKingScene(void);
void PlayHandsScene(void);
void PlayTunnelScene(void);
void PlayAvalancheScene(void);
void RevealInStrips(void);
void DrawGrid(void);
extern void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ 0986:000E */
void LoadPalette(char far *name);
void LoadPackedImage(char far *name, int srcKind, int dstKind);
void Dissolve(int x, int y, unsigned width, unsigned height, int fromPage, int toPage);
void PlaySound(int sound);

/* Functions of this file, in address order (forward declarations: a call to one defined
   later compiles through the overlay's far call, which TLINK rewrites to nop; push cs; call
   near, the form the original shows as 90 0E E8). */
void PlayIntroSequence(void);
void PlayLogoSequence(void);

/* ovl0:0000: the introduction overlay's entry point. Clears the abort flag, for the two
   non-VGA modes (gGraphicsChoice 1 and 2) starts the driver through ds:1EE2 and applies the
   fixed palette at ds:19E7, plays the logo sequence (ovl0:2A99), then the scenes
   (ovl0:003A) unless a key aborted the intro. */
/*@ ovl0:0000 */ void RunIntro(void)
{
    gAbort = 0;
    if (gGraphicsChoice == 1 || gGraphicsChoice == 2) {
        gUnknown1EE2();
        gSetPalette(gEgaPalette);
    }
    PlayLogoSequence();
    if (!gAbort)
        PlayIntroSequence();
}

/* ovl0:003A: plays the title card and then every scene. It sets the intro's driver flag
   (ds:1F76 bit 0), draws on page 0 and holds the title for 120 ticks: the TITLE-V card on the
   VGA mode (gGraphicsChoice 3), TITLE-E otherwise, then the WESTWOOD palette, sound 2, the
   buffer copies and the dissolve from page 2 to page 0. Then TEXT.CMP, another copy and a
   6-tick wait. Each of the eight scene functions follows guarded by the abort flag, so a key
   during a wait skips the rest of the intro. */
/*@ ovl0:003A */ void PlayIntroSequence(void)
{
    gDriverState |= 1;
    gDrawPage = 0;
    gFrameDelay = 0x78;
    if (gGraphicsChoice != 3)
        LoadPackedImage("TITLE-E.CMP", 3, 5);
    else
        LoadPackedImage("TITLE-V.CMP", 3, 5);
    WaitFrame();
    LoadPalette("WESTWOOD.COL");
    PlaySound(2);
    f19EC_002A(5, 2, FP_OFF(gCopyBuf), FP_SEG(gCopyBuf));
    Dissolve(0, 0, 320, 200, 2, 0);
    LoadPackedImage("TEXT.CMP", 3, 5);
    f19EC_002A(5, 6, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    WaitFrameDelay();
    if (!gAbort)
        PlayTowerMageScene();
    if (!gAbort)
        PlayOrbScene();
    if (!gAbort)
        PlayWtrdpScene();
    if (!gAbort)
        PlayKingScene();
    if (!gAbort)
        PlayHandsScene();
    if (!gAbort)
        PlayTunnelScene();
    if (!gAbort)
        PlayAvalancheScene();
    if (!gAbort)
        DrawGrid();
}
/* One animation tile slot: an image pointer, or (in slot 3, once it has been freed) the moving
   window position the main loop of ovl0:015D slides across the screen. */
union SpriteSlot {
    void far *image;
    struct { int y; int x; } win;
};

/* The byte and word tables ovl0:015D walks: each has one entry per animation tile. What the
   entries mean is not established yet; the code uses the first three to cut 31 tiles out of
   gOverlayBuf and the last two to place a tile at random. */
extern unsigned char gTileX[31];        /*@ ds:1C2A */  /* Purpose unknown: x of each tile, ovl0:015D */
extern unsigned char gTileY[31];        /*@ ds:1C49 */  /* Purpose unknown: y row of each tile, ovl0:015D */
extern unsigned gStarX[31];             /*@ ds:1C68 */  /* Purpose unknown: x to draw each star, ovl0:015D */
extern unsigned char gStarY[31];        /*@ ds:1CA6 */  /* Purpose unknown: y/colour of each star, ovl0:015D */
extern unsigned char gTileScale[31];    /*@ ds:1CC5 */  /* Purpose unknown: size of each tile, ovl0:015D */
extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 0998:0008 */
extern void MemFree(void far *block); /*@ 094F:0137 */

/* ovl0:015D: the WTRDP scene, the tutorial corridor. It loads the WTRDP2 palette and the
   WTRDP1..3 images, cuts the title sprite and, in the main loop, slides a window (x from 0x108
   down, y from 0x0B up) across the screen: each step draws one of the three 5 by 0x20 frames
   four times and drops five random star tiles taken from gOverlayBuf, occasionally playing a
   sound. A three-step wipe then clears the screen. All the scratch images (the four sprites,
   31 stars and three frames) are freed at the end. */
/*@ ovl0:015D */ void PlayWtrdpScene(void)
{
    int k;
    union SpriteSlot sprites[4];
    void far *stars[31];
    void far *frames[3];
    int i;
    int frame;

    LoadPalette("WTRDP2.COL");
    gDrawPage = 2;
    LoadPackedImage("WTRDP1.CMP", 5, 3);
    sprites[3].image = gMakeImage(0, 0, 0x14, 0x88, gOverlayBuf);
    for (i = 1; i < 4; i++) {
        BlitImage(0, 0, 0, 0, 0xa0, 0x88, i);
        sprites[3 - i].image = gMakeImage(0, 0, 0x14, 0x88, gOverlayBuf);
    }
    f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    WaitFrameDelay();
    ClearBottomBar();
    PlaySound(6);
    for (i = 0; i < 4; i++) {
        gFrameDelay = 3;
        f1A29_000F(0, sprites[i].image, 0x50, 0x18, 0);
        MemFree(sprites[i].image);
        WaitFrameDelay();
    }
    ClipRect(0, 0x50, 0, 0xa8, 0x28, 0x10, 6, 0);
    gDrawPage = 2;
    gFrameDelay = 0x32;
    sprites[0].image = gMakeImage(0x14, 0, 0x14, 0x88, gOverlayBuf);
    LoadPackedImage("WTRDP2.CMP", 5, 3);
    sprites[1].image = gMakeImage(0, 0, 0x14, 0x88, gOverlayBuf);
    sprites[2].image = gMakeImage(0x14, 0, 0x14, 0x88, gOverlayBuf);
    LoadPackedImage("WTRDP3.CMP", 5, 3);
    gDrawPage = 2;
    for (i = 0; i < 31; i++)
        stars[i] = gMakeImage(gTileX[i], gTileY[i] * 8 + 0x88, gTileScale[i], gTileScale[i] * 8, gOverlayBuf);
    for (i = 0; i < 3; i++)
        frames[i] = gMakeImage(i * 5, 0x98, 5, 0x20, gOverlayBuf);
    f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    for (i = 0; i < 3; i++) {
        WaitFrameDelay();
        gFillRect(0, 0, 0x9f, 0x87, 0xc);
        f1A29_000F(2, sprites[i].image, 0, 0, 0);
        ClipRect(0, 0, 0xa, 0x18, 0x14, 0x88, 2, 0);
        gFrameDelay = 3;
    }
    gFrameDelay = 4;
    ClipRect(0, 0, 0xa, 0x18, 0x14, 0x88, 4, 0);
    WaitFrameDelay();
    gFrameDelay = 4;
    ClipRect(0x14, 0, 0xa, 0x18, 0x14, 0x88, 4, 0);
    ClearBottomBar();
    WaitFrameDelay();
    ClipRect(0, 0xb8, 5, 0xb8, 0x1d, 0x10, 4, 0);
    sprites[3].win.x = 0x108;
    sprites[3].win.y = 0xb;
    for (i = 0; i <= 0x46; i++) {
        gFrameDelay = 3;
        ClipRect((sprites[3].win.x - 2) >> 3, sprites[3].win.y - 2, 0, 0, 6, 0x24, 4, 4);
        frame = i & 3;
        if (frame == 3)
            frame = 1;
        f1A29_000F(4, frames[frame], sprites[3].win.x, sprites[3].win.y, 0);
        ClipRect((sprites[3].win.x - 2) >> 3, sprites[3].win.y - 2,
                 (sprites[3].win.x - 0x52) >> 3, sprites[3].win.y + 0x16, 6, 0x24, 4, 0);
        ClipRect(0, 0, (sprites[3].win.x - 2) >> 3, sprites[3].win.y - 2, 6, 0x24, 4, 4);
        sprites[3].win.x--;
        sprites[3].win.y++;
        for (k = 0; k < 5; k++) {
            frame = RandomByte() % 31;
            f1A29_000F(0, stars[frame], gStarX[frame] - 0x50, gStarY[frame] + 0x18, 0);
        }
        if (!(RandomByte() & 7))
            PlaySound((RandomByte() & 1) ? 5 : 0xe);
        WaitFrameDelay();
    }
    gDrawPage = 0;
    for (i = 0; i < 31; i++) {
        if (i < 3) {
            MemFree(sprites[i].image);
            MemFree(frames[i]);
        }
        MemFree(stars[i]);
    }
}
/* ovl0:06E2: the ORB/TOWRMAGE scene, the orb and the tower mage. It loads ORB.CMP and cuts the
   orb sprite, loads TOWRMAGE.CMP and cuts the mage's four animation frames, then in two passes
   over five steps (i from -1 to 3, then from 3 down to -1) draws the orb and, once i is a real
   frame, the frame over it, each step wiping a 0x14 by 0x88 window. A key aborts like the other
   scenes. All the scratch images are freed at the end. */
/*@ ovl0:06E2 */ void PlayOrbScene(void)
{
    void far *orb;
    void far *mage[4];
    int i;

    LoadPackedImage("ORB.CMP", 5, 3);
    gDrawPage = 2;
    orb = gMakeImage(0, 0, 0x14, 0x88, gOverlayBuf);
    LoadPackedImage("TOWRMAGE.CMP", 5, 3);
    mage[3] = gMakeImage(0x10, 0, 0x10, 0x68, gOverlayBuf);
    gFillRect(0, 0, 0x7f, 0x67, 0xc);
    for (i = 1; i < 4; i++) {
        BlitImage(0x80, 0, 0, 0, 0x80, 0x68, i);
        mage[3 - i] = gMakeImage(0, 0, 0x10, 0x68, gOverlayBuf);
    }
    gFillRect(0, 0, 0x9f, 0x87, 0xc);
    gDrawPage = 0;
    f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    WaitFrameDelay();
    gUnknown1EE2();
    PlaySound(6);
    for (i = -1; i < 4; i++) {
        gFrameDelay = 3;
        if (i >= 0)
            f1A29_000F(2, mage[i], 0x10, 0x10, 0);
        f1A29_000F(2, orb, 0, 0, 0);
        ClipRect(0, 0, 0xa, 0x18, 0x14, 0x88, 2, 0);
        WaitFrameDelay();
    }
    ClipRect(0, 0x40, 0, 0xa8, 0x28, 0x10, 6, 0);
    DelayFrames(0x28);
    PlaySound(6);
    for (i = 3; i > -2; i--) {
        gFrameDelay = 3;
        gDrawPage = 2;
        gFillRect(0x10, 0x10, 0x8f, 0x77, 0xc);
        gDrawPage = 0;
        if (i >= 0)
            f1A29_000F(2, mage[i], 0x10, 0x10, 0);
        f1A29_000F(2, orb, 0, 0, 0);
        ClipRect(0, 0, 0xa, 0x18, 0x14, 0x88, 2, 0);
        WaitFrameDelay();
    }
    for (i = 0; i < 4; i++)
        MemFree(mage[i]);
    gFrameDelay = 0x28;
    MemFree(orb);
}
/* ovl0:09AD: copies an image between two rectangles of the same video buffer (slot 3, the
   segment f19EC_0002(3) returns), from the rectangle at (x1,y1) to the one at (x2,y2), each
   row `width` bytes wide and `height` lines high. `mode` selects the byte interleave:
     0: nothing;
     1: two source rows per pass, the low bytes (i even) and the high bytes (i odd) written
        into alternate destination rows, the other byte cleared;
     2: one source row per pass in the low bytes, the destination's second row cleared, the
        source advancing two rows;
     3: every third source row is written with three clears per byte, the two rows between
        are cleared.
   Used by the scene functions to build the layered sprites (ovl0:015D, ovl0:06E2). */
/*@ ovl0:09AD */ void BlitImage(int x1, int y1, int x2, int y2, int width, int height, int mode)
{
    int i;
    char far *src;
    char far *dst;
    int row;

    src = dst = (char far *)f19EC_0002(3);
    src += x1 + y1 * 320;
    dst += x2 + y2 * 320;
    switch (mode) {
    case 1:
        while (height > 0) {
            for (i = 0; i < width; i += 2) {
                dst[i] = src[i];
                dst[i + 1] = 0;
            }
            dst += 320;
            src += 320;
            for (i = 0; i < width; i += 2) {
                dst[i] = 0;
                dst[i + 1] = src[i + 1];
            }
            dst += 320;
            src += 320;
            height -= 2;
        }
        break;
    case 2:
        while (height > 0) {
            for (i = 0; i < width; i += 2) {
                dst[i] = src[i];
                dst[i + 1] = 0;
            }
            dst += 320;
            for (i = 0; i < width; i++)
                dst[i] = 0;
            dst += 320;
            src += 640;
            height -= 2;
        }
        break;
    case 3:
        row = 0;
        while (row < height) {
            if (row % 3) {
                for (i = 0; i < width; i++)
                    dst[i] = 0;
            } else {
                for (i = 0; i < width - 3; i += 3) {
                    dst[i] = src[i];
                    dst[i + 1] = 0;
                    dst[i + 2] = 0;
                }
                for (; i < width; i++)
                    dst[i] = 0;
            }
            row++;
            dst += 320;
            src += 320;
        }
        break;
    }
}
/* ovl0:0B20: the TOWRMAGE scene. It draws the grid twice (once on the hidden page), wipes the
   bottom bar, cuts the tower mage sprite out of gOverlayBuf, then plays three frame
   animations: a 0x40-step reveal, then two 2-pixel sweeps that slide the mage and its frame
   across the screen. `center` is the fixed 0x20 column the two sweeps pivot on. The mage
   image is freed at the end. */
/*@ ovl0:0B20 */ void PlayTowerMageScene(void)
{
    int page;
    void far *mage;
    int i;
    int center;

    center = 0x20;
    DrawGrid();
    LoadPalette("TOWRMAGE.COL");
    LoadPackedImage("TOWRMAGE.CMP", 5, 3);
    gDrawPage = 2;
    mage = gMakeImage(0, 0, 0x10, 0x38, gOverlayBuf);
    f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    gUnknown1EE2();
    for (i = 0; i < 0xc8; i += 0x40)
        ClipRect(0x10, 0x68, 0xc, i, 0x10, 0x40, 4, 2);
    gFillRect(0, 0xb8, 0x13f, 0xc7, 0xc);
    page = gDrawPage;
    gDrawPage = 0;
    DrawGrid();
    gDrawPage = page;
    gUnknown1EE2();
    for (i = 0; i < 0xc8; i += 0x40)
        ClipRect(0x10, 0x68, 0, i, 0x10, 0x40, 4, 2);
    gDrawPage = 0;
    for (i = 0; i < 0x40; i += 2) {
        gFrameDelay = 2;
        ClipRect(0, 0x8e - i, 0xc, 0, 0x10, i + 1, 4, 0);
        ClipRect(0, 0, 0xc, i + 1, 0x10, 0xa7 - i, 2, 0);
        if (i == 0)
            ClipRect(0, 0, 0, 0xa8, 0x28, 0x20, 6, 0);
        WaitFrameDelay();
    }
    for (i = 0; i < 0x18; i += 2) {
        gFrameDelay = 2;
        ClipRect(0, 0x8f - (i + 0x40), 0xc, 0, 3, i + 0x41, 4, 0);
        ClipRect(0xd, 0x8f - (i + 0x40), 0x19, 0, 3, i + 0x41, 4, 0);
        ClipRect(3, 0x6e, 0xf, i + 0x1f, 0xa, 0x22, 4, 0);
        ClipRect(0x13, 0, 0xf, center, 0xa, i + 1, 4, 0);
        ClipRect(0, 0, 0xc, i + 0x41, 0x10, 0xa7 - (i + 0x40), 2, 0);
        WaitFrameDelay();
    }
    for (i = 0; i < 0x38; i += 2) {
        gFrameDelay = 2;
        ClipRect(0, 0x38, 0xc, i, 3, 0x36, 4, 0);
        ClipRect(0xd, 0x38, 0x19, i, 3, 0x36, 4, 0);
        ClipRect(0, 0x6e, 0xc, i + 0x36, 0x10, 0x22, 4, 0);
        if (i < center) {
            gDrawPage = 2;
            gFillRect(0x80, 0, 0xff, i + 1, 0xc);
            gDrawPage = 0;
            ClipRect(0x13, 0, 0xf, center, 0xa, i + 0x19, 4, 0);
        } else {
            gDrawPage = 2;
            gFillRect(0x80, 0, 0xff, i + 1, 0xc);
            gDrawPage = 0;
            ClipRect(0x13, i + 1, 0xf, center + i + 1, 0xa, 0x17, 4, 0);
            ClipRect(0x13, 0, 0x13, center, 0xa, i + 1, 4, 2);
        }
        f1A29_000F(2, mage, 0x80, i - 0x37, 0);
        ClipRect(0x10, 0, 0xc, 0, 0x10, i + 1, 2, 0);
        ClipRect(0, 0, 0xc, i + 0x59, 0x10, 0xa7 - (i + 0x58), 2, 0);
        WaitFrameDelay();
    }
    ClipRect(0, 0x20, 0, 0xa8, 0x28, 0x20, 6, 0);
    gFrameDelay = 0x41;
    MemFree(mage);
}
/* ovl0:0FAA: clears the bottom bar of the screen, the rectangle from (0,168) to (319,199),
   with colour 12 on the page the caller wants visible (page 0), then puts the driver's
   current page (gDrawPage) back. Called by the scene functions between frames (ovl0:015D at
   0217 and 04CF, and ovl0:15F9, ovl0:1DF3 and ovl0:23A3) to wipe the text on the bar. */
/*@ ovl0:0FAA */ void ClearBottomBar(void)
{
    int page;

    page = gDrawPage;
    gDrawPage = 0;
    gFillRect(0, 0xA8, 0x13F, 0xC7, 0xC);
    gDrawPage = page;
}

/* ovl0:0FD9: waits out gFrameDelay, one pass per timer tick, polling the keyboard. A pending
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

/* ovl0:1025: waits `frames` timer ticks by setting gFrameDelay and calling WaitFrameDelay
   (unless a key already aborted the intro). The high word is cleared, so `frames` is zero
   extended: it cannot be made negative. */
/*@ ovl0:1025 */ void DelayFrames(unsigned frames)
{
    gFrameDelay = frames;
    if (!gAbort)
        WaitFrameDelay();
}

/* ovl0:1041: grows and moves a framed rectangle, one frame at a time, with the clip rectangle
   passed to the driver each frame. The frame starts at (left, top) and walks towards
   (dstLeft, dstTop) a pixel at a time (SignOf), while its size (growW, growH) grows by 2 up to
   (width, height) and the inner rect (curWidth, curHeight) grows to the destination; the
   edges (right, bottom) shrink until the frame reaches them, then the loop ends. Called twice
   by the ovl0:15F9 scene (16E6, 18DF) and twice by ovl0:1DF3 (1B1E, 1CCA). */
/*@ ovl0:1041 */ void GrowFrame(int left, int top, int dstLeft, int dstTop, int x, int y, int width, int height,
                               int curWidth, int curHeight, int addW, int addH)
{
    int growW;
    int growH;
    int done;
    int right;
    int bottom;

    right = curWidth + addW;
    bottom = curHeight + addH;
    if (bottom >= 0x15)
        bottom = 0x15;
    growW = growH = 1;
    done = 0;
    while (!done) {
        gFrameDelay = 1;
        ClipRect(x, y * 8, left, top * 8, growW, growH * 8, 2, 0);
        if (curWidth < left)
            ClipRect(0x27, 0, curWidth, 0, 1, 0xB0, 0, 0);
        if (curHeight < top)
            ClipRect(0, 0xC0, 0, curHeight * 8, 0x28, 8, 0, 0);
        if (dstLeft + width <= right)
            ClipRect(0x27, 0, right, 0, 1, 0xB0, 0, 0);
        if (dstTop + height <= bottom)
            ClipRect(0, 0xC0, 0, bottom * 8, 0x28, 8, 0, 0);
        if (left == dstLeft && top == dstTop && growW == width && growH == height &&
            curWidth >= dstLeft && curHeight >= dstTop &&
            dstLeft + width >= right && dstTop + height >= bottom)
            done = 1;
        left += SignOf(dstLeft - left);
        top += SignOf(dstTop - top);
        if (growW != width)
            growW += 2;
        if (growW > width)
            growW = width;
        if (growH != height)
            growH += 2;
        if (growH > height)
            growH = height;
        if (++curWidth > dstLeft)
            curWidth = dstLeft;
        if (++curHeight > dstTop)
            curHeight = dstTop;
        if (dstLeft + width < right)
            right--;
        if (dstTop + height < bottom)
            bottom--;
        WaitFrameDelay();
    }
}

/* ovl0:1248: the sign of `value` as -1, 0 or 1. Used by the animations to turn a direction
   delta into a step. */
/*@ ovl0:1248 */ int SignOf(int value)
{
    if (value < 0)
        return -1;
    if (value > 0)
        return 1;
    return 0;
}
/* The KING/ADVENTUR scene tables ovl0:1265 reads: the x and width of each of the four king
   sprites, and the per-frame step of the adventurer's descent. */
extern int gKingX[4];                   /*@ ds:1CE4 */  /* Purpose unknown: x of each king sprite, ovl0:1265 */
extern int gKingW[4];                   /*@ ds:1CEC */  /* Purpose unknown: width of each king sprite, ovl0:1265 */
extern int gKingStep[8];                /*@ ds:1CF4 */  /* Purpose unknown: per-frame descent step, ovl0:1265 */

/* ovl0:1265: the KING/ADVENTUR scene. It loads KING.CMP and steps a framed rectangle open over
   ten frames, then loads ADVENTUR.CMP, cuts four king sprites out of gOverlayBuf at gKingX/
   gKingW and gives each a random start height y and animation phase. In a loop it walks each
   sprite down by gKingStep[phase] (clamped to 0x52..0xB6), drawing it and its frame, until all
   four have fallen past the bottom; sound 7 plays on some frames. It clears the box and frees
   the four sprites. */
/*@ ovl0:1265 */ void PlayKingScene(void)
{
    int inset;
    int width;
    int height;
    int y[4];
    int done;
    int anim[4];
    void far *imgs[4];
    register int i;
    register int edge;

    LoadPackedImage("KING.CMP", 5, 3);
    f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    inset = 0xf;
    edge = 0xe;
    width = height = 1;
    for (i = 0; i < 0xa; i++) {
        gFrameDelay = 1;
        ClipRect(inset, edge * 8, inset, edge * 8, width, height * 8, 4, 0);
        if (inset > 6)
            inset--;
        if (edge > 0)
            edge -= 2;
        width += 3;
        if (width + inset > 0x22)
            width = 0x22 - inset;
        height += 3;
        if (edge + height > 0x17)
            height = 0x17 - edge;
        WaitFrameDelay();
    }
    gFrameDelay = 0x19;
    LoadPackedImage("ADVENTUR.CMP", 5, 3);
    gDrawPage = 2;
    for (i = 0; i < 4; i++) {
        imgs[i] = gMakeImage(gKingX[i], 0, gKingW[i], 0x62, gOverlayBuf);
        y[i] = (RandomByte() & 3) * 8 + 0xb4;
        anim[i] = (i * 5) & 3;
    }
    gDrawPage = 0;
    WaitFrameDelay();
    ClipRect(0, 0, 0, 0, 0x28, 0xc8, 0, 4);
    done = 0;
    while (done == 0) {
        done = 1;
        gFrameDelay = 2;
        for (i = 0; i < 4; i++) {
            if (y[i] <= 0x52)
                continue;
            anim[i] = ++anim[i] & 7;
            y[i] += gKingStep[anim[i]];
            if (y[i] < 0x52)
                y[i] = 0x52;
            if (y[i] >= 0xb4) {
                done = 0;
                continue;
            }
            ClipRect(gKingX[i] + 8, y[i] - 2, 0, y[i] - 2, gKingW[i], 0xb6 - y[i], 4, 4);
            f1A29_000F(4, imgs[i], 0, y[i], 0);
            ClipRect(0, y[i] - 2, gKingX[i] + 8, y[i] - 2, gKingW[i], 0xb6 - y[i], 4, 0);
            done = 0;
        }
        if (!(RandomByte() & 3))
            PlaySound(7);
        WaitFrameDelay();
    }
    ClipRect(0, 0x60, 0, 0xa0, 0x28, 0x20, 6, 0);
    gFrameDelay = 0x46;
    for (i = 0; i < 4; i++)
        MemFree(imgs[i]);
}
/* ovl0:15F9: the HANDS scene. Three phases of hand animation, each a GrowFrame opening a framed
   box, a DelayFrames hold and a loop that slides two or three images cut from gOverlayBuf and
   wipes a ClipRect band. `tall` is the second image's moving height in the last phase. Every
   phase frees its images before the next. Sound 0xB/0xC/0xD/0xA play at the phase boundaries. */
/*@ ovl0:15F9 */ void PlayHandsScene(void)
{
    void far *sprites[3];
    register int i;
    register int tall;

    gDrawPage = 2;
    sprites[0] = gMakeImage(0, 0x8c, 0x15, 0x3c, gOverlayBuf);
    sprites[1] = gMakeImage(0x15, 0x8c, 0xc, 0x3c, gOverlayBuf);
    LoadPackedImage("HANDS.CMP", 3, 5);
    WaitFrameDelay();
    ClearBottomBar();
    gFillRect(0, 0, 0xbf, 0x3f, 0x9d);
    f1A29_000F(2, sprites[0], 0, 4, 0);
    f1A29_000F(2, sprites[1], 0x97, 4, 0);
    GrowFrame(0x19, 8, 0x12, 4, 3, 0, 0x15, 8, 6, 0, 0x1c, 0x17);
    ClipRect(0, 0x80, 0, 0xb0, 0x28, 0x10, 6, 0);
    DelayFrames(0xf);
    PlaySound(0xb);
    for (i = 0xffea; i <= 0x14; i += 4) {
        gFrameDelay = 1;
        gFillRect(0, 0, 0xa7, 0x3f, 0x9d);
        f1A29_000F(2, sprites[0], i, 4, 0);
        f1A29_000F(2, sprites[1], 0x7f - (i + 0x16), 4, 0);
        ClipRect(0, 0, 0x12, 0x20, 0x15, 0x40, 2, 0);
        WaitFrameDelay();
    }
    PlaySound(0xa);
    MemFree(sprites[1]);
    MemFree(sprites[0]);
    DelayFrames(0xf);
    gDrawPage = 4;
    sprites[0] = gMakeImage(0x11, 0, 0xb, 0x78, gOverlayBuf);
    sprites[1] = gMakeImage(0x1c, 0x70, 1, 0x1f, gOverlayBuf);
    sprites[2] = gMakeImage(9, 0x8a, 0xe, 0x36, gOverlayBuf);
    gDrawPage = 2;
    gFillRect(0, 0, 0x87, 0x3f, 0x9d);
    f1A29_000F(2, sprites[0], 0x20, 0xffb0, 0);
    f1A29_000F(2, sprites[1], 0x28, 0xfff0, 0);
    GrowFrame(0x12, 0x10, 0xa, 0xc, 0, 0, 0x11, 8, 0x11, 3, 0x19, 0xa);
    DelayFrames(0xf);
    for (i = 0xffb0; i <= 0; i += 4) {
        gFrameDelay = 1;
        gFillRect(0, 0, 0x87, 0x3f, 0x9d);
        f1A29_000F(2, sprites[0], 0x20, i, 0);
        f1A29_000F(2, sprites[1], 0x28, i + 0x40, 0);
        ClipRect(0, 0, 0xa, 0x60, 0x11, 0x40, 2, 0);
        WaitFrameDelay();
    }
    PlaySound(0xc);
    DelayFrames(5);
    for (i = 0; i > -0x36; i -= 4) {
        gFrameDelay = 1;
        gFillRect(0, 0, 0x87, 0x3f, 0x9d);
        f1A29_000F(2, sprites[2], 0xc, i + 0x40, 0);
        f1A29_000F(2, sprites[0], 0x20, i, 0);
        ClipRect(0, 0, 0xa, 0x60, 0x11, 0x40, 2, 0);
        WaitFrameDelay();
    }
    DelayFrames(0xf);
    MemFree(sprites[2]);
    MemFree(sprites[1]);
    MemFree(sprites[0]);
    gDrawPage = 4;
    sprites[0] = gMakeImage(0, 0, 0x11, 0x88, gOverlayBuf);
    sprites[1] = gMakeImage(0, 0x88, 9, 0x30, gOverlayBuf);
    gDrawPage = 2;
    gFillRect(0, 0, 0x8f, 0x5f, 0x9d);
    f1A29_000F(2, sprites[0], 0xffc8, 0xffc8, 0);
    f1A29_000F(2, sprites[1], 0x34, 0x31, 0);
    GrowFrame(9, 6, 0, 0, 0, 0, 0x12, 0xc, 8, 0xb, 0x15, 0xa);
    DelayFrames(0xf);
    PlaySound(0xb);
    for (i = 0xffc8; i <= -8; i += 4) {
        gFrameDelay = 1;
        gFillRect(0, 0, 0x8f, 0x5f, 0x9d);
        f1A29_000F(2, sprites[0], i, i, 0);
        f1A29_000F(2, sprites[1], (i == -8 ? 1 : 0) * 3 + 0x34, (i == -8 ? 1 : 0) * 3 + 0x31, 0);
        ClipRect(0, 0, 0, 0, 0x12, 0x60, 2, 0);
        WaitFrameDelay();
    }
    PlaySound(0xa);
    DelayFrames(0xf);
    MemFree(sprites[1]);
    MemFree(sprites[0]);
    DelayFrames(0xf);
    gDrawPage = 4;
    sprites[0] = gMakeImage(0x1c, 0, 0xb, 0x28, gOverlayBuf);
    sprites[1] = gMakeImage(0x1c, 0x28, 0xa, 0x48, gOverlayBuf);
    gDrawPage = 2;
    gFillRect(0, 0, 0x57, 0x70, 0x9d);
    f1A29_000F(2, sprites[1], 0, 0x5a, 0);
    GrowFrame(0x14, 0xd, 0xf, 6, 0, 0, 0xb, 0xe, 0, 0, 0x18, 0x10);
    DelayFrames(0xf);
    tall = 0x5a;
    for (i = 0xffd8; i <= 0; i += 4) {
        gFrameDelay = 1;
        gFillRect(0, 0, 0x57, 0x70, 0x9d);
        f1A29_000F(2, sprites[1], 0, tall, 0);
        ClipRect(0, 0, 0xf, 0x30, 0xb, 0x70, 2, 0);
        WaitFrameDelay();
        tall -= 5;
    }
    PlaySound(0xd);
    for (i = 0xffd8; i <= 0; i += 4) {
        gFrameDelay = 1;
        gFillRect(0, 0, 0x57, 0x27, 0x9d);
        f1A29_000F(2, sprites[0], 0, i, 0);
        ClipRect(0, 0, 0xf, 0x30, 0xb, 0x28, 2, 0);
        WaitFrameDelay();
    }
    MemFree(sprites[1]);
    MemFree(sprites[0]);
    gFrameDelay = 0x30;
}
/* ovl0:1DF3: the WTRDP3/ORB/TUNNEL/ZOOMTUNL scene. It cuts the 31 star tiles and three frames
   out of WTRDP3.CMP and slides a window (x from 0x8C up, y from 0x80 down) with the same star
   drop as ovl0:015D. Then it shows ORB.CMP through two GridFrame wipes, TUNNEL.CMP through a
   shrinking box and a reveal, and ends on the ZOOMTUNL.COL palette. */
/*@ ovl0:1DF3 */ void PlayTunnelScene(void)
{
    void far *img;
    int k;
    int x;
    int y;
    int left;
    int top;
    void far *stars[31];
    void far *frames[3];
    register int i;
    register int frame;

    gDrawPage = 2;
    LoadPackedImage("WTRDP3.CMP", 5, 3);
    for (i = 0; i < 31; i++)
        stars[i] = gMakeImage(gTileX[i], gTileY[i] * 8 + 0x88, gTileScale[i], gTileScale[i] * 8, gOverlayBuf);
    for (i = 0; i < 3; i++)
        frames[i] = gMakeImage(i * 5 + 0xf, 0x98, 5, 0x20, gOverlayBuf);
    img = gMakeImage(0x1f, 0x88, 5, 0x20, gOverlayBuf);
    f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    ClipRect(0, 0, 0, 0x88, 6, 0x24, 4, 4);
    WaitFrameDelay();
    ClearBottomBar();
    ClipRect(0x14, 0, 0xa, 0x18, 0x14, 0x88, 4, 0);
    x = 0x8c;
    y = 0x80;
    WaitFrameDelay();
    for (i = 0; i <= 0x46; i++) {
        gFrameDelay = 3;
        left = (x - 2) >> 3;
        if (left < 0x14)
            left = 0x14;
        top = y - 2;
        if (top > 0x62)
            top = 0x62;
        ClipRect(left, top, 0, 0, 6, 0x24, 4, 4);
        frame = i & 3;
        if (frame == 3)
            frame = 1;
        f1A29_000F(4, frames[frame], x, y, 0);
        f1A29_000F(4, img, 0xa0, 0x68, 0);
        ClipRect(left, top, left - 0xa, top + 0x18, 6, 0x24, 4, 0);
        ClipRect(0, 0, left, top, 6, 0x24, 4, 4);
        x++;
        y--;
        for (k = 0; k < 5; k++) {
            frame = RandomByte() % 31;
            f1A29_000F(0, stars[frame], gStarX[frame] - 0x50, gStarY[frame] + 0x18, 0);
        }
        if (!(RandomByte() & 7))
            PlaySound((RandomByte() & 1) ? 5 : 0xe);
        WaitFrameDelay();
    }
    gDrawPage = 0;
    for (i = 0; i < 31; i++) {
        if (i < 3)
            MemFree(frames[i]);
        MemFree(stars[i]);
    }
    MemFree(img);
    ClearBottomBar();
    gFrameDelay = 0x14;
    ClipRect(0, 0x88, 0, 0, 6, 0x24, 4, 4);
    LoadPalette("ORB.COL");
    LoadPackedImage("ORB.CMP", 3, 5);
    f19EC_002A(5, 2, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    gFrameDelay = 6;
    DrawGrid();
    WaitFrameDelay();
    ClipRect(0, 0x90, 0, 0xb8, 0x28, 0x10, 6, 0);
    x = top = 0;
    y = 0x88;
    for (i = 0; i < 0x13; i++) {
        gFrameDelay = 1;
        ClipRect(x, y, 0xa, top + 0x10, 0x14, 8, 2, 0);
        y += 8;
        top += 8;
        if (i == 6) {
            x = 0x14;
            y = 0;
        }
        WaitFrameDelay();
    }
    PlaySound(3);
    gFrameDelay = 0x3c;
    for (i = 0; i < 0x38; i++) {
        ClipRect(0, i + 0x88, 0xa, 0x10, 0x14, 0x38 - i, 2, 0);
        ClipRect(0x14, 0, 0xa, 0x48 - i, 0x14, 0x98 - (0x38 - i), 2, 0);
        WaitFrameDelay();
        gFrameDelay = 1;
    }
    for (i = 1; i <= 0x30; i++) {
        gFrameDelay = 1;
        ClipRect(0x14, i, 0xa, 0x10, 0x14, 0x98, 2, 0);
        WaitFrameDelay();
    }
    gFrameDelay = 0x1e;
    LoadPackedImage("TUNNEL.CMP", 3, 5);
    f19EC_002A(5, 2, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    WaitFrameDelay();
    gDrawPage = 0;
    gFillRect(0, 0x10, 0x13f, 0x1f, 0xc);
    gFillRect(0, 0x88, 0x13f, 0xc7, 0xc);
    ClipRect(0, 0, 0xa, 0x20, 0x14, 0x78, 2, 0);
    gFrameDelay = 0x32;
    LoadPalette("ZOOMTUNL.COL");
}
/* The six-entry x/y/width/height tables ovl0:23A3 reads for the ZOOMTUNL rectangles. */
extern unsigned char gZoomX1[6];        /*@ ds:1D04 */  /* Purpose unknown: x1 of each zoom rect, ovl0:23A3 */
extern unsigned char gZoomY1[6];        /*@ ds:1D0A */  /* Purpose unknown: y1 of each zoom rect, ovl0:23A3 */
extern unsigned char gZoomW[6];         /*@ ds:1D10 */  /* Purpose unknown: width of each zoom rect, ovl0:23A3 */
extern unsigned char gZoomH[6];         /*@ ds:1D16 */  /* Purpose unknown: height of each zoom rect, ovl0:23A3 */
extern unsigned char gZoomX2[6];        /*@ ds:1D1C */  /* Purpose unknown: x2 of each zoom rect, ovl0:23A3 */
extern unsigned char gZoomY2[6];        /*@ ds:1D22 */  /* Purpose unknown: y2 of each zoom rect, ovl0:23A3 */
extern void f1A29_004D(int mode, void far *src, int x, int y, int region); /*@ 0998:0046 */

/* ovl0:23A3: the AVALANCH/ZOOMTUNL scene. It cuts two 0x14 by 0x78 panels out of gOverlayBuf,
   flashes them three times, then wipes the screen with a growing box and a strip reveal before
   loading AVALANCH.CMP and ZOOMTUNL.CMP, drawing the six ZOOMTUNL rectangles and closing the
   bottom bar. Sound 8/9/6 play at the phase boundaries. */
/*@ ovl0:23A3 */ void PlayAvalancheScene(void)
{
    void far *imgA;
    void far *imgB;
    register int i;
    register int j;

    gDrawPage = 4;
    imgA = gMakeImage(0x14, 0, 0x14, 0x78, gOverlayBuf);
    imgB = gMakeImage(0, 0, 0x14, 0x78, gOverlayBuf);
    f1A29_004D(4, imgA, 0xa0, 0, 0);
    f1A29_004D(4, imgB, 0, 0, 0);
    WaitFrameDelay();
    for (i = 0; i < 3; i++) {
        gFrameDelay = 8;
        ClipRect(0, 0, 0xa, 0x20, 0x14, 0x78, 4, 0);
        PlaySound(7);
        WaitFrameDelay();
        gFrameDelay = 8;
        ClipRect(0, 0, 0xa, 0x20, 0x14, 0x78, 2, 0);
        PlaySound(7);
        WaitFrameDelay();
    }
    ClipRect(0, 0xa0, 0, 0xb8, 0x28, 0x10, 6, 0);
    DelayFrames(0x12);
    gFrameDelay = 5;
    ClipRect(0x14, 0, 0xa, 0x20, 0x14, 0x78, 2, 0);
    WaitFrameDelay();
    gFrameDelay = 5;
    ClipRect(0, 0x7a, 0xa, 0x20, 0x14, 0x3c, 2, 0);
    ClipRect(0x14, 0x7a, 0xa, 0x5c, 0x14, 0x3c, 2, 0);
    WaitFrameDelay();
    gFrameDelay = 5;
    ClipRect(0x14, 0, 0xa, 0x20, 0x14, 0x78, 4, 0);
    for (i = 0; i < 6; i++)
        ClipRect(i * 6, 0xb9, 7, i * 8 + 0x18, 6, 8, 2, 2);
    MemFree(imgB);
    MemFree(imgA);
    WaitFrameDelay();
    gFrameDelay = 0x28;
    ClipRect(0, 0, 0xa, 0x20, 0x14, 0x78, 2, 0);
    LoadPackedImage("AVALANCH.CMP", 5, 3);
    f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    WaitFrameDelay();
    gFrameDelay = 0x10;
    ClipRect(0x21, 0, 0x11, 0x38, 6, 0x30, 4, 0);
    PlaySound(8);
    ClipRect(0, 0, 0, 0, 0x28, 0xb8, 0, 2);
    WaitFrameDelay();
    PlaySound(4);
    for (i = 0; i < 0x1e; i++) {
        WaitFrameDelay();
        if (i == 0)
            ClearBottomBar();
        gFrameDelay = 1;
        ClipRect(0xa, (RandomByte() & 7) + 0x19, 0xa, 0x18, 0x14, 0x90, 2, 0);
    }
    PlaySound(9);
    for (i = 0; i < 6; i++) {
        gFrameDelay = 1;
        ClipRect(gZoomX1[i], gZoomY1[i], gZoomX2[i], gZoomY2[i], gZoomW[i], gZoomH[i], 4, 2);
        for (j = 0; j < 4; j++) {
            WaitFrameDelay();
            gFrameDelay = 1;
            ClipRect(0xa, (RandomByte() & 7) + 0x19, 0xa, 0x18, 0x14, 0x90, 2, 0);
        }
    }
    ClipRect(0, 0, 0, 0, 0x28, 0xa8, 2, 0);
    gFrameDelay = 0x28;
    LoadPackedImage("ZOOMTUNL.CMP", 5, 3);
    f19EC_002A(3, 4, FP_OFF(gOverlayBuf), FP_SEG(gOverlayBuf));
    WaitFrameDelay();
    PlaySound(6);
    gFrameDelay = 2;
    ClipRect(0, 0, 0xa, 0x20, 0x14, 0x78, 4, 0);
    WaitFrameDelay();
    gFrameDelay = 2;
    ClipRect(0x14, 0, 0xa, 0x20, 0x14, 0x78, 4, 0);
    WaitFrameDelay();
    ClipRect(0, 0x78, 0xa, 0x1e, 0x14, 0x40, 4, 0);
    ClipRect(0x14, 0x78, 0xa, 0x5e, 0x14, 0x40, 4, 0);
    gDrawPage = 0;
    ClipRect(0, 0xb0, 0, 0xb8, 0x28, 0x10, 6, 0);
    DelayFrames(0x32);
}
/* ovl0:2913: reveals the screen in horizontal strips. For each step 0..6 it walks the 25
   eight-line strips top to bottom and clips each strip twice: the whole strip (height
   7 - step, so the bottom part of it) and its single moving line at si*8 + 7 - step; a frame
   is waited after each step, and the screen is finally unclipped (the whole 40 by 200 region
   again). No caller of this function was found in INTRO.EXE (make packet). */
/*@ ovl0:2913 */ void RevealInStrips(void)
{
    int step;
    int strip;

    for (step = 0; step < 7; step++) {
        gFrameDelay = 1;
        for (strip = 0; strip < 25; strip++) {
            ClipRect(0, strip * 8 + 1, 0, strip * 8, 40, 7 - step, 0, 0);
            ClipRect(0, strip * 8 + 7 - step, 0, strip * 8 + 7 - step, 40, 1, 2, 0);
        }
        WaitFrameDelay();
    }
    ClipRect(0, 0, 0, 0, 40, 200, 2, 0);
}

/* ovl0:29C9: draws a grid over the whole screen in colour 12. In each of two passes, the odd
   lines of the eight-line band are vertical lines every 8 pixels starting at x = line + pass,
   the even ones horizontal lines starting at y = line + pass; the second pass fills the pixels
   the first left out. Called by the title scene (ovl0:015D), the TOWRMAGE scene (ovl0:0B20,
   twice) and ovl0:23A3. */
/*@ ovl0:29C9 */ void DrawGrid(void)
{
    int line;
    int pass;
    int x;
    int y;

    for (pass = 0; pass < 2; pass++)
        for (line = 0; line < 8; line++) {
            if (line & 1) {
                x = line + pass;
                while (x < 320) {
                    gDrawLine(x, 0, x, 199, 12);
                    x += 8;
                }
            } else {
                y = line + pass;
                while (y < 200) {
                    gDrawLine(0, y, 319, y, 12);
                    y += 8;
                }
            }
        }
}
/* ovl0:2A4E: reads a 768-byte .COL palette file into gPaletteBuf and applies it, only on the VGA
   mode (gGraphicsChoice 3). The first colour (black) is cleared and one frame is waited before
   the driver is given the palette, so the screen blanks first. */
/*@ ovl0:2A4E */ void LoadPalette(char far *name)
{
    if (gGraphicsChoice == 3) {
        LoadFile(name, gPaletteBuf, 0x300);
        gPaletteBuf[0] = gPaletteBuf[1] = gPaletteBuf[2] = 0;
        WaitFrame();
        gSetPalette(gPaletteBuf);
    }
}
/* ovl0:2A99: plays the Westwood logo card: loads WESTWOOD.CMP into the buffer slots 5/3,
   starts the driver, applies the WESTWOOD.COL palette, copies the buffers through
   f19EC_002A, makes sound 1 and shows the card for gWestwoodDelay ticks. Then, unless a key
   aborted, it shows the four SSI logo cards in turn (each from gLogoFiles into slots 5/3,
   held for its gLogoDelays entry and dissolved from page 4 to page 0), and leaves a 50-tick
   delay set. */
/*@ ovl0:2A99 */ void PlayLogoSequence(void)
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
/* ovl0:2B7F: loads a packed .CMP image into the buffer slot `dstKind`: slot `srcKind` is the
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
/* ovl0:2C0D: copies the rectangle (x, y, width, height) pixel by pixel from the page
   `fromPage` to the page `toPage`, in a random order: two permutation tables (gOrderX for the
   columns at ds:290C, gOrderY for the rows at ds:2B8C) are filled 0,1,2,... and then shuffled
   with RandomRange; the copy walks the columns in order and the shuffled rows cyclically, so
   the pixels appear to dissolve in. gDrawPage selects the read page for gGetPixel and the
   write page for gPutPixel and is restored at the end. Called by the logo sequence
   (ovl0:2A99). `value` is the swap temporary of the two shuffles and the pixel colour of the
   copy; `pos` is the shuffled table entry and then the x coordinate. */
/*@ ovl0:2C0D */ void Dissolve(int x, int y, unsigned width, unsigned height, int fromPage, int toPage)
{
    int value;
    int row;
    int pos;
    int i;
    int savedPage;
    int k;
    int col;

    for (col = 0; col < width; col++)
        gOrderX[col] = col;
    for (col = 0; col < height; col++)
        gOrderY[col] = col;
    for (col = 0; col < width; col++) {
        pos = RandomRange(0, width - 1);
        i = col;
        value = gOrderX[pos];
        gOrderX[pos] = gOrderX[i];
        gOrderX[i] = value;
    }
    for (col = 0; col < height; col++) {
        pos = RandomRange(0, height - 1);
        i = col;
        value = gOrderY[pos];
        gOrderY[pos] = gOrderY[i];
        gOrderY[i] = value;
    }
    savedPage = gDrawPage;
    for (row = 0; row < height; row++) {
        k = row;
        for (col = 0; col < width; col++) {
            pos = x + gOrderX[col];
            i = y + gOrderY[k];
            k++;
            if (k >= height)
                k = 0;
            gDrawPage = fromPage;
            value = gGetPixel(pos, i);
            gDrawPage = toPage;
            gPutPixel(pos, i, value);
        }
    }
    gDrawPage = savedPage;
}

/* ovl0:2D4E: plays the game sound numbered `sound` (the argument is an index, not a driver
   number). The driver number comes from gSoundBuf (a byte; 0xFF means silence). gSoundChoice
   selects the path: 1 (AdLib) queues the driver number and returns, 2 (PC speaker) and 3
   (Tandy) start the speaker routine the low seven bits select through GetSoundData; an
   unknown card does nothing. On the Tandy path a nonzero driver number resumes the driver
   only while gTandyOn is clear. This is the EOB PlaySound (src/E177D.C:166) with a one-based
   gSoundChoice, without the sound-on test and the 0x96 map cutoff.
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
    mapByte = gSoundBuf[driverSound];
    if (mapByte == 0xFF)
        mapByte = 0;
    driverSound = mapByte;
    switch (gSoundChoice) {
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