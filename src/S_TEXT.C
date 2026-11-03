/* Unit at START1.EXE 0000:0239 (827 bytes, 6 functions): the setup program that runs before
   INTRO.EXE. Main is the graphics, sound and mouse menu, ending in an execl of INTRO.EXE; PrintString
   writes a NUL-terminated string a character at a time. The rest probe the video hardware. Small
   model (-ms): every callee is a near call, and the library routines are named by their address. */

/*@flags -ms */

/* library routines of START1.EXE's load module, reached by near call */
extern void PutChar(char c); /*@ 0000:05F6 */
extern int GetKey(void); /*@ 0000:0608 */
extern int DetectMouse(void); /*@ 0000:0642 */
extern int execl(char *path, char *arg0, ...); /*@ 0000:0837 */

/* the video-hardware probes (src/S_TEXT.ASM): fills the four-byte info record at gDetectedMode */
extern void InitVideo(char *info); /*@ 0000:052D */

/* this unit's data, in START1.EXE's DGROUP */
extern char gModeMask; /*@ ds:00A8 */           /* bit 1 << n set when graphics mode n is available */
extern char gModeMaskTable[]; /*@ ds:00A9 */    /* per detected mode, its gModeMask */
extern char gArgGraphics[]; /*@ ds:00B0 */      /* execl's one-character graphics argument */
extern char gArgSound[]; /*@ ds:00B2 */         /* execl's one-character sound argument */
extern char gArgMouse[]; /*@ ds:00B4 */         /* execl's one-character mouse argument */
extern char *gMenuText[]; /*@ ds:00B6 */        /* the five mode-menu lines */
extern char *gModeName[]; /*@ ds:00C0 */        /* the four mode names */
extern int gMenuIndex; /*@ ds:052C */           /* the menu line being printed */
extern int gSelection; /*@ ds:052E */           /* the key the menu loop last read */
extern int gSound; /*@ ds:0530 */               /* the sound choice */
extern int gMouse; /*@ ds:0532 */               /* the mouse choice */
extern char gMode; /*@ ds:0534 */               /* the graphics mode the menu works on */
extern char gMaskBit; /*@ ds:0535 */            /* the gModeMask bit of the menu line being printed */
extern char gDetectedMode; /*@ ds:0536 */       /* the mode the video probes detected */

extern char gBanner[]; /*@ ds:0113 */
extern char gStar[]; /*@ ds:016E */
extern char gSpace[]; /*@ ds:0170 */
extern char gNote[]; /*@ ds:0172 */
extern char gModePrompt[]; /*@ ds:01A6 */
extern char gWarningHead[]; /*@ ds:01AE */
extern char gWarningTail[]; /*@ ds:01BD */
extern char gSurePrompt[]; /*@ ds:01EC */
extern char gSoundPrompt[]; /*@ ds:021F */
extern char gMousePrompt[]; /*@ ds:02AA */
extern char gIntroPath[]; /*@ ds:02FA */
extern char gIntroName[]; /*@ ds:0304 */
extern char gHomie[]; /*@ ds:030E */

void PrintString(char *text);

/* 0000:0239: the setup menu: initialise the video info, then loop over the graphics menu, the sound
   menu and the mouse prompt, and execl INTRO.EXE with the three choices as one-character arguments
   ("0" + the choice). The loop restarts on a "no" answer. The loop exits through the `done` label
   inside it, not a `break`, and after the `return` a dead `continue` is kept: TCC lays the `done`
   return (0000:046C) between the loop body's end jump (0000:046A) and the back edge (0000:046E),
   and without the dead `continue` the body's end jump folds into the back edge and the tail is two
   bytes short (make match, 2026-11-02). */
/*@ 0000:0239 */ void Main(void)
{
    int answer;

    InitVideo(&gDetectedMode);
    for (;;) {
restart:
        gSelection = 0;
        gMaskBit = 1;
        PrintString(gBanner);
        gMode = gDetectedMode;
        if (gMode > 5)
            gMode = 0;
        if (*(char far *)0xF000C000 == 0x21)
            gMode = 6;
        gModeMask = gModeMaskTable[gMode];
        for (gMenuIndex = 0; gMenuIndex < 5; gMenuIndex++) {
            if (gModeMask & gMaskBit)
                PrintString(gStar);
            else
                PrintString(gSpace);
            PrintString(gMenuText[gMenuIndex]);
            gMaskBit <<= 1;
        }
        PrintString(gNote);
        PrintString(gModePrompt);
        while (gSelection < '1' || gSelection > '5')
            gSelection = GetKey();
        PutChar(gSelection);
        gSelection -= '1';
        if (gSelection < 4) {
            answer = 0;
            if (!(gModeMask & (1 << gSelection))) {
                PrintString(gWarningHead);
                PrintString(gModeName[gSelection]);
                PrintString(gWarningTail);
                PrintString(gSurePrompt);
                while (answer != 'Y' && answer != 'y' && answer != 'n' && answer != 'N')
                    answer = GetKey();
            }
            if (answer == 'N' || answer == 'n')
                gSelection = 'N';
        }
        if (gSelection == 'N')
            goto restart;
        if (gSelection == 'n')
            goto restart;
        if (gSelection >= 4)
            goto done;
        gArgGraphics[0] = gSelection + '0';
        PrintString(gSoundPrompt);
        gSound = 0;
        while (gSound < '1' || gSound > '4')
            gSound = GetKey();
        PutChar(gSound);
        PutChar('\n');
        PutChar('\r');
        gArgSound[0] = gSound;
        gArgMouse[0] = '0';
        if (DetectMouse()) {
            PrintString(gMousePrompt);
            gMouse = 0;
            while (gMouse != 'y' && gMouse != 'Y' && gMouse != 'n' && gMouse != 'N')
                gMouse = GetKey();
            PutChar(gMouse);
            PutChar('\n');
            PutChar('\r');
            if (gMouse == 'y' || gMouse == 'Y')
                gMouse = 1;
            else
                gMouse = 0;
            gArgMouse[0] = gMouse + '0';
        }
        execl(gIntroPath, gIntroName, gArgGraphics, gArgSound, gArgMouse, gHomie, 0);
        return;
        /* QUIRK 0000:046A: dead (`return` above), but the loop tail's layout needs its jump. */
        continue;
done:
        return;
    }
}

/* 0000:0474: write the NUL-terminated string at s to the console, one character at a time through
   the library's PutChar (0000:05F6). No frame variable survives the loop, only the index i. */
/*@ 0000:0474 */ void PrintString(char *text)
{
    int i = 0;

    while (text[i] != 0)
        PutChar(text[i++]);
}
