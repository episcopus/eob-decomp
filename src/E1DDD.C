/* Unit at EOB.EXE 1DDD:0006 (106 bytes, 2 functions). Two INT 10h video BIOS calls through
   the runtime int86 at 0000:60BD. That routine is Borland's int86(intno, inregs, outregs): it
   saves the segment registers with segread (0000:66BB) and then runs 0000:60EF, which builds
   an "int N" stub on its own stack (0xCD at [bp-0xd], N = the low byte of its first argument
   at 0000:6109) and lcalls it (0000:614B); the 25h/26h test at 0000:6114 is the absolute disk
   read/write special case. The unit passes ds:FF46 as the in register union and ds:FF36 as the
   out one; each is the .x.ax word of a 16 byte union REGS [H].
   f1DDD_0006 is INT 10h AH=0Fh (get current video mode): it puts 0F00h in the in union
   (AH=0Fh), calls int86(0x10, &gFF46, &gFF36), and returns the low byte (AL) of the out AX.
   f1DDD_002D is INT 10h AH=00h (set video mode) with AL = the mode: for an index below 7 it
   takes the mode from the 7 word table at ds:84A2 (4, 9, 0Dh, 13h, 13h, 7, 2), otherwise the
   word at ds:84A0 (argv[5]; 07C8:0074 calls 0000:5D63 with it and 07C8:007B stores only AX,
   so 5D63 is atoi-like with an int result, not atol [H]; the video mode restored on exit).
   It puts the mode in the in union and calls int86 only when g84A2[i] differs from ds:84A0.
   The 8 far call sites in the image push either 7 (07C8:01C4, 0265, 0524, 05E8, 0628 and
   19BC:02B9), the restore case: i = 7 takes the else branch (gFF46 = g84A0) and the
   comparison reads one word past the 7 entry table, g84A2[7] = ds:84B0, which is 0 in the
   image, so the call happens only when ds:84A0 is nonzero; or the word ds:8102 (07C8:01E4
   and 022E, the graphics mode from argv[1][0] minus '1', 0..6 [H]), the table case: i < 7,
   so the mode comes from g84A2[i] and the call is skipped when that already equals the
   restore mode ds:84A0.
   Placeholder names; the runtime int86 is not in build/re/rtl_compare.json. The 6 bytes at
   1DDD:0000..0005 before the first function (pop dx, cx, bx, ax; pop bp; retf) are code, not
   padding, and are outside this unit. */

/*@flags -ml -O -Z */

extern void f60BD(int intno, void far *inregs, void far *outregs); /*@ 0000:60BD */
extern unsigned g84A0; /*@ ds:84A0 */
extern unsigned g84A2[]; /*@ ds:84A2 */
extern unsigned gFF46; /*@ ds:FF46 */
extern unsigned gFF36; /*@ ds:FF36 */

/* int86 returns the output AX; declared void here because both callers ignore it. */

/*@ 1DDD:0006 */ int f1DDD_0006(void)
{
    gFF46 = 0xF00;
    f60BD(0x10, &gFF46, &gFF36);
    return gFF36 & 0xff;
}

/*@ 1DDD:002D */ void f1DDD_002D(int i)
{
    if (i < 7)
        gFF46 = g84A2[i];
    else
        gFF46 = g84A0;
    if (g84A2[i] != g84A0)
        f60BD(0x10, &gFF46, &gFF36);
}
