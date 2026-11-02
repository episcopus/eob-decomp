/* Unit at EOB.EXE 19F3:000E (137 bytes, 1 function): adjust a rectangle request before
   passing it to the far routine behind the function pointer at ds:8052. The eight ints are
   two origins (x1, y1 and x2, y2), a width, a height and two extras that pass through
   untouched. The width is read as the extent from x1 to the far edge [H]: when an origin is
   raised to 0, the width moves by that origin's amount (19F3:0036 for x1, 19F3:004B for x2),
   so x1 + width, the far edge, stays where it was; the height moves the same way with
   whichever y origin is raised (19F3:005C, 19F3:0070). No code states that reading; it is an
   inference [H] from the caps and region record 0, whose units the screen-limits block below
   argues. The callers support x in columns (E1DE7.C:245 passes gTextLeft as x1 and
   x2 and gTextWidth unshifted to ClipRect, with y2 = gTextTop unshifted and y1 = gTextTop +
   gFontHeight1A47, one line down, and the height `line` derived from gTextHeight at
   E1DE7.C:244, while EOV6.C:376 and :481 pass gTextHeight itself; the same
   variables are multiplied by 8 or shifted by 3 for the pixel arguments of gFillRect, gDrawText
   and f0D1C_0C21 there). All the compares are signed. The width and height are capped at
   SCREEN_COLUMNS and SCREEN_LINES when larger, as plain size caps not tied to any origin;
   nothing clips the far edge x1 + width (x1 = 30, width = 40 passes unclipped). x1 and x2 are
   rejected, the function returns without calling, when they reach SCREEN_COLUMNS. The
   negative-x1 shift runs first: it adds -x1 to x2 (x2 -= x1), adds x1 to the width and clears
   x1. The x1 test follows that shift, so it sees x1 already at 0 when x1 was negative (a
   negative x1 never makes it return) but catches a positive x1 at or past SCREEN_COLUMNS
   columns. The x2 test comes next and sees x2 after the negative-x1 shift has moved it (x2 has
   had -x1 added), so a clipped x1 can push x2 to SCREEN_COLUMNS and make the function return.
   QUIRK 19F3:0045: the negative-x2 shift comes after both x tests (19F3:003B cmp di,28h,
   19F3:0040 cmp si,28h) and rewrites x1 (di) with no later test reading it, so an origin past
   SCREEN_COLUMNS can still reach the far routine: ClipRect(0, 0, -50, 0, 40, 200, 0, 0)
   moves x1 by -x2 (= 50), passes x1 = 50, at or past SCREEN_COLUMNS, after the last x
   test, and (19F3:004B) also passes width = 40 + (-50) = -10, a negative size. The
   negative-origin shifts add the origin to the size (19F3:0036 and 19F3:004B add a negative
   x to the width, 19F3:005C and 19F3:0070 add a negative y to the height) with no size test
   after the shifts, so ClipRect(-30, 0, 0, 0, 20, 200, 0, 0) passes x2 = 30 and width = -10:
   the far edge x1 + width = -10, below column 0, and the negative width reaches the far
   routine unchanged, while the second origin's extent x2 + width = 20 stays inside the screen.
   QUIRK 19F3:0036/004B/005C/0070: a
   negative size reaches the far routine unchanged; what the far routine does with it is
   unknown.
   y1 and y2 have no upper test and nothing is capped on that side beyond the height.
   Each of x1, y1, x2, y2 is raised to 0 when negative, and that move shifts its partner on
   the same axis by the same amount but the size by the opposite amount: x1 moves x2 by -x1
   and the width by x1, x2 moves x1 by -x2 and the width by x2, y1 moves y2 by -y1 and the
   height by y1, y2 moves y1 by -y2 and the height by y2. The far edge x1 + width (and
   y1 + height) is unchanged by the shift, the reason the far-edge sentence above holds.
   The eight ints are then passed on, pushed last argument first, so the bytes push extra2,
   extra1, height, width, y2, x2, y1, x1. extra1 and extra2 are passed through untouched;
   nothing in this unit reads them and their roles are unknown, and the callers do not settle
   them either: E1DE7.C:245 passes g8115 for both extras, whose role E1DE7.C:69 says is not
   established. One lead [H]: E082C.C:280 and :285 pass extra1 equal to the value given as
   19EC:002A's second argument in both LoadScreenImage cases (4 with extras 4,2; 2 with extras
   2,0), which hints extra1 is a driver kind, but the other call sites (E082C.C:113 with 0,2;
   E082C.C:209 with 2,0; E1DE7.C:245 with g8115 twice) do not fit that. The five call sites
   below push the eight ints (add sp,10h); they are the 082C
   and 0EF3 call sites this header checked, not every caller, since the packet lists 30
   callers across the resident code and overlays ovl0 to ovl6. Calls (x1, y1, x2, y2, width,
   height, extra1, extra2):
     082C:05DD  (0,0,0,0, 40,200, 4,2) when g8102 (src/E07C8.C:27) is 0 or 1 and si is 2
                (082C:058F); (0,0,0,0, 40,200, 2,0) when g8102 is 2 and si is 0 (082C:05C8)
     0EF3:071E  (0,0,0,0, 22,120, 0,2)
     0EF3:0860  (0,0,0,0, 22,120, 2,0)
     082C:006D  ([bp+a]-1, [bp+c], 0, 191, [bp+e]+2, 9, 0,2)
     082C:038A  (0, 191, [bp+a]-1, [bp+c], [bp+e]+2, 9, 2,0)
   Three src/ calls pass y origins that differ: the one in DrawTextLine, whose entry is
   1DE7:03B2 (E1DE7.C:245), is made at 1DE7:0474 (pushes 1DE7:0450 to 0470) with y1 =
   gTextTop + one line and y2 = gTextTop, which E1DE7.C:228 calls a clip and refill of the last
   visible line (a one-line scroll is the [H] reading of the source and destination pattern,
   lines 66 to 67); 082C:006D (E082C.C:113) has y1 = y and y2 = 0xbf (the pushes at
   082C:0062 and 082C:005B); 082C:038A (E082C.C:209) has the same pair in the other order,
   y1 = 0xbf and y2 = y (the pushes at 082C:0383 and 082C:0378). The
   1DE7:0474 reading fits x1, y1 as a source and x2, y2 as a destination, a lead [H] to test
   against the far routine, not evidence yet. The x1 = x2, y1 = y2 pattern holds for the
   named-origin calls at EOV6.C:376, :385 and :481, and for the literal-zero calls at
   EOV2.C:365, 082C:05DD and the two 0EF3 calls, but the list is partial and not all callers
   fit: E0ACD.C:272 (0ACD:01E3) has x2 = 0x16 and x1 = 0 in ClipRect(0, 0, 0x16, 0, 0x12, 0xa8,
   2, 0), and the byte-only 0EF3:2512 (pushes at 0EF3:2505 to 2511) gives origins 9 and 122 each
   twice.
   The two call sites 082C:006D and 082C:038A disagree on which y origin holds 191 (y2 for
   006D, y1 for 038A), so the 006D rectangle's coverage is not settled by the bytes. The
   whitepaper has no section on the
   region table, the 8-pixel columns, the screen bounds or ds:8052 yet. */

/* Screen limits. The region table's units are 8-pixel columns and lines (src/E1A29.C: x in
   8-pixel columns, gRegionLeftPx = left * 8), so SCREEN_COLUMNS columns span the 320-pixel
   screen and SCREEN_LINES its 200 lines. Evidence: region record 0 is 0,0,40,200
   (src/E1A29.C, EOB.EXE file offset 0x23B7F), the DGROUP screen bounds g814E = 13Fh (319) and
   g8152 = 0C7h (199), screen width and height minus one, set at src/E1A72.ASM:191-192 and
   declared at src/E1A72.ASM:45-46, and the 40x200 screen the pointer sprite is clipped to
   (src/E1BBE.ASM:79). The same region record at 0x23B7F (00 00 00 00 28 00 c8 00) lands at
   ds:009F under the DGROUP base 0x212E used below (0x23B7F - 0x2800 header - 0x212E0 = 0x9F),
   and the region table is at ds:009F (src/E1A29.C:1-4); the two agree, but that is the [H]
   base checked against the table address, not two independent sources, so support, not proof. */
#define SCREEN_COLUMNS 40
#define SCREEN_LINES 200

/* ds:8052: the function pointer this unit calls through. It is a 4-byte far pointer, and the
   load image holds 00 00 00 00 here, so it is zero in the file. The DGROUP base 0x212E (the
   disassembler's default, a [H] until the startup code that sets DS is cited) puts ds:8052 at
   file offset 0x2BB32 (the 0x2800 MZ header + 0x212E0 + 0x8052), inside the 0x31350-byte load
   image that starts at the 0x2800-byte header, and the 8 bytes there are zero: the 4-byte
   pointer at ds:8052 to ds:8055 and the four bytes after the pointer at ds:8056 to ds:8059,
   zero in the file, whose use is not traced. A
   zero pointer makes the lcall at 19F3:008C call
   0000:0000, the interrupt vector table, not a defined routine [H]; it must be set before
   19F3:008C calls through it (an inference from the zero image; no direct write to ds:8052 in
   EOB.EXE or the overlay binaries: a scan of eob/ and build/re/*-overlays/ (15 files) for
   direct stores with a disp16 of 0x8052 (encodings C7 06, C6 06, A3, 89 06, 89 16, 89 1E,
   88 06) found none; indirect writes not searched). ds:8052 may
   be one slot of the screen driver's pointer table, filled outside the resident code [H]: it
   sits on the same 2 mod 4 offset grid as the driver pointers named in src/ (ds:8062
   gDrawLine, ds:8096, ds:809A, ds:80A6, ds:80C2, ds:80D6, ds:80DA, ds:80AE, ds:80B6), which
   fits a table of far pointers but does not prove one. The setter and the target routine are
   not traced. */
extern void (far *gFn8052)(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ ds:8052 */

/* 19F3:000E: clips a rectangle request, then hands it to the far routine ds:8052. Takes the
   two origins (x1, y1 and x2, y2), a width and a height in the region table's units (the
   screen-limits block above), and extra1 and extra2, which it forwards
   untouched. It caps the width at SCREEN_COLUMNS columns and the height at SCREEN_LINES
   lines, moves a negative origin to 0 together with its partner origin and the size, and
   returns early, without the call, when an x origin has reached SCREEN_COLUMNS columns. It
   clips the origins and the sizes only: it does not clip the far edge x1 + width, so an
   origin below SCREEN_COLUMNS passes with any width up to SCREEN_COLUMNS, and a negative
   width unchanged (x1 = 30, width = 40 passes). */
/*@ 19F3:000E */ void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2)
{
    if (width > SCREEN_COLUMNS) width = SCREEN_COLUMNS;
    if (height > SCREEN_LINES) height = SCREEN_LINES;
    /* A negative origin is raised to 0 and its partner origin and the size take its offset.
       QUIRK 19F3:0036/004B/005C/0070: a negative-origin shift adds the origin to its size, so a
       negative width or height reaches the far routine unchanged; the header has the
       ClipRect(-30, 0, 0, 0, 20, 200, 0, 0) example. */
    if (x1 < 0) {
        x2 -= x1;
        width += x1;
        x1 = 0;
    }
    if (x1 >= SCREEN_COLUMNS) return;
    if (x2 >= SCREEN_COLUMNS) return;
    /* QUIRK 19F3:0045: this shift comes after both x tests and rewrites x1, which no later
       test reads; see the header for an origin past SCREEN_COLUMNS that still reaches the far
       routine. */
    if (x2 < 0) {
        x1 -= x2;
        width += x2;
        x2 = 0;
    }
    if (y1 < 0) {
        y2 -= y1;
        height += y1;
        y1 = 0;
    }
    if (y2 < 0) {
        y1 -= y2;
        height += y2;
        y2 = 0;
    }
    gFn8052(x1, y1, x2, y2, width, height, extra1, extra2);
}
