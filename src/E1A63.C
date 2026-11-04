/* Unit at EOB.EXE 1A63:0004 (42 bytes, 2 functions): install the DOS hard error (int 24h)
   handler, and the handler itself, which records the error code and returns to the program.
   Compiled with -G (optimise for speed: `inc sp; inc sp` after the call), unlike the -O -Z
   spike units. */

/*@flags -ml -O -Z -G */

extern int gHardErr; /*@ ds:FE5E */
extern void harderr(void (*handler)(int)); /*@ 0000:604C */
extern void hardretn(int retn); /*@ 0000:608A */

void HardErrHandler(int code);

/* 1A63:0004: the DOS critical error (int 24h) handler: keeps the error code in gHardErr and
   returns to the program (hardretn(0)) instead of DOS's abort, retry, fail prompt. */
/*@ 1A63:0004 */ void HardErrHandler(int code)
{
    gHardErr = code;
    hardretn(0);
}

/* 1A63:0019: installs HardErrHandler as the critical error handler through the runtime's
   harderr. */
/*@ 1A63:0019 */ void InstallHardErr(void)
{
    harderr(HardErrHandler);
}
