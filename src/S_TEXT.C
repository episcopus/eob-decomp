/* Unit at START1.EXE 0000:0239 (1097 bytes, 15 functions in two modules): the setup program that
   runs before INTRO.EXE. Main (0239) is the graphics, sound and mouse menu, ending in an execl of
   INTRO.EXE; PrintString (0474) writes a NUL-terminated string a character at a time. The video
   probes (0496 to 05F5) and the console and mouse routines (05F6 to 0681) are assembly, in
   src/S_TEXT.ASM. Small model (-ms): every callee is a near call. execl (0000:0837) is the
   library's _execl, the name build/re/rtl_start1.json gives the library entry there. */

/*@flags -ms */

/* the console and mouse routines of this unit's assembly module (src/S_TEXT.ASM), near calls */
extern void PutChar(char c); /*@ 0000:05F6 */   /* prints one character: Main at :91, :117, :127, PrintString at :153 */
extern int GetKey(void); /*@ 0000:0608 */        /* reads one key: the menu loop at :90, :101, :116, :126 */
extern int DetectMouse(void); /*@ 0000:0642 */   /* true when a mouse driver answers; gates the prompt at :122 */

/* the C library's exec, reached by near call */
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
extern int gSelection; /*@ ds:052E */           /* the key the menu loop read (:90), then the zero-based menu index (:92, gSelection -= '1'), then 'N' as the restart flag (:104) */
extern int gSound; /*@ ds:0530 */               /* the sound choice */
extern int gMouse; /*@ ds:0532 */               /* the mouse choice */
extern char gMode; /*@ ds:0534 */               /* index 0 to 6 into gModeMaskTable (:78), from gDetectedMode or 6 when the byte at F000:C000 is 21h */
extern char gMaskBit; /*@ ds:0535 */            /* the gModeMask bit of the menu line being printed */
extern char gDetectedMode; /*@ ds:0536 */       /* first byte of the four-byte video info record; InitVideo zeroes both words (S_TEXT.ASM:145) and NormalizeVideoInfo reads the second (S_TEXT.ASM:109) */

extern char gBanner[]; /*@ ds:0113 */      /* leading CRLFs and "Eye of the Beholder Version 1.9" then "Which graphics mode do you wish to use?" */
extern char gStar[]; /*@ ds:016E */        /* "*", the mark next to the menu line of a detected mode */
extern char gSpace[]; /*@ ds:0170 */       /* " ", the mark next to an undetected mode */
extern char gNote[]; /*@ ds:0172 */        /* "NOTE - * denotes a mode detected in your machine." */
extern char gModePrompt[]; /*@ ds:01A6 */  /* "Mode?" */
extern char gWarningHead[]; /*@ ds:01AE */ /* "WARNING! - " before the rejected mode's name */
extern char gWarningTail[]; /*@ ds:01BD */ /* " mode has not been detected in your machine." */
extern char gSurePrompt[]; /*@ ds:01EC */  /* "Are you SURE that you want to use that mode? (Y/N)" */
extern char gSoundPrompt[]; /*@ ds:021F */ /* the sound menu (Adlib, speaker, Tandy, none) and "Sound?" */
extern char gMousePrompt[]; /*@ ds:02AA */ /* "A mouse has been detected on your system. Do you wish to use your mouse?" */
extern char gIntroPath[]; /*@ ds:02FA */  /* "INTRO.EXE", execl's path */
extern char gIntroName[]; /*@ ds:0304 */  /* "INTRO.EXE", execl's argv[0] */
extern char gHomie[]; /*@ ds:030E */       /* "homie", execl's fifth argument; INTRO.EXE's main strcmp's it (whitepaper 2.6) */

void PrintString(char *text);

/* 0000:0239: the setup menu: initialise the video info, then loop over the graphics menu, the sound
   menu and the mouse prompt, and execl INTRO.EXE with the three choices as one-character arguments.
   The graphics and mouse arguments are "0" plus the choice (:112, :134); the sound argument is the
   prompt's own key '1' to '4' (:120). The loop restarts on a "no" answer. The loop exits through the `done` label
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
        /* Purpose unknown: the byte at F000:C000 is unidentified (whitepaper 2.4); when it is 21h the
           menu uses table entry 6 (ds:00AF = 03h, the CGA and Tandy mask) */
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

/* 0000:0474: write the NUL-terminated string at text to the console, one character at a time
   through PutChar (0000:05F6). The index i stays across the PutChar call, so the loop reads and
   advances it one character at a time. */
/*@ 0000:0474 */ void PrintString(char *text)
{
    int i = 0;

    while (text[i] != 0)
        PutChar(text[i++]);
}
