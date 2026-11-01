/* Unit at EOB.EXE 19F3:000E (137 bytes, 1 function): adjust a rectangle request before
   passing it to the far routine behind the function pointer at ds:8052. The eight ints are
   two origins, a width and a height. The width is read as the shared extent of the x origin
   pair [H]: the bytes move it with whichever x origin is raised (19F3:0036 for x1, 19F3:004B
   for x2), and move the height with whichever y origin is raised (19F3:005C, 19F3:0070); no
   code states that reading. The units are the region table's [H]: x in 8-pixel columns, y in
   lines (src/E1A29.C), so the cap of SCREEN_COLUMNS on the width is the 320-pixel screen and
   the cap of SCREEN_LINES on the height its 200 lines; region record 0 is 0,0,40,200, the
   full screen, matching the call at 082C:05DD. That reading is an inference [H] from the caps
   and record 0; no code states
   it. All the compares are signed. The width and height are capped at SCREEN_COLUMNS and
   SCREEN_LINES when larger, as plain size caps not tied to any origin; nothing clips the far
   edge x1 + width (x1 = 30, width = 40 passes unclipped). x1 and x2 are rejected, the
   function returns without calling, when they reach SCREEN_COLUMNS. The negative-x1 shift
   runs first: it adds -x1 to x2 (x2 -= x1), adds x1 to the width and clears x1. The x1 test
   follows that shift, so it sees x1 already at 0 when x1 was negative (a negative x1 never
   makes it return) but catches a positive x1 at or past SCREEN_COLUMNS columns. The x2 test
   comes next and sees x2 after the negative-x1 shift has moved it (x2 has had -x1 added), so
   a clipped x1 can push x2 to SCREEN_COLUMNS and make the function return.
   QUIRK 19F3:0045: the negative-x2 shift comes after both x tests (19F3:003B cmp di,28h,
   19F3:0040 cmp si,28h) and rewrites x1 (di) with no later test reading it, so an origin past
   SCREEN_COLUMNS can still reach the far routine: ClipRect(0, 0, -50, 0, 40, 200, 0, 0)
   moves x1 by -x2 (= 50), passes x1 = 50, at or past SCREEN_COLUMNS, after the last x
   test, and (19F3:004B) also passes width = 40 + (-50) = -10, a negative size. The
   negative-origin shifts add the origin to the size (19F3:0036 and 19F3:004B add a negative
   x to the width, 19F3:005C and 19F3:0070 add a negative y to the height) with no size test
   after the shifts, so ClipRect(-30, 0, 0, 0, 20, 200, 0, 0) passes x2 = 30 and width = -10,
   whose far edge x2 + width = 20 is inside the screen. QUIRK 19F3:0036/004B/005C/0070: a
   negative size reaches the far routine unchanged; what the far routine does with it is
   unknown.
   y1 and y2 have no upper test and nothing is capped on that side beyond the height.
   Each of x1, y1, x2, y2 is raised to 0 when negative, and that move shifts its partner on
   the same axis and the size by the same amount: x1 moves x2 and the width, x2 moves x1 and
   the width, y1 moves y2 and the height, y2 moves y1 and the height. The eight ints are then
   passed on in source order (their push order in the bytes is extra2, extra1, height,
   width, y2, x2, y1, x1). extra1 and extra2 are passed through untouched; nothing in this
   unit reads them and their roles are unknown. The five call sites push the eight ints
   (add sp,10h). Calls (x1, y1, x2, y2, width, height, extra1, extra2):
     082C:05DD  (0,0,0,0, 40,200, 4,2) when g8102 (src/E07C8.C:27) is 0 or 1 and si is 2
                (082C:058F); (0,0,0,0, 40,200, 2,0) when g8102 is 2 and si is 0 (082C:05C8)
     0EF3:071E  (0,0,0,0, 22,120, 0,2)
     0EF3:0860  (0,0,0,0, 22,120, 2,0)
     082C:006D  ([bp+a]-1, [bp+c], 0, 191, [bp+e]+2, 9, 0,2)
     082C:038A  (0, 191, [bp+a]-1, [bp+c], [bp+e]+2, 9, 2,0)
   The two callers disagree on which y origin holds 191 (y2 for 006D, y1 for 038A), so the
   006D rectangle's coverage is not settled by the bytes; the [H] reading of y in lines rests
   on the height cap and region record 0, both below. The whitepaper has no section on the
   region table, the 8-pixel columns, the screen bounds or ds:8052 yet. What the far routine
   does is still unknown. */

/* Screen limits. The region table's units are 8-pixel columns and lines (src/E1A29.C: x in
   8-pixel columns, gRegionLeftPx = left * 8), so SCREEN_COLUMNS columns span the 320-pixel
   screen and SCREEN_LINES its 200 lines. Evidence: region record 0 is 0,0,40,200
   (src/E1A29.C, EOB.EXE file offset 0x23B7F), the DGROUP screen bounds g814E = 13Fh (319) and
   g8152 = 0C7h (199), screen width and height minus one, set at src/E1A72.ASM:191-192 and
   declared at src/E1A72.ASM:45-46, and the 40x200 screen the pointer sprite is clipped to
   (src/E1BBE.ASM:79). */
#define SCREEN_COLUMNS 40
#define SCREEN_LINES 200

/* ds:8052: the function pointer this unit calls through. The load image holds 00 00 00 00
   here, so it is zero in the file. A zero pointer makes the lcall at 19F3:008C call
   0000:0000, the interrupt vector table, not a defined routine [H]; it must be set before
   19F3:008C calls through it (an inference from the zero image; no setter found). ds:8052 may
   be one slot of the screen driver's pointer table, filled outside the resident code [H]: it
   sits on the same 2 mod 4 offset grid as the driver pointers named in src/ (ds:8062
   gDrawLine, ds:8096, ds:809A, ds:80A6, ds:80C2, ds:80D6, ds:80DA, ds:80AE, ds:80B6), which
   fits a table of far pointers but does not prove one. The setter and the target routine are
   not traced. */
extern void (far *gFar8052)(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2); /*@ ds:8052 */

/* 19F3:000E: clips a rectangle request, then hands it to the far routine ds:8052. Takes the
   two origins (x1, y1 and x2, y2), a width and a height in the region table's units (x in
   8-pixel columns, y in lines; src/E1A29.C), and extra1 and extra2, which it forwards
   untouched. It caps the width at SCREEN_COLUMNS columns and the height at SCREEN_LINES
   lines, moves a negative origin to 0 together with its partner origin and the size, and
   returns early, without the call, when an x origin has reached SCREEN_COLUMNS columns. It
   clips the origins and the sizes only: it does not clip the far edge x1 + width, so an
   origin below SCREEN_COLUMNS passes with any width (x1 = 30, width = 40 passes). */
/*@ 19F3:000E */ void ClipRect(int x1, int y1, int x2, int y2, int width, int height, int extra1, int extra2)
{
    if (width > SCREEN_COLUMNS) width = SCREEN_COLUMNS;
    if (height > SCREEN_LINES) height = SCREEN_LINES;
    /* A negative origin is raised to 0 and its partner origin and the size take its offset. */
    if (x1 < 0) {
        x2 -= x1;
        width += x1;
        x1 = 0;
    }
    if (x1 >= SCREEN_COLUMNS) return;
    if (x2 >= SCREEN_COLUMNS) return;
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
    gFar8052(x1, y1, x2, y2, width, height, extra1, extra2);
}
