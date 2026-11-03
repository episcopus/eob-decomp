/* Unit at START1.EXE 0000:0239 (827 bytes, 6 functions): the setup program Westwood's launcher runs
   before INTRO.EXE. Its first function is the graphics, sound and mouse menu; the rest are a string
   printer and the video-hardware probes it calls. Small model (-ms), so every callee is a near call.
   The library routines it reaches are declared with their load-module addresses. */

/*@flags -ms -O -Z */

extern void putch(char c); /*@ 0000:05F6 */

/*@ 0000:0239 stub */ void f0000_0239(void) {}
/*@ 0000:0474 */ void PrintString(char *s)
{
    int i = 0;

    while (s[i] != 0)
        putch(s[i++]);
}
/*@ 0000:0496 stub */ void f0000_0496(void) {}
/*@ 0000:04A3 stub */ void f0000_04A3(void) {}
/*@ 0000:04FC stub */ void f0000_04FC(void) {}
/*@ 0000:052D stub */ void f0000_052D(void) {}
